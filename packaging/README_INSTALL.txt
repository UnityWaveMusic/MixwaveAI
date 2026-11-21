MIXWAVE AI - Phase 1 (development build)  -  Windows x64, VST3
=================================================================

Contents
  MIXWAVE AI Track.vst3   one instance per mixer insert
  MIXWAVE AI Hub.vst3     one instance on the Master insert
  install.ps1          installer script

What this build does (Phase 1 only)
  Track: gain knob, input/output meters, link status.
  Hub:   pass-through, live table of all linked Track instances.
  No mixing intelligence yet: that arrives in later phases.

Install
  1. Close FL Studio.
  2. Right-click the Start button > "Terminal (Admin)" / "PowerShell (Admin)".
  3. cd to this folder, then run:
       powershell -ExecutionPolicy Bypass -File .\install.ps1
     (Or copy both .vst3 folders by hand into
      C:\Program Files\Common Files\VST3 )
  4. Open FL Studio > Plugin database > "Find installed plugins".
  5. Load MIXWAVE AI Track on your mixer inserts and MIXWAVE AI Hub on the Master.
     The Hub window should list every Track.

Notes
  - Do not enable "Make bridged" for these plugins: bridged plugins run in a
    separate process and cannot see each other.
  - If Windows SmartScreen or antivirus blocks the files, the build is
    unsigned (code signing comes before any public release).
  - This is a development build. Report what you see in FL Studio (does the
    Hub list your tracks? are track names shown?) so Phase 2 can build on it.

Uninstall
  Delete "MIXWAVE AI Track.vst3" and "MIXWAVE AI Hub.vst3" from
  C:\Program Files\Common Files\VST3
