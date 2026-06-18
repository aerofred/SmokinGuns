#!/bin/bash
# Build Smokin' Guns for iOS (device) and package into build/ios/SmokinGuns.app
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
ROOT_DIR="$(cd "${SCRIPT_DIR}/../.." && pwd)"
cd "${ROOT_DIR}"

SDL_VERSION="${SDL_VERSION:-2.30.5}"
SDL_DIR="${ROOT_DIR}/misc/ios/sdl2-build/SDL2-${SDL_VERSION}"
SDL_LIB="${ROOT_DIR}/misc/ios/sdl2-build/libSDL2-ios.a"
IOS_BUILD="${ROOT_DIR}/build/ios-port"
APP="${ROOT_DIR}/build/ios/SmokinGuns.app"
BIN="${IOS_BUILD}/release-ios-arm64/smokinguns.arm64"

# Xcode exports SDKROOT/ARCHS/BUILD_DIR — breaks host QVM tools.
unset SDKROOT SDKROOT_iphoneos IPHONEOS_DEPLOYMENT_TARGET
unset EFFECTIVE_PLATFORM_NAME PLATFORM_NAME ARCHS CONFIGURATION BUILD_DIR

if [ ! -f "${SDL_LIB}" ]; then
	"${SCRIPT_DIR}/setup_sdl2.sh"
fi

HOST_ARCH="$(uname -m)"
case "${HOST_ARCH}" in
arm64) HOST_ARCH=arm64 ;;
x86_64) HOST_ARCH=x86_64 ;;
*) HOST_ARCH=x86 ;;
esac
HOST_BUILD="${ROOT_DIR}/build/release-darwin-${HOST_ARCH}"
QVM_DIR="${HOST_BUILD}/smokinguns/vm"

echo "=== Building QVMs (host: darwin-${HOST_ARCH}) ==="
make -f Makefile -f Makefile.local \
	BUILD_CLIENT=0 BUILD_SERVER=0 BUILD_GAME_SO=0 BUILD_GAME_QVM=1 \
	PLATFORM=darwin ARCH="${HOST_ARCH}" \
	B="${HOST_BUILD}" \
	-j"$(sysctl -n hw.ncpu 2>/dev/null || echo 4)" \
	build/release-darwin-${HOST_ARCH}/smokinguns/vm/ui.qvm

echo "=== Building iOS client ==="
rm -rf "${IOS_BUILD}/release-ios-arm64"
make -f Makefile -f Makefile.local \
	PLATFORM=ios \
	ARCH=arm64 \
	BUILD_DIR="${IOS_BUILD}" \
	OPTIMIZE="${IOS_OPTIMIZE:--DNDEBUG -O2 -gline-tables-only}" \
	BUILD_SDK_DIFF=0 \
	BUILD_CLIENT=1 BUILD_SERVER=0 \
	BUILD_GAME_SO=0 BUILD_GAME_QVM=0 \
	BUILD_RENDERER_OPENGL2=0 \
	USE_RENDERER_DLOPEN=0 \
	USE_OPENAL=0 \
	USE_CURL=0 \
	USE_MUMBLE=0 \
	USE_VOIP=0 \
	USE_FREETYPE=0 \
	FRAMEBUFFER_AND_GLSL_SUPPORT=0 \
	IOS_SDL_ROOT="${SDL_DIR}" \
	IOS_SDL_LIB="${SDL_LIB}" \
	-j"$(sysctl -n hw.ncpu 2>/dev/null || echo 4)" \
	release

if [ ! -f "${BIN}" ]; then
	echo "error: iOS binary not found at ${BIN}" >&2
	exit 1
fi

rm -rf "${APP}"
mkdir -p "${APP}"

cp "${SCRIPT_DIR}/Info.plist" "${APP}/Info.plist"
cp "${SCRIPT_DIR}/LaunchScreen.storyboard" "${APP}/LaunchScreen.storyboard"
"${SCRIPT_DIR}/copy_game_data.sh" "${APP}"

cp "${BIN}" "${APP}/SGClient"
chmod +x "${APP}/SGClient"

chmod +x "${SCRIPT_DIR}/generate_app_icon.sh"
"${SCRIPT_DIR}/generate_app_icon.sh" "${APP}"

echo ""
echo "Built: ${APP}"
