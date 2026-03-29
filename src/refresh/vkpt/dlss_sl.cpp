/*
 * dlss_sl.cpp — C++ wrapper for NVIDIA Streamline SDK
 *
 * LINKING STRATEGY:
 * Streamline ships without sl.interposer.lib. All core SL_API functions
 * (slInit, slShutdown, etc.) are declared extern "C" in sl_core_api.h.
 * We provide our own shim implementations that load sl.interposer.dll at
 * runtime via LoadLibrary/GetProcAddress. The linker is satisfied, and if
 * sl.interposer.dll is missing the game starts without DLSS.
 *
 * Feature functions (slDLSSSetOptions etc.) call slGetFeatureFunction which
 * is also a shim here, so the inline helpers in sl_dlss.h work transparently.
 *
 * IAT HOOK — GetModuleHandleW in sl.interposer.dll:
 * sl.interposer determines interposer='yes'/'no' inside slInit()/mapPlugins()
 * by calling GetModuleHandleW("vulkan-1.dll") and comparing the result to its
 * own HMODULE.  When loaded as "sl.interposer.dll" this comparison fails → 'no'.
 *
 * After LoadLibraryW("sl.interposer.dll") we patch its IAT entry for
 * KERNEL32.dll!GetModuleHandleW.  Our hook intercepts queries for "vulkan-1.dll"
 * and returns sl.interposer's own HMODULE instead, making the comparison succeed
 * → interposer='yes' → full FG hook chain (CmdBindPipeline etc.) is enabled.
 * The hook is kept active through slInit() and removed after slInit() returns.
 *
 * We also patch GetModuleHandleExW in case sl.interposer uses that variant,
 * and modify sl.interposer's LDR BaseDllName entry to "vulkan-1.dll" as a
 * belt-and-suspenders measure for any PEB/LDR direct reads.
 */

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winternl.h>   /* UNICODE_STRING, LDR structures, PEB */
#pragma comment(lib, "ntdll.lib")
#pragma comment(lib, "Version.lib")
#endif

/* Vulkan headers must come before Streamline to satisfy sl_helpers_vk.h */
#include <vulkan/vulkan.h>

#ifdef __cplusplus
extern "C" {
#endif
#include "dlss.h"
#ifdef __cplusplus
}
#endif

/* sl.h is the single entry point: pulls in sl_struct.h, sl_consts.h,
   sl_result.h, sl_appidentity.h, sl_core_api.h, sl_core_types.h and
   defines SL_FEATURE_FUN_IMPORT_STATIC, PFun_sl* typedefs, kFeatureDLSS, etc. */
#include "sl.h"

/* Feature-specific headers */
#include "sl_dlss.h"
#include "sl_dlss_d.h"
#include "sl_dlss_g.h"
#include "sl_reflex.h"
#include "sl_pcl.h"          /* PCLMarker, slPCLSetMarker — used to mark present boundaries for DLSS-G */
#include "sl_helpers_vk.h"   /* sl::VulkanInfo, PFun_slSetVulkanInfo, slSetVulkanInfo decl */

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <math.h>
#include <chrono>
#include <vector>

/* ==========================================================================
 * Function pointer table — loaded from sl.interposer.dll
 *
 * PFun_sl* typedefs come from sl_core_api.h (included via sl.h).
 * PFun_slSetVulkanInfo comes from sl_helpers_vk.h (global scope, not sl::).
 * ========================================================================*/

static HMODULE s_sl_lib = nullptr;
static bool    s_sl_hijack_mode = false; /* true if sl.interposer loaded as vulkan-1.dll */

static PFun_slInit*               pfn_slInit               = nullptr;
static PFun_slShutdown*           pfn_slShutdown           = nullptr;
static PFun_slIsFeatureSupported* pfn_slIsFeatureSupported = nullptr;
static PFun_slIsFeatureLoaded*    pfn_slIsFeatureLoaded    = nullptr;
static PFun_slSetFeatureLoaded*   pfn_slSetFeatureLoaded   = nullptr;
static PFun_slSetTagForFrame*     pfn_slSetTagForFrame     = nullptr;
static PFun_slSetConstants*       pfn_slSetConstants       = nullptr;
static PFun_slGetFeatureRequirements* pfn_slGetFeatureRequirements = nullptr;
static PFun_slAllocateResources*  pfn_slAllocateResources  = nullptr;
static PFun_slFreeResources*      pfn_slFreeResources      = nullptr;
static PFun_slEvaluateFeature*    pfn_slEvaluateFeature    = nullptr;
static PFun_slGetNewFrameToken*   pfn_slGetNewFrameToken   = nullptr;
static PFun_slGetFeatureFunction* pfn_slGetFeatureFunction = nullptr;
static PFun_slSetVulkanInfo*      pfn_slSetVulkanInfo      = nullptr;

/* Reflex function pointers (via slGetFeatureFunction).
 * Types are already declared in sl_reflex.h — do NOT redefine them. */
static PFun_slReflexSetOptions* pfn_slReflexSetOptions = nullptr;
static PFun_slReflexSleep*      pfn_slReflexSleep      = nullptr;
static PFun_slReflexGetState*   pfn_slReflexGetState   = nullptr;
static PFun_slDLSSDGetOptimalSettings* pfn_slDLSSDGetOptimalSettings = nullptr;
static PFun_slDLSSDSetOptions*         pfn_slDLSSDSetOptions         = nullptr;
static PFun_slDLSSDGetState*           pfn_slDLSSDGetState           = nullptr;
static PFun_slDLSSGGetState*    pfn_slDLSSGGetState    = nullptr;

/* PCL function pointer (via slGetFeatureFunction).
 * slPCLSetMarker(ePresentStart/End, frame) is required by DLSS-G to match present → frame token.
 * Without these markers, DLSS-G cannot find common constants → "missing common constants" every frame. */
static PFun_slPCLSetMarker*     pfn_slPCLSetMarker     = nullptr;
using PFun_slHookVkCmdBindPipeline = void(VkCommandBuffer, VkPipelineBindPoint, VkPipeline);
using PFun_slHookVkCmdBindDescriptorSets = void(VkCommandBuffer, VkPipelineBindPoint, VkPipelineLayout, uint32_t, uint32_t, const VkDescriptorSet*, uint32_t, const uint32_t*);
using PFun_slHookVkBeginCommandBuffer = void(VkCommandBuffer, const VkCommandBufferBeginInfo*);
static PFun_slHookVkCmdBindPipeline*       pfn_slHookVkCmdBindPipeline = nullptr;
static PFun_slHookVkCmdBindDescriptorSets* pfn_slHookVkCmdBindDescriptorSets = nullptr;
static PFun_slHookVkBeginCommandBuffer*    pfn_slHookVkBeginCommandBuffer = nullptr;

/* SL proxy vkGetDeviceProcAddr / vkGetInstanceProcAddr exported from sl.interposer.dll.
 * Per ProgrammingGuideManualHooking.md §4.2: to get SL-hooked Vulkan function pointers
 * you MUST query them through sl.interposer's own vkGetDeviceProcAddr, not vulkan-1.dll's.
 * sl.interposer returns its own intercepted versions for the functions listed in sl_hooks.h
 * and falls through to vulkan-1.dll for everything else. */
static PFN_vkGetDeviceProcAddr   s_sl_vkGetDeviceProcAddrProxy   = nullptr;
static PFN_vkGetInstanceProcAddr s_sl_vkGetInstanceProcAddrProxy = nullptr;

/* CR53: slGetParameters() exported from sl.interposer — returns sl::param::IParameters*.
 * Used to fetch &s_vk (kVulkanTable) so we can patch s_vk.getDeviceProcAddr to
 * sl.interposer's own vkGetDeviceProcAddr after slSetVulkanInfo resets it to native. */
typedef void* (*PFun_slGetParameters)(void);
static PFun_slGetParameters pfn_slGetParameters = nullptr;

/* SL-patched Vulkan function pointers (fetched via sl.interposer's vkGetDeviceProcAddr).
 * Using these routes Vulkan calls through Streamline hooks so SL can intercept
 * present (presentCommon → GC, frame counter), swapchain creation (DLSS-G setup),
 * and swapchain images (DLSS-G internal buffer management). */
static PFN_vkCreateSwapchainKHR    s_sl_vkCreateSwapchainKHR    = nullptr;
static PFN_vkDestroySwapchainKHR   s_sl_vkDestroySwapchainKHR   = nullptr;
static PFN_vkGetSwapchainImagesKHR s_sl_vkGetSwapchainImagesKHR = nullptr;
static PFN_vkQueuePresentKHR       s_sl_vkQueuePresentKHR       = nullptr;
static PFN_vkAcquireNextImageKHR   s_sl_vkAcquireNextImageKHR   = nullptr;
static PFN_vkDeviceWaitIdle        s_sl_vkDeviceWaitIdle        = nullptr;
static PFN_vkDestroySurfaceKHR     s_sl_vkDestroySurfaceKHR     = nullptr;
static PFN_vkWaitSemaphores        s_vkWaitSemaphoresNative     = nullptr;
static VkDevice                    s_vk_device                  = VK_NULL_HANDLE;

static bool load_sl_procs(HMODULE h)
{
#define LOAD(f) pfn_##f = (PFun_##f*)GetProcAddress(h, #f); if (!pfn_##f) { \
    fprintf(stderr,"[DLSS] GetProcAddress failed: " #f "\n"); return false; }

    LOAD(slInit)
    LOAD(slShutdown)
    LOAD(slIsFeatureSupported)
    LOAD(slIsFeatureLoaded)
    LOAD(slSetFeatureLoaded)
    LOAD(slSetTagForFrame)
    LOAD(slSetConstants)
    LOAD(slGetFeatureRequirements)
    LOAD(slAllocateResources)
    LOAD(slFreeResources)
    LOAD(slEvaluateFeature)
    LOAD(slGetNewFrameToken)
    LOAD(slGetFeatureFunction)
#undef LOAD
    pfn_slSetVulkanInfo = (PFun_slSetVulkanInfo*)GetProcAddress(h, "slSetVulkanInfo");
    if (!pfn_slSetVulkanInfo) {
        fprintf(stderr, "[DLSS] GetProcAddress failed: slSetVulkanInfo\n");
        return false;
    }

    /* Vulkan proc-addr proxies exported by sl.interposer.dll.
     * These return SL-hooked functions for the calls listed in sl_hooks.h
     * and fall through to vulkan-1.dll for everything else.
     * Non-fatal if missing — we will fall back to vulkan-1.dll (no MFG/GC fix). */
    s_sl_vkGetDeviceProcAddrProxy   = (PFN_vkGetDeviceProcAddr)  GetProcAddress(h, "vkGetDeviceProcAddr");
    s_sl_vkGetInstanceProcAddrProxy = (PFN_vkGetInstanceProcAddr)GetProcAddress(h, "vkGetInstanceProcAddr");
    if (!s_sl_vkGetDeviceProcAddrProxy)
        fprintf(stderr, "[DLSS] sl.interposer.dll did not export vkGetDeviceProcAddr — SL present hook won't fire\n");

    /* CR53: slGetParameters — non-fatal if absent (older sl.interposer builds) */
    pfn_slGetParameters = (PFun_slGetParameters)GetProcAddress(h, "slGetParameters");
    if (!pfn_slGetParameters)
        fprintf(stderr, "[DLSS] sl.interposer.dll did not export slGetParameters — CR53 s_vk patch unavailable\n");

    return true;
}

/* ==========================================================================
 * SL_API shim implementations
 *
 * These provide the external symbols declared in sl_core_api.h so the linker
 * is satisfied. They forward every call to the loaded function pointer.
 *
 * The sl_dlss.h inline wrappers call slGetFeatureFunction (also a shim here),
 * so all feature calls work without any additional setup.
 * ========================================================================*/
extern "C" {

static void transpose4x4(const float *src, sl::float4x4 &dst);
static sl::DLSSPreset map_preset(int p);
static sl::DLSSDPreset map_rr_preset(int p);

sl::Result slInit(const sl::Preferences& p, uint64_t v)
{
    if (!pfn_slInit) return sl::Result::eErrorInvalidState;
    return pfn_slInit(p, v);
}
sl::Result slShutdown()
{
    if (!pfn_slShutdown) return sl::Result::eErrorInvalidState;
    return pfn_slShutdown();
}
sl::Result slIsFeatureSupported(sl::Feature f, const sl::AdapterInfo& a)
{
    if (!pfn_slIsFeatureSupported) return sl::Result::eErrorInvalidState;
    return pfn_slIsFeatureSupported(f, a);
}
sl::Result slIsFeatureLoaded(sl::Feature f, bool& loaded)
{
    if (!pfn_slIsFeatureLoaded) return sl::Result::eErrorInvalidState;
    return pfn_slIsFeatureLoaded(f, loaded);
}
sl::Result slSetFeatureLoaded(sl::Feature f, bool loaded)
{
    if (!pfn_slSetFeatureLoaded) return sl::Result::eErrorInvalidState;
    return pfn_slSetFeatureLoaded(f, loaded);
}
sl::Result slSetTagForFrame(const sl::FrameToken& frame, const sl::ViewportHandle& vp,
                            const sl::ResourceTag* tags, uint32_t n, sl::CommandBuffer* cmd)
{
    if (!pfn_slSetTagForFrame) return sl::Result::eErrorInvalidState;
    return pfn_slSetTagForFrame(frame, vp, tags, n, cmd);
}
sl::Result slSetConstants(const sl::Constants& c, const sl::FrameToken& f,
                          const sl::ViewportHandle& vp)
{
    if (!pfn_slSetConstants) return sl::Result::eErrorInvalidState;
    return pfn_slSetConstants(c, f, vp);
}
sl::Result slGetFeatureRequirements(sl::Feature f, sl::FeatureRequirements& r)
{
    if (!pfn_slGetFeatureRequirements) return sl::Result::eErrorInvalidState;
    return pfn_slGetFeatureRequirements(f, r);
}
sl::Result slGetFeatureVersion(sl::Feature f, sl::FeatureVersion& v)           { (void)f; (void)v; return sl::Result::eErrorInvalidState; }
sl::Result slAllocateResources(sl::CommandBuffer* cmd, sl::Feature f, const sl::ViewportHandle& vp)
{
    if (!pfn_slAllocateResources) return sl::Result::eErrorInvalidState;
    return pfn_slAllocateResources(cmd, f, vp);
}
sl::Result slFreeResources(sl::Feature f, const sl::ViewportHandle& vp)
{
    if (!pfn_slFreeResources) return sl::Result::eErrorInvalidState;
    return pfn_slFreeResources(f, vp);
}
sl::Result slEvaluateFeature(sl::Feature f, const sl::FrameToken& frame,
                             const sl::BaseStructure** inputs, uint32_t n, sl::CommandBuffer* cmd)
{
    if (!pfn_slEvaluateFeature) return sl::Result::eErrorInvalidState;
    return pfn_slEvaluateFeature(f, frame, inputs, n, cmd);
}
sl::Result slUpgradeInterface(void** i)   { (void)i; return sl::Result::eErrorInvalidState; }
sl::Result slGetNativeInterface(void* p, void** n) { (void)p; (void)n; return sl::Result::eErrorInvalidState; }
sl::Result slGetNewFrameToken(sl::FrameToken*& t, const uint32_t* idx)
{
    if (!pfn_slGetNewFrameToken) return sl::Result::eErrorInvalidState;
    return pfn_slGetNewFrameToken(t, idx);
}
sl::Result slGetFeatureFunction(sl::Feature f, const char* name, void*& func)
{
    if (!pfn_slGetFeatureFunction) return sl::Result::eErrorInvalidState;
    return pfn_slGetFeatureFunction(f, name, func);
}
sl::Result slSetVulkanInfo(const sl::VulkanInfo& info)
{
    if (!pfn_slSetVulkanInfo) return sl::Result::eErrorInvalidState;
    return pfn_slSetVulkanInfo(info);
}

} /* extern "C" */

/* ==========================================================================
 * Optional SL verbose log callback — writes to sl_debug.log next to the exe
 * Controlled by flt_dlss_sl_debug_log (default off).
 * ========================================================================*/

static FILE* s_sl_log_file = nullptr;

#ifdef _WIN32
static FILE* open_sl_log_file_next_to_exe(void)
{
    wchar_t path[MAX_PATH];
    DWORD len = GetModuleFileNameW(nullptr, path, (DWORD)(sizeof(path) / sizeof(path[0])));
    if (len == 0 || len >= (DWORD)(sizeof(path) / sizeof(path[0])))
        return _wfopen(L"sl_debug.log", L"w");

    for (DWORD i = len; i > 0; --i) {
        if (path[i - 1] == L'\\' || path[i - 1] == L'/') {
            path[i] = L'\0';
            break;
        }
    }

    if (wcslen(path) + wcslen(L"sl_debug.log") + 1 >= (sizeof(path) / sizeof(path[0])))
        return _wfopen(L"sl_debug.log", L"w");

    wcscat_s(path, L"sl_debug.log");
    return _wfopen(path, L"w");
}
#else
static FILE* open_sl_log_file_next_to_exe(void)
{
    return fopen("sl_debug.log", "w");
}
#endif

static void sl_log_printf(const char* fmt, ...)
{
    if (!s_sl_log_file)
        return;

#ifdef _WIN32
    SYSTEMTIME st{};
    GetLocalTime(&st);
    fprintf(s_sl_log_file, "[%04u-%02u-%02u %02u:%02u:%02u.%03u] ",
        (unsigned)st.wYear, (unsigned)st.wMonth, (unsigned)st.wDay,
        (unsigned)st.wHour, (unsigned)st.wMinute, (unsigned)st.wSecond, (unsigned)st.wMilliseconds);
#endif

    va_list args;
    va_start(args, fmt);
    vfprintf(s_sl_log_file, fmt, args);
    va_end(args);
    fflush(s_sl_log_file);
}

static void sl_log_callback(sl::LogType type, const char* msg)
{
    const char* prefix = (type == sl::LogType::eError) ? "ERR" :
                         (type == sl::LogType::eWarn)  ? "WRN" : "INF";
    sl_log_printf("[SL][%s] %s\n", prefix, msg);
}

static int s_display_fps = 0;
static int s_display_multiplier = 0;
static int s_mfg_cap = 0;
static int s_effective_reflex_mode = -1;
static int s_last_dlssg_status = 0;
static int s_last_dlssg_frames_presented = 0;
static int s_last_dlssg_vsync_support = -1;
static uint64_t s_presented_frames_accum = 0;
static uint64_t s_presented_frames_total = 0;
static VkSemaphore s_dlssg_inputs_fence = VK_NULL_HANDLE;
static uint64_t s_dlssg_inputs_fence_value = 0;
static uint64_t s_dlssg_inputs_fence_waited_value = 0;
static auto s_display_fps_window_start = std::chrono::steady_clock::now();
static char s_sr_dll_version[64] = "n/a";
static char s_rr_dll_version[64] = "n/a";
static char s_fg_dll_version[64] = "n/a";

#ifdef _WIN32
static void query_module_file_version(HMODULE module, char *out, size_t out_size)
{
    if (!out || !out_size) {
        return;
    }

    snprintf(out, out_size, "n/a");

    if (!module) {
        return;
    }

    wchar_t path[MAX_PATH];
    DWORD len = GetModuleFileNameW(module, path, (DWORD)(sizeof(path) / sizeof(path[0])));
    if (!len || len >= (DWORD)(sizeof(path) / sizeof(path[0]))) {
        return;
    }

    DWORD handle = 0;
    DWORD ver_size = GetFileVersionInfoSizeW(path, &handle);
    if (!ver_size) {
        return;
    }

    std::vector<BYTE> data(ver_size);
    if (!GetFileVersionInfoW(path, 0, ver_size, data.data())) {
        return;
    }

    VS_FIXEDFILEINFO *info = nullptr;
    UINT info_size = 0;
    if (!VerQueryValueW(data.data(), L"\\", (LPVOID*)&info, &info_size) ||
        info_size < sizeof(*info) || !info) {
        return;
    }

    snprintf(out, out_size, "%u.%u.%u.%u",
        HIWORD(info->dwFileVersionMS),
        LOWORD(info->dwFileVersionMS),
        HIWORD(info->dwFileVersionLS),
        LOWORD(info->dwFileVersionLS));
}
#else
static void query_module_file_version(void *module, char *out, size_t out_size)
{
    (void)module;
    if (out && out_size) {
        snprintf(out, out_size, "n/a");
    }
}
#endif

static void refresh_runtime_feature_versions(void)
{
#ifdef _WIN32
    query_module_file_version(GetModuleHandleW(L"nvngx_dlss.dll"), s_sr_dll_version, sizeof(s_sr_dll_version));
    query_module_file_version(GetModuleHandleW(L"nvngx_dlssd.dll"), s_rr_dll_version, sizeof(s_rr_dll_version));
    query_module_file_version(GetModuleHandleW(L"nvngx_dlssg.dll"), s_fg_dll_version, sizeof(s_fg_dll_version));
#else
    query_module_file_version(nullptr, s_sr_dll_version, sizeof(s_sr_dll_version));
    query_module_file_version(nullptr, s_rr_dll_version, sizeof(s_rr_dll_version));
    query_module_file_version(nullptr, s_fg_dll_version, sizeof(s_fg_dll_version));
#endif
}

static float reflex_delta_us_to_ms(uint64_t begin, uint64_t end)
{
    if (end <= begin)
        return 0.0f;
    return (float)((double)(end - begin) / 1000.0);
}

static void reset_runtime_status_counters(void)
{
    s_display_fps = 0;
    s_display_multiplier = 0;
    s_mfg_cap = 0;
    s_effective_reflex_mode = -1;
    s_last_dlssg_status = 0;
    s_last_dlssg_frames_presented = 0;
    s_last_dlssg_vsync_support = -1;
    s_presented_frames_accum = 0;
    s_presented_frames_total = 0;
    s_dlssg_inputs_fence = VK_NULL_HANDLE;
    s_dlssg_inputs_fence_value = 0;
    s_dlssg_inputs_fence_waited_value = 0;
    s_display_fps_window_start = std::chrono::steady_clock::now();
    snprintf(s_sr_dll_version, sizeof(s_sr_dll_version), "n/a");
    snprintf(s_rr_dll_version, sizeof(s_rr_dll_version), "n/a");
    snprintf(s_fg_dll_version, sizeof(s_fg_dll_version), "n/a");
}

static void clear_sl_function_pointers(void)
{
    pfn_slInit = nullptr;
    pfn_slShutdown = nullptr;
    pfn_slIsFeatureSupported = nullptr;
    pfn_slIsFeatureLoaded = nullptr;
    pfn_slSetFeatureLoaded = nullptr;
    pfn_slSetTagForFrame = nullptr;
    pfn_slSetConstants = nullptr;
    pfn_slGetFeatureRequirements = nullptr;
    pfn_slAllocateResources = nullptr;
    pfn_slFreeResources = nullptr;
    pfn_slEvaluateFeature = nullptr;
    pfn_slGetNewFrameToken = nullptr;
    pfn_slGetFeatureFunction = nullptr;
    pfn_slSetVulkanInfo = nullptr;
    pfn_slReflexSetOptions = nullptr;
    pfn_slReflexSleep = nullptr;
    pfn_slReflexGetState = nullptr;
    pfn_slDLSSDGetOptimalSettings = nullptr;
    pfn_slDLSSDSetOptions = nullptr;
    pfn_slDLSSDGetState = nullptr;
    pfn_slDLSSGGetState = nullptr;
    pfn_slPCLSetMarker = nullptr;
    pfn_slHookVkCmdBindPipeline = nullptr;
    pfn_slHookVkCmdBindDescriptorSets = nullptr;
    pfn_slHookVkBeginCommandBuffer = nullptr;
    pfn_slGetParameters = nullptr;

    s_sl_vkGetDeviceProcAddrProxy = nullptr;
    s_sl_vkGetInstanceProcAddrProxy = nullptr;
    s_sl_vkCreateSwapchainKHR = nullptr;
    s_sl_vkDestroySwapchainKHR = nullptr;
    s_sl_vkGetSwapchainImagesKHR = nullptr;
    s_sl_vkQueuePresentKHR = nullptr;
    s_sl_vkAcquireNextImageKHR = nullptr;
    s_sl_vkDeviceWaitIdle = nullptr;
    s_sl_vkDestroySurfaceKHR = nullptr;
    s_vkWaitSemaphoresNative = nullptr;
    s_vk_device = VK_NULL_HANDLE;
}

/* ==========================================================================
 * C interface (called from dlss.c)
 * ========================================================================*/

extern "C" {

int g_dlss_sl_available      = 0;
int g_dlss_sl_rr_available   = 0;
int g_dlss_sl_g_available    = 0;
int g_dlss_sl_init_result    = 0;  /* slInit return code */
int g_dlss_sl_sr_result      = 0;  /* slIsFeatureSupported(DLSS) return code */
int g_dlss_sl_rr_result      = 0;  /* slIsFeatureSupported(DLSS_RR) return code */
int g_dlss_sl_g_result       = 0;  /* slIsFeatureSupported(DLSS_G) return code */
int g_dlss_sl_setvk_result   = 0;  /* slSetVulkanInfo return code */
int g_dlss_sl_reflex_available = 0;
static uint32_t s_sl_req_graphics_queues = 0;
static uint32_t s_sl_req_compute_queues = 0;
static uint32_t s_sl_req_optflow_queues = 0;
static uint32_t s_sl_req_device_extension_count = 0;
static uint32_t s_sl_req_instance_extension_count = 0;
static uint32_t s_sl_req_feature12_count = 0;
static uint32_t s_sl_req_feature13_count = 0;
static const char* s_sl_req_device_extensions[32] = {};
static const char* s_sl_req_instance_extensions[16] = {};
static const char* s_sl_req_features12[32] = {};
static const char* s_sl_req_features13[32] = {};

static bool            s_sl_loaded   = false;
static sl::FrameToken* s_frame_token = nullptr;
static const sl::ViewportHandle s_vp{0};

/* Track whether slAllocateResources succeeded for each feature.
 * Only free what was actually allocated to avoid NGX crash on exit. */
static bool s_dlss_resources_allocated   = false;
static bool s_dlss_rr_resources_allocated = false;
static bool s_dlss_g_resources_allocated = false;

/* Guard: DLSS-G stays eOff until dlss_sl_tag_g_resources has been called at least
 * once after the last swapchain create/recreate.  Without this, the first present
 * after swapchain init happens before any frame tags are set → DLSS-G crashes
 * with a null-pointer dereference accessing the unset resource slots. */
static bool s_g_tags_valid = false;

int dlss_sl_get_display_fps(void)
{
    return s_display_fps;
}

int dlss_sl_get_display_multiplier(void)
{
    return s_display_multiplier;
}

uint64_t dlss_sl_get_total_presented_frames(void)
{
    return s_presented_frames_total;
}

int dlss_sl_get_mfg_cap(void)
{
    return s_mfg_cap;
}

int dlss_sl_get_effective_reflex_mode(void)
{
    return s_effective_reflex_mode;
}

int dlss_sl_get_last_dlssg_status(void)
{
    return s_last_dlssg_status;
}

int dlss_sl_get_last_dlssg_frames_presented(void)
{
    return s_last_dlssg_frames_presented;
}

int dlss_sl_get_last_dlssg_vsync_support(void)
{
    return s_last_dlssg_vsync_support;
}

const char* dlss_sl_get_sr_dll_version(void)
{
    refresh_runtime_feature_versions();
    return s_sr_dll_version;
}

const char* dlss_sl_get_rr_dll_version(void)
{
    refresh_runtime_feature_versions();
    return s_rr_dll_version;
}

const char* dlss_sl_get_fg_dll_version(void)
{
    refresh_runtime_feature_versions();
    return s_fg_dll_version;
}

bool dlss_sl_get_latest_reflex_report(DlssReflexDebugReport_t *out)
{
    if (!out) {
        return false;
    }

    memset(out, 0, sizeof(*out));

    if (!s_sl_loaded || !pfn_slReflexGetState) {
        return false;
    }

    sl::ReflexState state{};
    if (pfn_slReflexGetState(state) != sl::Result::eOk) {
        return false;
    }

    out->low_latency_available = state.lowLatencyAvailable ? 1 : 0;
    out->latency_report_available = state.latencyReportAvailable ? 1 : 0;

    if (!state.latencyReportAvailable) {
        return false;
    }

    const sl::ReflexReport *best = nullptr;
    for (int i = 0; i < sl::kReflexFrameReportCount; ++i) {
        const sl::ReflexReport &candidate = state.frameReport[i];
        if (!candidate.frameID) {
            continue;
        }
        if (!best || candidate.frameID > best->frameID) {
            best = &candidate;
        }
    }

    if (!best) {
        return false;
    }

    out->frame_id = best->frameID;
    out->sim_ms = reflex_delta_us_to_ms(best->simStartTime, best->simEndTime);
    out->submit_ms = reflex_delta_us_to_ms(best->renderSubmitStartTime, best->renderSubmitEndTime);
    out->present_ms = reflex_delta_us_to_ms(best->presentStartTime, best->presentEndTime);
    out->driver_ms = reflex_delta_us_to_ms(best->driverStartTime, best->driverEndTime);
    out->os_queue_ms = reflex_delta_us_to_ms(best->osRenderQueueStartTime, best->osRenderQueueEndTime);
    out->gpu_active_ms = (float)((double)best->gpuActiveRenderTimeUs / 1000.0);
    out->gpu_frame_ms = (float)((double)best->gpuFrameTimeUs / 1000.0);
    return true;
}

uint32_t dlss_sl_get_required_graphics_queue_count(void)
{
    return s_sl_req_graphics_queues;
}

uint32_t dlss_sl_get_required_compute_queue_count(void)
{
    return s_sl_req_compute_queues;
}

uint32_t dlss_sl_get_required_optical_flow_queue_count(void)
{
    return s_sl_req_optflow_queues;
}

uint32_t dlss_sl_get_required_device_extension_count(void)
{
    return s_sl_req_device_extension_count;
}

const char* dlss_sl_get_required_device_extension(uint32_t index)
{
    if (index >= s_sl_req_device_extension_count)
        return nullptr;
    return s_sl_req_device_extensions[index];
}

uint32_t dlss_sl_get_required_instance_extension_count(void)
{
    return s_sl_req_instance_extension_count;
}

const char* dlss_sl_get_required_instance_extension(uint32_t index)
{
    if (index >= s_sl_req_instance_extension_count)
        return nullptr;
    return s_sl_req_instance_extensions[index];
}

bool dlss_sl_requires_vk_feature12(const char *name)
{
    if (!name)
        return false;
    for (uint32_t i = 0; i < s_sl_req_feature12_count; ++i) {
        if (s_sl_req_features12[i] && strcmp(s_sl_req_features12[i], name) == 0)
            return true;
    }
    return false;
}

bool dlss_sl_requires_vk_feature13(const char *name)
{
    if (!name)
        return false;
    for (uint32_t i = 0; i < s_sl_req_feature13_count; ++i) {
        if (s_sl_req_features13[i] && strcmp(s_sl_req_features13[i], name) == 0)
            return true;
    }
    return false;
}

/* ---- Mode / preset mapping ---- */
static sl::DLSSMode map_mode(int m)
{
    switch (m) {
    case 1: return sl::DLSSMode::eUltraPerformance;
    case 2: return sl::DLSSMode::eMaxPerformance;
    case 3: return sl::DLSSMode::eBalanced;
    case 4: return sl::DLSSMode::eMaxQuality;
    case 5: return sl::DLSSMode::eUltraQuality;
    case 6: return sl::DLSSMode::eDLAA;
    default: return sl::DLSSMode::eOff;
    }
}

static bool get_dlss_optimal_settings_for_mode(
    int mode,
    uint32_t display_w, uint32_t display_h,
    uint32_t *render_w, uint32_t *render_h)
{
    if (!s_sl_loaded || !g_dlss_sl_available)
        return false;

    sl::DLSSOptions opts{};
    opts.mode = map_mode(mode);
    opts.outputWidth = display_w;
    opts.outputHeight = display_h;

    sl::DLSSOptimalSettings optimal{};
    if (slDLSSGetOptimalSettings(opts, optimal) != sl::Result::eOk)
        return false;

    if (optimal.optimalRenderWidth == 0 || optimal.optimalRenderHeight == 0)
        return false;

    *render_w = optimal.optimalRenderWidth;
    *render_h = optimal.optimalRenderHeight;
    return true;
}

static void get_emulated_ultra_quality_resolution(
    uint32_t display_w, uint32_t display_h,
    uint32_t *render_w, uint32_t *render_h)
{
    /* NVIDIA tools expose Ultra Quality as a custom scale around 77%.
     * If the current runtime rejects the formal Ultra Quality enum, emulate the
     * expected behavior via a custom render ratio while driving the supported
     * Quality model under the hood. */
    uint32_t rw = (display_w * 77u + 50u) / 100u;
    uint32_t rh = (display_h * 77u + 50u) / 100u;
    if (rw > display_w) rw = display_w;
    if (rh > display_h) rh = display_h;
    if (rw < 16u) rw = 16u;
    if (rh < 16u) rh = 16u;
    rw &= ~1u;
    if (rw < 16u) rw = 16u;
    *render_w = rw;
    *render_h = rh;
}

static int resolve_supported_dlss_mode(
    int requested_mode,
    uint32_t display_w, uint32_t display_h,
    uint32_t *render_w, uint32_t *render_h)
{
    if (requested_mode != 5)
        return requested_mode;

    if (get_dlss_optimal_settings_for_mode(requested_mode, display_w, display_h, render_w, render_h))
        return requested_mode;

    /* Current NV runtime on this project/DLL stack rejects Ultra Quality with
     * invalid (0x0) optimal settings.  Fall back to Quality instead of
     * propagating a broken mode into extent_render / feature creation.
     *
     * Preserve the user's expected "less aggressive than Quality" behavior by
     * emulating Ultra Quality with a custom ~77% render ratio while using the
     * supported Quality mode for the actual DLSS feature creation. */
    uint32_t quality_render_w = 0, quality_render_h = 0;
    if (requested_mode == 5 &&
        get_dlss_optimal_settings_for_mode(4, display_w, display_h,
                                           &quality_render_w, &quality_render_h)) {
        get_emulated_ultra_quality_resolution(display_w, display_h, render_w, render_h);
        static bool s_logged_uq_fallback = false;
        if (!s_logged_uq_fallback) {
            s_logged_uq_fallback = true;
            sl_log_printf("[DLSS] Ultra Quality is unsupported by the current runtime; emulating it via a custom ratio on Quality.\n");
            fprintf(stderr, "[DLSS] Ultra Quality is unsupported by the current runtime; emulating it via a custom ratio on Quality.\n");
        }
        return 4;
    }

    return requested_mode;
}

static bool get_dlss_rr_optimal_settings_for_mode(
    int mode, int preset,
    uint32_t display_w, uint32_t display_h,
    uint32_t *render_w, uint32_t *render_h)
{
    if (!s_sl_loaded || !g_dlss_sl_rr_available || !pfn_slDLSSDGetOptimalSettings)
        return false;

    sl::DLSSDOptions opts{};
    opts.mode = map_mode(mode);
    opts.outputWidth = display_w;
    opts.outputHeight = display_h;
    opts.colorBuffersHDR = sl::Boolean::eTrue;
    opts.normalRoughnessMode = sl::DLSSDNormalRoughnessMode::eUnpacked;
    opts.dlaaPreset = map_rr_preset(preset);
    opts.qualityPreset = map_rr_preset(preset);
    opts.balancedPreset = map_rr_preset(preset);
    opts.performancePreset = map_rr_preset(preset);
    opts.ultraPerformancePreset = map_rr_preset(preset);
    opts.ultraQualityPreset = map_rr_preset(preset);

    sl::DLSSDOptimalSettings optimal{};
    if (pfn_slDLSSDGetOptimalSettings(opts, optimal) != sl::Result::eOk)
        return false;

    if (optimal.optimalRenderWidth == 0 || optimal.optimalRenderHeight == 0)
        return false;

    *render_w = optimal.optimalRenderWidth;
    *render_h = optimal.optimalRenderHeight;
    return true;
}

static int resolve_supported_dlss_rr_mode(
    int requested_mode, int preset,
    uint32_t display_w, uint32_t display_h,
    uint32_t *render_w, uint32_t *render_h)
{
    if (requested_mode != 5)
        return requested_mode;

    if (get_dlss_rr_optimal_settings_for_mode(requested_mode, preset, display_w, display_h, render_w, render_h))
        return requested_mode;

    uint32_t quality_render_w = 0, quality_render_h = 0;
    if (requested_mode == 5 &&
        get_dlss_rr_optimal_settings_for_mode(4, preset, display_w, display_h,
                                              &quality_render_w, &quality_render_h)) {
        get_emulated_ultra_quality_resolution(display_w, display_h, render_w, render_h);
        static bool s_logged_rr_uq_fallback = false;
        if (!s_logged_rr_uq_fallback) {
            s_logged_rr_uq_fallback = true;
            sl_log_printf("[DLSS-RR] Ultra Quality is unsupported by the current runtime; emulating it via a custom ratio on Quality.\n");
            fprintf(stderr, "[DLSS-RR] Ultra Quality is unsupported by the current runtime; emulating it via a custom ratio on Quality.\n");
        }
        return 4;
    }

    return requested_mode;
}

/*
 * Preset mapping (DlssPreset_t → sl::DLSSPreset):
 * D/E are deprecated for DLSS SR in this Streamline SDK, so map them to K.
 */
static sl::DLSSPreset map_preset(int p)
{
    switch (p) {
    case DLSS_PRESET_D: return sl::DLSSPreset::ePresetK;
    case DLSS_PRESET_E: return sl::DLSSPreset::ePresetK;
    case DLSS_PRESET_F: return sl::DLSSPreset::ePresetF;
    case DLSS_PRESET_J: return sl::DLSSPreset::ePresetJ;
    case DLSS_PRESET_K: return sl::DLSSPreset::ePresetK;
    case DLSS_PRESET_L: return sl::DLSSPreset::ePresetL;
    case DLSS_PRESET_M: return sl::DLSSPreset::ePresetM;
    default: return sl::DLSSPreset::eDefault;
    }
}

static sl::DLSSDPreset map_rr_preset(int p)
{
    switch (p) {
    case DLSS_PRESET_E: return sl::DLSSDPreset::ePresetE;
    case DLSS_PRESET_D: return sl::DLSSDPreset::ePresetD;
    default: return sl::DLSSDPreset::eDefault;
    }
}

/* ======================================================================
 * IAT hook: GetModuleHandleW / GetModuleHandleExW in sl.interposer.dll
 *
 * sl.interposer calls GetModuleHandleW("vulkan-1.dll") during slInit/mapPlugins
 * to check if it was loaded AS "vulkan-1.dll".  We hook this in its IAT so
 * the query returns sl.interposer's own HMODULE → interposer='yes'.
 * ====================================================================*/

/* Saved IAT slots for restoration */
static void** s_iat_gmhw  = nullptr;   /* &IAT[GetModuleHandleW]   */
static void*  s_orig_gmhw = nullptr;
static void** s_iat_gmhex = nullptr;   /* &IAT[GetModuleHandleExW] */
static void*  s_orig_gmhex = nullptr;

/* Hook: intercept GetModuleHandleW("vulkan-1.dll") → return sl.interposer HMODULE */
static HMODULE WINAPI hook_GetModuleHandleW(LPCWSTR lpName)
{
    if (lpName && _wcsicmp(lpName, L"vulkan-1.dll") == 0) {
        sl_log_printf("[DLSS] hook_GMHW: intercepted GetModuleHandleW(\"vulkan-1.dll\") -> returning sl.interposer %p\n",
            (void*)s_sl_lib);
        return s_sl_lib;
    }
    return GetModuleHandleW(lpName);
}

/* Hook: intercept GetModuleHandleExW("vulkan-1.dll", ...) → return sl.interposer HMODULE */
static BOOL WINAPI hook_GetModuleHandleExW(DWORD dwFlags, LPCWSTR lpName, HMODULE* phModule)
{
    if (lpName && !(dwFlags & GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS)
               && _wcsicmp(lpName, L"vulkan-1.dll") == 0) {
        sl_log_printf("[DLSS] hook_GMHExW: intercepted GetModuleHandleExW(\"vulkan-1.dll\") -> returning sl.interposer %p\n",
            (void*)s_sl_lib);
        if (phModule) *phModule = s_sl_lib;
        return TRUE;
    }
    return GetModuleHandleExW(dwFlags, lpName, phModule);
}

/* Walk sl.interposer's IAT and patch a named function in a given import DLL */
static bool patch_iat_entry(HMODULE hMod, const char* dllName, const char* funcName,
                             void* newFn, void*** outSlot, void** outOrig)
{
    BYTE* base = (BYTE*)hMod;
    IMAGE_DOS_HEADER*  dos = (IMAGE_DOS_HEADER*)base;
    IMAGE_NT_HEADERS*  nt  = (IMAGE_NT_HEADERS*)(base + dos->e_lfanew);
    IMAGE_DATA_DIRECTORY* impDir = &nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT];
    if (!impDir->VirtualAddress) return false;

    IMAGE_IMPORT_DESCRIPTOR* imp = (IMAGE_IMPORT_DESCRIPTOR*)(base + impDir->VirtualAddress);
    for (; imp->Name; ++imp) {
        if (_stricmp((char*)(base + imp->Name), dllName) != 0) continue;

        IMAGE_THUNK_DATA* orig = (IMAGE_THUNK_DATA*)(base + imp->OriginalFirstThunk);
        IMAGE_THUNK_DATA* iat  = (IMAGE_THUNK_DATA*)(base + imp->FirstThunk);
        for (; orig->u1.Ordinal; ++orig, ++iat) {
            if (IMAGE_SNAP_BY_ORDINAL(orig->u1.Ordinal)) continue;
            IMAGE_IMPORT_BY_NAME* ibn = (IMAGE_IMPORT_BY_NAME*)(base + orig->u1.AddressOfData);
            if (strcmp((char*)ibn->Name, funcName) != 0) continue;

            *outSlot = (void**)&iat->u1.Function;
            *outOrig = (void*)iat->u1.Function;
            DWORD old;
            VirtualProtect(*outSlot, sizeof(void*), PAGE_READWRITE, &old);
            **outSlot = newFn;
            VirtualProtect(*outSlot, sizeof(void*), old, &old);
            sl_log_printf("[DLSS] IAT patched: %s!%s -> %p (was %p)\n",
                dllName, funcName, newFn, *outOrig);
            return true;
        }
    }
    sl_log_printf("[DLSS] IAT patch: %s!%s NOT FOUND in IAT\n", dllName, funcName);
    return false;
}

static void restore_iat_entry(void** slot, void* orig)
{
    if (!slot || !orig) return;
    DWORD old;
    VirtualProtect(slot, sizeof(void*), PAGE_READWRITE, &old);
    *slot = orig;
    VirtualProtect(slot, sizeof(void*), old, &old);
}

/* ============================================================
 * LDR BaseDllName rename: change sl.interposer's entry in the
 * PEB loader list to "vulkan-1.dll" so any direct PEB/LDR
 * reads by sl.interposer also see the correct name.
 * ============================================================*/
static WCHAR s_ldr_orig_name[MAX_PATH] = {};
static USHORT s_ldr_orig_len = 0;
static USHORT s_ldr_orig_maxlen = 0;
static PWSTR  s_ldr_orig_buf = nullptr;
static void*  s_ldr_entry = nullptr;

/* Offset of BaseDllName within LDR_DATA_TABLE_ENTRY on x64 Windows 10/11.
 * Layout: Flink(8)+Blink(8) × 3 lists = 48, DllBase(8), EntryPoint(8), SizeOfImage(4)+pad(4),
 * FullDllName(UNICODE_STRING=16), BaseDllName(UNICODE_STRING=16) → offset = 48+8+8+8+16 = 88 */
#define LDR_BASEDLLNAME_OFFSET 88

static const wchar_t s_fake_name[] = L"vulkan-1.dll";

static void ldr_rename_to_vulkan(HMODULE hMod)
{
    /* Walk PEB->Ldr->InLoadOrderModuleList to find our entry */
    typedef struct _PEB_LDR { ULONG Length; BOOLEAN Init; HANDLE SsHandle;
        LIST_ENTRY InLoadOrderModuleList; } PEB_LDR;
    /* On x64: GS:[0x60] = PEB, PEB+0x18 = Ldr */
#ifdef _WIN64
    ULONG_PTR peb = __readgsqword(0x60);
    PEB_LDR*  ldr = *(PEB_LDR**)(peb + 0x18);
#else
    return; /* 32-bit not needed */
#endif
    LIST_ENTRY* head = &ldr->InLoadOrderModuleList;
    for (LIST_ENTRY* e = head->Flink; e != head; e = e->Flink) {
        /* BaseDllName is at a fixed offset within LDR_DATA_TABLE_ENTRY */
        BYTE* entry = (BYTE*)e; /* e == &entry->InLoadOrderLinks (first field) */
        /* LDR_DATA_TABLE_ENTRY x64 layout: InLoadOrderLinks(16) + InMemoryOrderLinks(16)
         * + InInitializationOrderLinks(16) = 48, then DllBase at offset 48. */
        void** dllBase = (void**)(entry + 48); /* DllBase at offset 48 on x64 */
        if (*dllBase != (void*)hMod) continue;

        /* Found our entry */
        s_ldr_entry = entry;
        UNICODE_STRING* baseName = (UNICODE_STRING*)(entry + LDR_BASEDLLNAME_OFFSET);
        s_ldr_orig_buf    = baseName->Buffer;
        s_ldr_orig_len    = baseName->Length;
        s_ldr_orig_maxlen = baseName->MaximumLength;
        wcscpy(s_ldr_orig_name, baseName->Buffer ? baseName->Buffer : L"");

        /* Replace with "vulkan-1.dll" */
        baseName->Buffer         = (PWSTR)s_fake_name;
        baseName->Length         = (USHORT)(wcslen(s_fake_name) * sizeof(wchar_t));
        baseName->MaximumLength  = baseName->Length + sizeof(wchar_t);
        sl_log_printf("[DLSS] LDR: renamed BaseDllName \"%ls\" -> \"vulkan-1.dll\"\n",
            s_ldr_orig_name);
        return;
    }
    sl_log_printf("[DLSS] LDR: entry for sl.interposer NOT FOUND in PEB list\n");
}

static void ldr_restore_name(void)
{
    if (!s_ldr_entry) return;
    UNICODE_STRING* baseName = (UNICODE_STRING*)((BYTE*)s_ldr_entry + LDR_BASEDLLNAME_OFFSET);
    baseName->Buffer         = s_ldr_orig_buf;
    baseName->Length         = s_ldr_orig_len;
    baseName->MaximumLength  = s_ldr_orig_maxlen;
    s_ldr_entry = nullptr;
    sl_log_printf("[DLSS] LDR: BaseDllName restored to \"%ls\"\n", s_ldr_orig_name);
}

/* ======================================================================
 * Startup / shutdown
 * ====================================================================*/

int dlss_sl_startup(int want_mfg)
{
    const bool enable_sl_debug_log = vkpt_dlss_is_sl_debug_log_enabled();

    if (enable_sl_debug_log) {
        /* Open debug log file next to exe so we can see SL internals */
        s_sl_log_file = open_sl_log_file_next_to_exe();
    }

    s_sl_hijack_mode = false;
    s_sl_lib = LoadLibraryW(L"sl.interposer.dll");

    if (!s_sl_lib) {
        fprintf(stderr, "[DLSS] sl.interposer.dll not found — DLSS unavailable\n");
        sl_log_printf("[DLSS] sl.interposer.dll not found\n");
        if (s_sl_log_file) { fclose(s_sl_log_file); s_sl_log_file = nullptr; }
        return 0;
    }

    if (!load_sl_procs(s_sl_lib)) {
        FreeLibrary(s_sl_lib);
        s_sl_lib = nullptr;
        if (s_sl_log_file) { fclose(s_sl_log_file); s_sl_log_file = nullptr; }
        return 0;
    }

    /* Get directory of sl.interposer.dll to tell SL where to find plugins.
       Without this, SL may look in wrong locations and fail with eErrorFeatureMissing. */
    static wchar_t s_sl_dir[MAX_PATH] = {};
    GetModuleFileNameW(s_sl_lib, s_sl_dir, MAX_PATH);
    wchar_t* last_slash = wcsrchr(s_sl_dir, L'\\');
    if (last_slash) last_slash[1] = L'\0'; /* keep trailing backslash */
    const wchar_t* plugin_paths[] = { s_sl_dir };

    /* Log the plugin search path for diagnostics */
    sl_log_printf("[DLSS] sl.interposer.dll dir: %ls\n", s_sl_dir);
    if (enable_sl_debug_log)
        sl_log_printf("[DLSS] flt_dlss_sl_debug_log=1 -> verbose Streamline logging enabled\n");

    /* IAT hooks (GetModuleHandleW / GetModuleHandleExW) and LDR rename were attempted
     * to achieve interposer='yes', but GetModuleHandleW is NOT called by sl.interposer
     * during mapPlugins — the interposer identity check uses a different mechanism.
     * LDR rename also failed (PEB entry not found with tested offsets).
     * These hooks are removed; eUseManualHooking is used instead. */

    /* CR51: Always load DLSS-G plugin regardless of want_mfg.
     * Previously sl.dlss_g was gated on want_mfg to avoid a crash at first present
     * (sl.dlss_g hooks vkCreateSwapchainKHR/vkQueuePresentKHR and requires Reflex/NvLowLatency).
     * However, q2config.cfg is not yet executed when dlss_sl_startup() is called (Cbuf_Execute
     * runs later in the main loop), so cvar_dlss_mfg->integer is always 0 at this point —
     * meaning want_mfg is always 0 and DLSS-G was never actually loaded.
     * Since sl.reflex (kFeatureReflex) is always loaded, NvLowLatency is available and
     * sl.dlss_g no longer crashes when MFG=0. Load it unconditionally so the user can
     * toggle MFG at runtime without restarting. */
    sl::Feature features_all[] = { sl::kFeatureDLSS, sl::kFeatureDLSS_RR, sl::kFeatureDLSS_G, sl::kFeatureReflex };

    sl::Preferences prefs{};
    prefs.showConsole         = false;
    prefs.logLevel            = enable_sl_debug_log ? sl::LogLevel::eVerbose : sl::LogLevel::eDefault;
    prefs.logMessageCallback  = enable_sl_debug_log ? sl_log_callback : nullptr;
    prefs.renderAPI           = sl::RenderAPI::eVulkan;
    prefs.pathsToPlugins      = plugin_paths;
    prefs.numPathsToPlugins   = 1;
    prefs.featuresToLoad      = features_all;
    prefs.numFeaturesToLoad   = 4;
    /* applicationId must be non-zero for NGX initialization (DLSS SR requires NGX).
       Using Steam App ID for Quake II RTX: 1089130 */
    prefs.applicationId       = 1089130;
    /* OTA flags:
     *   eAllowOTA             — let nvngx_update.exe download newer plugin binaries
     *   eLoadDownloadedPlugins — use the downloaded OTA binaries instead of local ones
     *
     * DLSS-G OTA 2.10.3 + sl.interposer 2.8.0 = version mismatch → crash (CR27/CR28):
     * OTA sl.dlss_g 2.10.3 expects hook callbacks available only in sl.interposer 2.10.x.
     * With sl.interposer 2.8.0 those callbacks are null → RAX=0 in presentCommon on first
     * eOn present → ACCESS VIOLATION.
     *
     * However, disabling eLoadDownloadedPlugins also breaks DLSS SR and Reflex:
     * local sl.dlss/sl.reflex 2.8.0 crash in sl.common during early init (CR29).
     * OTA plugins must be used — they are required for Transformer 2 presets (DLSS 4.5).
     *
     * Current strategy (post-CR29): keep eLoadDownloadedPlugins ON so DLSS SR + Reflex
     * use OTA 2.10.3 and work correctly.  DLSS-G (sl.dlss_g OTA 2.10.3) is loaded but
     * kept in eOff mode until a compatible sl.interposer 2.10.x is found.  The guard
     * s_g_tags_valid ensures eOn is never activated (see dlss_sl_set_g_options).
     *
     * eUseManualHooking: SL does not auto-hook vkBeginCommandBuffer / vkCmdBindPipeline /
     * vkCmdBindDescriptorSets via dispatch chain.  Required in our configuration because
     * sl.interposer is loaded via LoadLibrary (not as vulkan-1.dll), so it is NOT in the
     * Vulkan dispatch chain.  Without this flag SL tries to access dispatch chain hooks
     * that were never established → crash in sl.interposer (RIP=0).
     *
     * CR50: We also route vkCreateDevice through sl.interposer (see main.c /
     * vkpt_dlss_prepare_device_creation) so that pluginManager->initializePlugins() fires
     * and DLSS-G plugin hooks are established internally.  eUseManualHooking is kept to
     * prevent the dispatch chain hook crash, but initializePlugins() running should be
     * enough to exit WAR4639162 fallback mode. */
    prefs.flags               = sl::PreferenceFlags::eDisableCLStateTracking
                              | sl::PreferenceFlags::eAllowOTA
                              | sl::PreferenceFlags::eLoadDownloadedPlugins
                              | sl::PreferenceFlags::eUseFrameBasedResourceTagging
                              | sl::PreferenceFlags::eUseManualHooking;

    sl::Result res = slInit(prefs, sl::kSDKVersion);
    g_dlss_sl_init_result = (int)res;

    if (res != sl::Result::eOk) {
        FreeLibrary(s_sl_lib);
        s_sl_lib = nullptr;
        return 0;
    }

    s_sl_loaded = true;
    {
        sl::FeatureRequirements req{};
        if (slGetFeatureRequirements(sl::kFeatureDLSS_G, req) == sl::Result::eOk) {
            s_sl_req_graphics_queues = req.vkNumGraphicsQueuesRequired;
            s_sl_req_compute_queues = req.vkNumComputeQueuesRequired;
            s_sl_req_optflow_queues = req.vkNumOpticalFlowQueuesRequired;
            s_sl_req_device_extension_count = (req.vkNumDeviceExtensions < 32u) ? req.vkNumDeviceExtensions : 32u;
            s_sl_req_instance_extension_count = (req.vkNumInstanceExtensions < 16u) ? req.vkNumInstanceExtensions : 16u;
            s_sl_req_feature12_count = (req.vkNumFeatures12 < 32u) ? req.vkNumFeatures12 : 32u;
            s_sl_req_feature13_count = (req.vkNumFeatures13 < 32u) ? req.vkNumFeatures13 : 32u;
            memset(s_sl_req_device_extensions, 0, sizeof(s_sl_req_device_extensions));
            memset(s_sl_req_instance_extensions, 0, sizeof(s_sl_req_instance_extensions));
            memset(s_sl_req_features12, 0, sizeof(s_sl_req_features12));
            memset(s_sl_req_features13, 0, sizeof(s_sl_req_features13));
            for (uint32_t i = 0; i < s_sl_req_device_extension_count; ++i)
                s_sl_req_device_extensions[i] = req.vkDeviceExtensions[i];
            for (uint32_t i = 0; i < s_sl_req_instance_extension_count; ++i)
                s_sl_req_instance_extensions[i] = req.vkInstanceExtensions[i];
            for (uint32_t i = 0; i < s_sl_req_feature12_count; ++i)
                s_sl_req_features12[i] = req.vkFeatures12[i];
            for (uint32_t i = 0; i < s_sl_req_feature13_count; ++i)
                s_sl_req_features13[i] = req.vkFeatures13[i];
            sl_log_printf("[DLSS-G] requirements: gfxQueues=%u computeQueues=%u opticalFlowQueues=%u\n",
                s_sl_req_graphics_queues,
                s_sl_req_compute_queues,
                s_sl_req_optflow_queues);
            for (uint32_t i = 0; i < s_sl_req_device_extension_count; ++i)
                sl_log_printf("[DLSS-G] req device extension[%u]: %s\n", i, s_sl_req_device_extensions[i]);
            for (uint32_t i = 0; i < s_sl_req_instance_extension_count; ++i)
                sl_log_printf("[DLSS-G] req instance extension[%u]: %s\n", i, s_sl_req_instance_extensions[i]);
            for (uint32_t i = 0; i < s_sl_req_feature12_count; ++i)
                sl_log_printf("[DLSS-G] req feature12[%u]: %s\n", i, s_sl_req_features12[i]);
            for (uint32_t i = 0; i < s_sl_req_feature13_count; ++i)
                sl_log_printf("[DLSS-G] req feature13[%u]: %s\n", i, s_sl_req_features13[i]);
        }
    }
    reset_runtime_status_counters();
    return 1;
}

/* Expose sl.interposer's vkGetInstanceProcAddr proxy so main.c can call
 * vkCreateInstance and vkCreateDevice through sl.interposer.  This is required
 * for sl.interposer to intercept vkCreateDevice and set up its internal plugin
 * dispatch table (including hooks for vkCmdBindPipeline, vkCmdBindDescriptorSets,
 * vkBeginCommandBuffer that sl.common needs for correct DLSS-G operation).
 * Without this, sl.common logs "Hook ... is NOT supported" and DLSS-G crashes. */
extern "C" PFN_vkGetInstanceProcAddr dlss_sl_get_vkGetInstanceProcAddr_proxy(void)
{
    return s_sl_vkGetInstanceProcAddrProxy;
}

void dlss_sl_shutdown(void)
{
    if (!s_sl_loaded) return;
    /* Do NOT call slFreeResources before slShutdown.
     *
     * slFreeResources destroys the NGX feature context (DLSS SR / DLSS-G).
     * slSetTagForFrame registers destroy lambdas inside the NGX context that
     * SL's collectGarbage processes during slShutdown.  If we free the NGX
     * context first those lambdas access freed memory → ACCESS VIOLATION.
     *
     * The correct pattern: call slShutdown alone.  SL internally runs GC and
     * tears down all feature contexts as part of its own shutdown sequence.
     * slFreeResources is only for explicit mid-session resource recycling
     * (e.g. before a swapchain resize), not for final application exit. */
    slShutdown();
    s_dlss_resources_allocated   = false;
    s_dlss_rr_resources_allocated = false;
    s_dlss_g_resources_allocated = false;
    s_g_tags_valid = false;
    s_sl_loaded   = false;
    s_frame_token = nullptr;
    /* In hijack mode s_sl_lib == GetModuleHandle("vulkan-1.dll") — do NOT FreeLibrary,
     * that would decrement the refcount of the primary Vulkan ICD mid-session. */
    if (s_sl_lib && !s_sl_hijack_mode) { FreeLibrary(s_sl_lib); }
    s_sl_lib = nullptr;
    s_sl_hijack_mode = false;
    clear_sl_function_pointers();
    reset_runtime_status_counters();
    g_dlss_sl_available = 0;
    g_dlss_sl_rr_available = 0;
    g_dlss_sl_g_available = 0;
    g_dlss_sl_init_result = 0;
    g_dlss_sl_sr_result = 0;
    g_dlss_sl_rr_result = 0;
    g_dlss_sl_g_result = 0;
    g_dlss_sl_setvk_result = 0;
    g_dlss_sl_reflex_available = 0;
    s_sl_req_graphics_queues = 0;
    s_sl_req_compute_queues = 0;
    s_sl_req_optflow_queues = 0;
    s_sl_req_device_extension_count = 0;
    s_sl_req_instance_extension_count = 0;
    s_sl_req_feature12_count = 0;
    s_sl_req_feature13_count = 0;
    memset(s_sl_req_device_extensions, 0, sizeof(s_sl_req_device_extensions));
    memset(s_sl_req_instance_extensions, 0, sizeof(s_sl_req_instance_extensions));
    memset(s_sl_req_features12, 0, sizeof(s_sl_req_features12));
    memset(s_sl_req_features13, 0, sizeof(s_sl_req_features13));
    fprintf(stdout, "[DLSS] Streamline shut down\n");
    if (s_sl_log_file) { fclose(s_sl_log_file); s_sl_log_file = nullptr; }
}

/* ======================================================================
 * Vulkan device info
 * ====================================================================*/

static void dlss_sl_complete_vulkan_setup(
    VkInstance       instance,
    VkPhysicalDevice phys_device,
    VkDevice         device,
    uint32_t         gfx_queue_family, uint32_t gfx_queue_index,
    uint32_t         compute_queue_family, uint32_t compute_queue_index,
    uint32_t         optical_flow_queue_family, uint32_t optical_flow_queue_index,
    int              use_native_optical_flow,
    int              call_set_vulkan_info)
{
    if (!s_sl_loaded) return;

    s_vk_device = device;

    if (call_set_vulkan_info) {
        sl::VulkanInfo vk_info{};
        vk_info.instance = instance;
        vk_info.physicalDevice = phys_device;
        vk_info.device = device;
        vk_info.graphicsQueueFamily = gfx_queue_family;
        vk_info.graphicsQueueIndex = gfx_queue_index;
        vk_info.computeQueueFamily = compute_queue_family;
        vk_info.computeQueueIndex = compute_queue_index;
        vk_info.opticalFlowQueueFamily = optical_flow_queue_family;
        vk_info.opticalFlowQueueIndex = optical_flow_queue_index;
        vk_info.useNativeOpticalFlowMode = use_native_optical_flow ? true : false;

        sl::Result setvk = slSetVulkanInfo(vk_info);
        g_dlss_sl_setvk_result = (int)setvk;
        if (setvk != sl::Result::eOk) {
            fprintf(stderr, "[DLSS] slSetVulkanInfo returned %d (continuing with proxy/manual path)\n", (int)setvk);
        }
    } else {
        g_dlss_sl_setvk_result = 0;
    }
    sl_log_printf("[DLSS] slSetVulkanInfo: result=%d gfxFamily=%u gfxIndex=%u computeFamily=%u computeIndex=%u ofFamily=%u ofIndex=%u nativeOF=%d reqGfx=%u reqCompute=%u reqOF=%u\n",
        g_dlss_sl_setvk_result,
        gfx_queue_family,
        gfx_queue_index,
        compute_queue_family,
        compute_queue_index,
        optical_flow_queue_family,
        optical_flow_queue_index,
        use_native_optical_flow,
        s_sl_req_graphics_queues,
        s_sl_req_compute_queues,
        s_sl_req_optflow_queues);

    /* CR53: slSetVulkanInfo can overwrite the global Vulkan dispatch table with
     * native proc addrs. Re-patch sl.param.global.vulkanTable so present-time code
     * re-queries sl.interposer's own vkGetDeviceProcAddr/vkGetInstanceProcAddr. */
    if (pfn_slGetParameters && s_sl_vkGetDeviceProcAddrProxy) {
        void* iparams = pfn_slGetParameters();
        if (iparams) {
            void** vtbl = *(void***)iparams;
            typedef bool (*GetPtrFn)(void* self, const char* key, void** value);
            GetPtrFn get_fn = (GetPtrFn)vtbl[13];

            void* s_vk_ptr = nullptr;
            bool got = get_fn(iparams, "sl.param.global.vulkanTable", &s_vk_ptr);

            if (got && s_vk_ptr) {
                void** gdpa_slot = (void**)((BYTE*)s_vk_ptr + 16);
                void** gipa_slot = (void**)((BYTE*)s_vk_ptr + 24);
                void* sl_gdpa = (void*)s_sl_vkGetDeviceProcAddrProxy;
                void* sl_gipa = (void*)s_sl_vkGetInstanceProcAddrProxy;

                fprintf(stderr, "[DLSS] CR53: s_vk @ %p  gdpa %p -> %p  gipa %p -> %p\n",
                    s_vk_ptr, *gdpa_slot, sl_gdpa, *gipa_slot, sl_gipa);
                sl_log_printf("[DLSS] CR53: patching s_vk.getDeviceProcAddr %p -> %p (sl.interposer own)\n",
                    *gdpa_slot, sl_gdpa);

                *gdpa_slot = sl_gdpa;
                if (sl_gipa)
                    *gipa_slot = sl_gipa;
            } else {
                fprintf(stderr, "[DLSS] CR53: get(kVulkanTable) returned got=%d ptr=%p - patch skipped\n",
                    (int)got, s_vk_ptr);
            }
        } else {
            fprintf(stderr, "[DLSS] CR53: slGetParameters() returned null - patch skipped\n");
        }
    }

    /* Fetch SL-hooked Vulkan function pointers via sl.interposer.dll's own vkGetDeviceProcAddr.
     * Per ProgrammingGuideManualHooking.md §4.2: sl.interposer exports its own
     * vkGetDeviceProcAddr that returns SL's intercepted versions for the functions in
     * sl_hooks.h (eVulkan_Present, eVulkan_CreateSwapchainKHR, etc.) and delegates to
     * vulkan-1.dll for everything else.
     * Using the stock vulkan-1.dll vkGetDeviceProcAddr would return non-hooked pointers
     * and presentCommon() would never fire → GC accumulates → crash on exit + no MFG. */
    PFN_vkGetDeviceProcAddr getDeviceProc =
        s_sl_vkGetDeviceProcAddrProxy ? s_sl_vkGetDeviceProcAddrProxy : vkGetDeviceProcAddr;

    s_sl_vkCreateSwapchainKHR    = (PFN_vkCreateSwapchainKHR)   getDeviceProc(device, "vkCreateSwapchainKHR");
    s_sl_vkDestroySwapchainKHR   = (PFN_vkDestroySwapchainKHR)  getDeviceProc(device, "vkDestroySwapchainKHR");
    s_sl_vkGetSwapchainImagesKHR = (PFN_vkGetSwapchainImagesKHR)getDeviceProc(device, "vkGetSwapchainImagesKHR");
    s_sl_vkQueuePresentKHR       = (PFN_vkQueuePresentKHR)      getDeviceProc(device, "vkQueuePresentKHR");
    s_sl_vkAcquireNextImageKHR   = (PFN_vkAcquireNextImageKHR)  getDeviceProc(device, "vkAcquireNextImageKHR");
    s_sl_vkDeviceWaitIdle        = (PFN_vkDeviceWaitIdle)        getDeviceProc(device, "vkDeviceWaitIdle");
    s_vkWaitSemaphoresNative     = (PFN_vkWaitSemaphores)       vkGetDeviceProcAddr(device, "vkWaitSemaphores");
    /* vkDestroySurfaceKHR is instance-level, not device-level */
    s_sl_vkDestroySurfaceKHR     = (PFN_vkDestroySurfaceKHR)    vkGetInstanceProcAddr(instance, "vkDestroySurfaceKHR");

    fprintf(stderr, "[DLSS] SL proxy vkGetDeviceProcAddr: %s\n",
            s_sl_vkGetDeviceProcAddrProxy ? "from sl.interposer.dll" : "FALLBACK to vulkan-1.dll");
    fprintf(stderr, "[DLSS] s_sl_vkQueuePresentKHR = %p  (vulkan native = %p)\n",
            (void*)s_sl_vkQueuePresentKHR, (void*)vkGetDeviceProcAddr(device, "vkQueuePresentKHR"));

    /* Initialise Reflex function pointers via slGetFeatureFunction */
    slGetFeatureFunction(sl::kFeatureDLSS_RR, "slDLSSDGetOptimalSettings", (void*&)pfn_slDLSSDGetOptimalSettings);
    slGetFeatureFunction(sl::kFeatureDLSS_RR, "slDLSSDSetOptions",         (void*&)pfn_slDLSSDSetOptions);
    slGetFeatureFunction(sl::kFeatureDLSS_RR, "slDLSSDGetState",           (void*&)pfn_slDLSSDGetState);
    slGetFeatureFunction(sl::kFeatureReflex, "slReflexSetOptions", (void*&)pfn_slReflexSetOptions);
    slGetFeatureFunction(sl::kFeatureReflex, "slReflexSleep",      (void*&)pfn_slReflexSleep);
    slGetFeatureFunction(sl::kFeatureReflex, "slReflexGetState",   (void*&)pfn_slReflexGetState);
    slGetFeatureFunction(sl::kFeatureDLSS_G, "slDLSSGGetState",    (void*&)pfn_slDLSSGGetState);
    slGetFeatureFunction(sl::kFeatureCommon, "slHookVkCmdBindPipeline",       (void*&)pfn_slHookVkCmdBindPipeline);
    slGetFeatureFunction(sl::kFeatureCommon, "slHookVkCmdBindDescriptorSets", (void*&)pfn_slHookVkCmdBindDescriptorSets);
    slGetFeatureFunction(sl::kFeatureCommon, "slHookVkBeginCommandBuffer",    (void*&)pfn_slHookVkBeginCommandBuffer);

    /* Initialise PCL marker function pointer.
     * slPCLSetMarker(ePresentStart/End) is required by DLSS-G to match the vkQueuePresentKHR
     * call to the correct frame token, enabling lookup of common constants at present time.
     * Without these markers, DLSS-G logs "missing common constants" and disables FG every frame. */
    slGetFeatureFunction(sl::kFeaturePCL, "slPCLSetMarker", (void*&)pfn_slPCLSetMarker);
    fprintf(stdout, "[DLSS-G] slPCLSetMarker: %s\n", pfn_slPCLSetMarker ? "loaded" : "MISSING");
    sl_log_printf("[SL common] hooks: BeginCommandBuffer=%s CmdBindPipeline=%s CmdBindDescriptorSets=%s\n",
        pfn_slHookVkBeginCommandBuffer ? "loaded" : "missing",
        pfn_slHookVkCmdBindPipeline ? "loaded" : "missing",
        pfn_slHookVkCmdBindDescriptorSets ? "loaded" : "missing");

    sl::AdapterInfo ai{};
    ai.vkPhysicalDevice = phys_device;

    sl::Result res_sr  = slIsFeatureSupported(sl::kFeatureDLSS,    ai);
    sl::Result res_rr  = slIsFeatureSupported(sl::kFeatureDLSS_RR, ai);
    sl::Result res_mfg = slIsFeatureSupported(sl::kFeatureDLSS_G,  ai);
    g_dlss_sl_sr_result   = (int)res_sr;
    g_dlss_sl_rr_result   = (int)res_rr;
    g_dlss_sl_g_result    = (int)res_mfg;
    g_dlss_sl_available   = (res_sr  == sl::Result::eOk) ? 1 : 0;
    g_dlss_sl_rr_available = (res_rr == sl::Result::eOk) ? 1 : 0;
    g_dlss_sl_g_available = (res_mfg == sl::Result::eOk) ? 1 : 0;

    if (g_dlss_sl_available) {
        sl::Result r = slAllocateResources(nullptr, sl::kFeatureDLSS, s_vp);
        s_dlss_resources_allocated = (r == sl::Result::eOk);
        if (!s_dlss_resources_allocated)
            fprintf(stderr, "[DLSS] slAllocateResources(DLSS) failed: %d\n", (int)r);
    }
    if (g_dlss_sl_rr_available) {
        /* Do not preallocate DLSS-RR here.
         *
         * RR feature creation in Streamline needs current-frame global tags
         * (ScalingInputColor / Depth / MotionVectors / etc).  Calling
         * slAllocateResources() during startup happens before those tags exist,
         * so the preallocation path fails and then our own guard can suppress
         * all later RR tagging.  Let slEvaluateFeature create the RR feature
         * lazily on the first real frame after tags/constants/options are set.
         */
        s_dlss_rr_resources_allocated = true;
    }
    /* DLSS-G requires the swapchain to exist — do NOT call slAllocateResources here.
     * Calling it before swapchain creation causes an exception in the SL runtime.
     * DLSS-G allocation is deferred to Phase 2 (MFG swapchain integration). */
    (void)s_dlss_g_resources_allocated;
}

/* ======================================================================
 * Optimal settings
 * ====================================================================*/

void dlss_sl_get_optimal_settings(int mode,
    uint32_t display_w, uint32_t display_h,
    uint32_t *render_w,  uint32_t *render_h)
{
    static int      s_cached_mode = -1;
    static uint32_t s_cached_display_w = 0;
    static uint32_t s_cached_display_h = 0;
    static uint32_t s_cached_render_w = 0;
    static uint32_t s_cached_render_h = 0;
    static bool     s_cached_valid = false;

    *render_w = display_w;
    *render_h = display_h;
    if (!s_sl_loaded || !g_dlss_sl_available) return;

    if (s_cached_valid &&
        s_cached_mode == mode &&
        s_cached_display_w == display_w &&
        s_cached_display_h == display_h) {
        *render_w = s_cached_render_w;
        *render_h = s_cached_render_h;
        return;
    }

    uint32_t resolved_render_w = 0;
    uint32_t resolved_render_h = 0;
    bool ok = get_dlss_optimal_settings_for_mode(mode, display_w, display_h,
                                                 &resolved_render_w, &resolved_render_h);
    if (!ok && mode == 5) {
        resolve_supported_dlss_mode(mode, display_w, display_h,
                                    &resolved_render_w, &resolved_render_h);
        ok = resolved_render_w != 0 && resolved_render_h != 0;
    }

    if (ok) {
        *render_w = resolved_render_w;
        *render_h = resolved_render_h;
        s_cached_mode = mode;
        s_cached_display_w = display_w;
        s_cached_display_h = display_h;
        s_cached_render_w = *render_w;
        s_cached_render_h = *render_h;
        s_cached_valid = true;
    }
}

void dlss_sl_set_vulkan_info(
    VkInstance       instance,
    VkPhysicalDevice phys_device,
    VkDevice         device,
    uint32_t         gfx_queue_family, uint32_t gfx_queue_index,
    uint32_t         compute_queue_family, uint32_t compute_queue_index,
    uint32_t         optical_flow_queue_family, uint32_t optical_flow_queue_index,
    int              use_native_optical_flow)
{
    dlss_sl_complete_vulkan_setup(
        instance,
        phys_device,
        device,
        gfx_queue_family, gfx_queue_index,
        compute_queue_family, compute_queue_index,
        optical_flow_queue_family, optical_flow_queue_index,
        use_native_optical_flow,
        1);
}

void dlss_sl_finish_proxy_vulkan_setup(
    VkInstance instance,
    VkPhysicalDevice phys_device,
    VkDevice device)
{
    dlss_sl_complete_vulkan_setup(
        instance,
        phys_device,
        device,
        0u, 0u,
        0u, 0u,
        0u, 0u,
        0,
        0);
}

void dlss_sl_rr_get_optimal_settings(int mode, int preset,
    uint32_t display_w, uint32_t display_h,
    uint32_t *render_w, uint32_t *render_h)
{
    static int      s_cached_mode = -1;
    static int      s_cached_preset = -1;
    static uint32_t s_cached_display_w = 0;
    static uint32_t s_cached_display_h = 0;
    static uint32_t s_cached_render_w = 0;
    static uint32_t s_cached_render_h = 0;
    static bool     s_cached_valid = false;

    *render_w = display_w;
    *render_h = display_h;
    if (!s_sl_loaded || !g_dlss_sl_rr_available || !pfn_slDLSSDGetOptimalSettings)
        return;

    if (s_cached_valid &&
        s_cached_mode == mode &&
        s_cached_preset == preset &&
        s_cached_display_w == display_w &&
        s_cached_display_h == display_h) {
        *render_w = s_cached_render_w;
        *render_h = s_cached_render_h;
        return;
    }

    uint32_t resolved_render_w = 0;
    uint32_t resolved_render_h = 0;
    bool ok = get_dlss_rr_optimal_settings_for_mode(mode, preset, display_w, display_h,
                                                    &resolved_render_w, &resolved_render_h);
    if (!ok && mode == 5) {
        resolve_supported_dlss_rr_mode(mode, preset, display_w, display_h,
                                       &resolved_render_w, &resolved_render_h);
        ok = resolved_render_w != 0 && resolved_render_h != 0;
    }

    if (ok) {
        *render_w = resolved_render_w;
        *render_h = resolved_render_h;
        s_cached_mode = mode;
        s_cached_preset = preset;
        s_cached_display_w = display_w;
        s_cached_display_h = display_h;
        s_cached_render_w = *render_w;
        s_cached_render_h = *render_h;
        s_cached_valid = true;
    }
}

/* ======================================================================
 * Per-viewport options
 * ====================================================================*/

void dlss_sl_set_options(int mode, int preset,
    uint32_t render_w, uint32_t render_h,
    uint32_t out_w, uint32_t out_h, int hdr,
    float sharpness, int auto_exposure)
{
    if (!s_sl_loaded || !g_dlss_sl_available) return;
    (void)render_w; (void)render_h; /* reserved: not in DLSSOptions v2.8 */

    sl::DLSSPreset p = map_preset(preset);
    int effective_mode = mode;
    if (mode == 5) {
        uint32_t ignored_w = 0, ignored_h = 0;
        effective_mode = resolve_supported_dlss_mode(mode, out_w, out_h, &ignored_w, &ignored_h);
    }

    sl::DLSSOptions opts{};
    opts.mode                   = map_mode(effective_mode);
    opts.outputWidth            = out_w;
    opts.outputHeight           = out_h;
    opts.sharpness              = sharpness;
    opts.colorBuffersHDR        = hdr ? sl::Boolean::eTrue : sl::Boolean::eFalse;
    opts.useAutoExposure        = auto_exposure ? sl::Boolean::eTrue : sl::Boolean::eFalse;
    opts.dlaaPreset             = p;
    opts.qualityPreset          = p;
    opts.balancedPreset         = p;
    opts.performancePreset      = p;
    opts.ultraPerformancePreset = p;
    opts.ultraQualityPreset     = p;

    slDLSSSetOptions(s_vp, opts);
}

void dlss_sl_rr_set_options(int mode, int preset,
    uint32_t out_w, uint32_t out_h,
    const float *world_to_camera_view,
    const float *camera_view_to_world)
{
    if (!s_sl_loaded || !g_dlss_sl_rr_available || !pfn_slDLSSDSetOptions)
        return;

    int effective_mode = mode;
    if (mode == 5) {
        uint32_t ignored_w = 0, ignored_h = 0;
        effective_mode = resolve_supported_dlss_rr_mode(mode, preset, out_w, out_h, &ignored_w, &ignored_h);
    }

    sl::DLSSDOptions opts{};
    opts.mode = map_mode(effective_mode);
    opts.outputWidth = out_w;
    opts.outputHeight = out_h;
    /* RR is fed from the HDR render path, not from the SDR swapchain. */
    opts.colorBuffersHDR = sl::Boolean::eTrue;
    opts.normalRoughnessMode = sl::DLSSDNormalRoughnessMode::eUnpacked;
    opts.dlaaPreset = map_rr_preset(preset);
    opts.qualityPreset = map_rr_preset(preset);
    opts.balancedPreset = map_rr_preset(preset);
    opts.performancePreset = map_rr_preset(preset);
    opts.ultraPerformancePreset = map_rr_preset(preset);
    opts.ultraQualityPreset = map_rr_preset(preset);
    transpose4x4(world_to_camera_view, opts.worldToCameraView);
    transpose4x4(camera_view_to_world, opts.cameraViewToWorld);

    sl::Result res = pfn_slDLSSDSetOptions(s_vp, opts);
    if (res != sl::Result::eOk) {
        fprintf(stderr, "[DLSS-RR] slDLSSDSetOptions failed: %d\n", (int)res);
        sl_log_printf("[DLSS-RR] slDLSSDSetOptions failed: %d\n", (int)res);
    }
}

void dlss_sl_rr_tag_resources(
    VkCommandBuffer cmd_buf,
    VkImage color_in, VkImageView color_in_view, uint32_t layout_color_in, uint32_t fmt_color_in,
    VkImage depth, VkImageView depth_view, uint32_t layout_depth, uint32_t fmt_depth,
    VkImage mvec, VkImageView mvec_view, uint32_t layout_mvec, uint32_t fmt_mvec,
    VkImage albedo, VkImageView albedo_view, uint32_t layout_albedo, uint32_t fmt_albedo,
    VkImage specular_albedo, VkImageView specular_albedo_view, uint32_t layout_specular_albedo, uint32_t fmt_specular_albedo,
    VkImage normals, VkImageView normals_view, uint32_t layout_normals, uint32_t fmt_normals,
    VkImage roughness, VkImageView roughness_view, uint32_t layout_roughness, uint32_t fmt_roughness,
    VkImage color_before_transparency, VkImageView color_before_transparency_view, uint32_t layout_color_before_transparency, uint32_t fmt_color_before_transparency,
    VkImage spec_motion, VkImageView spec_motion_view, uint32_t layout_spec_motion, uint32_t fmt_spec_motion,
    VkImage spec_hit_dist, VkImageView spec_hit_dist_view, uint32_t layout_spec_hit_dist, uint32_t fmt_spec_hit_dist,
    VkImage spec_ray_dir_hit_dist, VkImageView spec_ray_dir_hit_dist_view, uint32_t layout_spec_ray_dir_hit_dist, uint32_t fmt_spec_ray_dir_hit_dist,
    VkImage color_out, VkImageView color_out_view, uint32_t layout_color_out, uint32_t fmt_color_out,
    uint32_t input_alloc_w, uint32_t input_alloc_h,
    uint32_t render_w, uint32_t render_h,
    uint32_t output_alloc_w, uint32_t output_alloc_h)
{
    if (!s_sl_loaded || !g_dlss_sl_rr_available || !s_frame_token)
        return;

    sl::Resource r_in(sl::ResourceType::eTex2d, (void*)color_in, nullptr, (void*)color_in_view, layout_color_in);
    sl::Resource r_depth(sl::ResourceType::eTex2d, (void*)depth, nullptr, (void*)depth_view, layout_depth);
    sl::Resource r_mv(sl::ResourceType::eTex2d, (void*)mvec, nullptr, (void*)mvec_view, layout_mvec);
    sl::Resource r_albedo(sl::ResourceType::eTex2d, (void*)albedo, nullptr, (void*)albedo_view, layout_albedo);
    sl::Resource r_spec_albedo(sl::ResourceType::eTex2d, (void*)specular_albedo, nullptr, (void*)specular_albedo_view, layout_specular_albedo);
    sl::Resource r_normals(sl::ResourceType::eTex2d, (void*)normals, nullptr, (void*)normals_view, layout_normals);
    sl::Resource r_roughness(sl::ResourceType::eTex2d, (void*)roughness, nullptr, (void*)roughness_view, layout_roughness);
    sl::Resource r_color_before_transparency(sl::ResourceType::eTex2d, (void*)color_before_transparency, nullptr, (void*)color_before_transparency_view, layout_color_before_transparency);
    sl::Resource r_spec_motion(sl::ResourceType::eTex2d, (void*)spec_motion, nullptr, (void*)spec_motion_view, layout_spec_motion);
    sl::Resource r_spec_hit_dist(sl::ResourceType::eTex2d, (void*)spec_hit_dist, nullptr, (void*)spec_hit_dist_view, layout_spec_hit_dist);
    sl::Resource r_spec_ray_dir_hit_dist(sl::ResourceType::eTex2d, (void*)spec_ray_dir_hit_dist, nullptr, (void*)spec_ray_dir_hit_dist_view, layout_spec_ray_dir_hit_dist);
    sl::Resource r_out(sl::ResourceType::eTex2d, (void*)color_out, nullptr, (void*)color_out_view, layout_color_out);

    r_in.nativeFormat = fmt_color_in;
    r_depth.nativeFormat = fmt_depth;
    r_mv.nativeFormat = fmt_mvec;
    r_albedo.nativeFormat = fmt_albedo;
    r_spec_albedo.nativeFormat = fmt_specular_albedo;
    r_normals.nativeFormat = fmt_normals;
    r_roughness.nativeFormat = fmt_roughness;
    r_color_before_transparency.nativeFormat = fmt_color_before_transparency;
    r_spec_motion.nativeFormat = fmt_spec_motion;
    r_spec_hit_dist.nativeFormat = fmt_spec_hit_dist;
    r_spec_ray_dir_hit_dist.nativeFormat = fmt_spec_ray_dir_hit_dist;
    r_out.nativeFormat = fmt_color_out;

    r_in.width = input_alloc_w; r_in.height = input_alloc_h;
    r_depth.width = input_alloc_w; r_depth.height = input_alloc_h;
    r_mv.width = input_alloc_w; r_mv.height = input_alloc_h;
    r_albedo.width = input_alloc_w; r_albedo.height = input_alloc_h;
    r_spec_albedo.width = input_alloc_w; r_spec_albedo.height = input_alloc_h;
    r_normals.width = input_alloc_w; r_normals.height = input_alloc_h;
    r_roughness.width = input_alloc_w; r_roughness.height = input_alloc_h;
    r_color_before_transparency.width = input_alloc_w; r_color_before_transparency.height = input_alloc_h;
    r_spec_motion.width = input_alloc_w; r_spec_motion.height = input_alloc_h;
    r_spec_hit_dist.width = input_alloc_w; r_spec_hit_dist.height = input_alloc_h;
    r_spec_ray_dir_hit_dist.width = input_alloc_w; r_spec_ray_dir_hit_dist.height = input_alloc_h;
    r_out.width = output_alloc_w; r_out.height = output_alloc_h;

    sl::Extent in_ext = { 0, 0, render_w, render_h };
    sl::Extent out_ext = { 0, 0, output_alloc_w, output_alloc_h };

    constexpr sl::ResourceLifecycle kRRRequiredLifecycle =
        sl::ResourceLifecycle::eValidUntilEvaluate;

    sl::ResourceTag tags[] = {
        /* Mirrors and highly specular reflective surfaces benefit strongly from
           the explicit specular hit guides. Q2RTX already prepares these RR
           inputs in dlss_rr_prep.comp and also provides the required
           worldToCameraView / cameraViewToWorld matrices via DLSSDOptions.
           Leaving them untagged degrades RR on reflective surfaces even though
           the data is available. */
        sl::ResourceTag(&r_in, sl::kBufferTypeScalingInputColor, kRRRequiredLifecycle, &in_ext),
        sl::ResourceTag(&r_depth, sl::kBufferTypeLinearDepth, kRRRequiredLifecycle, &in_ext),
        sl::ResourceTag(&r_mv, sl::kBufferTypeMotionVectors, kRRRequiredLifecycle, &in_ext),
        sl::ResourceTag(&r_albedo, sl::kBufferTypeAlbedo, kRRRequiredLifecycle, &in_ext),
        sl::ResourceTag(&r_spec_albedo, sl::kBufferTypeSpecularAlbedo, kRRRequiredLifecycle, &in_ext),
        sl::ResourceTag(&r_normals, sl::kBufferTypeNormals, kRRRequiredLifecycle, &in_ext),
        sl::ResourceTag(&r_roughness, sl::kBufferTypeRoughness, kRRRequiredLifecycle, &in_ext),
        sl::ResourceTag(&r_color_before_transparency, sl::kBufferTypeColorBeforeTransparency, kRRRequiredLifecycle, &in_ext),
        sl::ResourceTag(&r_spec_motion, sl::kBufferTypeSpecularMotionVectors, kRRRequiredLifecycle, &in_ext),
        sl::ResourceTag(&r_out, sl::kBufferTypeScalingOutputColor, kRRRequiredLifecycle, &out_ext),
    };

    sl::Result res = slSetTagForFrame(*s_frame_token, s_vp, tags, 10,
        reinterpret_cast<sl::CommandBuffer*>(cmd_buf));
    if (res != sl::Result::eOk) {
        fprintf(stderr, "[DLSS-RR] slSetTagForFrame failed: %d\n", (int)res);
        sl_log_printf("[DLSS-RR] slSetTagForFrame failed: %d\n", (int)res);
    }
}

void dlss_sl_rr_evaluate(VkCommandBuffer cmd_buf)
{
    if (!s_sl_loaded || !g_dlss_sl_rr_available || !s_frame_token)
        return;

    const sl::BaseStructure* inputs[] = {
        reinterpret_cast<const sl::BaseStructure*>(&s_vp)
    };

    sl::Result res = slEvaluateFeature(
        sl::kFeatureDLSS_RR, *s_frame_token, inputs, 1,
        reinterpret_cast<sl::CommandBuffer*>(cmd_buf));
    if (res != sl::Result::eOk) {
        fprintf(stderr, "[DLSS-RR] slEvaluateFeature failed: %d\n", (int)res);
        sl_log_printf("[DLSS-RR] slEvaluateFeature failed: %d\n", (int)res);
    }
}

/* ======================================================================
 * DLSS-G / MFG
 * ====================================================================*/

void dlss_sl_set_g_options(int mfg_mode,
    uint32_t color_w, uint32_t color_h,
    uint32_t mvec_w,  uint32_t mvec_h,
    uint32_t num_backbuffers,
    uint32_t color_fmt,
    uint32_t mvec_fmt,
    uint32_t depth_fmt,
    uint32_t hudless_fmt,
    uint32_t ui_fmt,
    int dynamic_resolution)
{
    if (!s_sl_loaded || !g_dlss_sl_g_available) return;

    /* When MFG is off and we have never called slDLSSGSetOptions, skip entirely.
     * DLSS-G plugin v2.10.3 (OTA) activates its present hook as soon as it receives
     * any slDLSSGSetOptions call — even eOff — and then crashes on the first present
     * if Reflex was not yet active at swapchain-creation time (CR23). Avoiding the
     * call keeps the plugin fully passive when the user does not want frame generation.
     * Once MFG is turned on the first eOn call activates the plugin normally. */
    static bool s_g_options_ever_set = false;
    static bool s_g_options_valid = false;
    static sl::DLSSGOptions s_last_g_options{};
    if (mfg_mode == 0 && !s_g_options_ever_set) return;
    s_g_options_ever_set = true;

    sl::DLSSGOptions opts{};
    /* Keep eOff until at least one tag call has happened since the last swapchain
     * create/recreate — otherwise DLSS-G dereferences unset resource slots → crash. */
    if (mfg_mode == 0 || !s_g_tags_valid) {
        opts.mode = sl::DLSSGMode::eOff;
    } else {
        opts.mode                = sl::DLSSGMode::eOn;
        opts.numFramesToGenerate = (uint32_t)(mfg_mode - 1); /* 2X=1, 3X=2, 4X=3 */
    }
    opts.colorWidth           = color_w;
    opts.colorHeight          = color_h;
    opts.mvecDepthWidth       = mvec_w;
    opts.mvecDepthHeight      = mvec_h;
    opts.numBackBuffers       = num_backbuffers;
    opts.colorBufferFormat    = color_fmt;
    opts.mvecBufferFormat     = mvec_fmt;
    opts.depthBufferFormat    = depth_fmt;
    opts.hudLessBufferFormat  = hudless_fmt;
    opts.uiBufferFormat       = ui_fmt;
    if (dynamic_resolution)
        opts.flags = sl::DLSSGFlags::eDynamicResolutionEnabled;
    /* Q2RTX currently submits almost all render work through a single presenting
     * graphics queue. NVIDIA's guide notes that eBlockNoClientQueues is mainly
     * beneficial for workloads with meaningful multi-queue parallelism. On this
     * single-queue path it tends to over-constrain cadence because the host then
     * must explicitly wait on the inputs-processing fence every frame. Keep the
     * default mode instead. */

    if (s_g_options_valid && memcmp(&opts, &s_last_g_options, sizeof(opts)) == 0)
        return;

    sl::Result r = slDLSSGSetOptions(s_vp, opts);
    if (r != sl::Result::eOk) {
        fprintf(stderr, "[DLSS-G] slDLSSGSetOptions failed: %d\n", (int)r);
    } else {
        s_last_g_options = opts;
        s_g_options_valid = true;
    }
    /* NOTE: slDLSSGGetState removed — per SL docs it must be called from the
     * present thread only; calling it here (outside present) triggered the
     * "slDLSSGGetState must be synchronized with the present thread" warning
     * and likely contributed to CR37 state corruption at second present. */
}

/* ======================================================================
 * SL-patched Vulkan wrappers — call these instead of the bare vk* symbols
 * so Streamline can intercept presents (presentCommon / GC) and swapchain
 * lifecycle events (DLSS-G frame generation setup).
 * ====================================================================*/

static void update_dlssg_present_state(void)
{
    if (!s_sl_loaded || !pfn_slDLSSGGetState)
        return;

    sl::DLSSGState state{};
    sl::Result r = pfn_slDLSSGGetState(s_vp, state, nullptr);
    if (r != sl::Result::eOk)
        return;

    s_mfg_cap = (state.numFramesToGenerateMax >= 3) ? 4 :
                (state.numFramesToGenerateMax >= 1) ? 2 : 0;
    s_last_dlssg_status = (int)state.status;
    s_last_dlssg_frames_presented = (int)state.numFramesActuallyPresented;
    s_last_dlssg_vsync_support = (state.bIsVsyncSupportAvailable == sl::Boolean::eTrue) ? 1 :
                                 (state.bIsVsyncSupportAvailable == sl::Boolean::eFalse) ? 0 : -1;

    if (state.inputsProcessingCompletionFence &&
        state.lastPresentInputsProcessingCompletionFenceValue > 0) {
        s_dlssg_inputs_fence =
            reinterpret_cast<VkSemaphore>(state.inputsProcessingCompletionFence);
        s_dlssg_inputs_fence_value =
            state.lastPresentInputsProcessingCompletionFenceValue;
    }

    s_display_multiplier = (state.numFramesActuallyPresented > 0)
        ? (int)state.numFramesActuallyPresented
        : 1;

    s_presented_frames_accum += (uint64_t)s_display_multiplier;
    s_presented_frames_total += (uint64_t)s_display_multiplier;
    auto now = std::chrono::steady_clock::now();
    auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now - s_display_fps_window_start).count();

    if (elapsed_ms >= 250) {
        s_display_fps = (int)llround((double)s_presented_frames_accum * 1000.0 / (double)elapsed_ms);
        s_presented_frames_accum = 0;
        s_display_fps_window_start = now;
    }
}

void dlss_sl_wait_for_g_inputs_consumed(void)
{
    if (!s_sl_loaded || !s_vkWaitSemaphoresNative)
        return;
    if (s_dlssg_inputs_fence == VK_NULL_HANDLE || s_dlssg_inputs_fence_value == 0)
        return;
    if (s_dlssg_inputs_fence_waited_value >= s_dlssg_inputs_fence_value)
        return;

    VkSemaphore semaphore = s_dlssg_inputs_fence;
    uint64_t value = s_dlssg_inputs_fence_value;
    VkSemaphoreWaitInfo wait_info{};
    wait_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO;
    wait_info.pNext = nullptr;
    wait_info.flags = 0;
    wait_info.semaphoreCount = 1;
    wait_info.pSemaphores = &semaphore;
    wait_info.pValues = &value;

    VkResult res = s_vkWaitSemaphoresNative(s_vk_device, &wait_info, UINT64_MAX);
    if (res == VK_SUCCESS) {
        s_dlssg_inputs_fence_waited_value = value;
    } else {
        fprintf(stderr, "[DLSS-G] vkWaitSemaphores(inputs fence) failed: %d\n", (int)res);
        sl_log_printf("[DLSS-G] vkWaitSemaphores(inputs fence) failed: %d\n", (int)res);
    }
}

extern "C" int dlss_sl_get_g_inputs_fence_wait(VkSemaphore *sem, uint64_t *value)
{
    if (!s_sl_loaded || !sem || !value)
        return 0;
    if (s_dlssg_inputs_fence == VK_NULL_HANDLE || s_dlssg_inputs_fence_value == 0)
        return 0;
    if (s_dlssg_inputs_fence_waited_value >= s_dlssg_inputs_fence_value)
        return 0;

    *sem = s_dlssg_inputs_fence;
    *value = s_dlssg_inputs_fence_value;
    return 1;
}

extern "C" void dlss_sl_mark_g_inputs_fence_waited(uint64_t value)
{
    if (value > s_dlssg_inputs_fence_waited_value)
        s_dlssg_inputs_fence_waited_value = value;
}

VkResult dlss_sl_vkCreateSwapchainKHR(VkDevice device, const VkSwapchainCreateInfoKHR *ci,
                                       const VkAllocationCallbacks *alloc, VkSwapchainKHR *sw)
{
    if (s_sl_vkCreateSwapchainKHR) return s_sl_vkCreateSwapchainKHR(device, ci, alloc, sw);
    return vkCreateSwapchainKHR(device, ci, alloc, sw);
}

void dlss_sl_vkDestroySwapchainKHR(VkDevice device, VkSwapchainKHR sw,
                                    const VkAllocationCallbacks *alloc)
{
    if (s_sl_vkDestroySwapchainKHR) s_sl_vkDestroySwapchainKHR(device, sw, alloc);
    else                            vkDestroySwapchainKHR(device, sw, alloc);
}

VkResult dlss_sl_vkGetSwapchainImagesKHR(VkDevice device, VkSwapchainKHR sw,
                                          uint32_t *count, VkImage *images)
{
    if (s_sl_vkGetSwapchainImagesKHR) return s_sl_vkGetSwapchainImagesKHR(device, sw, count, images);
    return vkGetSwapchainImagesKHR(device, sw, count, images);
}

VkResult dlss_sl_vkQueuePresentKHR(VkQueue queue, const VkPresentInfoKHR *info)
{
    /* PCL present markers: required by DLSS-G to correlate this vkQueuePresentKHR call with the
     * frame token that was used in slSetConstants() earlier in the frame.  Without ePresentStart
     * before the present, sl.dlss_g cannot locate the common constants for the frame being
     * presented and logs "missing common constants — Frame Generation will be disabled for the
     * frame" on every present, causing MFG to silently no-op (confirmed: FPS identical at
     * MFG=0 and MFG=2 before this fix).  The markers must bracket the SL-intercepted present. */
    if (pfn_slPCLSetMarker && s_frame_token)
        pfn_slPCLSetMarker(sl::PCLMarker::ePresentStart, *s_frame_token);

    VkResult res;
    if (s_sl_vkQueuePresentKHR)
        res = s_sl_vkQueuePresentKHR(queue, info);
    else
        res = vkQueuePresentKHR(queue, info);

    if (pfn_slPCLSetMarker && s_frame_token)
        pfn_slPCLSetMarker(sl::PCLMarker::ePresentEnd, *s_frame_token);

    update_dlssg_present_state();

    return res;
}

VkResult dlss_sl_vkAcquireNextImageKHR(VkDevice device, VkSwapchainKHR swapchain,
                                        uint64_t timeout, VkSemaphore semaphore,
                                        VkFence fence, uint32_t *pImageIndex)
{
    if (s_sl_vkAcquireNextImageKHR)
        return s_sl_vkAcquireNextImageKHR(device, swapchain, timeout, semaphore, fence, pImageIndex);
    return vkAcquireNextImageKHR(device, swapchain, timeout, semaphore, fence, pImageIndex);
}

VkResult dlss_sl_vkDeviceWaitIdle(VkDevice device)
{
    if (s_sl_vkDeviceWaitIdle) return s_sl_vkDeviceWaitIdle(device);
    return vkDeviceWaitIdle(device);
}

void dlss_sl_vkDestroySurfaceKHR(VkInstance instance, VkSurfaceKHR surface,
                                  const VkAllocationCallbacks *pAllocator)
{
    if (s_sl_vkDestroySurfaceKHR) s_sl_vkDestroySurfaceKHR(instance, surface, pAllocator);
    else                          vkDestroySurfaceKHR(instance, surface, pAllocator);
}

/* ======================================================================
 * DLSS-G resource allocation (call after swapchain creation)
 * ====================================================================*/

void dlss_sl_alloc_g_resources(void)
{
    /* DLSS-G does NOT need explicit slAllocateResources — SL allocates all
     * frame-gen resources internally inside the vkCreateSwapchainKHR hook
     * (sl.dlfg allocates ~500 MB of VRAM for generated frames automatically).
     * Calling slAllocateResources(DLSS_G) on top of that causes an exception
     * inside the SL runtime (crash observed in CrashReport12/sl_debug.log).
     * Mark as "allocated" so callers do not retry. */
    s_dlss_g_resources_allocated = true;
    s_g_tags_valid = false;   /* reset: require at least one tag call before enabling eOn */
    fprintf(stdout, "[DLSS-G] Resources managed by SL swapchain hook — no explicit alloc needed\n");
}

/* ======================================================================
 * DLSS-G per-frame resource tagging (call before present each frame)
 * ====================================================================*/

void dlss_sl_tag_g_resources(
    VkCommandBuffer cmd_buf,
    VkImage depth,     VkImageView depth_view,  uint32_t layout_depth,  uint32_t fmt_depth,
    VkImage mvec,      VkImageView mvec_view,   uint32_t layout_mvec,   uint32_t fmt_mvec,
    VkImage hudless,   VkImageView hudless_view,uint32_t layout_hudless,uint32_t fmt_hudless,
    uint32_t render_w, uint32_t render_h,
    uint32_t display_w, uint32_t display_h,
    uint32_t backbuffer_x, uint32_t backbuffer_y)
{
    if (!s_sl_loaded || !g_dlss_sl_g_available || !s_dlss_g_resources_allocated) return;
    if (!s_frame_token) return;

    s_g_tags_valid = true;   /* resources tagged — eOn safe for this and subsequent frames */
    const bool has_hudless = hudless != VK_NULL_HANDLE && hudless_view != VK_NULL_HANDLE && fmt_hudless != 0;

    sl::Resource r_dep(sl::ResourceType::eTex2d, (void*)depth,   nullptr, (void*)depth_view,   layout_depth);
    sl::Resource r_mv (sl::ResourceType::eTex2d, (void*)mvec,    nullptr, (void*)mvec_view,    layout_mvec);
    sl::Resource r_hud(sl::ResourceType::eTex2d, (void*)hudless, nullptr, (void*)hudless_view, layout_hudless);

    r_dep.nativeFormat = fmt_depth;
    r_mv.nativeFormat  = fmt_mvec;
    r_hud.nativeFormat = fmt_hudless;

    /* Resource dimensions must match the actual VkImage allocation size.
     *
     * In Q2RTX the PT depth/mvec images are allocated with IMG_WIDTH/IMG_HEIGHT,
     * which map to qvk.extent_screen_images. With DLSS SR active and DRS off,
     * extent_screen_images becomes max(extent_render, extent_unscaled), i.e.
     * effectively display-sized. Only the valid subrect inside those images is
     * render-sized. Declaring them as render_w/render_h makes the mismatch grow
     * as the upscale ratio increases and can push DLSS-G into an unnecessarily
     * expensive path. Keep the allocation dimensions here and describe the
     * actual rendered region via render_ext below. HUDless color remains a
     * display-sized resource as before. */
    r_dep.width = display_w; r_dep.height = display_h;
    r_mv.width  = display_w; r_mv.height  = display_h;
    r_hud.width = display_w; r_hud.height = display_h;

    sl::Extent render_ext     = { 0, 0, render_w,  render_h  };
    sl::Extent display_ext    = { 0, 0, display_w, display_h };
    sl::Extent backbuffer_ext = { backbuffer_x, backbuffer_y, display_w, display_h };

    /* Start with the DLSS-G-recommended immutable lifecycle for all present-time
     * inputs. Streamline's own guide explicitly recommends trying
     * eValidUntilPresent first and only falling back to eOnlyValidNow for
     * individual buffers if visual inspection shows corruption at present time.
     *
     * In the current Q2RTX path, marking depth/mvec/hudless as eOnlyValidNow
     * forces Streamline to make copies for resources required on present. That
     * adds a fixed display-sized cost per host frame and matches the observed
     * behaviour where MFG becomes disproportionately expensive as the DLSS
     * upscale ratio increases. */
    constexpr sl::ResourceLifecycle kPresentValidMfgInput =
        sl::ResourceLifecycle::eValidUntilPresent;

    /* kBufferTypeUIColorAndAlpha is required by DLSS-G for UI compositing over
     * generated frames. Q2RTX has no separate UI layer here — set it to null so
     * SL knows there is no UI overlay to composite. */
    sl::ResourceTag g_tags[] = {
        sl::ResourceTag(&r_dep, sl::kBufferTypeDepth,           kPresentValidMfgInput,                     &render_ext),
        sl::ResourceTag(&r_mv,  sl::kBufferTypeMotionVectors,   kPresentValidMfgInput,                     &render_ext),
        sl::ResourceTag(has_hudless ? &r_hud : nullptr, sl::kBufferTypeHUDLessColor, kPresentValidMfgInput, &display_ext),
        sl::ResourceTag(nullptr, sl::kBufferTypeUIColorAndAlpha, sl::ResourceLifecycle::eValidUntilPresent, &display_ext),
        sl::ResourceTag(nullptr, sl::kBufferTypeBackbuffer,      sl::ResourceLifecycle{},                  &backbuffer_ext),
    };

    slSetTagForFrame(*s_frame_token, s_vp, g_tags, 5,
                     reinterpret_cast<sl::CommandBuffer*>(cmd_buf));
}

/* ======================================================================
 * Reflex
 * ====================================================================*/

void dlss_sl_reflex_set_options(int mode)
{
    if (!s_sl_loaded || !pfn_slReflexSetOptions) return;

    sl::ReflexOptions opts{};
    switch (mode) {
    case 1: opts.mode = sl::ReflexMode::eLowLatency;          break;
    case 2: opts.mode = sl::ReflexMode::eLowLatencyWithBoost; break;
    default: opts.mode = sl::ReflexMode::eOff;                break;
    }
    /* Streamline docs mark this as an advanced option that most integrations
     * should leave disabled unless specifically advised by the Reflex team.
     * Q2RTX already supplies the normal PCL marker chain; forcing this path can
     * over-constrain pacing and explode ReflexSleep cost under DLSS-G. */
    opts.useMarkersToOptimize = false;
    sl::Result r = pfn_slReflexSetOptions(opts);
    g_dlss_sl_reflex_available = (r == sl::Result::eOk || mode == 0) ? 1 : 0;
    s_effective_reflex_mode = (r == sl::Result::eOk) ? mode : 0;

    if (pfn_slReflexGetState) {
        sl::ReflexState state{};
        sl::Result state_res = pfn_slReflexGetState(state);
        if (state_res == sl::Result::eOk) {
            g_dlss_sl_reflex_available = state.lowLatencyAvailable ? 1 : (mode == 0);
            if (mode != 0 && !state.lowLatencyAvailable)
                s_effective_reflex_mode = 0;
            fprintf(stderr, "[Reflex] slReflexGetState -> result=%d lowLatencyAvailable=%d effective=%d\n",
                    (int)state_res, state.lowLatencyAvailable ? 1 : 0, s_effective_reflex_mode);
        }
    }

    fprintf(stderr, "[Reflex] slReflexSetOptions(mode=%d) -> result=%d available=%d\n",
            mode, (int)r, g_dlss_sl_reflex_available);
    if (r != sl::Result::eOk)
        fprintf(stderr, "[Reflex] *** slReflexSetOptions FAILED — DLSS-G will not work!\n");
}

void dlss_sl_reflex_sleep(void)
{
    if (!s_sl_loaded || !pfn_slReflexSleep || !s_frame_token) return;
    pfn_slReflexSleep(*s_frame_token);
}

void dlss_sl_reflex_mark_simulation_start(void)
{
    if (!s_sl_loaded || !pfn_slPCLSetMarker || !s_frame_token) return;
    pfn_slPCLSetMarker(sl::PCLMarker::eSimulationStart, *s_frame_token);
}

void dlss_sl_reflex_mark_simulation_end(void)
{
    if (!s_sl_loaded || !pfn_slPCLSetMarker || !s_frame_token) return;
    pfn_slPCLSetMarker(sl::PCLMarker::eSimulationEnd, *s_frame_token);
}

void dlss_sl_reflex_mark_render_submit_start(void)
{
    if (!s_sl_loaded || !pfn_slPCLSetMarker || !s_frame_token) return;
    pfn_slPCLSetMarker(sl::PCLMarker::eRenderSubmitStart, *s_frame_token);
}

void dlss_sl_reflex_mark_render_submit_end(void)
{
    if (!s_sl_loaded || !pfn_slPCLSetMarker || !s_frame_token) return;
    pfn_slPCLSetMarker(sl::PCLMarker::eRenderSubmitEnd, *s_frame_token);
}

void dlss_sl_hook_vk_begin_command_buffer(VkCommandBuffer cmd_buf, const VkCommandBufferBeginInfo *begin_info)
{
    (void)begin_info;
    if (!s_sl_loaded || !pfn_slHookVkBeginCommandBuffer)
        return;
    pfn_slHookVkBeginCommandBuffer(cmd_buf, begin_info);
}

void dlss_sl_hook_vk_cmd_bind_pipeline(VkCommandBuffer cmd_buf, VkPipelineBindPoint bind_point, VkPipeline pipeline)
{
    if (!s_sl_loaded || !pfn_slHookVkCmdBindPipeline)
        return;
    pfn_slHookVkCmdBindPipeline(cmd_buf, bind_point, pipeline);
}

void dlss_sl_hook_vk_cmd_bind_descriptor_sets(VkCommandBuffer cmd_buf,
    VkPipelineBindPoint bind_point,
    VkPipelineLayout layout,
    uint32_t first_set,
    uint32_t descriptor_set_count,
    const VkDescriptorSet *descriptor_sets,
    uint32_t dynamic_offset_count,
    const uint32_t *dynamic_offsets)
{
    if (!s_sl_loaded || !pfn_slHookVkCmdBindDescriptorSets)
        return;
    pfn_slHookVkCmdBindDescriptorSets(cmd_buf, bind_point, layout, first_set,
        descriptor_set_count, descriptor_sets, dynamic_offset_count, dynamic_offsets);
}

/* ======================================================================
 * Per-frame constants
 * ====================================================================*/

void dlss_sl_begin_frame(uint32_t frame_index)
{
    if (!s_sl_loaded) return;
    slGetNewFrameToken(s_frame_token, &frame_index);
}

/*
 * Transpose column-major float[16] (GLSL mat4) → row-major sl::float4x4
 */
static void transpose4x4(const float *src, sl::float4x4 &dst)
{
    /* src[col*4 + row] — column-major layout */
    for (int r = 0; r < 4; r++) {
        dst.row[r].x = src[0 * 4 + r];
        dst.row[r].y = src[1 * 4 + r];
        dst.row[r].z = src[2 * 4 + r];
        dst.row[r].w = src[3 * 4 + r];
    }
}

static sl::float4x4 identity4x4()
{
    sl::float4x4 m{};
    m.row[0].x = 1; m.row[1].y = 1; m.row[2].z = 1; m.row[3].w = 1;
    return m;
}

void dlss_sl_set_constants(
    const float *view_to_clip,
    const float *clip_to_view,
    const float *clip_to_prev_clip,
    const float *prev_clip_to_clip,
    float jitter_x, float jitter_y,
    float mv_scale_x, float mv_scale_y,
    const float *cam_pos,
    const float *cam_fwd, const float *cam_up, const float *cam_right,
    float cam_near, float cam_far, float cam_fov_y,
    int depth_inverted, int reset)
{
    if (!s_sl_loaded || !s_frame_token) return;

    sl::Constants c{};

    transpose4x4(view_to_clip, c.cameraViewToClip);
    transpose4x4(clip_to_view, c.clipToCameraView);
    if (clip_to_prev_clip)
        transpose4x4(clip_to_prev_clip, c.clipToPrevClip);
    else
        c.clipToPrevClip = identity4x4();
    if (prev_clip_to_clip)
        transpose4x4(prev_clip_to_clip, c.prevClipToClip);
    else
        c.prevClipToClip = identity4x4();
    c.cameraPinholeOffset = {0.f, 0.f};

    c.jitterOffset  = {jitter_x, jitter_y};
    c.mvecScale     = {mv_scale_x, mv_scale_y};
    c.cameraPos     = {cam_pos[0],   cam_pos[1],   cam_pos[2]};
    c.cameraFwd     = {cam_fwd[0],   cam_fwd[1],   cam_fwd[2]};
    c.cameraUp      = {cam_up[0],    cam_up[1],    cam_up[2]};
    c.cameraRight   = {cam_right[0], cam_right[1], cam_right[2]};

    c.cameraNear            = cam_near;
    c.cameraFar             = cam_far;
    c.cameraFOV             = cam_fov_y;
    c.cameraAspectRatio     = (cam_fov_y > 0.0f) ? (c.cameraViewToClip.row[1].y / c.cameraViewToClip.row[0].x) : 0.0f;
    c.depthInverted         = depth_inverted ? sl::Boolean::eTrue : sl::Boolean::eFalse;
    c.cameraMotionIncluded  = sl::Boolean::eTrue;
    c.motionVectors3D       = sl::Boolean::eFalse;
    c.reset                 = reset ? sl::Boolean::eTrue : sl::Boolean::eFalse;
    c.orthographicProjection = sl::Boolean::eFalse;
    c.motionVectorsDilated  = sl::Boolean::eFalse;
    c.motionVectorsJittered = sl::Boolean::eFalse;

    slSetConstants(c, *s_frame_token, s_vp);
}

/* ======================================================================
 * Resource tagging + evaluate
 * ====================================================================*/

void dlss_sl_tag_resources(
    VkCommandBuffer cmd_buf,
    VkImage color_in,  VkImageView color_in_view,  uint32_t layout_color_in,  uint32_t fmt_color_in,
    VkImage depth,     VkImageView depth_view,      uint32_t layout_depth,     uint32_t fmt_depth,
    VkImage mvec,      VkImageView mvec_view,       uint32_t layout_mvec,      uint32_t fmt_mvec,
    VkImage color_out, VkImageView color_out_view,  uint32_t layout_color_out, uint32_t fmt_color_out,
    uint32_t render_w, uint32_t render_h,
    uint32_t display_w, uint32_t display_h)
{
    if (!s_sl_loaded || !s_frame_token) return;

    sl::Resource r_in  (sl::ResourceType::eTex2d, (void*)color_in,  nullptr, (void*)color_in_view,  layout_color_in);
    sl::Resource r_dep (sl::ResourceType::eTex2d, (void*)depth,     nullptr, (void*)depth_view,     layout_depth);
    sl::Resource r_mv  (sl::ResourceType::eTex2d, (void*)mvec,      nullptr, (void*)mvec_view,      layout_mvec);
    sl::Resource r_out (sl::ResourceType::eTex2d, (void*)color_out, nullptr, (void*)color_out_view, layout_color_out);

    /* nativeFormat: Streamline validates this before NGX evaluate; VK_FORMAT_UNDEFINED (0) → 0xbad00005 */
    r_in.nativeFormat  = fmt_color_in;
    r_dep.nativeFormat = fmt_depth;
    r_mv.nativeFormat  = fmt_mvec;
    r_out.nativeFormat = fmt_color_out;

    /* Resource dimensions MUST match the actual VkImage allocation size.
     *
     * Q2RTX allocates all screen-space images (TAA_OUTPUT, depth, mvec) at
     * extent_screen_images = max(extent_render, extent_unscaled) = extent_unscaled
     * (DRS disabled path).  The DLSS output image is also extent_unscaled.
     * All four images are therefore display_w x display_h in actual GPU memory.
     *
     * The Extent (subrect) tells DLSS which region of the input contains valid
     * render data.  Streamline normalizes the subrect by the declared resource
     * dimensions to derive UV coordinates.  If we declare render_w x render_h
     * but the image is actually display_w x display_h, SL would compute UV=1
     * for the subrect instead of UV=render/display, causing it to sample the
     * entire display-sized image as "input" rather than just the rendered region.
     * The correct values are the true allocation dimensions. */
    r_in.width  = display_w;  r_in.height  = display_h;
    r_dep.width = display_w;  r_dep.height = display_h;
    r_mv.width  = display_w;  r_mv.height  = display_h;
    r_out.width = display_w;  r_out.height = display_h;

    /* Extent (render subrect) — NGX uses this to know how much of the input texture contains
       actual render data. Without it, RenderSubrect = (0x0) → InvalidParameter */
    sl::Extent in_ext  = { 0, 0, render_w,  render_h  };
    sl::Extent out_ext = { 0, 0, display_w, display_h };

    /* eValidUntilEvaluate: DLSS SR inputs/output are only needed during evaluate.
       eValidUntilPresent is for DLSS-G which needs resources until swapchain present. */
    sl::ResourceTag tags[] = {
        sl::ResourceTag(&r_in,  sl::kBufferTypeScalingInputColor,  sl::ResourceLifecycle::eValidUntilEvaluate, &in_ext),
        sl::ResourceTag(&r_dep, sl::kBufferTypeDepth,               sl::ResourceLifecycle::eValidUntilEvaluate, &in_ext),
        sl::ResourceTag(&r_mv,  sl::kBufferTypeMotionVectors,       sl::ResourceLifecycle::eValidUntilEvaluate, &in_ext),
        sl::ResourceTag(&r_out, sl::kBufferTypeScalingOutputColor,  sl::ResourceLifecycle::eValidUntilEvaluate, &out_ext),
    };

    slSetTagForFrame(*s_frame_token, s_vp, tags, 4,
                     reinterpret_cast<sl::CommandBuffer*>(cmd_buf));
}

void dlss_sl_evaluate(VkCommandBuffer cmd_buf)
{
    if (!s_sl_loaded || !g_dlss_sl_available || !s_frame_token) return;

    /* Streamline requires the ViewportHandle to be chained into inputs */
    const sl::BaseStructure* inputs[] = {
        reinterpret_cast<const sl::BaseStructure*>(&s_vp)
    };

    sl::Result res = slEvaluateFeature(
        sl::kFeatureDLSS, *s_frame_token, inputs, 1,
        reinterpret_cast<sl::CommandBuffer*>(cmd_buf));
    if (res != sl::Result::eOk)
        fprintf(stderr, "[DLSS] slEvaluateFeature failed: %d\n", (int)res);
}

} /* extern "C" */
