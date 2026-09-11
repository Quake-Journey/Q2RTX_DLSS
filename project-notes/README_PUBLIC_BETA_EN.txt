Q2RTX DLSS Edition by ly - Public Beta 0.95
Streamline 2.14.1 / DLSS 310.9.1
Based on Q2RTX 1.8.1

0.95 beta update (2026-09-11)
Full release notes: CHANGELOG_0.95_EN.md.
Fixed RR surface/lighting shimmer; added RR preset F and made it the default.
MFG 4X-6X motion smoothness remains a known issue. A Reflex cap of 300 does not fix it.
Dynamic MFG uses the Auto fallback on Vulkan, without native dynamic multiplier selection.
DeepDVC may display an NVIDIA service/development notice and is off in the starter configuration.
Package starter configuration: RR on / F, MFG 3X Fixed, DeepDVC off, experimental MFG input buffering off.
The Reflex cap includes generated frames in the total output FPS; 0 disables the cap.

Purpose
This public beta build is intended for testing NVIDIA Streamline 2.14.1, DLSS Super Resolution, DLSS Ray Reconstruction, DLSS Multi Frame Generation, NVIDIA Reflex, and RTX Dynamic Vibrance / DeepDVC in Q2RTX.

The build targets owners of compatible NVIDIA RTX GPUs and is meant for image-quality comparisons, performance testing, compatibility testing, and real gameplay validation of the new DLSS features.

Important
- Original game .pak files are not included in this package for licensing reasons.
- A lawful existing Quake II RTX / Quake II RTX Remaster installation with the required base game data is required.
- This package is an overlay for an existing Q2RTX directory.
- The package intentionally includes a working baseq2\q2config.cfg. Back up your own baseq2\q2config.cfg before installing.
- This is a beta/release-candidate build. Some modes depend on GPU, driver, and the current NVIDIA runtime.

Installation
1. Back up your current Q2RTX folder, or at least baseq2\q2config.cfg.
2. Extract the archive into your installed Quake II RTX Q2RTX directory.
3. Allow file replacement.
4. Make sure your lawful .pak files remain in baseq2.
5. Run q2rtx.exe.

Included in Public Beta 0.95
- NVIDIA Streamline 2.14.1.
- DLSS 310.9.1 runtime.
- NVIDIA DLSS Super Resolution.
- DLAA.
- DLSS Custom Scale.
- DLSS Ray Reconstruction.
- DLSS Multi Frame Generation 2X / 3X / 4X / 5X / 6X.
- Fixed / Auto / Dynamic-Variable MFG policy controls.
- Separate Variable MFG max limit.
- Dynamic/Variable MFG target FPS.
- NVIDIA Reflex.
- NVIDIA Reflex FPS cap.
- RTX Dynamic Vibrance / DeepDVC.
- Live application of all NVIDIA DLSS submenu parameters.
- Dedicated Video -> NVIDIA DLSS submenu.
- Improved colored DLSS/FPS performance overlay.
- Overlay above menus, but only when a game map is loaded.
- Menu background opacity control.
- Convenient r_maxfps menu entry inside the DLSS menu.

Video Menu
The core DLSS options are now available under:

Video -> NVIDIA DLSS

Parameters are applied live without closing the menu. This is intended for testing with a transparent menu and the overlay enabled.

Main Menu Options

1. Max FPS
- CVar: r_maxfps
- Range: 0..1000
- 0 = disabled.
- This is the standard Q2RTX variable, exposed in the menu for DLSS/MFG/Reflex testing convenience.

2. NVIDIA DLSS
- CVar: flt_dlss_enable
- Enables or disables DLSS Super Resolution.

3. DLSS mode
- CVar: flt_dlss_mode
- Values:
  - Ultra Performance
  - Performance
  - Balanced
  - Quality
  - Custom
  - DLAA

4. DLSS custom scale
- CVar: flt_dlss_custom_ratio
- Range: 33..99
- Used by Custom mode.

5. DLSS preset
- CVar: flt_dlss_preset
- Values:
  - recommended
  - F
  - J
  - K
  - L
  - M
- Recommended uses the current preset-selection logic for each DLSS mode.

6. DLSS sharpness
- CVar: flt_dlss_sharpness
- Range: 0..1.

7. DLSS auto-exposure
- CVar: flt_dlss_auto_exposure

8. RTX Dynamic Vibrance
- CVar: flt_deepdvc
- Streamline DeepDVC / RTX Dynamic Vibrance.
- Intended for SDR. HDR is not a target mode for DeepDVC.

9. DeepDVC intensity
- CVar: flt_deepdvc_intensity
- Range: 0..1.
- Applied live on a loaded map.

10. DeepDVC saturation
- CVar: flt_deepdvc_saturation_boost
- Range: 0..1.
- Applied live on a loaded map.

11. DLSS Ray Reconstruction
- CVar: flt_dlss_rr
- When RR is active, the legacy denoiser is considered replaced.

12. DLSS RR preset
- CVar: flt_dlss_rr_preset
- Values:
  - default
  - D
  - E
  - F

13. DLSS MFG
- CVar: flt_dlss_mfg
- Values:
  - off
  - 2X
  - 3X
  - 4X
  - 5X
  - 6X

14. DLSS MFG policy
- CVar: flt_dlss_mfg_policy
- Values:
  - fixed
  - auto
  - dynamic

15. DLSS MFG variable max
- CVar: flt_dlss_mfg_dynamic_max
- Values:
  - auto
  - 2X
  - 3X
  - 4X
  - 5X
  - 6X
- Used as the upper bound for Dynamic/Variable MFG.

16. DLSS MFG dynamic target
- CVar: flt_dlss_mfg_dynamic_target_fps
- Range: 0..480
- 0 = auto from display refresh.

17. DLSS MFG queue mode
- CVar: flt_dlss_mfg_queue_parallelism
- Values:
  - default
  - parallel

18. NVIDIA Reflex
- CVar: flt_dlss_reflex
- Values:
  - off
  - on
  - on + boost

19. NVIDIA Reflex FPS cap
- CVar: flt_dlss_reflex_fps_cap
- Range: 0..480
- 0 = disabled.

20. DLSS debug overlay
- CVar: r_debug_dlss_overlay
- Shows DLSS/RR/MFG/Reflex/DeepDVC state, presets, DLL versions, and render parameters.

Additional Video Options

Menu opacity
- CVar: cl_menu_alpha
- Range: 0..1
- Lets you adjust the menu background opacity while playing.

FPS counter
- CVar: scr_fps
- Values:
  - off
  - show FPS
  - show FPS and resolution scale

Overlay
When r_debug_dlss_overlay 1 and/or scr_fps is enabled, the overlay is shown:
- during gameplay on a loaded map;
- above any open menu if a map is already loaded;
- not above menus before a game map is loaded.

Limitations and Notes
- MFG 5X/6X and Variable MFG depend on GPU, driver, and the capabilities reported by the current Streamline/NVIDIA runtime.
- 2X is primarily intended for RTX 40/50.
- 3X/4X/5X/6X are primarily intended for RTX 50 and only when reported as supported by the runtime.
- For serious MFG testing, HDR is best kept off.
- DeepDVC is intended for SDR.
- RR is stable enough for regular gameplay, but complex mirror, glass, and reflection scenes may still show residual artifacts.
- Ultra Quality is not exposed as a separate menu item; use Custom scale for manual near-equivalent scaling.

Core Console Variables
- r_maxfps
- cl_menu_alpha
- scr_fps
- flt_dlss_enable
- flt_dlss_mode
- flt_dlss_custom_ratio
- flt_dlss_preset
- flt_dlss_sharpness
- flt_dlss_auto_exposure
- flt_deepdvc
- flt_deepdvc_intensity
- flt_deepdvc_saturation_boost
- flt_dlss_rr
- flt_dlss_rr_preset
- flt_dlss_mfg
- flt_dlss_mfg_policy
- flt_dlss_mfg_dynamic_max
- flt_dlss_mfg_dynamic_target_fps
- flt_dlss_mfg_queue_parallelism
- flt_dlss_reflex
- flt_dlss_reflex_fps_cap
- r_debug_dlss_overlay

Streamline Diagnostics
- flt_dlss_sl_debug_log 0 - default, sl_debug.log is not created.
- flt_dlss_sl_debug_log 1 - enables verbose Streamline logging to sl_debug.log next to q2rtx.exe.
- Use only for diagnostics.

Credits / Third-party Content
Part of the model set in this build uses data from Cinematic Mod for Quake II RTX:
https://www.moddb.com/mods/cinematic-mod-for-quake-ii-rtx/downloads

This package is released as a public beta/release-candidate build for testing. All rights to Quake II RTX, NVIDIA Streamline / NGX, and third-party components belong to their respective owners.

Project Channels
- Telegram: https://t.me/Q2RTX
- YouTube: https://www.youtube.com/@QuakeJourney
