# Prints the CMake generator name that matches the newest installed Visual Studio
# that has the C++ toolset (VS 2022 -> "Visual Studio 17 2022", VS 2026 -> "Visual Studio 18 2026").
$vswhere = Join-Path ${env:ProgramFiles(x86)} "Microsoft Visual Studio\Installer\vswhere.exe"
if (-not (Test-Path $vswhere)) {
    throw "vswhere.exe not found. Install Visual Studio 2022/2026 (or Build Tools) with 'Desktop development with C++'."
}
$ver = & $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationVersion |
       Select-Object -First 1
if (-not $ver) { throw "No Visual Studio installation with the C++ x64 toolset was found." }

$major = [int]($ver.Split('.')[0])
switch ($major) {
    17      { "Visual Studio 17 2022" }
    18      { "Visual Studio 18 2026" }
    default { throw "Unsupported Visual Studio major version $major (installationVersion $ver)." }
}
