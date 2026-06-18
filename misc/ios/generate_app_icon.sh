#!/bin/bash
# Generate iOS app icon assets from misc/smokinguns.png
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
ROOT_DIR="$(cd "${SCRIPT_DIR}/../.." && pwd)"
SOURCE="${ROOT_DIR}/misc/smokinguns.png"
APP="$1"

if [ ! -f "${SOURCE}" ]; then
	echo "error: ${SOURCE} not found" >&2
	exit 1
fi

WORK="$(mktemp -d)"
trap 'rm -rf "${WORK}"' EXIT

MASTER="${WORK}/master.png"
sips -z 1024 1024 "${SOURCE}" --out "${MASTER}" >/dev/null

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

PARTIAL_PLIST="${WORK}/partial.plist"
DEPLOY_TARGET="${IOS_DEPLOYMENT_TARGET:-12.0}"

if xcrun actool "${ASSETS}" \
	--compile "${APP}" \
	--platform iphoneos \
	--minimum-deployment-target "${DEPLOY_TARGET}" \
	--app-icon AppIcon \
	--output-partial-info-plist "${PARTIAL_PLIST}" >/dev/null; then
	echo "Assets.car generated"
else
	echo "warning: actool failed; copying icon PNG fallback without Assets.car" >&2
	cp "${ASSETS}/AppIcon.appiconset/Icon-120.png" "${APP}/AppIcon60x60@2x.png"
	cp "${ASSETS}/AppIcon.appiconset/Icon-180.png" "${APP}/AppIcon60x60@3x.png"
	cp "${ASSETS}/AppIcon.appiconset/Icon-152.png" "${APP}/AppIcon76x76@2x.png"
	cp "${ASSETS}/AppIcon.appiconset/Icon-152.png" "${APP}/AppIcon76x76@2x~ipad.png"
	cp "${ASSETS}/AppIcon.appiconset/Icon-1024.png" "${APP}/AppIcon.png"
fi

cp "${ASSETS}/AppIcon.appiconset/Icon-1024.png" "${APP}/SplashIcon.png"

INFO_PLIST="${APP}/Info.plist"
if [ -f "${PARTIAL_PLIST}" ] && [ -f "${INFO_PLIST}" ]; then
	/usr/libexec/PlistBuddy -c "Merge ${PARTIAL_PLIST}" "${INFO_PLIST}" 2>/dev/null || true
fi

echo "App icon generated from smokinguns.png"
