# Configure and build FeX on Windows with CMake + Ninja.
#
# Prerequisites:
#   - Visual Studio 2022 or newer with the "Desktop development with C++" workload
#     (supplies cl.exe, link.exe and the Windows SDK)
#   - CMake 3.19+ and Ninja on PATH (Ninja is also installed by the VS CMake tools)
#   - Rust (rustup) on PATH, the engine contains Rust code
#
# The script imports the MSVC environment from vcvars64.bat and drives a Ninja
# build, which is much faster than the Visual Studio generator and keeps the
# build directory toolchain-agnostic.
#
# Usage:
#   powershell -ExecutionPolicy Bypass -File scripts/build/windows.ps1
#   powershell -ExecutionPolicy Bypass -File scripts/build/windows.ps1 -Configuration Debug -BuildDir build-debug
#   powershell -ExecutionPolicy Bypass -File scripts/build/windows.ps1 -ServerOnly

param(
	[ValidateSet('Debug', 'Release', 'RelWithDebInfo', 'MinSizeRel')]
	[string]$Configuration = 'RelWithDebInfo',
	[string]$BuildDir = 'build',
	[switch]$ServerOnly
)

$ErrorActionPreference = 'Stop'

function Import-MsvcEnvironment {
	$vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
	if (-not (Test-Path $vswhere)) {
		return $false
	}
	$install = & $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
	if ([string]::IsNullOrWhiteSpace($install)) {
		return $false
	}
	$vcvars = Join-Path $install 'VC\Auxiliary\Build\vcvars64.bat'
	if (-not (Test-Path $vcvars)) {
		return $false
	}
	# vcvars64.bat only sets variables inside its own cmd session, so run it and
	# copy the resulting environment into this PowerShell process.
	$environment = & cmd.exe /c "`"$vcvars`" >nul 2>&1 && set"
	if ($LASTEXITCODE -ne 0) {
		return $false
	}
	foreach ($line in $environment) {
		if ($line -match '^([^=]+)=(.*)$') {
			Set-Item -Path "Env:$($matches[1])" -Value $matches[2]
		}
	}
	return $true
}

$root = Resolve-Path (Join-Path $PSScriptRoot '../..')
Push-Location $root
try {
	if (-not (Get-Command cmake -ErrorAction SilentlyContinue)) {
		throw 'cmake was not found on PATH. Install CMake and open a new terminal.'
	}
	if (-not (Get-Command ninja -ErrorAction SilentlyContinue)) {
		throw 'ninja was not found on PATH. Install it (winget install Ninja-build.Ninja) or add the copy shipped with the Visual Studio CMake tools to PATH.'
	}
	if (-not (Import-MsvcEnvironment)) {
		throw 'No Visual Studio installation with the C++ toolset was found. Install the "Desktop development with C++" workload.'
	}
	if (-not (Get-Command cl.exe -ErrorAction SilentlyContinue)) {
		throw 'cl.exe is still not available after importing the MSVC environment.'
	}

	$client = if ($ServerOnly) { 'OFF' } else { 'ON' }

	Write-Host "Configuring with CMake + Ninja ($Configuration)..."
	cmake -S . -B $BuildDir -G Ninja -DCMAKE_BUILD_TYPE=$Configuration -DCLIENT=$client -DSERVER=ON -DTOOLS=ON -DEXCEPTION_HANDLING=ON
	if ($LASTEXITCODE -ne 0) { throw "CMake configuration failed with exit code $LASTEXITCODE" }

	# Ninja is single-config, the configuration is baked in by CMAKE_BUILD_TYPE.
	cmake --build $BuildDir --parallel
	if ($LASTEXITCODE -ne 0) { throw "Build failed with exit code $LASTEXITCODE" }

	Write-Host "Build finished. Binaries are in $BuildDir"
} finally {
	Pop-Location
}
