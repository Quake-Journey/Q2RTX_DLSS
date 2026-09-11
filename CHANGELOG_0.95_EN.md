# Q2RTX DLSS Edition by ly — 0.95 beta

September 11, 2026 · Changes since 0.9

- Updated NVIDIA Streamline to 2.14.1 and the DLSS Super Resolution, Ray Reconstruction, Frame Generation and DeepDVC libraries to 310.9.1.
- Fixed surface and lighting shimmer with Ray Reconstruction enabled, including with a stationary camera. The fix passed the project owner's visual test.
- Added Ray Reconstruction preset **F** to the NVIDIA DLSS menu and made it the default. Default, D and E remain available.
- Added **VK_NV_low_latency2** support for NVIDIA Reflex when provided by the Vulkan driver, retaining the fallback path for other configurations.
- Fixed Streamline integration with Vulkan instance/surface handling, resolving the startup crashes found during the update.
- Fixed live application of NVIDIA Reflex mode and FPS-cap changes. The separate MFG render-FPS cap also takes effect immediately. The menu tooltip now explains that the Reflex cap includes generated frames.
- Removed the old empty startup window before the first rendered frame. The window is shown after the first successful Present.
- Retained an experimental MFG input-buffering menu option. It is off by default and is not a smoothness fix or a verified performance improvement.

## Known limitations

- **MFG 4X–6X motion smoothness remains unresolved.** A high total FPS can still come with uneven frame presentation. A Reflex cap of 300 is not a fix; the investigation is preserved in project history.
- Native Dynamic MFG in this Streamline version is available for D3D12. Selecting Dynamic in this Vulkan renderer falls back to Auto; this is not native dynamic multiplier adjustment.
- HDR with MFG remains unsupported in this beta. DeepDVC is intended for SDR; enabling it with the current NVIDIA runtime may display a service/development notice.

## Package

NoPAK: original game .pak files are not included. Install over an existing Quake II RTX / Quake II RTX Remaster installation. The archive includes a starter baseq2/q2config.cfg; back up your own configuration before replacing it. Package defaults: RR on, preset F, MFG 3X Fixed, DeepDVC off. Adjust settings for your GPU.
