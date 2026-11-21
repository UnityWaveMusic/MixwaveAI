# One-command local build for Windows:
#   powershell -ExecutionPolicy Bypass -File scripts\build.ps1
# Steps: check tools > fetch JUCE > configure > build Release x64 > IPC tests >
#        optional pluginval > package dist\MIXWAVE-AI-Phase1-Windows-x64.zip
param(
    [string]$JuceTag = "8.0.11",
    [string]$Pluginval = ""        # optional: full path to pluginval.exe
)
$ErrorActionPreference = "Stop"
$root = Split-Path -Parent $PSScriptRoot
Set-Location $root

function Need($cmd, $hint) {
    if (-not (Get-Command $cmd -ErrorAction SilentlyContinue)) { throw "'$cmd' not found. $hint" }
}
Need git   "Install Git for Windows."
Need cmake "Install CMake 3.22+ and tick 'add to PATH'."

if (-not (Test-Path "third_party\JUCE\CMakeLists.txt")) {
    git clone --depth 1 --branch $JuceTag https://github.com/juce-framework/JUCE.git third_party/JUCE
    if ($LASTEXITCODE -ne 0) { throw "Could not fetch JUCE tag $JuceTag" }
}

$generator = & "$PSScriptRoot\select-generator.ps1"
Write-Host "CMake generator: $generator"

cmake -S . -B build -G $generator -A x64
if ($LASTEXITCODE -ne 0) { throw "Configure failed" }

cmake --build build --config Release --parallel
if ($LASTEXITCODE -ne 0) { throw "Build failed" }

ctest --test-dir build -C Release --output-on-failure
if ($LASTEXITCODE -ne 0) { throw "IPC tests failed" }

$track = "build\plugins\track\MixwaveTrack_artefacts\Release\VST3\MIXWAVE AI Track.vst3"
$hub   = "build\plugins\hub\MixwaveHub_artefacts\Release\VST3\MIXWAVE AI Hub.vst3"

if ($Pluginval -and (Test-Path $Pluginval)) {
    foreach ($p in @($track, $hub)) {
        & $Pluginval --strictness-level 5 (Resolve-Path $p).Path
        if ($LASTEXITCODE -ne 0) { Write-Warning "pluginval reported problems for $p" }
    }
} else {
    Write-Host "pluginval skipped (pass -Pluginval C:\path\pluginval.exe to run it)."
}

$out = "dist\MIXWAVE-AI-Phase1-Windows-x64"
Remove-Item $out -Recurse -Force -ErrorAction SilentlyContinue
New-Item -ItemType Directory -Force $out | Out-Null
Copy-Item $track $out -Recurse
Copy-Item $hub   $out -Recurse
Copy-Item packaging\README_INSTALL.txt "$out\README.txt"
Copy-Item packaging\install_binaries.ps1 "$out\install.ps1"
Compress-Archive -Path "$out\*" -DestinationPath "dist\MIXWAVE-AI-Phase1-Windows-x64.zip" -Force
Write-Host "`nDone: dist\MIXWAVE-AI-Phase1-Windows-x64.zip"
