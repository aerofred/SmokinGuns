#!/bin/bash
# Generate iOS app icon assets from misc/smokinguns.icns
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
ROOT_DIR="$(cd "${SCRIPT_DIR}/../.." && pwd)"
ICNS="${ROOT_DIR}/misc/smokinguns.icns"
APP="$1"

if [ ! -f "${ICNS}" ]; then
	echo "error: ${ICNS} not found" >&2
	exit 1
fi

WORK="$(mktemp -d)"
trap 'rm -rf "${WORK}"' EXIT

iconutil --convert iconset -o "${WORK}/master.iconset" "${ICNS}"

MASTER=""
for candidate in \
	"${WORK}/master.iconset/icon_512x512@2x.png" \
	"${WORK}/master.iconset/icon_512x512.png" \
	"${WORK}/master.iconset/icon_256x256@2x.png" \
	"${WORK}/master.iconset/icon_256x256.png" \
	"${WORK}/master.iconset/icon_128x128@2x.png" \
	"${WORK}/master.iconset/icon_128x128.png"; do
	if [ -f "${candidate}" ]; then
		MASTER="${candidate}"
		break
	fi
done

if [ -z "${MASTER}" ]; then
	echo "error: no usable PNG found in ${ICNS}" >&2
	exit 1
fi

ICONSET="${WORK}/AppIcon.appiconset"
mkdir -p "${ICONSET}"

for spec in \
	"40:Icon-40.png" \
	"60:Icon-60.png" \
	"58:Icon-58.png" \
	"87:Icon-87.png" \
	"80:Icon-80.png" \
	"120:Icon-120.png" \
	"180:Icon-180.png" \
	"20:Icon-20.png" \
	"29:Icon-29.png" \
	"76:Icon-76.png" \
	"152:Icon-152.png" \
	"167:Icon-167.png" \
	"1024:Icon-1024.png"; do
	size="${spec%%:*}"
	out="${spec##*:}"
	sips -z "${size}" "${size}" "${MASTER}" --out "${ICONSET}/${out}" >/dev/null
done

cat > "${ICONSET}/Contents.json" <<'EOF'
{
  "images" : [
    { "filename" : "Icon-20.png", "idiom" : "ipad", "scale" : "1x", "size" : "20x20" },
    { "filename" : "Icon-40.png", "idiom" : "ipad", "scale" : "2x", "size" : "20x20" },
    { "filename" : "Icon-29.png", "idiom" : "ipad", "scale" : "1x", "size" : "29x29" },
    { "filename" : "Icon-58.png", "idiom" : "ipad", "scale" : "2x", "size" : "29x29" },
    { "filename" : "Icon-40.png", "idiom" : "ipad", "scale" : "1x", "size" : "40x40" },
    { "filename" : "Icon-80.png", "idiom" : "ipad", "scale" : "2x", "size" : "40x40" },
    { "filename" : "Icon-76.png", "idiom" : "ipad", "scale" : "1x", "size" : "76x76" },
    { "filename" : "Icon-152.png", "idiom" : "ipad", "scale" : "2x", "size" : "76x76" },
    { "filename" : "Icon-167.png", "idiom" : "ipad", "scale" : "2x", "size" : "83.5x83.5" },
    { "filename" : "Icon-40.png", "idiom" : "iphone", "scale" : "2x", "size" : "20x20" },
    { "filename" : "Icon-60.png", "idiom" : "iphone", "scale" : "3x", "size" : "20x20" },
    { "filename" : "Icon-58.png", "idiom" : "iphone", "scale" : "2x", "size" : "29x29" },
    { "filename" : "Icon-87.png", "idiom" : "iphone", "scale" : "3x", "size" : "29x29" },
    { "filename" : "Icon-80.png", "idiom" : "iphone", "scale" : "2x", "size" : "40x40" },
    { "filename" : "Icon-120.png", "idiom" : "iphone", "scale" : "2x", "size" : "60x60" },
    { "filename" : "Icon-180.png", "idiom" : "iphone", "scale" : "3x", "size" : "60x60" },
    { "filename" : "Icon-1024.png", "idiom" : "ios-marketing", "scale" : "1x", "size" : "1024x1024" }
  ],
  "info" : { "author" : "xcode", "version" : 1 }
}
EOF

ASSETS="${WORK}/Assets.xcassets"
mkdir -p "${ASSETS}"
mv "${ICONSET}" "${ASSETS}/AppIcon.appiconset"

if xcrun actool "${ASSETS}" \
	--compile "${APP}" \
	--platform iphoneos \
	--minimum-deployment-target 15.0 \
	--app-icon AppIcon \
	--output-partial-info-plist "${WORK}/partial.plist" >/dev/null; then
	echo "Assets.car generated"
else
	echo "warning: actool failed; copying icon PNG fallback without Assets.car" >&2
	cp "${ASSETS}/AppIcon.appiconset/Icon-120.png" "${APP}/AppIcon60x60@2x.png"
	cp "${ASSETS}/AppIcon.appiconset/Icon-180.png" "${APP}/AppIcon60x60@3x.png"
	cp "${ASSETS}/AppIcon.appiconset/Icon-152.png" "${APP}/AppIcon76x76@2x.png"
	cp "${ASSETS}/AppIcon.appiconset/Icon-1024.png" "${APP}/AppIcon.png"
fi

cp "${ASSETS}/AppIcon.appiconset/Icon-1024.png" "${APP}/SplashIcon.png"

echo "App icon generated from smokinguns.icns"
