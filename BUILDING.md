# Building FeX

FeX builds with CMake. The engine contains Rust code, so a Rust toolchain
(`rustup`) is required in addition to a C++17 compiler.

Clone with the library submodule, otherwise linking fails:

```sh
git clone --recursive https://github.com/faffa81/FeX-2026
# or, if already cloned:
git submodule update --init --recursive
```

| Platform | Status | Entry point |
| --- | --- | --- |
| Windows (MSVC) | supported | `scripts/build/windows.ps1` |
| Linux | supported | `scripts/build/linux.sh` |
| macOS | supported | `scripts/build/macos.sh` |
| Android | supported | `scripts/build/android.sh` |
| iOS | not supported yet, see below | — |

The binaries are written to the build directory (`FeX`, `FeX-Server`,
`FeX-Server-Launcher`).

## Windows

Requires Visual Studio 2022 or newer with the *Desktop development with C++*
workload, CMake, Ninja and Rust on `PATH`. The script imports the MSVC
environment from `vcvars64.bat` and drives a **CMake + Ninja** build, which
compiles noticeably faster than the Visual Studio generator and keeps the build
directory independent of the IDE. Because Ninja is single-config, the build type
is fixed at configure time through `CMAKE_BUILD_TYPE`.

```powershell
powershell -ExecutionPolicy Bypass -File scripts/build/windows.ps1
# debug build in a separate directory:
powershell -ExecutionPolicy Bypass -File scripts/build/windows.ps1 -Configuration Debug -BuildDir build-debug
```

Or by hand, from a *Developer PowerShell for VS* prompt:

```powershell
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo -DCLIENT=ON -DSERVER=OFF -DTOOLS=OFF -DDOWNLOAD_GTEST=OFF
cmake --build build --parallel
```

## Linux

```sh
sudo apt install build-essential cmake ninja-build pkg-config \
  libsdl2-dev libfreetype6-dev libsqlite3-dev libpng-dev libglew-dev \
  libavcodec-dev libavformat-dev libavutil-dev libswresample-dev libswscale-dev \
  libvulkan-dev libogg-dev libopus-dev libopusfile-dev libwavpack-dev
./scripts/build/linux.sh
```

`BUILD_TYPE`, `BUILD_DIR`, `JOBS`, `CLIENT`, `SERVER` and `TOOLS` can be
overridden via the environment, e.g. `BUILD_TYPE=Debug ./scripts/build/linux.sh`.

## macOS

```sh
xcode-select --install
brew install cmake ninja pkg-config sdl2 freetype sqlite3 ffmpeg molten-vk \
  vulkan-headers glslang spirv-tools rust
./scripts/build/macos.sh
```

Set `ARCHS="arm64;x86_64"` for a universal build.

## Android

The NDK/Gradle flow lives in `scripts/android/`; see `scripts/android/README.md`
for the toolchain requirements (NDK 26, Android SDK build-tools, `cargo-ndk`,
the four Android Rust targets and OpenJDK). Then:

```sh
./scripts/build/android.sh            # Release APK, org.fex.client
BUILD_TYPE=Debug ./scripts/build/android.sh
```

## iOS

iOS is **not supported by this tree yet**. DDNet gained iOS support upstream in
2026 and that work lives in commits that are not part of FeX's current base:

- `936f2b7c1` client: Add iOS support
- `51a2ea409` client: Finish adding iOS support
- `211a6b4e5` client: Fix iOS App Store upload validation
- `1fe2e62d1` ios: Support dark/tinted icons

Those commits add the iOS platform layer (`src/base/*`), the app bundle and
`scripts/ios/cmake_ios.sh`, and they depend on DDNet's newer source layout, so
they cannot be cherry-picked onto this base as-is. iOS support arrives with the
upstream merge; until that lands, an iOS build would have no touch controls and
no platform layer to run on.

## CMake options

| Option | Default | Meaning |
| --- | --- | --- |
| `CLIENT` | `ON` | build the game client |
| `SERVER` | `ON` | build the server |
| `TOOLS` | `ON` | build the map/tool binaries |
| `TESTS` | `ON` | build unit tests (needs GTest) |
| `EXCEPTION_HANDLING` | `OFF` | exceptions instead of crash dumps (MSVC) |
| `DOWNLOAD_GTEST` | `ON` | download GTest instead of using a system copy |
