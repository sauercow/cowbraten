#!/bin/bash

function release_version() {
    local version
    version=$(sed -nE 's/.*const char \*cowversion = "([^"]+)";.*/\1/p' src/cowbraten/version.cpp)
    if [[ -z "$version" ]]; then
        echo "Unable to determine the cowbraten release version." >&2
        return 1
    fi
    printf '%s\n' "$version"
}

function build() {
    local platform=$1
    local target="$platform"
    shift
    if [[ "0" != "$#" ]]; then
        target=$1
    fi

    # Build the binaries.
    make "$target" || exit
}

function pack() {
    local platform=$1
    local version
    version=$(release_version) || exit

    local files=(
        "bin_unix"
        "cowbraten.sh"
    )
    case "$platform" in
        windows)
            mkdir -p dist
            rm -f dist/cowbraten_setup*
            makensis -DVERSION="$version" src/windows/cowbraten.nsi
            mv cowbraten_setup_* dist/
            ;;
        macos)
            files=(
                "sauerbraten.app/Contents/MacOS"
            )
            ;&
        *)
            mkdir -p dist
            rm -rf dist/cowbraten_"$platform"_*.zip
            zip -r dist/cowbraten_"$platform"_"$version".zip "${files[@]}"
            ;;
    esac
}

platform=$1

case "$platform" in
    linux)
        build "$platform" "debian"
        pack "$platform"
        ;;
    windows | macos)
        build "$platform"
        pack "$platform"
        ;;
    all)
        build "linux" "debian"
        pack "linux"
        build "windows"
        pack "windows"
        build "macos"
        pack "macos"
        ;;
    *)
        echo "unknown platform $platform"
        ;;
esac
