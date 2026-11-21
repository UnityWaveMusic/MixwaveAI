# MIXWAVE AI

Two-plugin VST3 system for FL Studio (Windows x64):
**MIXWAVE AI Track** (one per mixer insert) and **MIXWAVE AI Hub** (on the Master).
CMake targets: `MixwaveTrack`, `MixwaveHub` (and the test executable `MixwaveCoreTests`).

## Build on GitHub (no local compiler needed)
1. Create a private repository and push this folder (see below).
2. Actions tab > **Build MIXWAVE AI (Windows x64)** > Run workflow.
3. When it finishes, open the run and download **MIXWAVE-AI-Phase1-Windows-x64-vs2022** from *Artifacts*.
   The run summary shows build / IPC tests / pluginval results.

    git init -b main
    git add .
    git commit -m "MIXWAVE AI phase 1"
    git remote add origin https://github.com/<your-user>/<your-repo>.git
    git push -u origin main

(Use git or GitHub Desktop rather than drag-and-drop: the hidden `.github` folder must be included.)

## Build locally on Windows
Needs Visual Studio 2022 or 2026 (Desktop development with C++), Git, CMake 3.22+ (4.2+ for VS 2026).

    powershell -ExecutionPolicy Bypass -File scripts\build.ps1

## Install
See `packaging/README_INSTALL.txt`.
