#Requires -RunAsAdministrator
# Installs MIXWAVE Track and MIXWAVE Hub into the system VST3 folder.
# Close FL Studio first (it locks loaded plugin files).
$dest = Join-Path $env:CommonProgramFiles "VST3"
foreach ($name in @("MIXWAVE AI Track.vst3", "MIXWAVE AI Hub.vst3")) {
    $src = Join-Path $PSScriptRoot $name
    if (-not (Test-Path $src)) { Write-Error "Missing: $src"; continue }
    $target = Join-Path $dest $name
    if (Test-Path $target) { Remove-Item $target -Recurse -Force }
    Copy-Item $src $dest -Recurse -Force
    Write-Host "Installed: $target"
}
