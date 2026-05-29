#!/bin/bash
# Génère AppIcon-1024.png et les tailles iOS à partir de misc/smokinguns.ico
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
ROOT_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"
SOURCE="${ROOT_DIR}/smokinguns.ico"
MASTER="${SCRIPT_DIR}/AppIcon-1024.png"
OUT_DIR="${1:-}"

if [ ! -f "${SOURCE}" ]; then
	echo "Icon source missing: ${SOURCE}" >&2
	exit 1
fi

echo "Generating iOS icons from ${SOURCE}"
sips -s format png -z 1024 1024 "${SOURCE}" --out "${MASTER}" >/dev/null

if [ -z "${OUT_DIR}" ]; then
	exit 0
fi

mkdir -p "${OUT_DIR}"
cp "${MASTER}" "${OUT_DIR}/AppIcon-1024.png"

for size in 20 29 40 58 60 76 80 87 120 152 167 180; do
	sips -z "${size}" "${size}" "${MASTER}" --out "${OUT_DIR}/AppIcon-${size}.png" >/dev/null
done

cp "${MASTER}" "${OUT_DIR}/AppIcon.png"
