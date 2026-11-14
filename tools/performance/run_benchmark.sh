#!/usr/bin/env bash
set -euo pipefail

if [[ $# -lt 4 || $# -gt 7 ]]; then
    echo "usage: $0 ROOT BINARY PACKAGE_DIR OUTPUT_DIR [SCENARIO] [DURATION_MS] [EXTRA_SETUP]" >&2
    exit 2
fi

root=$1
binary=$2
package_dir=$3
output_dir=$4
scenario=${5:-gameplay_hidden}
duration_ms=${6:-8000}
extra_setup=${7:-}

mkdir -p "$output_dir"
run_home="$output_dir/$scenario"
mkdir -p "$run_home"

common="vsync 0; maxfps 0; maxtps 0; menufps 0"
warmup_ms=5000
load_arg=()
during_capture=

case "$scenario" in
    main_menu)
        setup="$common; clearconsole"
        ;;
    gameplay_hidden)
        load_arg=(-lbase/complex)
        setup="$common; clearconsole"
        ;;
    gameplay_bypass)
        load_arg=(-lbase/complex)
        setup="$common; clearconsole; ui_perf_bypass 1"
        ;;
    ingame_menu)
        load_arg=(-lbase/complex)
        setup="$common; clearconsole; showgui main"
        ;;
    scoreboard)
        load_arg=(-lbase/complex)
        setup="$common; clearconsole; scoreboard 1"
        ;;
    demo_browser)
        load_arg=(-lbase/complex)
        setup="$common; clearconsole; ui_demos"
        ;;
    profile)
        load_arg=(-lbase/complex)
        setup="$common; clearconsole; ui_profile"
        ;;
    settings)
        load_arg=(-lbase/complex)
        setup="$common; clearconsole; ui_settings"
        ;;
    settings_transition)
        load_arg=(-lbase/complex)
        setup="$common; clearconsole"
        during_capture="sleep 250 [ui_settings]"
        ;;
    anisotropy_transition)
        load_arg=(-lbase/complex)
        setup="$common; clearconsole; aniso 0"
        during_capture="sleep 250 [aniso 16; sleep 250 [aniso 0]]"
        ;;
    trilinear_transition)
        load_arg=(-lbase/complex)
        setup="$common; clearconsole; trilinear 1"
        during_capture="sleep 250 [trilinear 0; sleep 250 [trilinear 1]]"
        ;;
    filter_sequence_a)
        load_arg=(-lbase/complex)
        setup="$common; clearconsole; bilinear 1; trilinear 1; aniso 0"
        during_capture="sleep 3000 [aniso 16; sleep 3000 [bilinear 0; trilinear 0; aniso 0; sleep 3000 [bilinear 1; trilinear 1]]]"
        ;;
    filter_sequence_b)
        load_arg=(-lbase/complex)
        setup="$common; clearconsole; bilinear 0; trilinear 0; aniso 0"
        during_capture="sleep 3000 [bilinear 1; trilinear 1; sleep 3000 [aniso 16; sleep 3000 [aniso 0]]]"
        ;;
    screen_cycle)
        load_arg=(-lbase/complex)
        setup="$common; clearconsole"
        during_capture="sleep 250 [ui_main; sleep 750 [ui_settings; sleep 750 [ui_profile; sleep 750 [ui_demos; sleep 750 [ui_games; sleep 750 [ui_admin; sleep 750 [ui_editor; sleep 750 [ui_main; sleep 750 [ui_settings; sleep 750 [ui_profile; sleep 750 [ui_demos; sleep 750 [ui_games; sleep 750 [ui_admin; sleep 750 [ui_editor; sleep 750 [ui_close]]]]]]]]]]]]]]]"
        ;;
    *)
        echo "unknown scenario: $scenario" >&2
        exit 2
        ;;
esac

if [[ -n "$extra_setup" ]]; then
    setup="$common; $extra_setup; ${setup#"$common; "}"
fi

capture="perf_capture_start capture.csv"
finish="perf_capture_stop; quit"
script="$setup; sleep $warmup_ms [$capture; $during_capture; sleep $duration_ms [$finish]]"

cd "$root"
SDL_AUDIODRIVER=dummy "$binary" \
    -k"$package_dir" \
    -q"$run_home" \
    -gbenchmark.log \
    -w1280 -h720 -t0 \
    "${load_arg[@]}" \
    -x"$script"

test -s "$run_home/capture.csv"
