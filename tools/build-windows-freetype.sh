#!/bin/sh
set -eu

version=2.14.3
expected_sha256=36bc4f1cc413335368ee656c42afca65c5a3987e8768cc28cf11ba775e785a5f
install_dir=${FREETYPE_WINDOWS_ROOT:-build/freetype-x86_64-windows}
case "$install_dir" in
    /*) install_prefix=$install_dir ;;
    *) install_prefix=$(pwd)/$install_dir ;;
esac

if [ -f "$install_dir/lib/libfreetype.a" ]; then
    exit 0
fi

work_dir=$(mktemp -d)
trap 'rm -rf "$work_dir"' EXIT HUP INT TERM
archive="$work_dir/freetype-$version.tar.xz"
source_dir="$work_dir/source"
build_dir="$work_dir/build"

curl --fail --location --silent --show-error \
    "https://download.savannah.gnu.org/releases/freetype/freetype-$version.tar.xz" \
    --output "$archive"

printf '%s  %s\n' "$expected_sha256" "$archive" | sha256sum --check --status
mkdir -p "$source_dir"
tar -xf "$archive" -C "$source_dir" --strip-components=1

# Zig does not provide a resource compiler. The version resource is optional
# for this static library and is not part of the Cowbraten executable metadata.
sed -i \
    -e '/  enable_language(RC)/d' \
    -e 's/list(APPEND BASE_SRCS builds\/windows\/ftdebug.c$/list(APPEND BASE_SRCS builds\/windows\/ftdebug.c)/' \
    -e '/                        src\/base\/ftver.rc)/d' \
    "$source_dir/CMakeLists.txt"

cmake -S "$source_dir" -B "$build_dir" \
    -DCMAKE_TOOLCHAIN_FILE="$(pwd)/src/windows/zig-windows-toolchain.cmake" \
    -DCMAKE_INSTALL_PREFIX="$install_prefix" \
    -DCMAKE_BUILD_TYPE=Release \
    -DBUILD_SHARED_LIBS=OFF \
    -DFT_DISABLE_BROTLI=ON \
    -DFT_DISABLE_BZIP2=ON \
    -DFT_DISABLE_HARFBUZZ=ON \
    -DFT_DISABLE_PNG=ON \
    -DFT_DISABLE_ZLIB=ON
cmake --build "$build_dir" --parallel
cmake --install "$build_dir"
