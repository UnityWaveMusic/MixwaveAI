#Requires -RunAsAdministrator
# Copies the freshly built VST3 bundles into the system VST3 folder.
# Close FL Studio first: it locks loaded plugin files.
param([string]$Config = "Release")

$root = Split-Path -Parent $PSScriptRoot
$dest = Join-Path $env:CommonProgramFiles "VST3"

$bundles = @(
    "$root\build\plugins\track\MixwaveTrack_artefacts\$Config\VST3\MIXWAVE AI Track.vst3",
    "$root\build\plugins\hub\MixwaveHub_artefacts\$Config\VST3\MIXWAVE AI Hub.vst3"
)

foreach ($bundle in $bundles) {
    if (-not (Test-Path $bundle)) {
        Write-Error "Not found (did the build succeed?): $bundle"
        continue
    }
    $target = Join-Path $dest (Split-Path $bundle -Leaf)
    if (Test-Path $target) { Remove-Item $target -Recurse -Force }
    Copy-Item $bundle $dest -Recurse -Force
    Write-Host "Installed: $target"
}
