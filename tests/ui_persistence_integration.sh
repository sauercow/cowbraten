#!/usr/bin/env bash
# Run from the repository root after `make -C src client`:
# bash tests/ui_persistence_integration.sh /path/to/sauerbraten /path/to/legacy/client
set -euo pipefail

assets=$(realpath "${1:?Pass the Sauerbraten asset directory}")
legacy=$(realpath "${2:?Pass a legacy Cowbraten client}")
client=$(realpath src/cowbraten_client)
state_test_dir=$(mktemp -d /tmp/cowbraten-ui-persistence.XXXXXX)
printf 'Test home: %s\n' "$state_test_dir"

export SDL_VIDEODRIVER=offscreen SDL_AUDIODRIVER=dummy LIBGL_ALWAYS_SOFTWARE=1

run_client()
{
    local binary=$1 label=$2 script=$3
    timeout 40s "$binary" "-q$state_test_dir" "-k$assets" "-g$label.log" \
        -t0 -w640 -h480 "-x$script" >"$state_test_dir/$label.stdout" 2>&1
}

run_client "$client" save \
    'profile_name_1 TestAlpha; profile_name_2 TestBeta; uitheme modern-metallic; writecfg; quit'
grep -q '^profile_name_1 "TestAlpha"$' "$state_test_dir/cowbraten-ui.cfg"
grep -q '^profile_name_2 "TestBeta"$' "$state_test_dir/cowbraten-ui.cfg"
grep -q '^uitheme "modern-metallic"$' "$state_test_dir/cowbraten-ui.cfg"

run_client "$legacy" legacy 'quit'
grep -q 'unknown command: profile_name_1' "$state_test_dir/legacy.log"
if grep -q '^profile_name_1 ' "$state_test_dir/config.cfg"; then
    printf 'Legacy client unexpectedly retained Cowbraten-only variables\n' >&2
    exit 1
fi

run_client "$client" restore \
    'echo (concatword UI_STATE_NAME_1= $profile_name_1); echo (concatword UI_STATE_NAME_2= $profile_name_2); echo (concatword UI_STATE_THEME= $uitheme); quit'
grep -q '^UI_STATE_NAME_1=TestAlpha$' "$state_test_dir/restore.log"
grep -q '^UI_STATE_NAME_2=TestBeta$' "$state_test_dir/restore.log"
grep -q '^UI_STATE_THEME=modern-metallic$' "$state_test_dir/restore.log"

printf 'PASS names and theme survive a legacy config rewrite\n'
