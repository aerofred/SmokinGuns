#!/bin/bash
# Build Smokin' Guns for iOS (device) and package into SmokinGuns.app
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
ROOT_DIR="$(cd "${SCRIPT_DIR}/../.." && pwd)"
cd "${ROOT_DIR}"

SDK_PATH="${IOS_SDK:-$(xcrun --sdk iphoneos --show-sdk-path)}"
SDL2_FW="${SDL2_IOS_FRAMEWORK:-${SCRIPT_DIR}/SDL2.framework}"

if [ ! -f "${SDL2_FW}/SDL2" ] && [ ! -f "${SDL2_FW}/Headers/SDL.h" ]; then
	echo "SDL2 iOS bundle not found at ${SDL2_FW} (run setup_sdl2.sh)" >&2
	echo "Run: ${SCRIPT_DIR}/setup_sdl2.sh" >&2
	exit 1
fi

export SDL2_IOS_FRAMEWORK="${SDL2_FW}"

# Xcode exports SDKROOT=iphoneos, ARCHS, B=DerivedData/... — breaks host QVM tools.
unset SDKROOT SDKROOT_iphoneos IPHONEOS_DEPLOYMENT_TARGET
unset EFFECTIVE_PLATFORM_NAME PLATFORM_NAME ARCHS CONFIGURATION BUILD_DIR

HOST_ARCH="$(uname -m)"
case "${HOST_ARCH}" in
arm64) HOST_ARCH=arm64 ;;
x86_64) HOST_ARCH=x86_64 ;;
*) HOST_ARCH=x86 ;;
esac
HOST_BUILD="${ROOT_DIR}/build/release-darwin-${HOST_ARCH}"
IOS_BUILD="${ROOT_DIR}/build/release-ios-arm64"
QVM_DIR="${HOST_BUILD}/smokinguns/vm"

echo "=== Building QVMs (host: darwin-${HOST_ARCH}) ==="
if [ -f "${QVM_DIR}/qagame.qvm" ] && [ -f "${QVM_DIR}/cgame.qvm" ] && [ -f "${QVM_DIR}/ui.qvm" ]; then
	echo "QVMs already present in ${QVM_DIR}, skipping host build."
else
	make -f Makefile -f Makefile.smokinguns \
		BUILD_CLIENT=0 BUILD_SERVER=0 BUILD_GAME_SO=0 BUILD_GAME_QVM=1 \
		PLATFORM=darwin ARCH="${HOST_ARCH}" \
		B="${HOST_BUILD}" \
		-j"$(sysctl -n hw.ncpu 2>/dev/null || echo 4)" \
		targets
fi

echo "=== Building iOS client ==="
# MMD .d files can list removed headers after refactors; drop affected objects.
if [ -d "${IOS_BUILD}" ]; then
	while IFS= read -r -d '' d; do
		if grep -qE 'ios_public\.h|ios_overlay' "$d" 2>/dev/null; then
			rm -f "${d%.d}.o" "$d"
		fi
	done < <(find "${IOS_BUILD}" -name '*.d' -print0 2>/dev/null)
fi
make -f Makefile -f Makefile.ios \
	PLATFORM=ios ARCH=arm64 \
	B="${IOS_BUILD}" \
	BUILD_CLIENT=1 BUILD_SERVER=0 \
	BUILD_GAME_SO=0 BUILD_GAME_QVM=0 \
	USE_RENDERER_DLOPEN=0 \
	BUILD_RENDERER_OPENGL2=0 \
	FRAMEBUFFER_AND_GLSL_SUPPORT=0 \
	SDL2_IOS_FRAMEWORK="${SDL2_FW}" \
	-j"$(sysctl -n hw.ncpu 2>/dev/null || echo 4)" \
	ios-client

BIN="${IOS_BUILD}/SmokinGuns.arm64"
APP="${ROOT_DIR}/build/ios/SmokinGuns.app"

rm -rf "${APP}"
mkdir -p "${APP}"

cp "${SCRIPT_DIR}/Info.plist" "${APP}/Info.plist"

chmod +x "${SCRIPT_DIR}/make_icons.sh"
"${SCRIPT_DIR}/make_icons.sh" "${APP}"

chmod +x "${SCRIPT_DIR}/copy_game_data.sh"
"${SCRIPT_DIR}/copy_game_data.sh" "${APP}"

# macOS default FS is case-insensitive: "SmokinGuns" collides with "smokinguns/".
SG_CLIENT="SGClient"
cp "${BIN}" "${APP}/${SG_CLIENT}"
chmod +x "${APP}/${SG_CLIENT}"

echo ""
echo "Built: ${APP}"
echo "Install with Xcode: open misc/ios/SmokinGuns.xcodeproj and deploy, or:"
echo "  ios-deploy --bundle ${APP}"
