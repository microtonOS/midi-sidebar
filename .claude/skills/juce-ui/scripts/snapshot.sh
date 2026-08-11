#!/usr/bin/env bash
# =============================================================================
#  snapshot.sh — build the snapshot tool if needed, run it, print the PNG path.
#  ---------------------------------------------------------------------------
#  This is the single command an agent should run to look at the GUI. It keeps
#  the build step and the run step in one invocation so that one allowed-tools
#  rule covers the whole workflow.
#
#      snapshot.sh --target MyPlugin_snapshot [--build-dir build] [-- <tool args>]
#      snapshot.sh --bin path/to/MyPlugin_snapshot [-- <tool args>]
#
#  Everything after `--` is passed straight through to SnapshotTool; run
#  `snapshot.sh --target X -- --help` to see those options.
#
#  PNGs are written to ./tmp inside the project, created if it is missing,
#  unless the caller passes --dir or --out. Delete them once you have looked
#  at them; they are working files, not output.
#
#  On success the last line of stdout is the absolute path of the PNG.
# =============================================================================

set -euo pipefail

build_dir="build"
default_dir="tmp"
target=""
bin=""
config=""
tool_args=()

usage() {
    sed -n '2,17p' "$0" | sed 's/^# \{0,2\}//'
    exit "${1:-0}"
}

while [[ $# -gt 0 ]]; do
    case "$1" in
        --build-dir) build_dir="${2:?--build-dir needs a value}"; shift 2 ;;
        --target)    target="${2:?--target needs a value}";       shift 2 ;;
        --bin)       bin="${2:?--bin needs a value}";             shift 2 ;;
        --config)    config="${2:?--config needs a value}";       shift 2 ;;
        --help|-h)   usage 0 ;;
        --)          shift; tool_args=("$@"); break ;;
        *)           echo "snapshot.sh: unknown option '$1'" >&2; usage 1 ;;
    esac
done

if [[ -z "$bin" ]]; then
    if [[ -z "$target" ]]; then
        echo "snapshot.sh: need --target (to build) or --bin (to run directly)" >&2
        exit 1
    fi

    if [[ ! -d "$build_dir" ]]; then
        echo "snapshot.sh: build directory '$build_dir' does not exist." >&2
        echo "             Configure the project first, e.g.:" >&2
        echo "               cmake -B '$build_dir' -DCMAKE_BUILD_TYPE=Debug" >&2
        exit 1
    fi

    build_cmd=(cmake --build "$build_dir" --target "$target")
    [[ -n "$config" ]] && build_cmd+=(--config "$config")

    # Build chatter goes to stderr so stdout stays clean for the PNG path.
    "${build_cmd[@]}" >&2

    # Multi-config generators nest the binary under Debug/, Release/ etc, so
    # search rather than assuming a layout.
    bin="$(find "$build_dir" -type f -perm -u+x -name "$target" -print 2>/dev/null | head -n 1)"

    if [[ -z "$bin" ]]; then
        echo "snapshot.sh: built '$target' but could not find its executable under '$build_dir'." >&2
        echo "             Pass --bin <path> explicitly." >&2
        exit 1
    fi
fi

if [[ ! -x "$bin" ]]; then
    echo "snapshot.sh: '$bin' is not executable" >&2
    exit 1
fi

# Inside the project rather than the system temp directory. A path outside the
# project is gated separately from the permission rules, so an agent that has
# just rendered its own GUI then has to ask before it may look at it — which is
# not a feedback loop. `tmp/` is the conventional place for working files and is
# usually already gitignored.
#
# Only when the caller has said nothing about where the file goes: an explicit
# --dir or --out still wins.
use_default_dir=true

if [[ ${#tool_args[@]} -gt 0 ]]; then
    for arg in "${tool_args[@]}"; do
        case "$arg" in
            --dir|--out) use_default_dir=false ;;
        esac
    done
fi

if [[ "$use_default_dir" == true ]]; then
    mkdir -p "$default_dir"
    tool_args+=(--dir "$default_dir")
fi

exec "$bin" "${tool_args[@]}"
