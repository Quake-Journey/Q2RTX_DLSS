# Q2RTX 0.95 beta - MFG motion investigation, 2026-09-11

## Outcome

**Unresolved. No motion fix is shipped by this investigation.** The user reports that the entire world moves smoothly at 3X but at 4X-6X looks closer to MFG off, despite high reported output FPS. This applies to movement and camera rotation, not only mouse response. It existed with the previous game and DLL versions. The user explicitly rejected the earlier Reflex 300 cap as a visual solution. RR disablement was an isolation experiment, not a proposed remedy or evidence that RR caused this bug.

The runtime and touched source are restored to the state immediately before this investigation. That preserves the accepted RR shimmer fix, official updated DLLs, 0.95 beta, startup-window fix, and earlier live Reflex-option/cap corrections. The restored EXE SHA256 is `013eb548ac83dcbf052aedb6247102d0f34c85d7228d081c3994af7c3959d0f0`.

## Methods and limitations

Tests use an isolated copy of save0 and q2config, sound disabled before initialization, fullscreen 5160x2160, and foreground-window checks. They do not modify shared user configuration or system audio. Constant keyboard yaw and injected relative mouse movement were both exercised. RR F stays enabled in all final comparisons. The majority of controlled experiments use Performance (2580x1080), fixed MFG policy, and equal 240 output caps. The final `po*` comparisons use the user's Custom 75% (3870x1620), Auto policy, and 300 cap.

The Desktop Duplication helper captures a center strip, and OpenCV tracks scene features between successive captured images. Native HDR format and asynchronous staging were tested to reduce capture overhead. Captured images contain distinct intermediate world positions at 6X; they are not simply six identical copies. The diagnostic displacement IQR/median is roughly 0.08-0.09 at 3X versus 0.21-0.30 at 6X in controlled runs (`native3`, `native6`, `pipeline6`, `slow3`, `slow6`). This flags irregular spatial steps in the sampled images; it does not by itself establish the cause of the severe subjective defect.

**Do not call Desktop Duplication capture FPS the physical display FPS.** AccumulatedFrames records updates that the capture did not acquire separately. Captures lose some updates, especially at 6X. Timestamp alignment with PresentMon places these notifications near GPU completion rather than hardware-metered display time. DDA timestamp bursts therefore cannot establish on-screen presentation bursts. Capture also adds GPU work; use the final `*_no_capture` PresentMon runs for pacing conclusions.

The standalone official PresentMon 2.5.1 executable was downloaded and checked against its release SHA256. It records actual display-change timing, flip delay, and presentation mode without the image-capture helper. `pm_only3` and `pm_only6` establish approximately 240 display changes/s at the lighter Performance workload. All rows are labeled Application by this driver/provider path, so that label cannot distinguish generated from native frames. Actual MFG multipliers must instead be checked against Streamline logs. Simultaneous FrameView + PresentMon attempts `pm3` and `pm_images6` produced no usable timing CSV and are excluded. `pm6` has a usable steady interval but an incomplete final CSV row after forced collector termination; final comparisons use the standalone runner with normal timed collector shutdown.

## Rejected experiments

Build caveat: the closing build exposed a stale object from the abandoned early-input trial after a source rollback preserved an older file timestamp. The final restored-source build explicitly touches all four restored source files to force recompilation. Do not treat earlier incremental trial binaries as certified single-variable experiments without checking which restored files were actually recompiled. The final `po*` tests do not have this ambiguity: they execute the exact original baseline EXE, verified by SHA256 in every result.json.

- RR off: irregular motion remains; accepted RR jitter correction is not reverted.
- Copying MFG depth/motion/HUDless inputs to a ring: no demonstrated smoothness fix.
- Parallel-queue mode with the ring: no fix; produced more near-zero motion pairs.
- RR-prepared 2D motion-vector input instead of raw path-tracer motion input: no fix.
- Common-matrix conversion change: no fix.
- Removing HUDless color input: worse diagnostic motion variation.
- Device-depth/projection/camera-basis experiments, separately and combined with prepared motion: no fix. The first depth trial clamped to 1 due to the engine projection convention and is invalid as a depth-format test; the later complete projection trial still failed.
- Zero camera jitter: no fix; removed to preserve the accepted RR path.
- FIFO presentation: insufficient improvement and no established visual fix.
- Screenshot Vulkan layer: hung while MFG was active, produced no usable capture; owned process was terminated. This is not a clean validation result.
- Moving Reflex sleep before input: an input-age measurement found an additional frame of stale mouse data, but the implementation trial was abandoned before runtime testing after the user clarified that latency was not the reported symptom. The partial edit and diagnostic camera logging were removed.

The baseline already presents on `qvk.queue_graphics`. A separately reserved `qvk.queue_present` exists but is not used by the actual Present call. Do not revive the earlier incorrect claim that changing from a separate present queue to the render queue was a meaningful baseline comparison.

## Artifacts

- `run_motion.py`: isolated launch, foreground enforcement, FrameView and DDA capture.
- `run_pm.py`: standalone PresentMon collection; `collector_exit` refers to PresentMon, despite legacy names inherited from the runner. The `presentmon` argument flag is not a backend indicator in this runner; it always runs PresentMon.
- `analyze_pm.py`: steady-window presentation statistics; malformed rows and unavailable metrics are excluded. Inspect the raw CSV before treating an unavailable metric as a dropped frame.
- `analyze_motion.py`, `check_motion.py`: optical-flow motion diagnostic; the 0.2 spread threshold is only a diagnostic threshold, not the user's acceptance criterion.
- `align_images.py`: DDA notification versus presentation/GPU/display timestamp comparison.
- Per-run `result.json`: actual arguments, foreground transitions, silence, config preservation, crash checks, EXE hash.
- `verification.json`: final source/runtime restoration, build result, and process/service cleanup.

Raw captures and logs are retained. No release packaging or public documentation was prepared during the investigation; the later 0.95 release archives its findings here. The user's 15-20 minute limit terminates this investigation; do not automatically resume it from this note.

## Final result without image-capture overhead

`po3_no_capture` and `po6_no_capture` run the exact restored EXE, Custom 75%, RR F, Auto MFG policy, Reflex 300 cap, foreground fullscreen, silent, with no Desktop Duplication helper. The standalone PresentMon collector terminates normally on its timer. CSV timestamps and `MsUntilDisplayed` are combined to reconstruct chronological display times; both runs have zero reversals of presentation order. This also handles simultaneous display timestamps rather than interpreting unavailable display-change metrics as a proven dropped frame.

| Mode | Mean display interval | Intervals under 1 ms | Intervals over 8 ms | p99 interval | Maximum |
| --- | ---: | ---: | ---: | ---: | ---: |
| 3X | 4.892 ms | 0% | 0.147% | 6.472 ms | 13.527 ms |
| 6X | 3.382 ms | 21.979% | 9.370% | 13.262 ms | 27.654 ms |

The 6X run has a higher average output rate but much more uneven display intervals. The 300 cap is not a fix under these settings. This is a measured output-cadence defect consistent with the report, **not a proven explanation of the exact visual severity or a proven code-level root cause**. Lighter-workload fixed-policy 240-cap tests cannot be generalized to this case. No new workaround is prescribed.

## Primary references

- https://github.com/NVIDIA-RTX/Streamline/blob/v2.14.1/docs/ProgrammingGuideDLSS_G.md
- https://github.com/GameTechDev/PresentMon/blob/v2.5.1/README-ConsoleApplication.md
- https://learn.microsoft.com/en-us/windows/win32/api/dxgi1_2/ns-dxgi1_2-dxgi_outdupl_frame_info
- https://learn.microsoft.com/en-us/windows/win32/api/dxgi1_5/nf-dxgi1_5-idxgioutput5-duplicateoutput1

An external Streamline Vulkan pacer semaphore report exists at https://github.com/NVIDIA-RTX/Streamline/issues/112. It is not established as the cause of this game's symptom and is not a substitute for reproducing the offending synchronization here.

## Permanent archive

This directory was added to project history for the 0.95 beta release. The two final raw PresentMon CSVs, case metadata, analyses, and analysis scripts are stored alongside this note. The full image captures, unsuccessful trials and launch harnesses remain in `O:\Claude2\_agent_temp\q2rtx_mfg_motion_20260911`. Analysis scripts in this archive operate on the saved CSVs and do not launch the game.

The two final cases use the same tested EXE hash. Their settings and active-window evidence are in result.json. These archived metadata retain historical local paths; publication does not initiate another investigation.
