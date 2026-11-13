#!/usr/bin/env bash
# Run from the repository root after `make -C src client`:
# bash tests/autoexec_profile_integration.sh /path/to/sauerbraten /path/to/legacy/client
# Uses offscreen rendering and a temporary home; logs are retained for diagnosis.
set -euo pipefail

assets=$(realpath "${1:?Pass the Sauerbraten asset directory}")
legacy=$(realpath "${2:?Pass a legacy Cowbraten client without autoexecprofile}")
client=$(realpath src/cowbraten_client)
profile_test_dir=$(mktemp -d /tmp/cowbraten-autoexec-regression.XXXXXX)
cp -R tests/fixtures/autoexec_profiles/. "$profile_test_dir/"
printf 'Test home: %s\n' "$profile_test_dir"

export SDL_VIDEODRIVER=offscreen SDL_AUDIODRIVER=dummy LIBGL_ALWAYS_SOFTWARE=1
map_check='echo (concat PROFILE_TEST_PROFILE $autoexecprofile); mode 0; map ot; sleep 300 [map douze; sleep 300 [echo PROFILE_TEST_DONE; quit]]'

run_client()
{
    local binary=$1 label=$2 script=$3
    timeout 40s "$binary" "-q$profile_test_dir" "-k$assets" "-g$label.log" \
        -t0 -w640 -h480 "-x$script" >"$profile_test_dir/$label.stdout" 2>&1 || {
            printf 'Client failed: %s (see %s)\n' "$label" "$profile_test_dir" >&2
            return 1
        }
}

assert_log()
{
    grep -Fq -- "$2" "$profile_test_dir/$1.log" || {
        printf 'Missing in %s.log: %s\n' "$1" "$2" >&2
        return 1
    }
}

assert_no_recording()
{
    assert_log "$1" PROFILE_TEST_DONE
    if grep -Fq 'recording client demo' "$profile_test_dir/$1.log"; then
        printf 'Unexpected recording in %s.log\n' "$1" >&2
        return 1
    fi
}

run_legacy()
{
    run_client "$legacy" "$1" 'echo PROFILE_TEST_LEGACY_DONE; quit'
    assert_log "$1" PROFILE_TEST_LEGACY_DONE
    assert_log "$1" PROFILE_TEST_ORIGINAL
    if grep -q '^autoexecprofile ' "$profile_test_dir/config.cfg"; then
        printf 'Legacy client did not discard the unknown profile variable\n' >&2
        return 1
    fi
    grep -q '^mapstart = .*recordclientdemo 2' "$profile_test_dir/config.cfg"
}

# The first launch migrates config.cfg and clears its stale recording hook.
run_client "$client" migration "$map_check"
assert_log migration PROFILE_TEST_DEFAULT
assert_log migration 'PROFILE_TEST_PROFILE default'
assert_no_recording migration
grep -q '^autoexecprofile "default"$' "$profile_test_dir/autoexec-profile.cfg"
printf 'PASS migration and two map loads without recording\n'

# The old binary really rewrites the shared config; the new client must recover.
run_legacy legacy-default
run_client "$client" after-legacy "$map_check"
assert_log after-legacy PROFILE_TEST_DEFAULT
assert_log after-legacy 'PROFILE_TEST_PROFILE default'
assert_no_recording after-legacy
printf 'PASS default survives legacy client and two more map loads\n'

# Exporting another configuration must not change the saved startup choice.
run_client "$client" export 'autoexecprofile original; writecfg exported.cfg; exec autoexec-profile.cfg; echo (concat PROFILE_TEST_EXPORT $autoexecprofile); quit'
assert_log export 'PROFILE_TEST_EXPORT default'
printf 'PASS named export preserves startup choice\n'

run_client "$client" select-none 'autoexecprofile none; writecfg; quit'
run_legacy legacy-none
run_client "$client" disabled "$map_check"
assert_log disabled 'PROFILE_TEST_PROFILE none'
assert_no_recording disabled
if grep -Eq 'PROFILE_TEST_(ORIGINAL|DEFAULT)' "$profile_test_dir/disabled.log"; then
    printf 'Disabled selection unexpectedly loaded an autoexec\n' >&2
    exit 1
fi
printf 'PASS disabled selection survives legacy client\n'

# Explicitly choosing the original profile must still honor its recording hook.
run_client "$client" select-original 'autoexecprofile original; writecfg; quit'
run_client "$client" original "$map_check"
assert_log original PROFILE_TEST_ORIGINAL
assert_log original 'PROFILE_TEST_PROFILE original'
assert_log original PROFILE_TEST_DONE
test "$(grep -Fc 'recording client demo' "$profile_test_dir/original.log")" -eq 2
printf 'PASS original profile still records both maps when selected\n'
