/*
 * dlss.c — NVIDIA DLSS 4 integration for Q2RTX
 *
 * Provides the C-side logic: CVars, init/shutdown, per-frame processing.
 * Heavy-lifting (Streamline API calls) is in dlss_sl.cpp (C++ wrapper).
 */

#include "vkpt.h"
#include "dlss.h"
#include "../../client/client.h"
#include "common/cvar.h"
#include "common/cmd.h"
#include "common/common.h"

#include <string.h>
#include <math.h>

extern bool scr_timerefresh_active;

/* ---- C++ bridge symbols from dlss_sl.cpp ---- */
extern int g_dlss_sl_available;
extern int g_dlss_sl_rr_available;
extern int g_dlss_sl_g_available;
extern int g_dlss_sl_init_result;
extern int g_dlss_sl_sr_result;
extern int g_dlss_sl_rr_result;
extern int g_dlss_sl_g_result;
extern int g_dlss_sl_setvk_result;

extern int dlss_sl_startup(int want_mfg);
extern PFN_vkGetInstanceProcAddr dlss_sl_get_vkGetInstanceProcAddr_proxy(void);
extern void dlss_sl_shutdown(void);
extern void dlss_sl_set_vulkan_info(VkInstance, VkPhysicalDevice, VkDevice,
    uint32_t gfx_fam, uint32_t gfx_idx,
    uint32_t comp_fam, uint32_t comp_idx,
    uint32_t optical_flow_fam, uint32_t optical_flow_idx,
    int use_native_optical_flow);
extern void dlss_sl_finish_proxy_vulkan_setup(VkInstance, VkPhysicalDevice, VkDevice);
extern uint32_t dlss_sl_get_required_graphics_queue_count(void);
extern uint32_t dlss_sl_get_required_compute_queue_count(void);
extern uint32_t dlss_sl_get_required_optical_flow_queue_count(void);
extern uint32_t dlss_sl_get_required_device_extension_count(void);
extern const char* dlss_sl_get_required_device_extension(uint32_t index);
extern uint32_t dlss_sl_get_required_instance_extension_count(void);
extern const char* dlss_sl_get_required_instance_extension(uint32_t index);
extern bool dlss_sl_requires_vk_feature12(const char *name);
extern bool dlss_sl_requires_vk_feature13(const char *name);
extern void dlss_sl_get_optimal_settings(int mode,
    uint32_t disp_w, uint32_t disp_h,
    uint32_t *render_w, uint32_t *render_h);
extern void dlss_sl_rr_get_optimal_settings(int mode, int preset,
    uint32_t disp_w, uint32_t disp_h,
    uint32_t *render_w, uint32_t *render_h);
extern void dlss_sl_set_options(int mode, int preset,
    uint32_t render_w, uint32_t render_h,
    uint32_t out_w, uint32_t out_h, int hdr,
    float sharpness, int auto_exposure);
extern void dlss_sl_rr_set_options(int mode, int preset,
    uint32_t out_w, uint32_t out_h,
    const float *world_to_camera_view,
    const float *camera_view_to_world);
extern void dlss_sl_set_g_options(int mfg_mode,
    uint32_t color_w, uint32_t color_h,
    uint32_t mvec_w,  uint32_t mvec_h,
    uint32_t num_backbuffers,
    uint32_t color_fmt,
    uint32_t mvec_fmt,
    uint32_t depth_fmt,
    uint32_t hudless_fmt,
    uint32_t ui_fmt,
    int dynamic_resolution);
extern void dlss_sl_begin_frame(uint32_t frame_index);
extern void dlss_sl_set_constants(
    const float *view_to_clip, const float *clip_to_view,
    const float *clip_to_prev_clip,
    const float *prev_clip_to_clip,
    float jitter_x, float jitter_y,
    float mv_scale_x, float mv_scale_y,
    const float *cam_pos,
    const float *cam_fwd, const float *cam_up, const float *cam_right,
    float cam_near, float cam_far, float cam_fov_y,
    int depth_inverted, int reset);
extern void dlss_sl_tag_resources(VkCommandBuffer cmd_buf,
    VkImage color_in,  VkImageView color_in_view,  uint32_t layout_color_in,  uint32_t fmt_color_in,
    VkImage depth,     VkImageView depth_view,      uint32_t layout_depth,     uint32_t fmt_depth,
    VkImage mvec,      VkImageView mvec_view,       uint32_t layout_mvec,      uint32_t fmt_mvec,
    VkImage color_out, VkImageView color_out_view,  uint32_t layout_color_out, uint32_t fmt_color_out,
    uint32_t render_w, uint32_t render_h,
    uint32_t display_w, uint32_t display_h);
extern void dlss_sl_evaluate(VkCommandBuffer cmd_buf);
extern void dlss_sl_rr_tag_resources(VkCommandBuffer cmd_buf,
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
    uint32_t output_alloc_w, uint32_t output_alloc_h);
extern void dlss_sl_rr_evaluate(VkCommandBuffer cmd_buf);
extern void dlss_sl_alloc_g_resources(void);
extern void dlss_sl_tag_g_resources(VkCommandBuffer cmd_buf,
    VkImage depth,   VkImageView depth_view,  uint32_t layout_depth,   uint32_t fmt_depth,
    VkImage mvec,    VkImageView mvec_view,   uint32_t layout_mvec,    uint32_t fmt_mvec,
    VkImage hudless, VkImageView hudless_view,uint32_t layout_hudless, uint32_t fmt_hudless,
    uint32_t render_w, uint32_t render_h,
    uint32_t display_w, uint32_t display_h,
    uint32_t backbuffer_x, uint32_t backbuffer_y);
extern int  g_dlss_sl_reflex_available;
extern void dlss_sl_reflex_set_options(int mode);
extern void dlss_sl_reflex_sleep(void);
extern void dlss_sl_reflex_mark_simulation_start(void);
extern void dlss_sl_reflex_mark_simulation_end(void);
extern void dlss_sl_reflex_mark_render_submit_start(void);
extern void dlss_sl_reflex_mark_render_submit_end(void);
extern void dlss_sl_hook_vk_begin_command_buffer(VkCommandBuffer cmd_buf, const VkCommandBufferBeginInfo *begin_info);
extern void dlss_sl_hook_vk_cmd_bind_pipeline(VkCommandBuffer cmd_buf, VkPipelineBindPoint bind_point, VkPipeline pipeline);
extern void dlss_sl_hook_vk_cmd_bind_descriptor_sets(VkCommandBuffer cmd_buf, VkPipelineBindPoint bind_point, VkPipelineLayout layout, uint32_t first_set, uint32_t descriptor_set_count, const VkDescriptorSet *descriptor_sets, uint32_t dynamic_offset_count, const uint32_t *dynamic_offsets);
extern int  dlss_sl_get_display_fps(void);
extern int  dlss_sl_get_display_multiplier(void);
extern uint64_t dlss_sl_get_total_presented_frames(void);
extern int  dlss_sl_get_mfg_cap(void);
extern int  dlss_sl_get_effective_reflex_mode(void);
extern int  dlss_sl_get_last_dlssg_status(void);
extern int  dlss_sl_get_last_dlssg_frames_presented(void);
extern int  dlss_sl_get_last_dlssg_vsync_support(void);
extern bool dlss_sl_get_latest_reflex_report(DlssReflexDebugReport_t *out);
extern void dlss_sl_wait_for_g_inputs_consumed(void);
extern const char* dlss_sl_get_sr_dll_version(void);
extern const char* dlss_sl_get_rr_dll_version(void);
extern const char* dlss_sl_get_fg_dll_version(void);

/* ---- Console variables ---- */
static cvar_t *cvar_dlss_enable  = NULL;  /* 0=off, 1=on                          */
static cvar_t *cvar_dlss_mode    = NULL;  /* DlssMode_t value (1-6)               */
static cvar_t *cvar_dlss_preset  = NULL;  /* DlssPreset_t value (0=default, 8=K)  */
static cvar_t *cvar_dlss_rr_preset = NULL; /* RR preset: 0=default, 4=D, 5=E       */
static cvar_t *cvar_dlss_rr      = NULL;  /* 0=off, 1=on                          */
static cvar_t *cvar_dlss_mfg     = NULL;  /* DlssMfgMode_t: 0=off 2=2X 3=3X 4=4X */
static cvar_t *cvar_dlss_mfg_fps_cap = NULL; /* 0=off, otherwise caps render FPS */
static cvar_t *cvar_dlss_reflex  = NULL;  /* DlssReflexMode_t: 0=off 1=on 2=boost */
static cvar_t *cvar_dlss_sharpness = NULL; /* DLSS sharpening in [0,1] */
static cvar_t *cvar_dlss_auto_exposure = NULL; /* 0=manual, 1=auto */
static cvar_t *cvar_dlss_custom_ratio = NULL; /* custom DLSS render scale in percent */
static cvar_t *cvar_dlss_available = NULL; /* DLSS SR supported on this GPU/runtime */
static cvar_t *cvar_dlss_rr_available = NULL; /* DLSS RR supported on this GPU/runtime */
static cvar_t *cvar_dlss_mfg_max_count_for_device = NULL;  /* 0=unsupported, 2=2X only, 4=2X/3X/4X */
static cvar_t *cvar_dlss_mfg_cap_compat = NULL;  /* compatibility alias */
static cvar_t *cvar_dlss_reflex_effective = NULL; /* effective runtime mode after enforcement */
static cvar_t *cvar_dlss_sl_debug_log = NULL; /* 0=off, 1=write sl_debug.log */

/* Whether slInit() succeeded */
static bool s_sl_started = false;
/* Whether slSetVulkanInfo() has been called */
static bool s_vk_info_set = false;
/* Tracks if previous frame was a reset (scene change) */
static bool s_prev_reset = true;
/* Tracks whether Reflex options have been applied at least once this session */
static bool s_reflex_applied = false;
/* GPU-generation fallback for MFG menu gating before DLSS-G state is queried */
static int  s_detected_mfg_cap = 0;

/* Standalone DLSS output image (not in the global image table to avoid
   shifting shader descriptor binding offsets) */
static VkImage        s_dlss_out_img  = VK_NULL_HANDLE;
static VkDeviceMemory s_dlss_out_mem  = VK_NULL_HANDLE;
static VkImageView    s_dlss_out_view = VK_NULL_HANDLE;

#define DLSS_MFG_RING_SLOTS MAX_FRAMES_IN_FLIGHT

typedef struct DlssStandaloneImage_s {
    VkImage image;
    VkDeviceMemory memory;
    VkImageView view;
    VkImageLayout layout;
} DlssStandaloneImage_t;

static DlssStandaloneImage_t s_mfg_depth_ring[DLSS_MFG_RING_SLOTS];
static DlssStandaloneImage_t s_mfg_mvec_ring[DLSS_MFG_RING_SLOTS];
static DlssStandaloneImage_t s_mfg_hudless_ring[DLSS_MFG_RING_SLOTS];
static uint32_t s_mfg_ring_slot_count = 0;

static void enforce_mfg_and_reflex_policy(void);

/* ======================================================================
 * CVars
 * ====================================================================*/

void vkpt_dlss_init_cvars(void)
{
    cvar_dlss_enable = Cvar_Get("flt_dlss_enable", "0", CVAR_ARCHIVE);
    cvar_dlss_mode   = Cvar_Get("flt_dlss_mode",   "4", CVAR_ARCHIVE); /* Quality */
    cvar_dlss_preset = Cvar_Get("flt_dlss_preset",  "8", CVAR_ARCHIVE); /* Preset K (Transformer) */
    cvar_dlss_rr_preset = Cvar_Get("flt_dlss_rr_preset", "4", CVAR_ARCHIVE); /* Preset D for RR */
    cvar_dlss_rr     = Cvar_Get("flt_dlss_rr",     "0", CVAR_ARCHIVE); /* RR off by default */
    cvar_dlss_mfg    = Cvar_Get("flt_dlss_mfg",    "0", CVAR_ARCHIVE); /* MFG off by default */
    cvar_dlss_mfg_fps_cap = Cvar_Get("flt_dlss_mfg_fps_cap", "0", CVAR_ARCHIVE);
    cvar_dlss_reflex = Cvar_Get("flt_dlss_reflex", "0", CVAR_ARCHIVE); /* Reflex off by default */
    cvar_dlss_sharpness = Cvar_Get("flt_dlss_sharpness", "0.0", CVAR_ARCHIVE);
    cvar_dlss_auto_exposure = Cvar_Get("flt_dlss_auto_exposure", "1", CVAR_ARCHIVE);
    cvar_dlss_custom_ratio = Cvar_Get("flt_dlss_custom_ratio", "77", CVAR_ARCHIVE);
    cvar_dlss_available = Cvar_Get("flt_dlss_available", "0", CVAR_ROM | CVAR_NOARCHIVE);
    cvar_dlss_rr_available = Cvar_Get("flt_dlss_rr_available", "0", CVAR_ROM | CVAR_NOARCHIVE);
    cvar_dlss_mfg_max_count_for_device = Cvar_Get("flt_dlss_mfg_max_count_for_device", "0", CVAR_ROM);
    cvar_dlss_mfg_cap_compat = Cvar_Get("flt_dlss_mfg_cap", "0", CVAR_ROM);
    cvar_dlss_reflex_effective = Cvar_Get("flt_dlss_reflex_effective", "0", CVAR_ROM | CVAR_NOARCHIVE);
    cvar_dlss_sl_debug_log = Cvar_Get("flt_dlss_sl_debug_log", "0", CVAR_ARCHIVE);
}

static int clamp_reflex_mode(int mode)
{
    if (mode < DLSS_REFLEX_OFF)      return DLSS_REFLEX_OFF;
    if (mode > DLSS_REFLEX_ON_BOOST) return DLSS_REFLEX_ON_BOOST;
    return mode;
}

static float clamp_dlss_sharpness(float sharpness)
{
    if (sharpness < 0.0f) return 0.0f;
    if (sharpness > 1.0f) return 1.0f;
    return sharpness;
}

static int clamp_bool_cvar_value(int value)
{
    return value ? 1 : 0;
}

static int clamp_dlss_custom_ratio_percent(int value)
{
    if (value < 33) return 33;
    if (value > 99) return 99;
    return value;
}

float vkpt_dlss_get_custom_ratio(void)
{
    int percent = cvar_dlss_custom_ratio ? cvar_dlss_custom_ratio->integer : 77;
    percent = clamp_dlss_custom_ratio_percent(percent);
    return (float)percent * 0.01f;
}

static void get_dlss_custom_render_resolution(
    uint32_t display_w, uint32_t display_h,
    uint32_t *render_w, uint32_t *render_h)
{
    float ratio = vkpt_dlss_get_custom_ratio();
    uint32_t rw = (uint32_t)floorf((float)display_w * ratio + 0.5f);
    uint32_t rh = (uint32_t)floorf((float)display_h * ratio + 0.5f);

    if (rw > display_w) rw = display_w;
    if (rh > display_h) rh = display_h;
    if (rw < 16u) rw = 16u;
    if (rh < 16u) rh = 16u;

    rw &= ~1u;
    rh &= ~1u;
    if (rw < 16u) rw = 16u;
    if (rh < 16u) rh = 16u;

    *render_w = rw;
    *render_h = rh;
}

static DlssMode_t get_effective_streamline_mode_for_ratio(float ratio)
{
    if (ratio <= 0.416f) return DLSS_MODE_ULTRA_PERFORMANCE;
    if (ratio <= 0.540f) return DLSS_MODE_PERFORMANCE;
    if (ratio <= 0.625f) return DLSS_MODE_BALANCED;
    return DLSS_MODE_QUALITY;
}

static DlssMode_t get_effective_streamline_mode(DlssMode_t mode)
{
    if (mode == DLSS_MODE_CUSTOM)
        return get_effective_streamline_mode_for_ratio(vkpt_dlss_get_custom_ratio());
    return mode;
}

DlssMode_t vkpt_dlss_get_effective_streamline_mode(void)
{
    return get_effective_streamline_mode(vkpt_dlss_get_mode());
}

static int normalize_runtime_mfg_cap(int runtime_cap)
{
    if (runtime_cap >= 3) return DLSS_MFG_4X;
    if (runtime_cap >= 1) return DLSS_MFG_2X;
    return DLSS_MFG_OFF;
}

static int detect_mfg_cap_from_gpu(void)
{
    if (qvk.physical_device == VK_NULL_HANDLE)
        return DLSS_MFG_OFF;

    VkPhysicalDeviceProperties props;
    vkGetPhysicalDeviceProperties(qvk.physical_device, &props);

    if (props.vendorID != 0x10de)
        return DLSS_MFG_OFF;

    if (strstr(props.deviceName, "RTX 50"))
        return DLSS_MFG_4X;
    if (strstr(props.deviceName, "RTX 40"))
        return DLSS_MFG_2X;

    return DLSS_MFG_OFF;
}

static int get_effective_mfg_cap(void)
{
    if (!s_vk_info_set || !g_dlss_sl_g_available)
        return DLSS_MFG_OFF;

    int runtime_cap = normalize_runtime_mfg_cap(dlss_sl_get_mfg_cap());
    return runtime_cap ? runtime_cap : s_detected_mfg_cap;
}

static int clamp_mfg_mode_to_cap(int mode, int cap)
{
    if (mode != DLSS_MFG_2X && mode != DLSS_MFG_3X && mode != DLSS_MFG_4X)
        return DLSS_MFG_OFF;

    if (cap <= DLSS_MFG_OFF)
        return DLSS_MFG_OFF;
    if (cap <= DLSS_MFG_2X)
        return DLSS_MFG_2X;

    return mode;
}

static int clamp_mfg_mode_raw(int mode)
{
    if (mode != DLSS_MFG_2X && mode != DLSS_MFG_3X && mode != DLSS_MFG_4X)
        return DLSS_MFG_OFF;
    return mode;
}

static int get_user_requested_mfg_mode(void)
{
    if (!cvar_dlss_mfg)
        return DLSS_MFG_OFF;
    return clamp_mfg_mode_raw(cvar_dlss_mfg->integer);
}

static int get_desired_reflex_mode(void)
{
    return cvar_dlss_reflex ? clamp_reflex_mode(cvar_dlss_reflex->integer) : DLSS_REFLEX_OFF;
}

static void sync_status_cvars(void)
{
    if (cvar_dlss_available)
        Cvar_SetInteger(cvar_dlss_available, vkpt_dlss_is_available() ? 1 : 0, FROM_CODE);

    if (cvar_dlss_rr_available)
        Cvar_SetInteger(cvar_dlss_rr_available, vkpt_dlss_rr_is_available() ? 1 : 0, FROM_CODE);

    if (cvar_dlss_mfg_max_count_for_device)
        Cvar_SetInteger(cvar_dlss_mfg_max_count_for_device, get_effective_mfg_cap(), FROM_CODE);
    if (cvar_dlss_mfg_cap_compat)
        Cvar_SetInteger(cvar_dlss_mfg_cap_compat, get_effective_mfg_cap(), FROM_CODE);

    if (cvar_dlss_reflex_effective)
        Cvar_SetInteger(cvar_dlss_reflex_effective, vkpt_dlss_get_effective_reflex_mode(), FROM_CODE);
}

static void enforce_mfg_and_reflex_policy(void)
{
    if (!cvar_dlss_enable || !cvar_dlss_rr || !cvar_dlss_mfg || !cvar_dlss_reflex || !cvar_dlss_sharpness ||
        !cvar_dlss_auto_exposure || !cvar_dlss_mfg_fps_cap || !cvar_dlss_custom_ratio)
        return;

    if (cvar_dlss_rr->integer != clamp_bool_cvar_value(cvar_dlss_rr->integer))
        Cvar_SetInteger(cvar_dlss_rr, clamp_bool_cvar_value(cvar_dlss_rr->integer), FROM_CODE);

    if (cvar_dlss_sharpness->value != clamp_dlss_sharpness(cvar_dlss_sharpness->value))
        Cvar_SetValue(cvar_dlss_sharpness, clamp_dlss_sharpness(cvar_dlss_sharpness->value), FROM_CODE);

    if (cvar_dlss_auto_exposure->integer != clamp_bool_cvar_value(cvar_dlss_auto_exposure->integer))
        Cvar_SetInteger(cvar_dlss_auto_exposure, clamp_bool_cvar_value(cvar_dlss_auto_exposure->integer), FROM_CODE);

    if (cvar_dlss_custom_ratio->integer != clamp_dlss_custom_ratio_percent(cvar_dlss_custom_ratio->integer))
        Cvar_SetInteger(cvar_dlss_custom_ratio, clamp_dlss_custom_ratio_percent(cvar_dlss_custom_ratio->integer), FROM_CODE);

    if (cvar_dlss_mfg_fps_cap->integer < 0)
        Cvar_SetInteger(cvar_dlss_mfg_fps_cap, 0, FROM_CODE);

    if (cvar_dlss_reflex->integer != clamp_reflex_mode(cvar_dlss_reflex->integer)) {
        Cvar_SetInteger(cvar_dlss_reflex, clamp_reflex_mode(cvar_dlss_reflex->integer), FROM_CODE);
    }

    if (cvar_dlss_rr_preset) {
        int rr_preset = cvar_dlss_rr_preset->integer;
        if (rr_preset != 0 && rr_preset != DLSS_PRESET_D && rr_preset != DLSS_PRESET_E)
            Cvar_SetInteger(cvar_dlss_rr_preset, DLSS_PRESET_D, FROM_CODE);
    }

    sync_status_cvars();
}

/* ======================================================================
 * Status queries
 * ====================================================================*/

bool vkpt_dlss_is_available(void)
{
    return s_vk_info_set && g_dlss_sl_available;
}

bool vkpt_dlss_is_enabled(void)
{
    if (!vkpt_dlss_is_available()) return false;
    if (!cvar_dlss_enable)         return false;
    return cvar_dlss_enable->integer != 0;
}

bool vkpt_dlss_rr_is_available(void)
{
    return s_vk_info_set && g_dlss_sl_rr_available;
}

bool vkpt_dlss_rr_is_enabled(void)
{
    if (!vkpt_dlss_rr_is_available()) return false;
    if (!vkpt_dlss_is_enabled()) return false;
    if (!cvar_dlss_rr) return false;
    return cvar_dlss_rr->integer != 0;
}

bool vkpt_dlss_needs_upscale(void)
{
    if (!vkpt_dlss_is_enabled()) return false;
    DlssMode_t mode = vkpt_dlss_get_mode();
    if (mode == DLSS_MODE_CUSTOM)
        return vkpt_dlss_get_custom_ratio() < 0.999f;
    return mode != DLSS_MODE_DLAA && mode != DLSS_MODE_OFF;
}

bool vkpt_dlss_g_is_available(void)
{
    return s_vk_info_set && g_dlss_sl_g_available && get_effective_mfg_cap() != DLSS_MFG_OFF;
}

bool vkpt_dlss_mfg_is_enabled(void)
{
    return vkpt_dlss_g_is_available() && vkpt_dlss_get_mfg_mode() != DLSS_MFG_OFF;
}

DlssMode_t vkpt_dlss_get_mode(void)
{
    if (!cvar_dlss_mode) return DLSS_MODE_QUALITY;
    int v = cvar_dlss_mode->integer;
    if (v < 1 || v >= DLSS_MODE_COUNT) v = DLSS_MODE_QUALITY;
    return (DlssMode_t)v;
}

DlssPreset_t vkpt_dlss_get_preset(void)
{
    if (!cvar_dlss_preset) return DLSS_PRESET_K;
    int v = cvar_dlss_preset->integer;
    if (v < 0 || v >= DLSS_PRESET_COUNT) v = DLSS_PRESET_K;
    return (DlssPreset_t)v;
}

DlssPreset_t vkpt_dlss_get_rr_preset(void)
{
    if (!cvar_dlss_rr_preset) return DLSS_PRESET_D;
    int v = cvar_dlss_rr_preset->integer;
    if (v < 0 || v >= DLSS_PRESET_COUNT) v = DLSS_PRESET_D;
    return (DlssPreset_t)v;
}

DlssMfgMode_t vkpt_dlss_get_mfg_mode(void)
{
    int v = clamp_mfg_mode_to_cap(get_user_requested_mfg_mode(), get_effective_mfg_cap());
    return (DlssMfgMode_t)v;
}

int vkpt_dlss_get_mfg_render_cap(void)
{
    if (!cvar_dlss_mfg_fps_cap)
        return 0;
    if (cvar_dlss_mfg_fps_cap->integer <= 0)
        return 0;
    return cvar_dlss_mfg_fps_cap->integer;
}

const char* vkpt_dlss_get_sr_dll_version(void)
{
    return dlss_sl_get_sr_dll_version();
}

const char* vkpt_dlss_get_rr_dll_version(void)
{
    return dlss_sl_get_rr_dll_version();
}

const char* vkpt_dlss_get_fg_dll_version(void)
{
    return dlss_sl_get_fg_dll_version();
}

bool vkpt_dlss_is_sl_debug_log_enabled(void)
{
    return cvar_dlss_sl_debug_log && cvar_dlss_sl_debug_log->integer != 0;
}

int vkpt_dlss_get_display_fps(void)
{
    return dlss_sl_get_display_fps();
}

int vkpt_dlss_get_display_multiplier(void)
{
    int multiplier = dlss_sl_get_display_multiplier();
    if (multiplier > 0)
        return multiplier;

    switch (vkpt_dlss_get_mfg_mode()) {
    case DLSS_MFG_2X: return 2;
    case DLSS_MFG_3X: return 3;
    case DLSS_MFG_4X: return 4;
    default:          return 1;
    }
}

uint64_t vkpt_dlss_get_total_presented_frames(void)
{
    return dlss_sl_get_total_presented_frames();
}

int vkpt_dlss_get_mfg_cap(void)
{
    return get_effective_mfg_cap();
}

int vkpt_dlss_get_requested_reflex_mode(void)
{
    return get_desired_reflex_mode();
}

int vkpt_dlss_get_effective_reflex_mode(void)
{
    if (!s_sl_started || !s_vk_info_set)
        return DLSS_REFLEX_OFF;

    int effective_mode = dlss_sl_get_effective_reflex_mode();
    if (effective_mode >= 0)
        return effective_mode;

    return get_desired_reflex_mode();
}

int vkpt_dlss_get_last_dlssg_status(void)
{
    return dlss_sl_get_last_dlssg_status();
}

int vkpt_dlss_get_last_dlssg_frames_presented(void)
{
    return dlss_sl_get_last_dlssg_frames_presented();
}

int vkpt_dlss_get_last_dlssg_vsync_support(void)
{
    return dlss_sl_get_last_dlssg_vsync_support();
}

bool vkpt_dlss_get_latest_reflex_report(DlssReflexDebugReport_t *out)
{
    return dlss_sl_get_latest_reflex_report(out);
}

uint32_t vkpt_dlss_get_required_graphics_queue_count(void)
{
    return dlss_sl_get_required_graphics_queue_count();
}

uint32_t vkpt_dlss_get_required_compute_queue_count(void)
{
    return dlss_sl_get_required_compute_queue_count();
}

uint32_t vkpt_dlss_get_required_optical_flow_queue_count(void)
{
    return dlss_sl_get_required_optical_flow_queue_count();
}

uint32_t vkpt_dlss_get_required_device_extension_count(void)
{
    return dlss_sl_get_required_device_extension_count();
}

const char* vkpt_dlss_get_required_device_extension(uint32_t index)
{
    return dlss_sl_get_required_device_extension(index);
}

uint32_t vkpt_dlss_get_required_instance_extension_count(void)
{
    return dlss_sl_get_required_instance_extension_count();
}

const char* vkpt_dlss_get_required_instance_extension(uint32_t index)
{
    return dlss_sl_get_required_instance_extension(index);
}

bool vkpt_dlss_requires_vk_feature12(const char *name)
{
    return dlss_sl_requires_vk_feature12(name);
}

bool vkpt_dlss_requires_vk_feature13(const char *name)
{
    return dlss_sl_requires_vk_feature13(name);
}

void vkpt_dlss_wait_for_frame_generation_inputs(void)
{
    if (!vkpt_dlss_mfg_is_enabled())
        return;

    dlss_sl_wait_for_g_inputs_consumed();
}

int vkpt_dlss_get_frame_generation_inputs_wait(VkSemaphore *sem, uint64_t *value)
{
    extern int dlss_sl_get_g_inputs_fence_wait(VkSemaphore *sem, uint64_t *value);

    if (!vkpt_dlss_mfg_is_enabled())
        return 0;

    return dlss_sl_get_g_inputs_fence_wait(sem, value);
}

void vkpt_dlss_mark_frame_generation_inputs_waited(uint64_t value)
{
    extern void dlss_sl_mark_g_inputs_fence_waited(uint64_t value);

    if (!vkpt_dlss_mfg_is_enabled())
        return;

    dlss_sl_mark_g_inputs_fence_waited(value);
}

/* ======================================================================
 * Lifecycle
 * ====================================================================*/

void vkpt_dlss_pre_init(void)
{
    /* Call slInit() before vkCreateDevice so SL can intercept it for DLSS-G.
       For DLSS SR only this ordering is not required but it's future-proof. */
    int want_mfg = cvar_dlss_mfg && (cvar_dlss_mfg->integer != 0);
    int ok = dlss_sl_startup(want_mfg);
    s_sl_started = (ok != 0);
    if (!s_sl_started)
        Com_WPrintf("[DLSS] slInit failed (sl.interposer.dll missing or error code %d)\n",
                    g_dlss_sl_init_result);
    else
        Com_Printf("[DLSS] Streamline initialized\n");
}

PFN_vkGetInstanceProcAddr vkpt_dlss_get_vkGetInstanceProcAddr_proxy(void)
{
    return dlss_sl_get_vkGetInstanceProcAddr_proxy();
}

PFN_vkCreateInstance vkpt_dlss_prepare_instance_creation(void)
{
    return vkCreateInstance;
}

PFN_vkCreateDevice vkpt_dlss_prepare_device_creation(VkInstance instance, VkPhysicalDevice phys_dev)
{
    (void)instance;
    (void)phys_dev;
    return vkCreateDevice;
}

void vkpt_dlss_init(void)
{
    if (!s_sl_started) return;

    dlss_sl_set_vulkan_info(
        qvk.instance,
        qvk.physical_device,
        qvk.device,
        qvk.queue_idx_graphics, (uint32_t)qvk.sl_graphics_queue_index,
        qvk.queue_idx_graphics, (uint32_t)qvk.sl_compute_queue_index,
        qvk.queue_idx_optical_flow >= 0 ? (uint32_t)qvk.queue_idx_optical_flow : 0u,
        (uint32_t)qvk.sl_optical_flow_queue_index,
        qvk.sl_use_native_optical_flow ? 1 : 0);

    s_vk_info_set = true;
    s_prev_reset  = true;
    s_detected_mfg_cap = detect_mfg_cap_from_gpu();
    enforce_mfg_and_reflex_policy();

    /* Apply Reflex options immediately — DLSS-G plugin checks for Reflex inside its
     * vkCreateSwapchainKHR hook (which fires before the first game frame).
     * Without this, "notifyOutOfBandCommandQueue] No reflex" fires at swapchain creation
     * and the plugin enters a broken state that crashes on the first present (CR23). */
    vkpt_dlss_reflex_apply_options();
    s_reflex_applied = true;

    Com_Printf("[DLSS] Vulkan proxy device path active (state code %d)\n", g_dlss_sl_setvk_result);
    Com_Printf("[DLSS] slIsFeatureSupported DLSS SR result: %d\n", g_dlss_sl_sr_result);
    Com_Printf("[DLSS] slIsFeatureSupported DLSS RR result: %d\n", g_dlss_sl_rr_result);
    Com_Printf("[DLSS] slIsFeatureSupported DLSS-G result:  %d\n", g_dlss_sl_g_result);

    if (!g_dlss_sl_available)
    {
        Com_WPrintf("[DLSS] GPU does not support DLSS SR (see result codes above)\n");
        return;
    }

    Com_Printf("[DLSS] DLSS SR available%s%s\n",
               g_dlss_sl_rr_available ? " | DLSS-RR available" : "",
               g_dlss_sl_g_available ? " | DLSS-G/MFG available" : "");
}

static VkResult dlss_alloc_output_image(void)
{
    uint32_t w = qvk.extent_unscaled.width;
    uint32_t h = qvk.extent_unscaled.height;

    VkImageCreateInfo ci = {
        .sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .imageType     = VK_IMAGE_TYPE_2D,
        .format        = VK_FORMAT_R16G16B16A16_SFLOAT,
        .extent        = { w, h, 1 },
        .mipLevels     = 1,
        .arrayLayers   = 1,
        .samples       = VK_SAMPLE_COUNT_1_BIT,
        .tiling        = VK_IMAGE_TILING_OPTIMAL,
        .usage         = VK_IMAGE_USAGE_STORAGE_BIT
                       | VK_IMAGE_USAGE_SAMPLED_BIT
                       | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
        .sharingMode   = VK_SHARING_MODE_EXCLUSIVE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
    };
    _VK(vkCreateImage(qvk.device, &ci, NULL, &s_dlss_out_img));

    VkMemoryRequirements mr;
    vkGetImageMemoryRequirements(qvk.device, s_dlss_out_img, &mr);

    uint32_t mem_type = UINT32_MAX;
    for (uint32_t i = 0; i < qvk.mem_properties.memoryTypeCount; i++) {
        if ((mr.memoryTypeBits & (1u << i)) &&
            (qvk.mem_properties.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)) {
            mem_type = i;
            break;
        }
    }
    VkMemoryAllocateInfo ai = {
        .sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize  = mr.size,
        .memoryTypeIndex = mem_type,
    };
    _VK(vkAllocateMemory(qvk.device, &ai, NULL, &s_dlss_out_mem));
    _VK(vkBindImageMemory(qvk.device, s_dlss_out_img, s_dlss_out_mem, 0));

    VkImageViewCreateInfo vi = {
        .sType            = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .image            = s_dlss_out_img,
        .viewType         = VK_IMAGE_VIEW_TYPE_2D,
        .format           = VK_FORMAT_R16G16B16A16_SFLOAT,
        .subresourceRange = {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .levelCount = 1,
            .layerCount = 1,
        },
    };
    _VK(vkCreateImageView(qvk.device, &vi, NULL, &s_dlss_out_view));
    return VK_SUCCESS;
}

static VkResult dlss_alloc_standalone_image(
    DlssStandaloneImage_t *img,
    uint32_t w, uint32_t h,
    VkFormat format,
    VkImageUsageFlags usage,
    VkImageAspectFlags aspect)
{
    memset(img, 0, sizeof(*img));
    img->layout = VK_IMAGE_LAYOUT_UNDEFINED;

    VkImageCreateInfo ci = {
        .sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .imageType     = VK_IMAGE_TYPE_2D,
        .format        = format,
        .extent        = { w, h, 1 },
        .mipLevels     = 1,
        .arrayLayers   = 1,
        .samples       = VK_SAMPLE_COUNT_1_BIT,
        .tiling        = VK_IMAGE_TILING_OPTIMAL,
        .usage         = usage,
        .sharingMode   = VK_SHARING_MODE_EXCLUSIVE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
    };
    _VK(vkCreateImage(qvk.device, &ci, NULL, &img->image));

    VkMemoryRequirements mr;
    vkGetImageMemoryRequirements(qvk.device, img->image, &mr);

    uint32_t mem_type = UINT32_MAX;
    for (uint32_t i = 0; i < qvk.mem_properties.memoryTypeCount; i++) {
        if ((mr.memoryTypeBits & (1u << i)) &&
            (qvk.mem_properties.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)) {
            mem_type = i;
            break;
        }
    }
    if (mem_type == UINT32_MAX)
        return VK_ERROR_MEMORY_MAP_FAILED;

    VkMemoryAllocateInfo ai = {
        .sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize  = mr.size,
        .memoryTypeIndex = mem_type,
    };
    _VK(vkAllocateMemory(qvk.device, &ai, NULL, &img->memory));
    _VK(vkBindImageMemory(qvk.device, img->image, img->memory, 0));

    VkImageViewCreateInfo vi = {
        .sType            = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .image            = img->image,
        .viewType         = VK_IMAGE_VIEW_TYPE_2D,
        .format           = format,
        .subresourceRange = {
            .aspectMask = aspect,
            .levelCount = 1,
            .layerCount = 1,
        },
    };
    _VK(vkCreateImageView(qvk.device, &vi, NULL, &img->view));
    return VK_SUCCESS;
}

static void dlss_free_standalone_image(DlssStandaloneImage_t *img)
{
    if (img->view != VK_NULL_HANDLE) {
        vkDestroyImageView(qvk.device, img->view, NULL);
        img->view = VK_NULL_HANDLE;
    }
    if (img->image != VK_NULL_HANDLE) {
        vkDestroyImage(qvk.device, img->image, NULL);
        img->image = VK_NULL_HANDLE;
    }
    if (img->memory != VK_NULL_HANDLE) {
        vkFreeMemory(qvk.device, img->memory, NULL);
        img->memory = VK_NULL_HANDLE;
    }
    img->layout = VK_IMAGE_LAYOUT_UNDEFINED;
}

static void dlss_free_mfg_ring_images(void)
{
    for (uint32_t i = 0; i < DLSS_MFG_RING_SLOTS; ++i) {
        dlss_free_standalone_image(&s_mfg_depth_ring[i]);
        dlss_free_standalone_image(&s_mfg_mvec_ring[i]);
        dlss_free_standalone_image(&s_mfg_hudless_ring[i]);
    }
    s_mfg_ring_slot_count = 0;
}

static VkResult dlss_alloc_mfg_ring_images(void)
{
    uint32_t slot_count = qvk.num_swap_chain_images;
    if (slot_count == 0)
        return VK_SUCCESS;
    if (slot_count > DLSS_MFG_RING_SLOTS)
        slot_count = DLSS_MFG_RING_SLOTS;

    dlss_free_mfg_ring_images();

    for (uint32_t i = 0; i < slot_count; ++i) {
        VkResult r = dlss_alloc_standalone_image(
            &s_mfg_depth_ring[i],
            qvk.extent_render.width,
            qvk.extent_render.height,
            VK_FORMAT_R16_SFLOAT,
            VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
            VK_IMAGE_ASPECT_COLOR_BIT);
        if (r != VK_SUCCESS) return r;

        r = dlss_alloc_standalone_image(
            &s_mfg_mvec_ring[i],
            qvk.extent_render.width,
            qvk.extent_render.height,
            VK_FORMAT_R16G16B16A16_SFLOAT,
            VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
            VK_IMAGE_ASPECT_COLOR_BIT);
        if (r != VK_SUCCESS) return r;

        r = dlss_alloc_standalone_image(
            &s_mfg_hudless_ring[i],
            qvk.extent_unscaled.width,
            qvk.extent_unscaled.height,
            VK_FORMAT_R16G16B16A16_SFLOAT,
            VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
            VK_IMAGE_ASPECT_COLOR_BIT);
        if (r != VK_SUCCESS) return r;
    }

    s_mfg_ring_slot_count = slot_count;
    return VK_SUCCESS;
}

static void dlss_free_output_image(void)
{
    if (s_dlss_out_view != VK_NULL_HANDLE) {
        vkDestroyImageView(qvk.device, s_dlss_out_view, NULL);
        s_dlss_out_view = VK_NULL_HANDLE;
    }
    if (s_dlss_out_img != VK_NULL_HANDLE) {
        vkDestroyImage(qvk.device, s_dlss_out_img, NULL);
        s_dlss_out_img = VK_NULL_HANDLE;
    }
    if (s_dlss_out_mem != VK_NULL_HANDLE) {
        vkFreeMemory(qvk.device, s_dlss_out_mem, NULL);
        s_dlss_out_mem = VK_NULL_HANDLE;
    }
}

static void dlss_copy_into_mfg_ring(
    VkCommandBuffer cmd_buf,
    VkImage src_image,
    VkImageLayout src_layout,
    DlssStandaloneImage_t *dst,
    VkImageLayout dst_final_layout,
    uint32_t width,
    uint32_t height)
{
    VkImageCopy copy_region = {
        .srcSubresource = {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .layerCount = 1,
        },
        .dstSubresource = {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .layerCount = 1,
        },
        .extent = { width, height, 1 },
    };

    VkImageSubresourceRange range = {
        .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
        .baseMipLevel = 0,
        .levelCount = 1,
        .baseArrayLayer = 0,
        .layerCount = 1,
    };

    IMAGE_BARRIER(cmd_buf,
        .image = src_image,
        .subresourceRange = range,
        .srcAccessMask = VK_ACCESS_SHADER_READ_BIT,
        .dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT,
        .oldLayout = src_layout,
        .newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL
    );

    IMAGE_BARRIER(cmd_buf,
        .image = dst->image,
        .subresourceRange = range,
        .srcAccessMask = (dst->layout == VK_IMAGE_LAYOUT_UNDEFINED) ? 0 : VK_ACCESS_SHADER_READ_BIT,
        .dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
        .oldLayout = dst->layout,
        .newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
    );

    vkCmdCopyImage(cmd_buf,
        src_image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        dst->image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        1, &copy_region);

    IMAGE_BARRIER(cmd_buf,
        .image = src_image,
        .subresourceRange = range,
        .srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT,
        .dstAccessMask = VK_ACCESS_SHADER_READ_BIT,
        .oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        .newLayout = src_layout
    );

    IMAGE_BARRIER(cmd_buf,
        .image = dst->image,
        .subresourceRange = range,
        .srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
        .dstAccessMask = VK_ACCESS_SHADER_READ_BIT,
        .oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        .newLayout = dst_final_layout
    );

    dst->layout = dst_final_layout;
}

VkResult vkpt_dlss_initialize(void)
{
    /* Streamline was already started in vkpt_dlss_pre_init / vkpt_dlss_init.
     * Output image allocation is handled by vkpt_dlss_init_output_image (SWAPCHAIN_RECREATE)
     * so it is correctly sized to qvk.extent_unscaled after swapchain creation. */
    return VK_SUCCESS;
}

VkResult vkpt_dlss_destroy(void)
{
    /* Ensure all GPU work is idle before releasing SL/NGX resources.
     * (vkpt_destroy_all calls vkDeviceWaitIdle at its start, but be explicit here.) */
    if (qvk.device != VK_NULL_HANDLE)
        vkDeviceWaitIdle(qvk.device);
    /* Shut down Streamline (releases NGX resources). Output image is freed
       separately by vkpt_dlss_destroy_output_image (SWAPCHAIN_RECREATE entry). */
    dlss_sl_shutdown();
    s_sl_started     = false;
    s_vk_info_set    = false;
    s_reflex_applied = false;
    s_detected_mfg_cap = 0;
    sync_status_cvars();
    return VK_SUCCESS;
}

/* Called on every swapchain recreate so s_dlss_out_img matches the current
   display resolution (qvk.extent_unscaled) rather than the startup value.
   Also allocates DLSS-G resources on first call (swapchain must exist for DLSS-G). */
VkResult vkpt_dlss_init_output_image(void)
{
    if (!s_sl_started) return VK_SUCCESS;
    VkResult r = dlss_alloc_output_image();
    if (r != VK_SUCCESS) return r;

    r = dlss_alloc_mfg_ring_images();
    if (r != VK_SUCCESS) return r;

    /* DLSS-G allocation: must happen after swapchain creation (SL needs to have
     * observed vkCreateSwapchainKHR via its hook to set up frame generation).
     * slAllocateResources(DLSS_G) is idempotent — safe to call on every recreate. */
    dlss_sl_alloc_g_resources();

    /* NOTE: slDLSSGSetOptions is NOT called here at init time.
     * Previously (CR26 fix) we called it here to avoid a null-ptr crash in
     * presentCommon if set_g_options was never invoked before the first present.
     * However, calling it both here AND in vkpt_dlss_process (per-frame) caused
     * "Repeated slDLSSGSetOptions() call for the same frame" — a race/state
     * corruption that led to the CR37 second-present crash.
     *
     * Safe alternative: vkpt_dlss_process always runs BEFORE vkQueuePresent,
     * so the first per-frame call guarantees set_options fires before any present.
     * The s_g_tags_valid guard keeps eOff on frame 0 (tags not yet set) and
     * enables eOn from frame 1 onwards once resources are properly tagged. */

    return VK_SUCCESS;
}

VkResult vkpt_dlss_destroy_output_image(void)
{
    dlss_free_output_image();
    dlss_free_mfg_ring_images();
    return VK_SUCCESS;
}

/* ======================================================================
 * Reflex
 * ====================================================================*/

void vkpt_dlss_reflex_apply_options(void)
{
    int mode = get_desired_reflex_mode();
    dlss_sl_reflex_set_options(mode);
    sync_status_cvars();
}

void vkpt_dlss_begin_frame(uint32_t frame_idx)
{
    dlss_sl_begin_frame(frame_idx);
}

void vkpt_dlss_reflex_mark_simulation_start(void)
{
    dlss_sl_reflex_mark_simulation_start();
}

void vkpt_dlss_reflex_sleep(void)
{
    if (scr_timerefresh_active)
        return;

    dlss_sl_reflex_sleep();
}

void vkpt_dlss_reflex_mark_simulation_end(void)
{
    dlss_sl_reflex_mark_simulation_end();
}

void vkpt_dlss_reflex_mark_render_submit_start(void)
{
    dlss_sl_reflex_mark_render_submit_start();
}

void vkpt_dlss_reflex_mark_render_submit_end(void)
{
    dlss_sl_reflex_mark_render_submit_end();
}

VkResult vkpt_dlss_vkBeginCommandBuffer(VkCommandBuffer cmd_buf, const VkCommandBufferBeginInfo *begin_info)
{
    VkResult res = vkBeginCommandBuffer(cmd_buf, begin_info);
    if (res == VK_SUCCESS)
        dlss_sl_hook_vk_begin_command_buffer(cmd_buf, begin_info);
    return res;
}

void vkpt_dlss_vkCmdBindPipeline(VkCommandBuffer cmd_buf, VkPipelineBindPoint bind_point, VkPipeline pipeline)
{
    vkCmdBindPipeline(cmd_buf, bind_point, pipeline);
    dlss_sl_hook_vk_cmd_bind_pipeline(cmd_buf, bind_point, pipeline);
}

void vkpt_dlss_vkCmdBindDescriptorSets(VkCommandBuffer cmd_buf,
    VkPipelineBindPoint bind_point,
    VkPipelineLayout layout,
    uint32_t first_set,
    uint32_t descriptor_set_count,
    const VkDescriptorSet *descriptor_sets,
    uint32_t dynamic_offset_count,
    const uint32_t *dynamic_offsets)
{
    vkCmdBindDescriptorSets(cmd_buf, bind_point, layout, first_set,
        descriptor_set_count, descriptor_sets, dynamic_offset_count, dynamic_offsets);
    dlss_sl_hook_vk_cmd_bind_descriptor_sets(cmd_buf, bind_point, layout, first_set,
        descriptor_set_count, descriptor_sets, dynamic_offset_count, dynamic_offsets);
}

/* ======================================================================
 * Resolution
 * ====================================================================*/

void vkpt_dlss_get_render_resolution(
    uint32_t display_w, uint32_t display_h,
    uint32_t *render_w, uint32_t *render_h)
{
    DlssMode_t selected_mode;

    if (!vkpt_dlss_is_available() || !vkpt_dlss_is_enabled())
    {
        *render_w = display_w;
        *render_h = display_h;
        return;
    }

    selected_mode = vkpt_dlss_get_mode();

    if (selected_mode == DLSS_MODE_CUSTOM)
    {
        get_dlss_custom_render_resolution(display_w, display_h, render_w, render_h);
        return;
    }

    if (vkpt_dlss_rr_is_enabled())
    {
        dlss_sl_rr_get_optimal_settings((int)selected_mode,
                                        cvar_dlss_rr_preset ? cvar_dlss_rr_preset->integer : DLSS_PRESET_D,
                                        display_w, display_h,
                                        render_w, render_h);
    }
    else
    {
        dlss_sl_get_optimal_settings((int)selected_mode,
                                     display_w, display_h,
                                     render_w, render_h);
    }

    /* Clamp to display size */
    if (*render_w > display_w) *render_w = display_w;
    if (*render_h > display_h) *render_h = display_h;
    if (*render_w < 16)        *render_w = 16;
    if (*render_h < 16)        *render_h = 16;
}

/* ======================================================================
 * Helper: extract camera vectors from column-major view matrix
 *
 * Standard Vulkan/OpenGL view matrix (column-major flat float[16]):
 *   col0: right,   col1: up,   col2: forward(-look), col3: translation
 * Element [col * 4 + row].
 * ====================================================================*/
static inline void mat4_get_right  (const float *V, float *v) { v[0]=V[0]; v[1]=V[4]; v[2]=V[8]; }
static inline void mat4_get_up     (const float *V, float *v) { v[0]=V[1]; v[1]=V[5]; v[2]=V[9]; }
static inline void mat4_get_fwd    (const float *V, float *v)
{
    /* Camera looks down -Z in view space → forward in world = -col2 of view matrix */
    v[0]=-V[2]; v[1]=-V[6]; v[2]=-V[10];
}

/*
 * Column-major 4x4 matrix multiply: C = A * B
 * Layout: M[col*4 + row]
 */
static void mat4_mul_cm(const float *A, const float *B, float *C)
{
    float tmp[16];
    for (int c = 0; c < 4; c++)
        for (int r = 0; r < 4; r++) {
            float s = 0.f;
            for (int k = 0; k < 4; k++)
                s += A[k*4+r] * B[c*4+k];
            tmp[c*4+r] = s;
        }
    memcpy(C, tmp, 16*sizeof(float));
}

/*
 * Inverse of a rigid-body (rotation + translation) view matrix in column-major layout.
 * For M = [R|t], inv(M) = [R^T | -R^T*t]
 */
static void mat4_rigid_inverse_cm(const float *M, float *Minv)
{
    float R[16] = {0};
    /* Transpose 3x3 rotation block */
    for (int i = 0; i < 3; i++)
        for (int j = 0; j < 3; j++)
            R[j*4+i] = M[i*4+j];
    /* -R^T * t */
    for (int i = 0; i < 3; i++) {
        float v = 0.f;
        for (int k = 0; k < 3; k++)
            v -= R[k*4+i] * M[3*4+k];
        R[3*4+i] = v;
    }
    R[3*4+3] = 1.f;
    memcpy(Minv, R, 16*sizeof(float));
}

/* Extract near/far from a reversed-Z Vulkan perspective projection matrix. */
static void mat4_extract_near_far(const float *P, float *near, float *far)
{
    /* Standard Vulkan reversed-Z infinite projection:
       P[10]=0, P[11]=-1, P[14]=near  (column-major: P[col*4+row])
       So P[2*4+2] = P[10] = 0 (near ignored) for infinite far.
       For finite far: P[10] = -near/(far-near), P[14] = -near*far/(far-near) */
    float p22 = P[2*4+2]; /* col2 row2 */
    float p23 = P[3*4+2]; /* col3 row2 */

    if (p22 == 0.0f)
    {
        /* Infinite far reversed-Z: near = P[14] */
        *near = P[3*4+2];
        *far  = 65536.0f; /* effectively infinite */
    }
    else
    {
        *near = p23 / (p22 - 1.0f);
        *far  = p23 / (p22 + 1.0f);
        if (*near < 0) { float t = *near; *near = *far; *far = t; }
    }

    /* Sanity clamp */
    if (*near < 0.1f)      *near = 0.1f;
    if (*far  < *near+1.f) *far  = *near + 65536.0f;
}

/* ======================================================================
 * Per-frame processing
 * ====================================================================*/

typedef struct DlssFrameContext_s {
    const QVKUniformBuffer_t *ubo;
    const float *V;
    const float *invV;
    int depth_idx;
} DlssFrameContext_t;

static void dlss_apply_reflex_if_needed(void)
{
    if (cvar_dlss_reflex && (cvar_dlss_reflex->changed || !s_reflex_applied))
    {
        vkpt_dlss_reflex_apply_options();
        cvar_dlss_reflex->changed = false;
        s_reflex_applied = true;
    }
}

static void dlss_prepare_frame_context(DlssFrameContext_t *ctx)
{
    const QVKUniformBuffer_t *ubo = &vkpt_refdef.uniform_buffer;

    memset(ctx, 0, sizeof(*ctx));
    ctx->ubo = ubo;

    /* UBO mat4 fields are GLSL mat4: float[4][4] col-major stored as float[16].
       The struct uses aligned C arrays; we cast to float* for matrix access. */
    ctx->V    = (const float *)&ubo->V;
    ctx->invV = (const float *)&ubo->invV;
    const float *V_prev    = (const float *)&ubo->V_prev;
    const float *P         = (const float *)&ubo->P;
    const float *invP      = (const float *)&ubo->invP;
    const float *P_prev    = (const float *)&ubo->P_prev;
    const float *invP_prev = (const float *)&ubo->invP_prev;

    float cam_right[3], cam_up[3], cam_fwd[3];
    mat4_get_right(ctx->V, cam_right);
    mat4_get_up(ctx->V, cam_up);
    mat4_get_fwd(ctx->V, cam_fwd);

    const float *cam_pos = (const float *)&ubo->cam_pos;

    float cam_near = vkpt_refdef.z_near;
    float cam_far  = vkpt_refdef.z_far;
    if (cam_near < 0.01f) cam_near = 1.0f;
    if (cam_far  < cam_near + 1.0f) cam_far = 65536.0f;

    float fov_y = 2.0f * atanf(1.0f / ((const float *)&ubo->projection_fov_scale)[1]);
    const float *jitter = (const float *)&ubo->sub_pixel_jitter;

    /* Q2RTX stores PT_MOTION as normalized screen-space delta
       (screen_pos_prev - screen_pos_curr), not in pixel units.
       Streamline expects mvecScale={1,1} for vectors already in [-1,1].
       Using 1/render dims shrinks vectors almost to zero and causes
       persistent smear/ghosting, especially for RR on distant geometry. */
    float mv_sx = 1.0f;
    float mv_sy = 1.0f;

    float tmp[16], ctp[16], pcc[16], invV_prev[16];

    mat4_mul_cm(ctx->invV, invP, tmp);
    mat4_mul_cm(V_prev, tmp, ctp);
    mat4_mul_cm(P_prev, ctp, tmp);
    memcpy(ctp, tmp, 16 * sizeof(float));

    mat4_rigid_inverse_cm(V_prev, invV_prev);
    mat4_mul_cm(invV_prev, invP_prev, tmp);
    mat4_mul_cm(ctx->V, tmp, pcc);
    mat4_mul_cm(P, pcc, tmp);
    memcpy(pcc, tmp, 16 * sizeof(float));

    dlss_sl_set_constants(
        P, invP,
        ctp,
        pcc,
        jitter[0], jitter[1],
        mv_sx, mv_sy,
        cam_pos, cam_fwd, cam_up, cam_right,
        cam_near, cam_far, fov_y,
        0,
        s_prev_reset ? 1 : 0);

    ctx->depth_idx = (qvk.frame_counter & 1)
        ? VKPT_IMG_PT_VIEW_DEPTH_B
        : VKPT_IMG_PT_VIEW_DEPTH_A;

    s_prev_reset = false;
}

static void dlss_tag_mfg_common(VkCommandBuffer cmd_buf,
    int depth_idx,
    VkImage hudless_img, VkImageView hudless_view,
    uint32_t layout_hudless, uint32_t fmt_hudless,
    uint32_t display_w, uint32_t display_h)
{
    uint32_t backbuffer_x = 0;
    uint32_t backbuffer_y = 0;

    if (!g_dlss_sl_g_available || !cvar_dlss_mfg)
        return;

    int mfg_mode = (int)vkpt_dlss_get_mfg_mode();

    dlss_sl_set_g_options(
        mfg_mode,
        display_w, display_h,
        qvk.extent_render.width, qvk.extent_render.height,
        (uint32_t)qvk.num_swap_chain_images,
        (uint32_t)qvk.surf_format.format,
        (uint32_t)VK_FORMAT_R16G16B16A16_SFLOAT,
        (uint32_t)VK_FORMAT_R16_SFLOAT,
        fmt_hudless,
        (uint32_t)qvk.surf_format.format,
        0);

    if (mfg_mode == DLSS_MFG_OFF)
        return;

    VkImage depth_img = qvk.images[depth_idx];
    VkImageView depth_view = qvk.images_views[depth_idx];
    uint32_t layout_depth = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    uint32_t fmt_depth = VK_FORMAT_R16_SFLOAT;

    VkImage mvec_img = qvk.images[VKPT_IMG_PT_MOTION];
    VkImageView mvec_view = qvk.images_views[VKPT_IMG_PT_MOTION];
    uint32_t layout_mvec = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    uint32_t fmt_mvec = VK_FORMAT_R16G16B16A16_SFLOAT;

    dlss_sl_tag_g_resources(
        cmd_buf,
        depth_img,
        depth_view,
        layout_depth,
        fmt_depth,
        mvec_img,
        mvec_view,
        layout_mvec,
        fmt_mvec,
        hudless_img,
        hudless_view,
        layout_hudless,
        fmt_hudless,
        qvk.extent_render.width, qvk.extent_render.height,
        display_w, display_h,
        backbuffer_x, backbuffer_y);
}

void vkpt_dlss_tag_mfg_output(VkCommandBuffer cmd_buf,
    VkImage hudless_img, VkImageView hudless_view,
    uint32_t layout_hudless, uint32_t fmt_hudless,
    uint32_t display_w, uint32_t display_h)
{
    if (!vkpt_dlss_is_enabled())
        return;

    enforce_mfg_and_reflex_policy();
    dlss_tag_mfg_common(cmd_buf,
        (qvk.frame_counter & 1) ? VKPT_IMG_PT_VIEW_DEPTH_B : VKPT_IMG_PT_VIEW_DEPTH_A,
        hudless_img, hudless_view,
        layout_hudless, fmt_hudless,
        display_w, display_h);
    dlss_apply_reflex_if_needed();
}

void vkpt_dlss_force_mfg_off_for_menu(void)
{
    if (!g_dlss_sl_g_available || !cvar_dlss_mfg)
        return;
    if (cvar_dlss_mfg->integer == 0)
        return;

    dlss_sl_set_g_options(
        DLSS_MFG_OFF,
        qvk.extent_unscaled.width,
        qvk.extent_unscaled.height,
        qvk.extent_render.width,
        qvk.extent_render.height,
        (uint32_t)qvk.num_swap_chain_images,
        (uint32_t)qvk.surf_format.format,
        (uint32_t)VK_FORMAT_R16G16B16A16_SFLOAT,
        (uint32_t)VK_FORMAT_R16_SFLOAT,
        (uint32_t)VK_FORMAT_R16G16B16A16_SFLOAT,
        0,
        0);
}

void vkpt_dlss_process(VkCommandBuffer cmd_buf)
{
    if (!vkpt_dlss_is_enabled() || vkpt_dlss_rr_is_enabled()) return;

    enforce_mfg_and_reflex_policy();
    DlssFrameContext_t ctx;
    dlss_prepare_frame_context(&ctx);

    /* ------------------------------------------------------------------
     * Push updated DLSS SR options (mode, preset, output resolution)
     * ----------------------------------------------------------------*/
    int is_hdr = qvk.surf_is_hdr ? 1 : 0;
    dlss_sl_set_options(
        (int)vkpt_dlss_get_effective_streamline_mode(),
        (int)vkpt_dlss_get_preset(),
        qvk.extent_render.width,
        qvk.extent_render.height,
        qvk.extent_unscaled.width,
        qvk.extent_unscaled.height,
        is_hdr,
        cvar_dlss_sharpness ? clamp_dlss_sharpness(cvar_dlss_sharpness->value) : 0.0f,
        cvar_dlss_auto_exposure ? clamp_bool_cvar_value(cvar_dlss_auto_exposure->integer) : 1);

    /* ------------------------------------------------------------------
     * Tag resources for DLSS SR
     * ----------------------------------------------------------------*/
    dlss_sl_tag_resources(
        cmd_buf,
        /* Input color: TAA output at render resolution — R16G16B16A16_SFLOAT */
        qvk.images      [VKPT_IMG_TAA_OUTPUT],
        qvk.images_views[VKPT_IMG_TAA_OUTPUT],
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        VK_FORMAT_R16G16B16A16_SFLOAT,
        /* Depth: linear view depth — R16_SFLOAT */
        qvk.images      [ctx.depth_idx],
        qvk.images_views[ctx.depth_idx],
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        VK_FORMAT_R16_SFLOAT,
        /* Motion vectors — R16G16B16A16_SFLOAT */
        qvk.images      [VKPT_IMG_PT_MOTION],
        qvk.images_views[VKPT_IMG_PT_MOTION],
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        VK_FORMAT_R16G16B16A16_SFLOAT,
        /* Output: standalone DLSS output image — R16G16B16A16_SFLOAT */
        s_dlss_out_img,
        s_dlss_out_view,
        VK_IMAGE_LAYOUT_GENERAL,
        VK_FORMAT_R16G16B16A16_SFLOAT,
        /* Render and display extents */
        qvk.extent_render.width,    qvk.extent_render.height,
        qvk.extent_unscaled.width,  qvk.extent_unscaled.height);

    /* ------------------------------------------------------------------
     * Run DLSS SR
     * ----------------------------------------------------------------*/
    dlss_sl_evaluate(cmd_buf);

    /* Ensure DLSS output is in GENERAL after evaluation so vkpt_final_blit_view
       can sample it as a combined image sampler. */
    IMAGE_BARRIER(cmd_buf,
        .image            = s_dlss_out_img,
        .oldLayout        = VK_IMAGE_LAYOUT_GENERAL,
        .newLayout        = VK_IMAGE_LAYOUT_GENERAL,
        .srcAccessMask    = VK_ACCESS_SHADER_WRITE_BIT,
        .dstAccessMask    = VK_ACCESS_SHADER_READ_BIT,
        .subresourceRange = { .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                              .levelCount = 1, .layerCount = 1 });

    dlss_tag_mfg_common(cmd_buf,
        ctx.depth_idx,
        s_dlss_out_img,
        s_dlss_out_view,
        VK_IMAGE_LAYOUT_GENERAL,
        VK_FORMAT_R16G16B16A16_SFLOAT,
        qvk.extent_unscaled.width,
        qvk.extent_unscaled.height);

    dlss_apply_reflex_if_needed();
}

void vkpt_dlss_rr_process(VkCommandBuffer cmd_buf)
{
    if (!vkpt_dlss_rr_is_enabled()) return;

    enforce_mfg_and_reflex_policy();

    DlssFrameContext_t ctx;
    dlss_prepare_frame_context(&ctx);

    /* RR is an extension of DLSS in Streamline terms. The RR guide requires
       compatible DLSS options to be kept in sync in addition to RR options. */
    dlss_sl_set_options(
        (int)vkpt_dlss_get_effective_streamline_mode(),
        (int)vkpt_dlss_get_preset(),
        qvk.extent_render.width,
        qvk.extent_render.height,
        qvk.extent_unscaled.width,
        qvk.extent_unscaled.height,
        1,
        cvar_dlss_sharpness ? clamp_dlss_sharpness(cvar_dlss_sharpness->value) : 0.0f,
        cvar_dlss_auto_exposure ? clamp_bool_cvar_value(cvar_dlss_auto_exposure->integer) : 1);

    dlss_sl_rr_set_options(
        (int)vkpt_dlss_get_effective_streamline_mode(),
        cvar_dlss_rr_preset ? cvar_dlss_rr_preset->integer : DLSS_PRESET_D,
        qvk.extent_unscaled.width,
        qvk.extent_unscaled.height,
        ctx.V,
        ctx.invV);

    vkpt_dlss_rr_prepare_resources(cmd_buf);

    dlss_sl_rr_tag_resources(
        cmd_buf,
        qvk.images[VKPT_IMG_DLSS_RR_COLOR],
        qvk.images_views[VKPT_IMG_DLSS_RR_COLOR],
        VK_IMAGE_LAYOUT_GENERAL,
        VK_FORMAT_R16G16B16A16_SFLOAT,
        qvk.images[VKPT_IMG_DLSS_RR_DEPTH],
        qvk.images_views[VKPT_IMG_DLSS_RR_DEPTH],
        VK_IMAGE_LAYOUT_GENERAL,
        VK_FORMAT_R16_SFLOAT,
        qvk.images[VKPT_IMG_DLSS_RR_MOTION],
        qvk.images_views[VKPT_IMG_DLSS_RR_MOTION],
        VK_IMAGE_LAYOUT_GENERAL,
        VK_FORMAT_R16G16_SFLOAT,
        qvk.images[VKPT_IMG_FSR_EASU_OUTPUT],
        qvk.images_views[VKPT_IMG_FSR_EASU_OUTPUT],
        VK_IMAGE_LAYOUT_GENERAL,
        VK_FORMAT_R16G16B16A16_SFLOAT,
        qvk.images[VKPT_IMG_FSR_RCAS_OUTPUT],
        qvk.images_views[VKPT_IMG_FSR_RCAS_OUTPUT],
        VK_IMAGE_LAYOUT_GENERAL,
        VK_FORMAT_R16G16B16A16_SFLOAT,
        qvk.images[VKPT_IMG_HQ_COLOR_INTERLEAVED],
        qvk.images_views[VKPT_IMG_HQ_COLOR_INTERLEAVED],
        VK_IMAGE_LAYOUT_GENERAL,
        VK_FORMAT_R32G32B32A32_SFLOAT,
        qvk.images[VKPT_IMG_DLSS_RR_ROUGHNESS],
        qvk.images_views[VKPT_IMG_DLSS_RR_ROUGHNESS],
        VK_IMAGE_LAYOUT_GENERAL,
        VK_FORMAT_R16_SFLOAT,
        qvk.images[VKPT_IMG_DLSS_RR_COLOR_BEFORE_TRANSPARENCY],
        qvk.images_views[VKPT_IMG_DLSS_RR_COLOR_BEFORE_TRANSPARENCY],
        VK_IMAGE_LAYOUT_GENERAL,
        VK_FORMAT_R16G16B16A16_SFLOAT,
        qvk.images[VKPT_IMG_DLSS_RR_SPEC_MOTION],
        qvk.images_views[VKPT_IMG_DLSS_RR_SPEC_MOTION],
        VK_IMAGE_LAYOUT_GENERAL,
        VK_FORMAT_R16G16_SFLOAT,
        qvk.images[VKPT_IMG_DLSS_RR_SPEC_HIT_DIST],
        qvk.images_views[VKPT_IMG_DLSS_RR_SPEC_HIT_DIST],
        VK_IMAGE_LAYOUT_GENERAL,
        VK_FORMAT_R16_SFLOAT,
        qvk.images[VKPT_IMG_DLSS_RR_SPEC_RAY_DIR_HIT_DIST],
        qvk.images_views[VKPT_IMG_DLSS_RR_SPEC_RAY_DIR_HIT_DIST],
        VK_IMAGE_LAYOUT_GENERAL,
        VK_FORMAT_R16G16B16A16_SFLOAT,
        qvk.images[VKPT_IMG_TAA_OUTPUT],
        qvk.images_views[VKPT_IMG_TAA_OUTPUT],
        VK_IMAGE_LAYOUT_GENERAL,
        VK_FORMAT_R16G16B16A16_SFLOAT,
        qvk.extent_screen_images.width, qvk.extent_screen_images.height,
        qvk.extent_render.width, qvk.extent_render.height,
        qvk.extent_taa_images.width, qvk.extent_taa_images.height);

    dlss_sl_rr_evaluate(cmd_buf);

    IMAGE_BARRIER(cmd_buf,
        .image            = qvk.images[VKPT_IMG_TAA_OUTPUT],
        .oldLayout        = VK_IMAGE_LAYOUT_GENERAL,
        .newLayout        = VK_IMAGE_LAYOUT_GENERAL,
        .srcAccessMask    = VK_ACCESS_SHADER_WRITE_BIT,
        .dstAccessMask    = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT,
        .subresourceRange = { .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                              .levelCount = 1, .layerCount = 1 });

    dlss_apply_reflex_if_needed();
}

void vkpt_dlss_final_blit(VkCommandBuffer cmd_buf, bool waterwarp)
{
    if (!vkpt_dlss_is_enabled() || s_dlss_out_view == VK_NULL_HANDLE) return;

    /* Use the render-pass path (same as FSR) — samples the DLSS output image
       (VK_IMAGE_LAYOUT_GENERAL) and writes to the swapchain framebuffer.
       Swapchain layout transitions are handled inside vkpt_final_blit_view.
       MUST pass extent_unscaled (display resolution) as input_dimensions.
       The final_blit shader computes: uv *= input_dims / taa_image_size
       where taa_image_size = max(screen_images, unscaled) = unscaled when DLSS
       upscaling is active. With input_dims == extent_unscaled the scale is 1.0,
       so UV [0,1] covers the full DLSS output (which is always at display res).
       Passing extent_render instead would cause a zoom effect: UV would be scaled
       to render_res/display_res fraction of the image. */
    vkpt_final_blit_view(cmd_buf, s_dlss_out_view, qvk.extent_unscaled, false, waterwarp);
}

VkImageView vkpt_dlss_get_output_view(void)
{
    return s_dlss_out_view;
}
