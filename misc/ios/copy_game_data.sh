#!/bin/bash
# Copy game data into the iOS app bundle.
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
ROOT_DIR="$(cd "${SCRIPT_DIR}/../.." && pwd)"
DEST="${1:-}"

if [ -z "${DEST}" ]; then
	echo "Usage: $0 <path-to-.app-bundle>" >&2
	exit 1
fi

mkdir -p "${DEST}"

for dir in baseq3 smokinguns ui; do
	src="${ROOT_DIR}/${dir}"
	if [ ! -d "${src}" ]; then
		echo "error: missing ${src}" >&2
		exit 1
	fi
	echo "Copying ${dir} -> ${DEST}/"
	rm -rf "${DEST}/${dir}"
	cp -R "${src}" "${DEST}/"
done

VM_SRC=""
for candidate in \
	"${ROOT_DIR}/build/ios-port/release-ios-arm64/smokinguns/vm" \
	"${ROOT_DIR}/build/release-ios-arm64/smokinguns/vm" \
	"${ROOT_DIR}/build/release-darwin-arm64/smokinguns/vm" \
	"${ROOT_DIR}/build/release-darwin-x86/smokinguns/vm" \
	"${ROOT_DIR}/build/release-darwin-x86_64/smokinguns/vm"; do
	if [ -d "${candidate}" ]; then
		VM_SRC="${candidate}"
		break
	fi
done

if [ -n "${VM_SRC}" ]; then
	mkdir -p "${DEST}/smokinguns/vm"
	cp -f "${VM_SRC}"/*.qvm "${VM_SRC}"/*.jts "${DEST}/smokinguns/vm/" 2>/dev/null || true
fi

echo "Game data installed in ${DEST}"
