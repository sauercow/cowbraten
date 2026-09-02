#!/bin/bash

function build() {
    local platform=$1
    local target="$platform"
    shift
    if [[ "0" != "$#" ]]; then
        target=$1
    fi

    # Set the version in the source for release builds.
    cp src/p1xbraten/version.cpp src/p1xbraten/version.cpp.orig
    local version
    version=$(git describe)
    sed -i "s/<git-dev>/$version/" src/p1xbraten/version.cpp

    # Build the binaries.
    make "$target" || exit

    # Restore the development version marker.
    rm src/p1xbraten/version.cpp
    mv src/p1xbraten/version.cpp.orig src/p1xbraten/version.cpp
}

function pack() {
    local platform=$1
    local version
    version=$(git describe)

    local files=(
        "bin_unix"
        "p1xbraten.sh"
    )
    case "$platform" in
        windows)
            rm -f dist/p1xbraten_setup*
            cp src/windows/p1xbraten.nsi src/windows/p1xbraten.nsi.orig
            sed -i "s/<git-dev>/$version/" src/windows/p1xbraten.nsi
            makensis src/windows/p1xbraten.nsi
            mv p1xbraten_setup_* dist/
            rm src/windows/p1xbraten.nsi
            mv src/windows/p1xbraten.nsi.orig src/windows/p1xbraten.nsi
            ;;
        macos)
            files=(
                "sauerbraten.app/Contents/MacOS"
            )
            ;&
        *)
            mkdir -p dist
            rm -rf dist/p1xbraten_"$platform"_*.zip
            zip -r dist/p1xbraten_"$platform"_"$version".zip "${files[@]}"
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
