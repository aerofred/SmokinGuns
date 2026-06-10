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

for dir in baseq3 smokinguns; do
	src="${ROOT_DIR}/${dir}"
	if [ ! -d "${src}" ]; then
		echo "error: missing ${src}" >&2
		exit 1
	fi
	echo "Copying ${dir} -> ${DEST}/"
	rm -rf "${DEST}/${dir}"
	cp -R "${src}" "${DEST}/"
done

# The engine only loads menus from the gamedir search paths (baseq3 /
# smokinguns) and their .pk3s. A top-level "ui" folder in the bundle is NOT a
# search path, so menu edits placed there are ignored. Loose files inside the
# gamedir override .pk3 contents, so install our modified menus directly under
# smokinguns/ui/ to take precedence over the stock paks (sg_pak0.pk3 ...).
#
# Only override the menus we actually changed, so we don't clobber the
# iOS-customized system menus that ship inside the newer paks (sg_pak9.pk3).
echo "Installing modified UI menus -> ${DEST}/smokinguns/ui/"
mkdir -p "${DEST}/smokinguns/ui"
for menu in pop_specify.menu ingame.menu ingame_about.menu; do
	src="${ROOT_DIR}/ui/${menu}"
	if [ ! -f "${src}" ]; then
		echo "error: missing ${src}" >&2
		exit 1
	fi
	cp -f "${src}" "${DEST}/smokinguns/ui/${menu}"
done

echo "Game data installed in ${DEST}"
