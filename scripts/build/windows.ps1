# Configure and build FeX on Windows.
#
# Prerequisites:
#   - Visual Studio 2022 or newer with the "Desktop development with C++" workload
#     (or LLVM/clang + Ninja as an alternative toolchain)
#   - CMake 3.19+ and Ninja on PATH
#   - Rust (rustup) on PATH, the engine contains Rust code
#
# Usage:
#   powershell -ExecutionPolicy Bypass -File scripts/build/windows.ps1
#   powershell -File scripts/build/windows.ps1 -Configuration Debug -BuildDir build-debug

param(
	[ValidateSet('Debug', 'Release', 'RelWithDebInfo', 'MinSizeRel')]
	[string]$Configuration = 'RelWithDebInfo',
	[string]$BuildDir = 'build',
	[switch]$ServerOnly
)

$ErrorActionPreference = 'Stop'

$root = Resolve-Path (Join-Path $PSScriptRoot '../..')
Push-Location $root
try {
	if (-not (Get-Command cmake -ErrorAction SilentlyContinue)) {
		throw 'cmake was not found on PATH. Install CMake and open a new terminal.'
	}

	# Prefer the Visual Studio generator when MSVC is available, because it is the
	# toolchain the project is tested with on Windows. Fall back to Ninja otherwise.
	$vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
	$hasMsvc = $false
	if (Test-Path $vswhere) {
		$install = & $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
		$hasMsvc = -not [string]::IsNullOrWhiteSpace($install)
	}

	$client = if ($ServerOnly) { 'OFF' } else { 'ON' }

	if ($hasMsvc) {
		Write-Host "Configuring with the Visual Studio toolchain (x64)..."
		cmake -S . -B $BuildDir -A x64 -DCLIENT=$client -DSERVER=ON -DTOOLS=ON -DEXCEPTION_HANDLING=ON
	} else {
		Write-Host "No MSVC installation found, configuring with Ninja..."
		cmake -S . -B $BuildDir -G Ninja -DCMAKE_BUILD_TYPE=$Configuration -DCLIENT=$client -DSERVER=ON -DTOOLS=ON
	}
	if ($LASTEXITCODE -ne 0) { throw "CMake configuration failed with exit code $LASTEXITCODE" }

	cmake --build $BuildDir --config $Configuration --parallel
	if ($LASTEXITCODE -ne 0) { throw "Build failed with exit code $LASTEXITCODE" }

	Write-Host "Build finished. Binaries are in $BuildDir/release or $BuildDir"
} finally {
	Pop-Location
}
