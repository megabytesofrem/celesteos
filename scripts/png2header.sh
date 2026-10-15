#!/usr/bin/env bash

set -euo pipefail

# Check if an input file was provided
if [ "$#" -lt 1 ] || [ "$#" -gt 2 ]; then
    echo "Usage: $0 <input_image.png> [output_header.h]"
    exit 1
fi

INPUT_FILE="$1"

if [ ! -f "$INPUT_FILE" ]; then
    echo "Error: Input file '$INPUT_FILE' does not exist."
    exit 1
fi

# Determine output filename (default to input name with .h extension)
if [ "$#" -ge 2 ]; then
    OUTPUT_FILE="$2"
else
    OUTPUT_FILE="${INPUT_FILE%.*}.h"
fi

# Ensure absolute paths for the batch process.
ABS_INPUT="$(realpath "$INPUT_FILE")"
ABS_OUTPUT="$(realpath -m "$OUTPUT_FILE")"

if [ "$ABS_INPUT" = "$ABS_OUTPUT" ]; then
    echo "Error: Input and output paths must differ." >&2
    exit 1
fi

TEMP_OUTPUT="$(mktemp -- "${ABS_OUTPUT}.tmp.XXXXXX")"
trap 'rm -f -- "$TEMP_OUTPUT"' EXIT

echo "[CelesteOS Toolchain] Converting '$INPUT_FILE' -> '$OUTPUT_FILE' via GIMP CLI..."

# Keep paths out of executable batch code, including quotes and backslashes.
PNG2HEADER_INPUT="$ABS_INPUT" PNG2HEADER_OUTPUT="$TEMP_OUTPUT" \
gimp --new-instance --no-interface --no-data --no-fonts \
    --batch-interpreter=python-fu-eval --quit --batch="$(cat <<'PY'
import os
from gi.repository import Gimp, Gio

procedure = Gimp.get_pdb().lookup_procedure("file-header-export")
if procedure is None:
    raise RuntimeError("GIMP 3 file-header-export is required")

image = Gimp.file_load(
    Gimp.RunMode.NONINTERACTIVE,
    Gio.File.new_for_path(os.environ["PNG2HEADER_INPUT"]),
)
if image is None:
    raise RuntimeError("Failed to load the input image")

try:
    config = procedure.create_config()
    config.set_property("run-mode", Gimp.RunMode.NONINTERACTIVE)
    config.set_property("image", image)
    config.set_property("file", Gio.File.new_for_path(os.environ["PNG2HEADER_OUTPUT"]))
    result = procedure.run(config)
    if result.index(0) != Gimp.PDBStatusType.SUCCESS:
        raise RuntimeError("GIMP header export failed: " + str(result.index(0)))
finally:
    image.delete()
PY
)"

if [ ! -s "$TEMP_OUTPUT" ]; then
    echo "Error: GIMP did not generate a header." >&2
    exit 1
fi
mv -- "$TEMP_OUTPUT" "$ABS_OUTPUT"

echo "[CelesteOS Toolchain] Success! Header generated at: $OUTPUT_FILE"