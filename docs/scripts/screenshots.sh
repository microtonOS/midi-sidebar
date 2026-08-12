#!/usr/bin/env bash
# =============================================================================
#  screenshots.sh — render every figure the docs use, into docs/figures/.
#  ---------------------------------------------------------------------------
#  The docs describe *decisions*; the pictures show what those decisions look
#  like. Keeping the pictures generated rather than pasted means they cannot go
#  stale silently: re-run this after a layout change and `git status` says which
#  documented screens moved.
#
#      docs/scripts/screenshots.sh              # every figure
#      docs/scripts/screenshots.sh channels     # only names starting thus
#      docs/scripts/screenshots.sh --list       # what it would render, no build
#
#  Add a figure by adding one line to the table below. Nothing else here needs
#  to change, and the name in the first column is the file name in docs/figures,
#  so a doc referring to `figures/channels-mpe.png` is referring to a row.
# =============================================================================

set -euo pipefail

cd "$(dirname "${BASH_SOURCE[0]}")/../.."

target="SidebarDemo_snapshot"
out="docs/figures"

# -----------------------------------------------------------------------------
#  The figures. One per line:  <name> <arguments to SnapshotTool>
#
#  Grouped by the doc that uses them, and named with that doc's prefix, so
#  `screenshots.sh tuning` renders exactly the tuning page's set.
#
#  `--param` drives the plugin through its own parameters and `--click` through
#  its own buttons, so no figure needs a special build or a hook in shipped
#  code. Menus cannot be captured at all — `PopupMenu` needs a display — so a
#  doc wanting one still needs a hand-taken picture; see the note at the end.
#
#  Every figure is of the *whole editor*, so the demo's own tab panel is behind
#  the sidebar in all of them. Only the `demo-` rows want that. See the note.
# -----------------------------------------------------------------------------
figures=(
  # sidebar.md
  "sidebar-collapsed        --param page=None"
  "sidebar-minimum          --param page=None --size 442x252"
  "sidebar-edge-right       --param page=Tuning --param edge=Right"

  # presets.md
  "presets                  --param page=Presets"

  # controllers.md
  "controllers              --param page=Controllers"
  "controllers-sorted       --param page=Controllers --click CC"

  # tuning.md
  "tuning                   --param page=Tuning"

  # channels.md
  "channels-omni            --param page=Channels"
  "channels-mpe             --param page=Channels --click MPE"
  "channels-zone            --param page=Channels --click MPE --click 8"

  # general.md — the schemes, all showing the same page so only colour varies
  "theme-light              --param page=Controllers --param theme=Light"
  "theme-midnight           --param page=Controllers --param theme=Midnight"
  "theme-grey               --param page=Controllers --param theme=Grey"

  # demo.md — and only these may show the demo's own panel
  "demo-settings            --param view=Settings --param page=None"
  "demo-synth               --param view=Synth --param page=None"
)

# -----------------------------------------------------------------------------
filter="${1-}"
[[ "$filter" == "--list" ]] && filter=""

mkdir -p "$out"

for figure in "${figures[@]}"; do
    read -r name args <<< "$figure"

    [[ -n "$filter" && "$name" != "$filter"* ]] && continue

    if [[ "${1-}" == "--list" ]]; then
        printf '%-24s %s\n' "$name" "$args"
        continue
    fi

    # snapshot.sh builds the tool if it is stale, so the first figure pays for
    # the build and the rest are a render each.
    .claude/skills/juce-ui/scripts/snapshot.sh --target "$target" -- \
        $args --out "$out/$name.png" >/dev/null

    echo "$out/$name.png"
done

# -----------------------------------------------------------------------------
#  What this cannot render
#  ---------------------------------------------------------------------------
#  * The sidebar on its own. SnapshotTool renders the editor, so every figure
#    carries the demo's tab panel beside the sidebar — fine for the `demo-`
#    rows, noise for the rest. The fix is a `--component <name>` option in
#    SnapshotTool that renders one subtree instead of the whole editor; until
#    then a page figure shows more than the page.
#  * Anything behind a `PopupMenu` — the channel menu in the editing table, the
#    tuning name menu, the right-click parameter menu. `PopupMenu::getParentArea`
#    dereferences `getDisplayForPoint(...)` with no null check, and a headless
#    process has no displays, so the tool crashes rather than drawing. JUCE's
#    bug, not this project's.
#  * Anything behind a table header — `--click` finds `Button`s by name, and a
#    `TableHeaderComponent` is not one. The three-state sort is therefore a
#    hand-taken picture for now.
#  * Hover and drag states, and the volume bubble, which only exists while the
#    fader is being moved.
# =============================================================================
