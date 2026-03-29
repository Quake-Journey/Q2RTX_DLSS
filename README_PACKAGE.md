# Q2RTX-1.8.1-GPT Source Package

This folder is prepared for publishing the current working source tree to GitHub.

Included:
- Current source tree exported from `Q2RTX-src`
- Local project notes copied into `project-notes/`
- Source-side Streamline integration files and required checked-in content

Excluded on purpose:
- Git metadata directories such as `.git/`
- Local compiled runtime files such as `q2rtx.exe`, `q2rtxded.exe`, and `baseq2/gamex86_64.dll`
- Local Streamline build/cache directories such as `extern/Streamline/_artifacts` and `extern/Streamline/_project`
- Local junction-based dependency links from `extern/Streamline/external/*` and `extern/Streamline/tools/premake5`

Notes:
- If Streamline dependency folders are missing after cloning on another machine, run `extern/Streamline/setup.bat` to restore the SDK-side external dependencies.
- Build notes from this workspace are stored in `project-notes/`.
- Game `.pak` files are not part of the source package and must not be redistributed.
