#!/bin/sh
set -eu

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
SDL_VERSION="${SDL_VERSION:-2.30.5}"
SDL_DIR="$ROOT/misc/ios/sdl2-build/SDL2-$SDL_VERSION"
SDL_TARBALL="$ROOT/misc/ios/sdl2-build/SDL2-$SDL_VERSION.tar.gz"
SDL_LIB="$ROOT/misc/ios/sdl2-build/libSDL2-ios.a"
SDL_BUILD="$ROOT/misc/ios/sdl2-build/xcode"
APP_DIR="$ROOT/build/ios/SmokinGuns.app"
BUILD_DIR="$ROOT/build/ios-port"
BIN="$BUILD_DIR/release-ios-arm64/smokinguns.arm64"

mkdir -p "$ROOT/misc/ios/sdl2-build"

if [ ! -f "$SDL_DIR/include/SDL.h" ]; then
	if [ ! -f "$SDL_TARBALL" ]; then
		curl -L "https://github.com/libsdl-org/SDL/releases/download/release-$SDL_VERSION/SDL2-$SDL_VERSION.tar.gz" -o "$SDL_TARBALL"
	fi
	rm -rf "$SDL_DIR"
	tar -xzf "$SDL_TARBALL" -C "$ROOT/misc/ios/sdl2-build"
fi

if [ ! -f "$SDL_LIB" ]; then
	xcodebuild \
		-project "$SDL_DIR/Xcode/SDL/SDL.xcodeproj" \
		-scheme "Static Library-iOS" \
		-configuration Release \
		-sdk iphoneos \
		BUILD_DIR="$SDL_BUILD" \
		ONLY_ACTIVE_ARCH=NO \
		ARCHS=arm64 \
		build
	LIB_FOUND="$(find "$SDL_BUILD" -name 'libSDL2.a' -print | head -n 1)"
	if [ -z "$LIB_FOUND" ]; then
		echo "Unable to find libSDL2.a after SDL build" >&2
		exit 1
	fi
	cp "$LIB_FOUND" "$SDL_LIB"
fi

make -C "$ROOT" \
	PLATFORM=ios \
	ARCH=arm64 \
	BUILD_SERVER=0 \
	BUILD_CLIENT=1 \
	BUILD_GAME_SO=0 \
	BUILD_GAME_QVM=0 \
	BUILD_RENDERER_OPENGL2=0 \
	USE_RENDERER_DLOPEN=0 \
	USE_OPENAL=0 \
	USE_CURL=0 \
	USE_MUMBLE=0 \
	USE_VOIP=0 \
	USE_FREETYPE=0 \
	FRAMEBUFFER_AND_GLSL_SUPPORT=0 \
	BUILD_DIR="$BUILD_DIR" \
	IOS_SDL_ROOT="$SDL_DIR" \
	IOS_SDL_LIB="$SDL_LIB"

rm -rf "$APP_DIR"
mkdir -p "$APP_DIR"
cp "$BIN" "$APP_DIR/SGClient"
cp -R "$ROOT/smokinguns" "$APP_DIR/smokinguns"
cp -R "$ROOT/baseq3" "$APP_DIR/baseq3"
cp -R "$ROOT/ui" "$APP_DIR/ui"

if [ -f "$ROOT/misc/smokinguns.png" ]; then
	python3 "$ROOT/misc/ios/composite_app_icon.py" \
		"$ROOT/misc/smokinguns.png" \
		"$APP_DIR/AppIcon.png" \
		1024
fi
if [ -f "$ROOT/misc/ios/LaunchScreen.storyboard" ]; then
	cp "$ROOT/misc/ios/LaunchScreen.storyboard" "$APP_DIR/LaunchScreen.storyboard"
fi

cat > "$APP_DIR/Info.plist" <<'PLIST'
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN"
 "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
	<key>CFBundleDevelopmentRegion</key><string>en</string>
	<key>CFBundleDisplayName</key><string>Smokin' Guns</string>
	<key>CFBundleExecutable</key><string>SGClient</string>
	<key>CFBundleIdentifier</key><string>org.smokinguns.ios</string>
	<key>CFBundleInfoDictionaryVersion</key><string>6.0</string>
	<key>CFBundleName</key><string>SmokinGuns</string>
	<key>CFBundlePackageType</key><string>APPL</string>
	<key>CFBundleShortVersionString</key><string>1.2</string>
	<key>CFBundleVersion</key><string>1</string>
	<key>LSRequiresIPhoneOS</key><true/>
	<key>UILaunchStoryboardName</key><string>LaunchScreen</string>
	<key>UIRequiresFullScreen</key><true/>
	<key>NSBonjourServices</key><array><string>_quake3._udp</string></array>
	<key>NSLocalNetworkUsageDescription</key>
	<string>Smokin' Guns uses the local network for LAN multiplayer games.</string>
	<key>UIApplicationSupportsIndirectInputEvents</key><true/>
	<key>UIRequiredDeviceCapabilities</key><array><string>arm64</string></array>
	<key>UIStatusBarHidden</key><true/>
	<key>UISupportedInterfaceOrientations</key>
	<array>
		<string>UIInterfaceOrientationLandscapeLeft</string>
		<string>UIInterfaceOrientationLandscapeRight</string>
	</array>
	<key>UISupportedInterfaceOrientations~ipad</key>
	<array>
		<string>UIInterfaceOrientationLandscapeLeft</string>
		<string>UIInterfaceOrientationLandscapeRight</string>
	</array>
	<key>UIViewControllerBasedStatusBarAppearance</key><false/>
</dict>
</plist>
PLIST

echo "Built $APP_DIR"
