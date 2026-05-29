#!/bin/bash
# Copy baseq3/ and smokinguns/ into the iOS app bundle Resources.
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
ROOT_DIR="$(cd "${SCRIPT_DIR}/../.." && pwd)"
DEST="${1:-}"

if [ -z "${DEST}" ]; then
	echo "Usage: $0 <path-to-.app-bundle>" >&2
	exit 1
fi

RESOURCES="${DEST}"
mkdir -p "${RESOURCES}"

for dir in baseq3 smokinguns; do
	src="${ROOT_DIR}/${dir}"
	if [ ! -d "${src}" ]; then
		echo "error: missing ${src}" >&2
		exit 1
	fi
	echo "Copying ${dir} -> ${RESOURCES}/"
	rm -rf "${RESOURCES}/${dir}"
	cp -R "${src}" "${RESOURCES}/"
done

# QVMs from build tree if present
VM_SRC=""
for candidate in \
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
	mkdir -p "${RESOURCES}/smokinguns/vm"
	cp -f "${VM_SRC}"/*.qvm "${VM_SRC}"/*.jts "${RESOURCES}/smokinguns/vm/" 2>/dev/null || true
fi

# Menus UI du dépôt (écrase les .menu du pk3 si présents en loose files)
UI_SRC="${ROOT_DIR}/ui"
if [ -d "${UI_SRC}" ]; then
	for dest in "${RESOURCES}/ui" "${RESOURCES}/smokinguns/ui"; do
		mkdir -p "${dest}"
		cp -f "${UI_SRC}"/*.menu "${UI_SRC}"/*.txt "${dest}/" 2>/dev/null || true
		echo "Copied ui/*.{menu,txt} -> ${dest}/"
	done
fi

echo "Game data installed in ${RESOURCES}"
