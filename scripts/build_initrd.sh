#!/usr/bin/env bash

set -euo pipefail

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd -- "$SCRIPT_DIR/.." && pwd)"

TEMPLATE="$PROJECT_ROOT/user/template"
INITRD="$PROJECT_ROOT/user/initrd"
BUILD_DIR="$SCRIPT_DIR/build"

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

# NOTE: /private/etc links to /etc
# NOTE: /private/var links to /var
# NOTE: /private/tmp links to /tmp

# Copy the template contents into the initrd directory
cp -r "$TEMPLATE/." "$INITRD/"

while IFS= read -r -d '' c_file; do
    relative_path="${c_file#"$TEMPLATE"/}"
    executable_path="$BUILD_DIR/${relative_path%.c}"
    initrd_executable="$INITRD/${relative_path%.c}"

    mkdir -p "$(dirname -- "$executable_path")" "$(dirname -- "$initrd_executable")"
    bash "$SCRIPT_DIR/build_macho.sh" "$c_file" "$executable_path"
    cp "$executable_path" "$initrd_executable"
done < <(find "$TEMPLATE" -type f -name '*.c' -print0)

# Compress the initrd into a tarball
tar --format=ustar -cf "$BUILD_DIR/initrd.tar" -C "$INITRD" .