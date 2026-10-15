#!/usr/bin/env bash

set -euo pipefail

TARGET=x86_64-elf
BINUTILS_VERSION=2.47
GCC_VERSION=16.2.0
PREFIX="$(pwd)/$TARGET-cross"

if [[ -z "${MAKEFLAGS:-}" && $# -gt 0 ]]; then
    MAKEFLAGS=$1
fi
if [[ -z "${MAKEFLAGS:-}" ]]; then
    MAKEFLAGS=-j12
fi
if [[ "$MAKEFLAGS" =~ ^[0-9]+$ ]]; then
    MAKEFLAGS="-j$MAKEFLAGS"
fi
export MAKEFLAGS

export PATH="$PREFIX/bin:$PATH"

if command -v gmake >/dev/null 2>&1; then
    mkdir -p "$PREFIX/bin"
    cat >"$PREFIX/bin/make" <<'EOF'
#!/usr/bin/env sh
exec gmake "$@"
EOF
    chmod +x "$PREFIX/bin/make"
fi

mkdir -p build
cd build

if [[ ! -f binutils-$BINUTILS_VERSION.tar.gz ]]; then
    wget "https://ftp.gnu.org/gnu/binutils/binutils-$BINUTILS_VERSION.tar.gz"
fi
if [[ ! -f gcc-$GCC_VERSION.tar.gz ]]; then
    wget "https://ftp.gnu.org/gnu/gcc/gcc-$GCC_VERSION/gcc-$GCC_VERSION.tar.gz"
fi

tar -xf "binutils-$BINUTILS_VERSION.tar.gz"
tar -xf "gcc-$GCC_VERSION.tar.gz"

rm -rf build-gcc build-binutils

mkdir build-binutils
cd build-binutils
../binutils-$BINUTILS_VERSION/configure \
    --target="$TARGET" \
    --prefix="$PREFIX" \
    --with-sysroot \
    --disable-nls \
    --disable-werror
make
make install
cd ..

cd "gcc-$GCC_VERSION"
contrib/download_prerequisites
cd ..

mkdir build-gcc
cd build-gcc
../gcc-$GCC_VERSION/configure \
    --target="$TARGET" \
    --prefix="$PREFIX" \
    --disable-nls \
    --enable-languages=c,c++ \
    --without-headers \
    --disable-hosted-libstdcxx

make all-gcc all-target-libgcc all-target-libstdc++-v3
make install-gcc install-target-libgcc install-target-libstdc++-v3

# Remove build directories to clean up
cd ..
rm -rf binutils-$BINUTILS_VERSION gcc-$GCC_VERSION build-gcc build-binutils

echo "Toolchain build and installation complete."
echo "You can now add $PREFIX/bin to your PATH to use the cross-compiler."