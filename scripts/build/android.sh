#!/usr/bin/env bash
# Build FeX for Android (APK).
#
# This wraps the existing NDK/Gradle flow in scripts/android/. See
# scripts/android/README.md for the full toolchain requirements (NDK 26,
# Android SDK build-tools, cargo-ndk, Android rust targets, OpenJDK).
#
# Environment variables:
#   APK_NAME     output APK base name  (default: FeX)
#   ANDROID_PKG  application id        (default: org.fex.client)
#   BUILD_TYPE   Release or Debug      (default: Release)
#   ANDROID_HOME SDK location          (default: $HOME/Android/Sdk)

set -euo pipefail

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd -- "${SCRIPT_DIR}/../.." && pwd)"

APK_NAME="${APK_NAME:-FeX}"
ANDROID_PKG="${ANDROID_PKG:-org.fex.client}"
BUILD_TYPE="${BUILD_TYPE:-Release}"

exec "${ROOT_DIR}/scripts/android/files/build.sh" "${APK_NAME}" "${ANDROID_PKG}" "${BUILD_TYPE}"
