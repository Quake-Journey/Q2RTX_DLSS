# Q2RTX DLSS 0.95 beta — 2026-09-11

Current release line: 0.95 beta, Streamline 2.14.1, DLSS 310.9.1, based on Q2RTX 1.8.1.

The accepted RR shimmer fix, RR preset F, startup-window correction and live Reflex option fixes are retained. The MFG 4X–6X smoothness problem remains unresolved. No further MFG investigation is active; future work requires a new request.

- Runtime: `O:\Claude2\Q2RTX-1.8.1-GPT\Q2RTX\q2rtx.exe`
- Tested runtime SHA256: `013eb548ac83dcbf052aedb6247102d0f34c85d7228d081c3994af7c3959d0f0`
- Source: `Q2RTX-src/`; release export: `github/Q2RTX-1.8.1-GPT-source/`.
- Package: `releases/Q2RTX-DLSS-Public-Beta-0.95-NoPAK.zip`.
- Release notes: `CHANGELOG_0.95_RU.md`, `CHANGELOG_0.95_EN.md`.
- Permanent investigation: `history/MFG_2026-09-11/README.md` and its evidence files.
- Public source: https://github.com/Quake-Journey/Q2RTX_DLSS

This supersedes the current-status claims in the historical 0.9 / Streamline 2.11.1 notes. Those notes are retained as history. Native Dynamic MFG is not supported by this Vulkan path; it uses Auto fallback. HDR+MFG and DeepDVC's displayed service notice remain documented limitations.

## Release verification

- NoPAK ZIP: 1,381,761,170 bytes; 978 files.
- ZIP SHA256: `37993d21c1d80ea93964d00f9af9fe12434e219cb4852c8b82fdce3042222d60`.
- Every archived entry was decompressed and verified against the staged SHA256 manifest. Original game PAKs, savegames, crash logs, debug symbols, model-editor sources and the redundant .pkz2 backup are excluded.
- Packaged EXE matches the previously tested runtime exactly. Exported changed source/SDK files were compared to the development tree; no new gameplay changes were made during packaging.
- All 19 packaged top-level NVIDIA DLL signatures are valid. Current production SDK binaries checked against the downloaded SDK match byte-for-byte.
- Package launch loaded base1 and opened/closed the DLSS menu with RR enabled, preset F, and the MFG 3 setting. It exited normally, silently, without a new crash or changes to user/package configuration.
- **Foreground validation was unavailable in this release smoke test:** Windows returned no foreground window and activation attempts failed. This run does not validate active-window MFG performance or smoothness. Earlier foreground tests of the identical EXE are retained in the MFG history.
- Latest existing user crash report remains dated 2026-09-11 17:57:02 +03:00.
- Local verification evidence: `O:\Claude2\_agent_temp\q2rtx_release095_20260911`.

## Package size compared with 0.9

The ZIP decreased from 2,479,166,898 to 1,381,761,170 bytes. Exactly seven files were omitted: the unused q2rtx_media.pkz2 backup (898,045,492 compressed bytes) and six .spp texture-authoring/autosave files (179,185,386 compressed bytes). Updated DLLs account for most of the remaining roughly 20 MB reduction. Among retained baseq2 files, only q2config.cfg and q2rtx.menu changed contents; runtime game assets and both media/noise packs match the 0.9 ZIP by size and CRC. Original local files were not deleted.
