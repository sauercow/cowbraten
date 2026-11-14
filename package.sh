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
        "data/cowbraten"
    )
    case "$platform" in
        windows)
            mkdir -p dist
            rm -f dist/cowbraten_setup*
            makensis -DVERSION="$version" src/windows/cowbraten.nsi
            mv cowbraten_setup_* dist/
            ;;
        linux)
            mkdir -p dist
            rm -f dist/cowbraten_linux_"$version".zip \
                dist/cowbraten_setup_linux_"$version".run
            zip -r dist/cowbraten_linux_"$version".zip "${files[@]}"

            local payload
            payload=$(mktemp)
            tar -czf "$payload" "${files[@]}"
            cp tools/linux-installer.sh dist/cowbraten_setup_linux_"$version".run
            cat "$payload" >> dist/cowbraten_setup_linux_"$version".run
            rm -f "$payload"
            chmod +x dist/cowbraten_setup_linux_"$version".run
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
        if [[ "$platform" == "windows" ]]; then
            tools/build-windows-freetype.sh || exit
        fi
        build "$platform"
        pack "$platform"
        ;;
    all)
        build "linux" "debian"
        pack "linux"
        tools/build-windows-freetype.sh || exit
        build "windows"
        pack "windows"
        build "macos"
        pack "macos"
        ;;
    *)
        echo "unknown platform $platform"
        ;;
esac
