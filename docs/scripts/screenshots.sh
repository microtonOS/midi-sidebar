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
#  The figures. One per line:  <name> <crop> <arguments to SnapshotTool>
#
#  <crop> is the component the picture is cropped to, or `-` for the whole
#  editor. Only the two demo figures use `-`, because they are the only two that
#  are *about* the demo plugin's panel; everywhere else it is scenery standing
#  behind the thing being documented. Underscores in a crop name stand for
#  spaces, so the field stays one word: Controllers_table.
#
#  Grouped by the doc that uses them, and named with that doc's prefix, so
#  `screenshots.sh channels` renders exactly the channels page's set.
#
#  `--param` drives the plugin through its own parameters and `--click` through
#  its own buttons, so no figure needs a special build or a hook in shipped
#  code.
#
#  No theme or edge figures. The docs show the dark scheme and a left-hand
#  sidebar and nothing else: which scheme is in force and which side the sidebar
#  sits on are the reader's choices, not behaviour to be taught, and a figure per
#  combination multiplies the set without adding a fact. They still want
#  *checking* after a layout change — that is what an ad-hoc
#  `snapshot.sh -- --param theme=Light` is for.
# -----------------------------------------------------------------------------
figures=(
  # sidebar.md — collapsed, which is the rail and nothing else. The rail has two
  # forms: above metrics::regularBreakpoint the volume control is the fader and
  # meter strip, below it a button that opens the fader in a call-out.
  "rail                     Sidebar            --param page=None"
  "rail-compact             Sidebar            --param page=None --size 442x252"

  # presets.md
  "presets                  Sidebar            --param page=Presets"

  # controllers.md
  "controllers              Sidebar            --param page=Controllers"
  "controllers-sorted       Sidebar            --param page=Controllers --click CC"
  # Parts of a page, in addition to the whole page above them, never instead.
  # The table twice, because it has two states worth documenting: as the sidebar
  # opens, where it has more columns than the panel is wide, and dragged out to
  # where every column fits. 520 is the narrowest panel that needs no horizontal
  # scrolling; the editor has to be wide enough to hold it, hence --size.
  "controllers-table        Controllers_table  --param page=Controllers"
  "controllers-table-full   Controllers_table  --param page=Controllers --param panelWidth=520 --size 760x470"

  # tuning.md
  "tuning                   Sidebar            --param page=Tuning"

  # channels.md
  "channels-omni            Sidebar            --param page=Channels"
  "channels-mpe             Sidebar            --param page=Channels --click MPE"
  "channels-zone            Sidebar            --param page=Channels --click MPE --click 8"

  # demo.md — the only figures that may show the demo's own panel
  "demo-settings            -                  --param view=Settings --param page=None"
  "demo-synth               -                  --param view=Synth --param page=None"
)

# -----------------------------------------------------------------------------
filter="${1-}"
[[ "$filter" == "--list" ]] && filter=""

mkdir -p "$out"

for figure in "${figures[@]}"; do
    read -r name crop args <<< "$figure"

    [[ -n "$filter" && "$name" != "$filter"* ]] && continue

    # `-` is no crop; otherwise underscores go back to the spaces the component
    # was actually named with.
    component=()
    [[ "$crop" != "-" ]] && component=(--component "${crop//_/ }")

    if [[ "${1-}" == "--list" ]]; then
        printf '%-20s %-20s %s\n' "$name" "$crop" "$args"
        continue
    fi

    # snapshot.sh builds the tool if it is stale, so the first figure pays for
    # the build and the rest are a render each.
    .claude/skills/juce-ui/scripts/snapshot.sh --target "$target" -- \
        $args ${component[@]+"${component[@]}"} --out "$out/$name.png" >/dev/null

    echo "$out/$name.png"
done

# -----------------------------------------------------------------------------
#  What this cannot render
#  ---------------------------------------------------------------------------
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
