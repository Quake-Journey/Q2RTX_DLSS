/*
 * NVIDIA DLSS 4 integration for Q2RTX
 *
 * Requires at runtime (place next to q2rtx.exe):
 *   sl.interposer.dll, sl.common.dll, sl.dlss.dll, sl.dlss_g.dll, sl.dlss_d.dll
 *   nvngx_dlss.dll, nvngx_dlssg.dll, nvngx_dlssd.dll
 *
 * DLSS Super Resolution replaces FSR EASU upscaling.
 * DLSS Frame Generation (MFG) adds frame interpolation (2X/3X/4X).
 * When DLSS is enabled, FSR EASU is bypassed. FSR RCAS sharpening
 * can optionally still run on the DLSS output.
 */
#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <vulkan/vulkan.h>

/* ---- Quality modes (maps to DLSSMode in Streamline) ---- */
typedef enum {
    DLSS_MODE_OFF               = 0,
    DLSS_MODE_ULTRA_PERFORMANCE = 1,  /* ~33% render res */
    DLSS_MODE_PERFORMANCE       = 2,  /* ~50% render res */
    DLSS_MODE_BALANCED          = 3,  /* ~58% render res */
    DLSS_MODE_QUALITY           = 4,  /* ~67% render res */
    DLSS_MODE_ULTRA_QUALITY     = 5,  /* ~77% render res */
    DLSS_MODE_DLAA              = 6,  /* 100% render res, AA only */
    DLSS_MODE_CUSTOM            = 7,  /* user-driven custom SR ratio */
    DLSS_MODE_COUNT
} DlssMode_t;

/* ---- Multi Frame Generation multiplier ---- */
typedef enum {
    DLSS_MFG_OFF = 0,
    DLSS_MFG_2X  = 2,   /* RTX 40xx+ */
    DLSS_MFG_3X  = 3,   /* RTX 50xx+ */
    DLSS_MFG_4X  = 4,   /* RTX 50xx+ */
} DlssMfgMode_t;

typedef struct DlssReflexDebugReport_s {
    uint64_t frame_id;
    int low_latency_available;
    int latency_report_available;
    float sim_ms;
    float submit_ms;
    float present_ms;
    float driver_ms;
    float os_queue_ms;
    float gpu_active_ms;
    float gpu_frame_ms;
} DlssReflexDebugReport_t;

/* ---- DLSS model preset ---- */
typedef enum {
    DLSS_PRESET_DEFAULT = 0, /* Let DLSS choose */
    DLSS_PRESET_A,
    DLSS_PRESET_B,
    DLSS_PRESET_C,
    DLSS_PRESET_D,
    DLSS_PRESET_E,
    DLSS_PRESET_F,
    DLSS_PRESET_J,
    DLSS_PRESET_K,  /* Transformer model (DLSS 4) */
    DLSS_PRESET_L,
    DLSS_PRESET_M,
    DLSS_PRESET_COUNT
} DlssPreset_t;

/* ---- Lifecycle ---- */

/* Call before init_vulkan() — needed so SL can hook vkCreateDevice for DLSS-G */
void vkpt_dlss_pre_init(void);

/* Returns sl.interposer's vkGetInstanceProcAddr proxy (NULL if SL not loaded).
 * main.c must use this proxy to call vkCreateInstance and vkCreateDevice so that
 * sl.interposer is in the Vulkan dispatch chain and can set up internal plugin hooks
 * (vkCmdBindPipeline, vkCmdBindDescriptorSets, vkBeginCommandBuffer for sl.common).
 * Without this, sl.common logs "Hook ... is NOT supported" and DLSS-G crashes. */
PFN_vkGetInstanceProcAddr vkpt_dlss_get_vkGetInstanceProcAddr_proxy(void);

/* CR50b: Return sl.interposer's hooked vkCreateInstance.  Call before vkCreateInstance;
 * use the returned function pointer as the call target.  This causes sl.interposer to
 * call mapVulkanInstanceAPI, populating s_idt so that subsequent hooks (vkEnumeratePhysical-
 * Devices, vkCreateDevice) can call through it without null-pointer crashes (CR45/CR46).
 * Returns plain vkCreateInstance if SL is not loaded. */
PFN_vkCreateInstance vkpt_dlss_prepare_instance_creation(void);

/* CR50: Populate sl.interposer's instanceDeviceMap and return its hooked vkCreateDevice.
 * Call immediately before vkCreateDevice; pass the returned function pointer as the
 * call target.  This causes pluginManager->initializePlugins() to fire, establishing
 * the dispatch chain hooks required for DLSS-G (fixes WAR4639162).
 * Returns plain vkCreateDevice if SL is not loaded. */
PFN_vkCreateDevice vkpt_dlss_prepare_device_creation(VkInstance instance, VkPhysicalDevice phys_dev);

/* Call after vkCreateDevice — provide Vulkan device info to SL */
void vkpt_dlss_init(void);

/* Plugs into vkpt_initialization table (VKPT_INIT_DEFAULT — Streamline lifecycle) */
VkResult vkpt_dlss_initialize(void);
VkResult vkpt_dlss_destroy(void);

/* Plugs into vkpt_initialization table (VKPT_INIT_SWAPCHAIN_RECREATE — output image).
 * Separated so the DLSS output image is recreated on every swapchain resize,
 * ensuring it always matches qvk.extent_unscaled (display resolution). */
VkResult vkpt_dlss_init_output_image(void);
VkResult vkpt_dlss_destroy_output_image(void);

/* Register console variables */
void vkpt_dlss_init_cvars(void);

/* ---- Status queries ---- */
bool vkpt_dlss_is_available(void);   /* SDK loaded + GPU supports DLSS */
bool vkpt_dlss_is_enabled(void);     /* Enabled by cvar */
bool vkpt_dlss_rr_is_available(void);/* GPU/runtime supports DLSS RR */
bool vkpt_dlss_rr_is_enabled(void);  /* Enabled by cvar + DLSS enabled */
bool vkpt_dlss_needs_upscale(void);  /* true when render res < display res */
bool vkpt_dlss_mfg_is_enabled(void); /* MFG enabled by cvar */
bool vkpt_dlss_g_is_available(void); /* GPU supports DLSS-G */
int  vkpt_dlss_get_display_fps(void);        /* display FPS measured on present thread */
int  vkpt_dlss_get_display_multiplier(void); /* frames actually presented per render frame */
uint64_t vkpt_dlss_get_total_presented_frames(void);
int  vkpt_dlss_get_mfg_cap(void);            /* 0=unsupported, 2=2X only, 4=2X/3X/4X */
int  vkpt_dlss_get_requested_reflex_mode(void);
int  vkpt_dlss_get_effective_reflex_mode(void);
int  vkpt_dlss_get_last_dlssg_status(void);
int  vkpt_dlss_get_last_dlssg_frames_presented(void);
int  vkpt_dlss_get_last_dlssg_vsync_support(void);
bool vkpt_dlss_get_latest_reflex_report(DlssReflexDebugReport_t *out);
void vkpt_dlss_wait_for_frame_generation_inputs(void);
int vkpt_dlss_get_frame_generation_inputs_wait(VkSemaphore *sem, uint64_t *value);
void vkpt_dlss_mark_frame_generation_inputs_waited(uint64_t value);
uint32_t vkpt_dlss_get_required_graphics_queue_count(void);
uint32_t vkpt_dlss_get_required_compute_queue_count(void);
uint32_t vkpt_dlss_get_required_optical_flow_queue_count(void);
uint32_t vkpt_dlss_get_required_device_extension_count(void);
const char* vkpt_dlss_get_required_device_extension(uint32_t index);
uint32_t vkpt_dlss_get_required_instance_extension_count(void);
const char* vkpt_dlss_get_required_instance_extension(uint32_t index);
bool vkpt_dlss_requires_vk_feature12(const char *name);
bool vkpt_dlss_requires_vk_feature13(const char *name);

/* Returns current mode enum from cvar */
DlssMode_t    vkpt_dlss_get_mode(void);
DlssMode_t    vkpt_dlss_get_effective_streamline_mode(void);
float         vkpt_dlss_get_custom_ratio(void);
DlssPreset_t  vkpt_dlss_get_preset(void);
DlssPreset_t  vkpt_dlss_get_rr_preset(void);
DlssMfgMode_t vkpt_dlss_get_mfg_mode(void);
int           vkpt_dlss_get_mfg_render_cap(void);
const char*   vkpt_dlss_get_sr_dll_version(void);
const char*   vkpt_dlss_get_rr_dll_version(void);
const char*   vkpt_dlss_get_fg_dll_version(void);

bool         vkpt_dlss_is_sl_debug_log_enabled(void);

/* ---- Resolution helpers ---- */

/* Given display resolution, returns optimal render resolution for the current mode */
void vkpt_dlss_get_render_resolution(uint32_t display_w, uint32_t display_h,
                                     uint32_t *render_w,  uint32_t *render_h);

/* ---- Per-frame rendering ---- */

/*
 * Executes DLSS SR upscaling pass.
 * Input:  TAA_OUTPUT at render resolution
 * Output: DLSS_OUTPUT at display resolution
 * Call after TAA pass, in place of FSR EASU.
 */
void vkpt_dlss_process(VkCommandBuffer cmd_buf);

/*
 * Executes DLSS Ray Reconstruction in place of TAA/TAAU.
 * Input:  FLAT_COLOR + FLAT_MOTION + auxiliary RR material buffers at render resolution
 * Output: TAA_OUTPUT at display resolution
 * Call after checkerboard interleave and before bloom / tone mapping.
 */
void vkpt_dlss_rr_process(VkCommandBuffer cmd_buf);

/*
 * Re-tag a post-tonemap display-resolution image for DLSS-G / MFG.
 * Used by the RR path where frame generation must see the post-RR, post-tonemap image
 * instead of the standalone DLSS SR output texture.
 */
void vkpt_dlss_tag_mfg_output(VkCommandBuffer cmd_buf,
    VkImage hudless_img, VkImageView hudless_view,
    uint32_t layout_hudless, uint32_t fmt_hudless,
    uint32_t display_w, uint32_t display_h);
void vkpt_dlss_force_mfg_off_for_menu(void);
VkImageView vkpt_dlss_get_output_view(void);

/*
 * Blit DLSS_OUTPUT to the swapchain image (replaces vkpt_fsr_final_blit).
 */
void vkpt_dlss_final_blit(VkCommandBuffer cmd_buf, bool waterwarp);

/* ---- Reflex ---- */

/* Nvidia Reflex mode: 0=Off, 1=On, 2=On+Boost */
typedef enum {
    DLSS_REFLEX_OFF        = 0,
    DLSS_REFLEX_ON         = 1,
    DLSS_REFLEX_ON_BOOST   = 2,
    DLSS_REFLEX_MODE_COUNT
} DlssReflexMode_t;

/* Apply Reflex mode from cvar; call once per mode change */
void vkpt_dlss_reflex_apply_options(void);
void vkpt_dlss_begin_frame(uint32_t frame_idx);
void vkpt_dlss_reflex_mark_simulation_start(void);

/* Sleep call — invoke once per frame, before rendering begins (but after input sampling).
 * Reduces render-ahead and GPU queue depth for lower latency. */
void vkpt_dlss_reflex_sleep(void);
void vkpt_dlss_reflex_mark_simulation_end(void);
void vkpt_dlss_reflex_mark_render_submit_start(void);
void vkpt_dlss_reflex_mark_render_submit_end(void);

/* ---- SL-patched Vulkan wrappers (use instead of bare vk* for present + swapchain) ---- */

/* All four functions below fall back to the corresponding vk* if SL is not loaded.
 * Using the SL-hooked versions ensures Streamline observes every present
 * (presentCommon → GC runs, frame counter advances) and every swapchain
 * create/destroy (needed for DLSS-G frame generation). */
VkResult dlss_sl_vkCreateSwapchainKHR(VkDevice device, const VkSwapchainCreateInfoKHR *ci,
                                       const VkAllocationCallbacks *alloc, VkSwapchainKHR *sw);
void     dlss_sl_vkDestroySwapchainKHR(VkDevice device, VkSwapchainKHR sw,
                                        const VkAllocationCallbacks *alloc);
VkResult dlss_sl_vkGetSwapchainImagesKHR(VkDevice device, VkSwapchainKHR sw,
                                          uint32_t *count, VkImage *images);
VkResult dlss_sl_vkQueuePresentKHR(VkQueue queue, const VkPresentInfoKHR *info);
VkResult dlss_sl_vkAcquireNextImageKHR(VkDevice device, VkSwapchainKHR swapchain,
                                        uint64_t timeout, VkSemaphore semaphore,
                                        VkFence fence, uint32_t *pImageIndex);
VkResult dlss_sl_vkDeviceWaitIdle(VkDevice device);
void     dlss_sl_vkDestroySurfaceKHR(VkInstance instance, VkSurfaceKHR surface,
                                      const VkAllocationCallbacks *pAllocator);
