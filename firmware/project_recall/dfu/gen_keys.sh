#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")"

if command -v imgtool >/dev/null 2>&1; then
	IMGTOOL=imgtool
elif [ -n "${ZEPHYR_BASE:-}" ] && [ -f "$ZEPHYR_BASE/../bootloader/mcuboot/scripts/imgtool.py" ]; then
	IMGTOOL="python3 $ZEPHYR_BASE/../bootloader/mcuboot/scripts/imgtool.py"
else
	echo "imgtool not found"
	exit 1
fi

$IMGTOOL keygen -k recall_root.pem -t rsa-2048
echo "wrote recall_root.pem"
