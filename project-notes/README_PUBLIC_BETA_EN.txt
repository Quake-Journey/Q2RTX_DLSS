Q2RTX DLSS Edition by ly — Public Beta 0.8
Based on Q2RTX 1.8.1

Purpose
This public beta build is intended for testing the integration of NVIDIA DLSS Super Resolution, DLSS Ray Reconstruction, DLSS Multi Frame Generation, and NVIDIA Reflex in Q2RTX.

The build targets owners of compatible NVIDIA RTX GPUs and is meant specifically for beta testing: image-quality comparisons, artifact hunting, and compatibility testing across different systems.

Important
- The package intentionally includes the current working q2config.cfg.
- It is strongly recommended to back up your own q2config.cfg before installing.
- This is a beta release. Some modes may still be unstable or may produce artifacts in specific scenes.
- Original game pak files are not included in this package for licensing reasons. A pre-existing lawful installation of Quake II RTX / Quake II RTX Remaster with the required base game data is required.

Installation
1. Back up your current Q2RTX folder, or at least baseq2\q2config.cfg.
2. Copy the contents of the Q2RTX-Beta folder/archive into your Quake II RTX Q2RTX directory.
3. Allow file replacement.
4. Run q2rtx.exe.

Included in this beta
- NVIDIA DLSS Super Resolution
- DLAA
- DLSS Custom Scale
- DLSS Ray Reconstruction
- DLSS Multi Frame Generation 2X / 3X / 4X
- NVIDIA Reflex
- DLSS debug overlay showing presets, DLL versions, and active runtime parameters
- Improved aggressive-mode DLSS baseline via r_dlss_taa_input_profile 2
- Improved RR reflection baseline via flt_dlss_rr_specular_stabilizers 2

Main limitations
- MFG already works, but render FPS can still drop significantly in aggressive DLSS SR modes, especially Ultra Performance.
- For MFG, HDR should be considered unsupported for serious testing and is best kept off.
- RR is now stable enough to use, but some mirror, glass, and complex reflection scenes may still show residual artifacts.
- Formal Ultra Quality is hidden from the menu; use Custom scale instead.
- MFG availability depends on GPU and driver. In this beta the intended usage is:
  - 2X mainly for RTX 40/50;
  - 3X and 4X mainly for RTX 50.
  Unsupported combinations may still appear in UI but are not guaranteed to work.

Video menu: added and changed options
All core options are available under Video -> RTX.

1. NVIDIA DLSS
- CVar: flt_dlss_enable
- Purpose: enables/disables DLSS SR.

2. DLSS Ray Reconstruction
- CVar: flt_dlss_rr
- Purpose: enables DLSS RR.
- When RR is active, the legacy denoiser is treated as replaced.

3. DLSS RR preset
- CVar: flt_dlss_rr_preset
- Values:
  - 0 = default
  - 4 = D
  - 5 = E

4. DLSS mode
- CVar: flt_dlss_mode
- Values:
  - 1 = Ultra Performance
  - 2 = Performance
  - 3 = Balanced
  - 4 = Quality
  - 6 = DLAA
  - 7 = Custom
- Ultra Quality is hidden from the menu. Use Custom for near-equivalent manual scaling.

5. DLSS custom scale
- CVar: flt_dlss_custom_ratio
- Range: 33..99
- Purpose: manual render scale for Custom mode.

6. DLSS preset
- CVar: flt_dlss_preset
- Values:
  - 0 = default
  - 6 = F
  - 7 = J
  - 8 = K (Transformer)
  - 9 = L (Transformer 2)
  - 10 = M (Transformer 2)

7. DLSS sharpness
- CVar: flt_dlss_sharpness
- Range: 0..1
- Purpose: DLSS sharpening amount.

8. DLSS auto-exposure
- CVar: flt_dlss_auto_exposure
- Purpose: enables DLSS auto exposure.

9. DLSS debug overlay
- CVar: r_debug_dlss_overlay
- Purpose: shows DLSS/RR/FG mode, presets, DLL versions, Reflex state, scale, and diagnostic parameters in the top overlay.

10. DLSS MFG
- CVar: flt_dlss_mfg
- Values:
  - 0 = off
  - 2 = 2X
  - 3 = 3X
  - 4 = 4X

11. NVIDIA Reflex
- CVar: flt_dlss_reflex
- Values:
  - 0 = off
  - 1 = on
  - 2 = on + boost

12. denoiser
- CVar: flt_enable
- With RR enabled, the legacy denoiser is no longer the primary reconstruction stage and is shown in the UI as replaced.

Core console variables
User-facing / practical
- flt_dlss_enable
- flt_dlss_rr
- flt_dlss_rr_preset
- flt_dlss_mode
- flt_dlss_custom_ratio
- flt_dlss_preset
- flt_dlss_sharpness
- flt_dlss_auto_exposure
- flt_dlss_mfg
- flt_dlss_reflex
- r_debug_dlss_overlay

Advanced / beta
- r_dlss_taa_input_profile
  - 0 = legacy
  - 1 = mild tuning
  - 2 = current recommended baseline for aggressive DLSS SR modes
- flt_dlss_rr_specular_stabilizers
  - 0 = raw RR behavior
  - 1 = partial RR reflection stabilization
  - 2 = current recommended RR baseline
- r_debug_pre_dlss_color
- r_debug_post_dlss_color
- r_debug_post_tonemap_color

Recommended testing baseline
- DLSS SR: user choice
- RR: optional depending on test case
- MFG: test separately from image-quality evaluation
- Reflex: on or on + boost when using MFG
- r_dlss_taa_input_profile 2
- flt_dlss_rr_specular_stabilizers 2

DLSS overlay
When r_debug_dlss_overlay 1 is enabled, the overlay shows:
- DLSS / RR / MFG mode
- SR and RR presets
- loaded DLL versions
- Reflex state
- render resolution -> output resolution
- scale
- mip bias
- TAA / anti-sparkle / variance parameters

Credits / Third-party Content
Part of the model set in this build uses data from Cinematic Mod for Quake II RTX:
https://www.moddb.com/mods/cinematic-mod-for-quake-ii-rtx/downloads

This package is released as a public beta for testing. All rights to Quake II RTX, NVIDIA Streamline / NGX, and third-party components belong to their respective owners.


DLSS / Streamline diagnostics
- `flt_dlss_sl_debug_log 0` — default, `sl_debug.log` is not created.
- `flt_dlss_sl_debug_log 1` — enables verbose Streamline logging to `sl_debug.log` next to `q2rtx.exe`. Use only for diagnostics.
- This variable is available only via console / `q2config.cfg` and is not exposed in the in-game menu.


Project channels
- Telegram: https://t.me/Q2RTX
- YouTube: https://www.youtube.com/@QuakeJourney
