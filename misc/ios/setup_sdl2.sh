#!/bin/bash
# Build SDL2 static library + headers for iphoneos (arm64).
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
SDL_VERSION="${SDL_VERSION:-2.30.5}"
SDL_TAR="SDL2-${SDL_VERSION}"
BUILD_DIR="${SCRIPT_DIR}/sdl2-build"
SRC_DIR="${BUILD_DIR}/${SDL_TAR}"
CMAKE_BUILD="${BUILD_DIR}/cmake-ios"
INSTALL_FW="${SCRIPT_DIR}/SDL2.framework"
IOS_SDK="${IOS_SDK:-$(xcrun --sdk iphoneos --show-sdk-path)}"
MIN_IOS="${IOS_MIN_VERSION:-15.0}"

if [ -f "${INSTALL_FW}/SDL2" ] && [ "${FORCE_SDL2_REBUILD:-0}" != "1" ]; then
	echo "OK: ${INSTALL_FW}"
	exit 0
fi

mkdir -p "${BUILD_DIR}"
cd "${BUILD_DIR}"

if [ ! -d "${SDL_TAR}" ]; then
	echo "Downloading SDL2 ${SDL_VERSION}..."
	curl -fL --retry 3 -o "${SDL_TAR}.tar.gz" \
		"https://github.com/libsdl-org/SDL/releases/download/release-${SDL_VERSION}/${SDL_TAR}.tar.gz"
	tar xzf "${SDL_TAR}.tar.gz"
fi

rm -rf "${CMAKE_BUILD}"
cmake -S "${SRC_DIR}" -B "${CMAKE_BUILD}" -G Ninja \
	-DCMAKE_SYSTEM_NAME=iOS \
	-DCMAKE_OSX_SYSROOT="${IOS_SDK}" \
	-DCMAKE_OSX_ARCHITECTURES=arm64 \
	-DCMAKE_OSX_DEPLOYMENT_TARGET="${MIN_IOS}" \
	-DCMAKE_BUILD_TYPE=Release \
	-DSDL_SHARED=OFF \
	-DSDL_STATIC=ON \
	-DSDL_TEST=OFF \
	-DSDL_RENDER_METAL=OFF \
	-DSDL_METAL=OFF \
	-DSDL_VIDEO_METAL=OFF \
	-DSDL_INSTALL=OFF

cmake --build "${CMAKE_BUILD}" -j"$(sysctl -n hw.ncpu 2>/dev/null || echo 4)"

LIB_A="${CMAKE_BUILD}/libSDL2.a"
if [ ! -f "${LIB_A}" ]; then
	LIB_A=$(find "${CMAKE_BUILD}" -name "libSDL2.a" | head -1)
fi
if [ ! -f "${LIB_A}" ]; then
	echo "libSDL2.a not found" >&2
	exit 1
fi

rm -rf "${INSTALL_FW}"
mkdir -p "${INSTALL_FW}/Headers"
cp -R "${SRC_DIR}/include/"* "${INSTALL_FW}/Headers/"
# Flat include layout for #include <SDL.h> (not SDL2/SDL.h)
for h in "${INSTALL_FW}/Headers/"*.h; do
	: # already flat
done

DEDUP_DIR="${CMAKE_BUILD}/dedup-ar"
DEDUP_A="${CMAKE_BUILD}/libSDL2-dedup.a"
rm -rf "${DEDUP_DIR}"
mkdir -p "${DEDUP_DIR}"
( cd "${DEDUP_DIR}" && ar x "${LIB_A}" && ar crs "${DEDUP_A}" *.o )
LIB_A="${DEDUP_A}"

cp "${LIB_A}" "${INSTALL_FW}/SDL2"
echo "Installed ${INSTALL_FW} (static SDL2 + Headers)"
