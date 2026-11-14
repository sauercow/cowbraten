#!/bin/sh
set -eu

archive_marker='__COWBRATEN_ARCHIVE_BELOW__'

find_default_installation()
{
    for candidate in \
        "${SAUER_DIR:-}" \
        "$HOME/.local/share/sauerbraten" \
        "$HOME/Games/sauerbraten" \
        "/usr/share/games/sauerbraten"
    do
        if [ -n "$candidate" ] && [ -d "$candidate/packages" ]; then
            printf '%s\n' "$candidate"
            return 0
        fi
    done
    return 1
}

destination=${1:-}
if [ -z "$destination" ]; then
    destination=$(find_default_installation || true)
fi

if [ -t 0 ]; then
    if [ -n "$destination" ]; then
        printf 'Sauerbraten installation [%s]: ' "$destination"
    else
        printf 'Sauerbraten installation directory: '
    fi
    IFS= read -r answer
    if [ -n "$answer" ]; then destination=$answer; fi
fi

if [ -z "$destination" ]; then
    printf 'No Sauerbraten installation was selected.\n' >&2
    printf 'Usage: %s /path/to/sauerbraten\n' "$0" >&2
    exit 1
fi

if [ ! -d "$destination/packages" ]; then
    printf 'The selected directory does not contain Sauerbraten packages: %s\n' "$destination" >&2
    exit 1
fi

if [ ! -w "$destination" ]; then
    printf 'The selected directory is not writable: %s\n' "$destination" >&2
    printf 'Choose a user-owned Sauerbraten installation or run this installer with suitable permissions.\n' >&2
    exit 1
fi

marker_line=$(awk -v marker="$archive_marker" '$0 == marker { print NR + 1; exit }' "$0")
if [ -z "$marker_line" ]; then
    printf 'The installer payload is missing or damaged.\n' >&2
    exit 1
fi

printf 'Installing Cowbraten into %s ...\n' "$destination"
tail -n +"$marker_line" "$0" | tar -xzf - -C "$destination"
printf 'Cowbraten installed successfully. Run %s/cowbraten.sh to start it.\n' "$destination"
exit 0

__COWBRATEN_ARCHIVE_BELOW__
