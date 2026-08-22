#!/usr/bin/env bash
#
# Build and run a one-off console check against a JUCE module, with no project
# of your own.
#
#   check.sh MyCheck.cpp --module ~/code/my_module [--juce ~/JUCE] [--std 20]
#                        [--link juce::juce_audio_basics] [--keep]
#
# Writes a throwaway CMake project to a temp directory, builds it, runs the
# binary and forwards its exit code — so a check that returns its failure count
# from main() works as a shell command and in CI.
#
# Why this exists: a module's logic headers can be exercised without a plugin, a
# host or a window, but wiring the CMake by hand takes long enough that the
# check does not get written. It also removes the failure mode this script was
# extracted from — a suite written into a session temp directory, passing, and
# gone the next day. For anything you want to *keep*, put it in your repository
# as a real target and use add_check_app.cmake instead.

set -euo pipefail

SOURCE=""
MODULES=()
LINKS=()
JUCE_DIR="${JUCE_DIR:-$HOME/JUCE}"
STD=17
KEEP=0

while [[ $# -gt 0 ]]; do
    case "$1" in
        --module) MODULES+=("$2"); shift 2 ;;
        --link)   LINKS+=("$2");   shift 2 ;;
        --juce)   JUCE_DIR="$2";   shift 2 ;;
        --std)    STD="$2";        shift 2 ;;
        --keep)   KEEP=1;          shift   ;;
        -h|--help) sed -n '2,20p' "$0" | sed 's/^# \{0,1\}//'; exit 0 ;;
        *)        SOURCE="$1";     shift   ;;
    esac
done

if [[ -z "$SOURCE" ]]; then
    echo "usage: check.sh <source.cpp> [--module <path>] [--juce <path>] [--std N] [--link <target>]" >&2
    exit 2
fi

# --module is optional: plenty of checks need only a module that ships with JUCE
# itself, which --link covers.
#
# A module with its own **external** dependency — one whose headers include a
# third-party header — cannot be built this way at all, because nothing here
# knows to add that dependency. Put such a check in your repository, where it is
# already wired, and use add_check_app.cmake.

if [[ ! -f "$JUCE_DIR/CMakeLists.txt" ]]; then
    echo "check.sh: no JUCE at '$JUCE_DIR' (pass --juce or set JUCE_DIR)" >&2
    exit 2
fi

SOURCE="$(cd "$(dirname "$SOURCE")" && pwd)/$(basename "$SOURCE")"
NAME="$(basename "${SOURCE%.*}")"

WORK="$(mktemp -d "${TMPDIR:-/tmp}/juce-check-XXXXXX")"
[[ $KEEP -eq 1 ]] || trap 'rm -rf "$WORK"' EXIT

{
    echo "cmake_minimum_required(VERSION 3.22)"
    # A VERSION is not optional: juce_add_console_app refuses a target without
    # one, and the error names the target rather than the cause.
    echo "project(JuceCheck VERSION 0.0.1 LANGUAGES C CXX)"
    echo "set(CMAKE_CXX_STANDARD $STD)"
    echo "set(CMAKE_CXX_STANDARD_REQUIRED ON)"
    echo "add_subdirectory(\"$JUCE_DIR\" juce EXCLUDE_FROM_ALL)"

    # `${arr[@]+"${arr[@]}"}` rather than `"${arr[@]}"`: macOS ships bash 3.2,
    # where an *empty* array expanded under `set -u` is an unbound variable and
    # aborts the script. The `+` form expands to nothing when the array is unset
    # and to its elements otherwise. Bash 4.4 fixed this; macOS will not ship it.
    for m in ${MODULES[@]+"${MODULES[@]}"}; do
        printf 'juce_add_module("%s")\n' "$(cd "$m" && pwd)"
    done

    echo "juce_add_console_app($NAME PRODUCT_NAME $NAME)"
    printf 'target_sources(%s PRIVATE "%s")\n' "$NAME" "$SOURCE"
    printf 'target_include_directories(%s PRIVATE "%s")\n' "$NAME" "$(dirname "$SOURCE")"
    echo "target_compile_definitions($NAME PRIVATE JUCE_STANDALONE_APPLICATION=1 JUCE_USE_CURL=0 JUCE_WEB_BROWSER=0)"

    printf 'target_link_libraries(%s PRIVATE' "$NAME"
    for m in ${MODULES[@]+"${MODULES[@]}"}; do printf ' %s' "$(basename "$m")"; done
    for l in ${LINKS[@]+"${LINKS[@]}"}; do printf ' %s' "$l"; done
    printf ' juce::juce_recommended_config_flags juce::juce_recommended_warning_flags)\n'
} > "$WORK/CMakeLists.txt"

cmake -S "$WORK" -B "$WORK/build" -DCMAKE_BUILD_TYPE=Debug > "$WORK/configure.log" 2>&1 || {
    echo "check.sh: configure failed" >&2; tail -25 "$WORK/configure.log" >&2; exit 1; }

# macOS note: builds print "couldn't create cache file '.../xcrun_db-…'" once per
# compiler invocation when the Darwin per-user temp dir is not writable.
# Harmless, and filtered rather than fixed — XCRUN_DB_PATH does not affect it.
cmake --build "$WORK/build" 2>&1 | grep -v "cache file" || {
    echo "check.sh: build failed" >&2; exit 1; }

# JUCE puts a console app under <target>_artefacts/, sometimes with a
# configuration directory inside it, so the binary is found rather than guessed.
BINARY="$(find "$WORK/build" -type f -perm -111 -name "$NAME" | head -1)"

if [[ -z "$BINARY" ]]; then
    echo "check.sh: built, but no '$NAME' binary found under $WORK/build" >&2
    exit 1
fi

"$BINARY"
