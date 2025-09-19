# tools/setup.ps1  (PowerShell 5+ compatible)

# 1) Resolve VCPKG_ROOT
if (-not $env:VCPKG_ROOT -or -not (Test-Path "$env:VCPKG_ROOT")) {
    $env:VCPKG_ROOT = "$env:USERPROFILE\vcpkg"
}

# 2) Ensure vcpkg exists
if (-not (Test-Path "$env:VCPKG_ROOT\vcpkg.exe")) {
    Write-Host "vcpkg not found, cloning + bootstrapping..." -ForegroundColor Cyan
    git clone https://github.com/microsoft/vcpkg "$env:VCPKG_ROOT"
    & "$env:VCPKG_ROOT\bootstrap-vcpkg.bat"
}

# 3) Configure the project (use preset if present, otherwise explicit flags)
$presetsPath = Join-Path (Get-Location) "CMakePresets.json"
if (Test-Path $presetsPath) {
    Write-Host "Configuring with preset 'vs2022-x64'..." -ForegroundColor Cyan
    cmake --preset vs2022-x64
} else {
    Write-Host "Configuring with explicit generator + vcpkg toolchain..." -ForegroundColor Cyan
    cmake -S . -B build -G "Visual Studio 17 2022" -A x64 `
      -DCMAKE_TOOLCHAIN_FILE="$env:VCPKG_ROOT\scripts\buildsystems\vcpkg.cmake"
}

Write-Host "Ready. Build with:" -ForegroundColor Green
Write-Host "  cmake --build --preset debug --target run" -ForegroundColor Yellow
