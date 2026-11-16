#!/usr/bin/env bash
# Configure and build FeX on macOS.
#
# Prerequisites:
#   xcode-select --install
#   brew install cmake ninja pkg-config sdl2 freetype sqlite3 ffmpeg molten-vk \
#     vulkan-headers glslang spirv-tools rust
#
# Environment variables:
#   BUILD_DIR   build directory (default: <repo>/build)
#   BUILD_TYPE  CMake build type (default: Release)
#   JOBS        parallel jobs (default: number of CPUs)
#   ARCHS       macOS architectures (default: host architecture, e.g. "arm64")

set -euo pipefail

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd -- "${SCRIPT_DIR}/../.." && pwd)"

BUILD_DIR="${BUILD_DIR:-${ROOT_DIR}/build}"
BUILD_TYPE="${BUILD_TYPE:-Release}"
JOBS="${JOBS:-$(sysctl -n hw.ncpu 2>/dev/null || echo 4)}"
ARCHS="${ARCHS:-$(uname -m)}"

cmake -S "${ROOT_DIR}" -B "${BUILD_DIR}" -G Ninja \
	-DCMAKE_BUILD_TYPE="${BUILD_TYPE}" \
	-DCMAKE_OSX_ARCHITECTURES="${ARCHS}" \
	-DCLIENT="${CLIENT:-ON}" \
	-DSERVER="${SERVER:-ON}" \
	-DTOOLS="${TOOLS:-ON}"

cmake --build "${BUILD_DIR}" --parallel "${JOBS}"

echo "Build finished. The app bundle is in ${BUILD_DIR}"
