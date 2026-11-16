#!/usr/bin/env bash
# Configure and build FeX on Linux.
#
# Prerequisites (Debian/Ubuntu):
#   sudo apt install build-essential cmake ninja-build pkg-config \
#     libsdl2-dev libfreetype6-dev libsqlite3-dev libpng-dev libglew-dev \
#     libavcodec-dev libavformat-dev libavutil-dev libswresample-dev libswscale-dev \
#     libvulkan-dev libogg-dev libopus-dev libopusfile-dev libwavpack-dev
# Rust (rustup) is required, the engine contains Rust code.
#
# Environment variables:
#   BUILD_DIR   build directory (default: <repo>/build)
#   BUILD_TYPE  CMake build type (default: Release)
#   JOBS        parallel jobs (default: number of CPUs)
#   CLIENT/SERVER/TOOLS  ON/OFF toggles (default: all ON)

set -euo pipefail

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd -- "${SCRIPT_DIR}/../.." && pwd)"

BUILD_DIR="${BUILD_DIR:-${ROOT_DIR}/build}"
BUILD_TYPE="${BUILD_TYPE:-Release}"
JOBS="${JOBS:-$(nproc 2>/dev/null || echo 4)}"

cmake -S "${ROOT_DIR}" -B "${BUILD_DIR}" -G Ninja \
	-DCMAKE_BUILD_TYPE="${BUILD_TYPE}" \
	-DCLIENT="${CLIENT:-ON}" \
	-DSERVER="${SERVER:-ON}" \
	-DTOOLS="${TOOLS:-ON}"

cmake --build "${BUILD_DIR}" --parallel "${JOBS}"

echo "Build finished. Binaries are in ${BUILD_DIR}"
