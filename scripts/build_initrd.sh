#!/usr/bin/env bash

set -euo pipefail

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd -- "$SCRIPT_DIR/.." && pwd)"

TEMPLATE="$PROJECT_ROOT/user/template"
INITRD="$PROJECT_ROOT/user/initrd"
BUILD_DIR="$SCRIPT_DIR/build"
SOURCES="$PROJECT_ROOT/user/sources"

echo "Initrd directory: $INITRD"
echo "Template directory: $TEMPLATE"
echo "Build directory: $BUILD_DIR"
mkdir -p "$INITRD"
mkdir -p "$BUILD_DIR"

# Create the template if it doesn't exist
mkdir -p \
    "$TEMPLATE/System/Library/Frameworks" \
    "$TEMPLATE/System/Library/Extensions" \
    "$TEMPLATE/System/Library/CoreServices" \
    "$TEMPLATE/System/Library/LaunchAgents" \
    "$TEMPLATE/System/Library/LaunchDaemons" \
    "$TEMPLATE/Library" \
    "$TEMPLATE/Library/Application Support" \
    "$TEMPLATE/Library/Preferences" \
    "$TEMPLATE/Library/Preferences/SystemConfiguration" \
    "$TEMPLATE/Library/LaunchAgents" \
    "$TEMPLATE/Library/LaunchDaemons" \
    "$TEMPLATE/Applications" \
    "$TEMPLATE/Volumes" \
    "$TEMPLATE/usr/bin" \
    "$TEMPLATE/usr/lib" \
    "$TEMPLATE/usr/libexec" \
    "$TEMPLATE/private/etc" \
    "$TEMPLATE/private/var" \
    "$TEMPLATE/private/tmp"

# Build user programs into the template filesystem
while IFS= read -r -d '' source_file; do
    binary_name="$(basename -- "${source_file%.c}")"

    case "$binary_name" in
        "launchd")
            # launchd is special-cased and goes into /usr/libexec
            "$SCRIPT_DIR/build_macho.sh" "$source_file" "$TEMPLATE/usr/libexec/$binary_name"
            ;;
        *)
            # All other binaries go into /usr/bin
            "$SCRIPT_DIR/build_macho.sh" "$source_file" "$TEMPLATE/usr/bin/$binary_name"
            ;;
    esac

done < <(find "$SOURCES" -type f -name '*.c' -print0)

# NOTE: /private/etc links to /etc
# NOTE: /private/var links to /var
# NOTE: /private/tmp links to /tmp

# Copy the template contents into the initrd directory
cp -r "$TEMPLATE/." "$INITRD/"

# Compress the initrd into a tarball
tar --format=ustar -cf "$BUILD_DIR/initrd.tar" -C "$INITRD" .