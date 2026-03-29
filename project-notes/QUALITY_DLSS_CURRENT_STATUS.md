# DLSS Quality Improvements Status

Date: 2026-03-26

## Current Base
- Stable runtime still pinned at `Q2RTX\\q2rtx.exe` with SHA256 `C2C8D53E1DC1C090E310998BF7C01C9CD2EDE5CBC1A45A7AA96A7F8494BD6319`.
- New candidate built from current sources is stored as `Q2RTX\\q2rtx_dlss_quality_lod_diag.exe`.

## Confirmed Findings So Far
1. Q2RTX already had a prior fix that forces DLSS SR to use render-resolution TAA input instead of display-resolution TAAU input.
2. The existing negative `pt_texture_lod_bias` compensation path only triggered for:
   - FSR
   - DLSS RR
   - legacy `AA_MODE_UPSCALE`
3. DLSS SR uses `AA_MODE_TAA`, so aggressive DLSS SR modes were not receiving the engine's existing low-resolution mip compensation.
4. This makes engine-side mip selection a strong first root-cause candidate for blur/muddiness before DLSS reconstruction.

## Changes Applied In This Pass
### 1. DLSS-aware mip/LOD policy
Added new cvars in `src/refresh/vkpt/main.c`:
- `r_dlss_mip_bias_enable`
- `r_dlss_mip_bias_global`
- `r_dlss_mip_bias_min`
- `r_dlss_mip_bias_max`
- `r_dlss_mip_bias_ultra_performance_scale`
- `r_dlss_mip_bias_performance_scale`
- `r_dlss_mip_bias_balanced_scale`
- `r_dlss_mip_bias_quality_scale`
- `r_dlss_mip_bias_ultra_quality_scale`
- `r_dlss_mip_bias_dlaa_scale`

Implementation notes:
- When DLSS SR is active and actually upscaling, mip bias now uses the real render/output extent ratio (`extent_render` vs `extent_unscaled`) instead of falling back to legacy FSR/TAAU-only logic.
- The effective bias is clamped by `r_dlss_mip_bias_min/max`.
- Existing FSR / RR / TAAU behavior was kept intact.

### 2. Basic DLSS quality overlay
Added debug cvars:
- `r_debug_dlss_overlay`
- `r_debug_accumulation_state`

Overlay currently shows:
- DLSS mode
- render resolution -> output resolution
- effective texture mip bias
- current DLSS render scale ratio
- RR state
- MFG state
- denoiser state
- accumulation count (when `r_debug_accumulation_state 1`)

Files changed:
- `Q2RTX-src\\src\\refresh\\vkpt\\main.c`
- `Q2RTX-src\\src\\client\\main.c`
- `Q2RTX-src\\src\\client\\screen.c`

## Build Produced
- `Q2RTX\\q2rtx_dlss_quality_lod_diag.exe`
- Built from `build-claude-off` Release

## Next Planned Steps
1. User test visual/fps impact of the new DLSS mip bias defaults.
2. If image becomes sharper without unacceptable shimmer, keep this as baseline and tune defaults.
3. Then continue Phase 1 diagnostics for pre-DLSS softness, likely in the TAA / denoiser path.

## 2026-03-26 Follow-up
- User screenshots showed no visible difference between `r_dlss_mip_bias_enable 0/1`.
- Root cause of the invalid comparison: overlay showed `mip -2.00` in both cases, meaning manual `pt_texture_lod_bias` was already saturating the final bias.
- Added overlay breakdown for `base`, `auto`, and `final` mip bias to make future DLSS quality tests deterministic.
- New candidate: `Q2RTX\\q2rtx_dlss_quality_lod_diag_v2.exe`.

## 2026-03-26 TAA Input Tuning Pass
- Implemented first real pre-DLSS softness reduction path instead of only LOD bias.
- Added DLSS-specific TAA input tuning cvars:
  - `r_dlss_taa_input_tuning_enable`
  - `r_dlss_taa_history_weight_ultra_performance`
  - `r_dlss_taa_history_weight_performance`
  - `r_dlss_taa_history_weight_balanced`
  - `r_dlss_taa_anti_sparkle_scale`
  - `r_dlss_taa_variance_scale`
- `flt_taa_history_weight` was previously effectively unused in `asvgf_taau.comp`; shader now uses it as a minimum new-frame weight floor.
- Overlay now shows effective `TAA`, `AS`, and `VAR` values alongside `base/auto/final` mip bias.
- New candidate: `Q2RTX\\q2rtx_dlss_quality_taa_input_diag.exe`.
- Streamline startup regression reproduced with long test binary name `q2rtx_dlss_quality_taa_input_diag.exe`, while the exact same binary runs with DLSS working when renamed to short `q2rtx_taa.exe`.
- For further DLSS quality experiments, use short executable names to avoid false negatives in SL startup.

## 2026-03-26 Stage Visualization Pass
- Added debug cvars:
  - `r_debug_pre_dlss_color`
  - `r_debug_post_dlss_color`
  - `r_debug_post_tonemap_color`
- These route the final blit to explicit pipeline stages so we can compare whether softness already exists in `TAA_OUTPUT` before DLSS or appears only after DLSS evaluation.
- Added helper export `vkpt_dlss_get_output_view()` for post-DLSS visualization.
- New short-name candidate: `Q2RTX\\q2rtx_dbg.exe`.
## 2026-03-26 Pre-vs-Post DLSS Finding
- User-tested stage visualization screenshots in `Q2RTX\\test` show that `post_dlss` looks noticeably better than `pre_dlss`.
- This is an important pivot: DLSS reconstruction itself is helping, not causing the main softness complaint.
- Therefore the next focused test is no longer "make DLSS sharper" but "strip pre-DLSS temporal softness from the DLSS input and see whether Ultra Performance improves materially".

## 2026-03-26 Hard TAA History Bypass Diagnostic
- Added debug cvar `r_debug_dlss_bypass_taa_history`.
- When enabled during DLSS SR upscale (with RR disabled), it forces:
  - `flt_taa_history_weight = 0.0`
  - `flt_taa_anti_sparkle = 0.0`
  - `flt_taa_variance = 0.0`
- Overlay mirrors these values so the test state is explicit.
- Fresh Release build copied to short-name candidate:
  - `Q2RTX\\q2rtx_taa_bypass.exe`
- Goal of this test:
  - if image becomes substantially crisper (even with more noise/shimmer), then the main blur source is pre-DLSS temporal accumulation/clamping;
  - if image barely changes, the blur is entering even earlier than current TAA history/clamp controls.
## 2026-03-26 Confirmed Pre-DLSS Temporal Blur Source
- User compared `r_debug_dlss_bypass_taa_history 0/1` on short-name candidate `q2rtx_taa_bypass.exe`.
- Result: bypass state (`history=0, anti_sparkle=0, variance=0`) looked noticeably better than baseline.
- This is the strongest DLSS-quality finding so far:
  - major blur is introduced in the pre-DLSS temporal smoothing / clamping path;
  - DLSS reconstruction itself is not the main blur source.

## 2026-03-26 Mode-Aware Temporal Input Profile
- Added new cvar: `r_dlss_taa_input_profile`
  - `0` = legacy / previous tuning path
  - `1` = softer quality-preserving reduction for aggressive DLSS modes
  - `2` = stronger reduction for aggressive DLSS modes (default in the new test build)
- Profile affects only DLSS SR upscale modes and keeps RR path excluded.
- New tuned behavior:
  - Ultra Performance: strongest reduction
  - Performance: strong reduction
  - Balanced: moderate reduction
  - Quality / Ultra Quality / DLAA: legacy behavior
- Fresh short-name candidate:
  - `Q2RTX\\q2rtx_taa_profile.exe`
- Goal:
  - keep most of the visual gain from full bypass,
  - but move from a crude debug switch to a usable DLSS-specific quality policy.
## 2026-03-26 Streamline Executable Name Regression (Again)
- User reported that `r_dlss_taa_input_profile 1/2` appeared to disable DLSS on `q2rtx_taa_profile.exe`.
- Inspection of `Q2RTX\\sl_debug.log` showed this was not caused by the profile logic itself.
- Root cause: Streamline crashed early for executable name `q2rtx_taa_profile.exe` before normal DLSS initialization:
  - `Seems like some DX/VK APIs were invoked before slInit()`
  - followed immediately by `Exception detected` and minidump creation under `ProgramData\\NVIDIA\\Streamline\\q2rtx_taa_profile`
- Therefore this was a false negative caused by executable naming/runtime startup, not by `r_dlss_taa_input_profile` values.
- Current candidate was re-copied under known-safe short name:
  - `Q2RTX\\q2rtx_taa.exe`
## 2026-03-26 Motion-Adaptive TAA Stabilization Pass
- User feedback on `r_dlss_taa_input_profile`:
  - profile `2` gives the best static sharpness;
  - at the time of this checkpoint it appeared to introduce strong residual halos / ghosting in motion;
  - profiles `0` and `1` are more stable but too soft.
- Implemented a motion-adaptive stabilization change in `asvgf_taau.comp`:
  - keep the sharper reduced temporal settings at low motion;
  - restore part of anti-sparkle and NCC variance clamp as local motion increases.
- This targets the exact observed tradeoff:
  - preserve static sharpness from the aggressive profile,
  - reduce motion artifacts by feeding DLSS a less unstable input during movement.
- Updated short-name test executable:
  - `Q2RTX\\q2rtx_taa.exe`

## 2026-03-26 Updated Finding After Runtime Shader Refresh
- After the refreshed runtime `asvgf_taau.comp.spv` was copied into `Q2RTX\\baseq2\\shader_vkpt`, the old motion verdict for `r_dlss_taa_input_profile 2` became outdated.
- User re-tested motion and reported that profile `2` no longer shows the earlier obvious ghosting problem.
- Current practical conclusion:
  - `r_dlss_taa_input_profile 2` is the best known baseline for aggressive DLSS SR modes at the moment;
  - profiles `0` and `1` remain softer and are currently less attractive as defaults;
  - further DLSS quality work should proceed from profile `2`, not from the legacy profile.

## 2026-03-27 RR Reflection Stabilization Breakthrough
- User found an important clue: setting in-game `global illumination` to `low` dramatically reduced bright speckles in RR reflections.
- Follow-up code review showed that RR path was explicitly disabling several q2rtx specular/indirect stabilizers.
- Added `flt_dlss_rr_specular_stabilizers` modes and tested them on the same mirror/glass scene.
- Confirmed result:
  - mode `1` reduced reflection speckles significantly;
  - mode `2` removed the residual bright speckles almost completely without forcing GI to `low`.
- Current practical conclusion:
  - `flt_dlss_rr_specular_stabilizers 2` is now the new RR baseline;
  - this improves RR reflections without changing global GI quality settings or bounce counts.

## 2026-03-27 Ultra-Mega-Quality Test Config
- Added a reusable high-end artifact-hunting config:
  - `Q2RTX\\baseq2\\q2rtx_ultra_mega_quality.cfg`
  - mirrored in source at `Q2RTX-src\\baseq2\\q2rtx_ultra_mega_quality.cfg`
- Goals of this cfg:
  - force maximum practical GI / reflections / glass / caustics settings
  - disable effects that make artifact inspection harder (`pt_dof`, accumulation rendering, HDR)
  - leave all DLSS / RR mode switching to the in-game menu so the user can test those combinations manually
- Follow-up finding:
  - `pt_enable_surface_lights 2` reintroduced RR ceiling-reflection speckles
  - reason: mode `2` synthesizes emissive materials for all BSP `SURF_LIGHT` surfaces, which can create extra tiny bright emitters
  - the config baseline was therefore revised to `pt_enable_surface_lights 1`
  - optional aliases were added so the user can still toggle the more extreme mode manually
- Key convenience aliases:
  - `umq_apply`
  - `umq_overlay_on` / `umq_overlay_off`
  - `umq_surflights_stable` / `umq_surflights_extreme`
  - `umq_restart`
  - `umq_dump`
## 2026-03-26 Render-Extent Recreate Fix
- Found a concrete engine-side bug in `R_BeginFrame_RTX()`.
- Before this fix, swapchain/image recreation was triggered by:
  - window mode changes
  - `extent_screen_images` changes
  - HDR/VSync/MFG changes
- But it did NOT trigger on plain `extent_render` changes caused by switching DLSS SR mode or enabling RR.
- This can leave render-sized images at the old size while Streamline receives a new render subrect.
- That matches the observed symptoms:
  - frozen world image while HUD keeps animating
  - DLSS Ultra Quality freezing
  - RR freezing after recent quality experiments
- Fix applied:
  - detect `extent_render` change explicitly
  - detect derived `extent_taa_images` change explicitly
  - recreate swapchain/image resources when either changes
- Updated short-name test executable:
  - `Q2RTX\\q2rtx_taa.exe`
- Re-copied `Q2RTX\\q2rtx_taa.exe` after confirming the previous runtime file had not actually updated.
- Current intended test binary is the refreshed short-name executable built from the render-extent recreate fix branch.

## 2026-03-26 Custom Scale + RR Contract Pass
- Current DLSS runtime in this project rejects formal `Ultra Quality` mode:
  - `NGXDLAA::CreateDlssInstance ... PerfQuality_Value_UltraQuality ... unsupported`
  - `getOptimalSettings returns invalid size (0 x 0)`
- Because of that, `Ultra Quality` was removed from the in-game DLSS mode menu.
- Added new user-facing mode:
  - `Custom`
  - menu slider: `flt_dlss_custom_ratio`
  - range: `33% .. 99%`
- `Custom` computes render resolution directly from output resolution instead of relying on DLSS preset buckets.
- Internally, `Custom` still maps to the nearest supported Streamline quality bucket so DLSS SR / RR can be configured safely.
- Backward compatibility is preserved:
  - hidden/manual `flt_dlss_mode 5` still exists for old configs
  - if current runtime rejects it, `dlss_sl.cpp` emulates old Ultra Quality behavior via a custom ~77% ratio on top of supported `Quality`.

### Ray Reconstruction
- RR was not dropped.
- RR tagging in `dlss_sl.cpp` was corrected to match the Streamline contract:
  - `kBufferTypeDepth` instead of `kBufferTypeLinearDepth`
  - `kBufferTypeMotionVectors`
  - required RR resources now use `eValidUntilEvaluate`
- This is meant to address the recent symptom where the world image freezes while HUD animation continues.

### Runtime updated
- Fresh Release build copied to:
  - `Q2RTX\\q2rtx.exe`
- Updated menu copied to:
  - `Q2RTX\\baseq2\\q2rtx.menu`

## 2026-03-26 RR Freeze Follow-up
- User config revealed `r_debug_post_dlss_color 1` was still enabled in normal runtime testing.
- In `R_EndFrame_RTX()` this debug path was taking priority over the RR final blit path:
  - if DLSS was enabled and a standalone DLSS output view existed, the engine blitted that stale SR debug image even while RR was active.
  - Result matched the user symptom exactly:
    - world image appears frozen / stale
    - HUD continues animating on top.
- Fix applied:
  - `debug_post_dlss` no longer overrides final blit when `vkpt_dlss_rr_is_enabled()`.
- Menu UX improvement applied:
  - `DLSS custom scale` slider now always stays visible
  - it is still only semantically used by `Custom` mode, but no longer requires closing/reopening the menu to appear.

## 2026-03-26 Stable Working Checkpoint
- User-confirmed current state:
  - `DLSS SR` works
  - `DLSS custom scale` slider is visible in the menu
  - `DLSS RR` now works again and no longer freezes the world image due to the old debug-post-DLSS override
- Current runtime checkpoint:
  - `Q2RTX\\q2rtx.exe`
  - SHA256: `2BDBEEC3ACE0670736F2C8C5DA39B3B854C92B6F1C81FB08B1D16676818BAC38`
  - LastWriteTime: `2026-03-26 19:14:46`
- Current known design decisions:
  - `Ultra Quality` stays removed from the menu because the current DLSS runtime rejects it natively
  - `Custom` is the supported replacement for fine-grained SR scaling
  - hidden/manual legacy mode `5` is still tolerated for backward compatibility
- Current open work items:
  - continue DLSS image-quality tuning under aggressive modes
  - later return to deeper RR / temporal-quality improvements if needed

## 2026-03-26 RR vs Legacy Denoiser Consistency Pass
- User-provided comparison captures in `Q2RTX\\test` showed that `RR ON + Denoiser OFF` and `RR ON + Denoiser ON` still produced materially different results.
- That indicated the legacy denoiser toggle was still influencing the RR path indirectly, which should not be the intended user-facing behavior.
- Root causes found in `main.c`:
  - `evaluate_taa_settings()` returned early when legacy denoiser was disabled, even if RR was active, leaving RR on the wrong `extent_taa_output` / AA scheduling path.
  - `prepare_ubo()` treated `flt_enable` purely as the legacy denoiser switch, even though RR prep and checkerboard-resolve helpers still depend on the temporal path being logically active.
- Fix applied:
  - introduced `is_dlss_rr_active_for_frame()`
  - RR now keeps the temporal reconstruction path alive even when the legacy denoiser toggle is off
  - `ubo->flt_enable` is driven by `legacy_denoiser || RR_active_for_frame`, so RR prep no longer changes behavior based on the user's denoiser toggle
  - RR-specific anti-flicker/fake-specular overrides are now keyed off active RR, not merely the raw cvar state
- UX clarification:
  - menu text for `denoiser` now explicitly says it is ignored while `DLSS RR` is enabled
  - overlay reports denoiser state as `replaced` while RR is actively owning the reconstruction path
- Fresh test build:
  - `Q2RTX\\q2rtx_rr.exe`
  - SHA256: `4C6C18656076948C408671C4E29BD73EDF7F2B505F168708863A28814D680AB9`

## 2026-03-26 RR Mirror / Specular Guide Follow-up
- User-provided mirror video (`Q2RTX\\test\\Quake 2 RTX Remaster 2026.03.25 - 10.33.55.01.mp4`) showed a separate issue:
  - mirror reflections look stable with RR off
  - but become noticeably noisy with RR on
- Inspection of `dlss_sl_rr_tag_resources()` found that Q2RTX was already generating RR specular guide textures in `dlss_rr_prep.comp`, but the RR integration was still omitting them from the actual Streamline tag list.
- For highly reflective / mirror-like surfaces this is a strong candidate, because DLSS-RR explicitly supports:
  - `kBufferTypeSpecularHitDistance`
  - `kBufferTypeSpecularRayDirectionHitDistance`
- Fix applied:
  - both specular guide resources are now tagged in `dlss_sl_rr_tag_resources()`
  - required world/camera matrices were already being provided via `dlss_sl_rr_set_options()`, so this closes the missing part of the RR contract
- Fresh mirror-focused test build:
  - `Q2RTX\\q2rtx_rr_specfix.exe`
  - SHA256: `4041C774CE8E2036C4E8817314297A0891C851B23E090950B6BEF20B319A7866`

## 2026-03-26 RR Mirror Status Update
- Mirror/glass quality remains an open RR issue, but the current understanding is now more precise:
  - the latest user comparison screenshots suggest the bug is not just “more RR noise”;
  - it looks like a checkerboard reflection guide mismatch, where RR receives resolved reflective color/motion from one split branch and auxiliary guides from the other.
- Practical current state:
  - `RR` is still functionally working again after the earlier freezes;
  - the active mirror-focused validation pass is now primarily shader-side, not exe-side;
  - runtime shader `Q2RTX\\baseq2\\shader_vkpt\\dlss_rr_prep.comp.spv` has been updated to keep checkerboard RR guides on the same `src_pos` branch as the resolved input.
- Current recommended validation executable:
  - `Q2RTX\\q2rtx_rr_restore.exe`

## 2026-03-27 Overlay Metadata Pass
- `r_debug_dlss_overlay 1` now also prints feature metadata in a separate line.
- Added runtime reporting for:
  - active SR preset letter (`DLSS`)
  - active RR preset letter (`RR`)
  - loaded `nvngx_dlss.dll` version when SR is active
  - loaded `nvngx_dlssd.dll` version when RR is active
  - loaded `nvngx_dlssg.dll` version when MFG is active
- Versions are queried from the loaded module file version info at runtime, so the overlay reflects the actual DLLs currently in use.
- Updated runtime executable:
  - `Q2RTX\\q2rtx.exe`
  - SHA256 `CF705DD46D7629680AF085E17C55D7583A8CB9AA8A49DD6B0674741CF6BB269E`

## 2026-03-27 Overlay Draw Decoupling
- Fixed `r_debug_dlss_overlay` so it no longer depends on `scr_fps` being enabled.
- Before this fix the DLSS overlay was drawn inside `SCR_DrawFPS()`, so it never appeared when `scr_fps 0`.
- Overlay drawing is now handled by a dedicated `SCR_DrawDlssOverlay()` pass.
- Behavior after fix:
  - `r_debug_dlss_overlay 1` works independently
  - if `scr_fps` is also enabled, the overlay is drawn one text line below the FPS line
- Updated runtime executable:
  - `Q2RTX\\q2rtx.exe`
- Overlay now also shows current Reflex state as `requested -> effective`.
- This reflects both the user-selected Reflex mode and the mode actually enforced by the runtime/policy.
- Overlay layout was reformatted into dedicated lines:
  1. DLSS / SR / RR presets and DLLs
  2. MFG / Reflex / FG DLL
  3. Render/output scaling information
  4. MIP and image-stability parameters
- Reflex now avoids duplicate text: when requested and effective mode match, only one Reflex state is shown.
