# GUI tools

Two feedback loops for layout work, which is the part of a GUI that cannot be
verified by reading the code: one to see the result, one to check the rules that
keep the result maintainable.

| file | what it is |
|------|------------|
| `SnapshotTool.cpp` | Renders a plugin editor to a PNG in software. Project-agnostic; configured by two compile definitions. |
| `add_snapshot_tool.cmake` | `juce_gui_add_snapshot_tool()`, which wires the above into a JUCE CMake project. |
| `snapshot.sh` | Build-and-run wrapper. Prints the PNG path as the last line of stdout. |
| `layout_lint.py` | Checks `resized()` bodies against Layout mechanics rules 1, 3 and 6. No build needed. |

# Snapshot tool

Renders a JUCE plugin editor to a PNG with no window, no audio device and no
screen capture, so it works inside a sandbox. This is how the agent looks at its
own GUI work instead of asking the user for a screenshot.

It can also drive the GUI before rendering — set parameters, click things — so
states that only exist after interaction can be captured too.

## Setup (once per project)

Add to the project's `CMakeLists.txt`, after the `juce_add_plugin` call:

```cmake
include(.claude/skills/juce-ui/scripts/add_snapshot_tool.cmake)

juce_gui_add_snapshot_tool(
    TARGET           MyPlugin_snapshot
    PLUGIN_TARGET    MyPlugin
    PROCESSOR_CLASS  MyPluginAudioProcessor
    PROCESSOR_HEADER "PluginProcessor.h"
    INCLUDE_DIRS     "${CMAKE_CURRENT_SOURCE_DIR}/plugin")
```

`PROCESSOR_HEADER` is included by the tool, so it must be reachable from
`INCLUDE_DIRS`.

## Use

```bash
.claude/skills/juce-ui/scripts/snapshot.sh --target MyPlugin_snapshot
```

That builds the target if needed, runs it, and prints something like
`/var/folders/.../T/juce-ui-snapshots/snapshot.png`. Read that file to see the
GUI.

Useful variations — everything after `--` goes to the tool:

```bash
# A specific control state, by parameter ID, using the text you would type in the GUI
snapshot.sh --target MyPlugin_snapshot -- --param leslie_speed=Fast --name leslie-fast

# ...or by normalised 0..1 value
snapshot.sh --target MyPlugin_snapshot -- --nparam drawbar0=1.0

# What are the IDs?
snapshot.sh --target MyPlugin_snapshot -- --list-params

# Click something first — a menu button, a panel toggle, a call-out
snapshot.sh --target MyPlugin_snapshot -- --click Volume --name volume-open

# What can be clicked? Names, sizes and positions of every component
snapshot.sh --target MyPlugin_snapshot -- --list-components

# Check the layout at an awkward window size
snapshot.sh --target MyPlugin_snapshot -- --size 640x360 --name narrow

# The editor is wrapped in a host-like window by default; opt out to snapshot
# it as the top-level component — see The host window, below.
snapshot.sh --target MyPlugin_snapshot -- --no-host-window

# Keep a series instead of overwriting, to compare before and after
snapshot.sh --target MyPlugin_snapshot -- --timestamp
```

`--param` is the reason this tool does not need environment-variable hooks
baked into the editor. Drive the GUI through its parameters, and the shipped
code stays free of dev-only branches.

## Driving the GUI: `--click` and `--list-components`

`--list-components` prints the editor's tree — name, size, position, and
whether each component is visible — *after* any `--click`, so it describes the
state being captured rather than the state before it. Names come from
`Component::getName()`, which is usually the string passed to the widget's
constructor. It is also the quickest way to answer "is this thing actually
where I think it is", which beats measuring pixels.

`--click <name>` finds a component by name and clicks it, and is repeatable, so
several clicks happen in order. Only `Button`s can be clicked so far; anything
else is reported as an error rather than silently ignored, and so is a name that
matches nothing.

**A call-out needs `--click-settle` left alone.** `juce::CallOutBox` starts a
200 ms timer when it opens and dismisses itself if its process is not in the
foreground — which a headless render never is. Everything between the click and
the paint has to fit inside that window, which is why the dwell after a click
defaults to 120 ms rather than the usual 250 ms, and why the final settle is
skipped once anything has been clicked. If you are capturing something slower
and no call-out is involved, raise it:

```bash
snapshot.sh --target X -- --click Tuning --click-settle 300
```

Raising it *with* a call-out involved is a trap worth naming, because of how it
presents: the box is constructed and destroyed before the frame, so a trace
through its constructor and destructor shows both running and it reads as a bug
in the launching code rather than a timing setting. If a call-out is missing from
a render, put `--click-settle` back to the default before suspecting anything
else. The `grabKeyboardFocus` assertion logged in the same run is unrelated and
harmless — see [popups](../references/popups.md#calloutbox).

**A call-out must also be launched with a parent component** to be captured at
all. `CallOutBox::launchAsynchronously (content, area, nullptr)` puts it on the
desktop, where it is a separate window: outside the editor's component tree, so
`createComponentSnapshot` cannot see it, and styled by the default LookAndFeel
rather than yours. Pass the editor instead.

## Framing the picture: `--component`

`--component <name>` crops the render to one component, so a figure can show a
single panel, page or table without the rest of the window around it. It takes
the same names as `--click`, is applied after every click, and fails the same
way on a name that matches nothing.

```bash
snapshot.sh --target X -- --component "Sidebar panel" --param page=Tuning
```

It **crops the full render** rather than snapshotting the component on its own.
A component that does not paint its own background — most containers do not —
would otherwise come out over transparency or over black, which is a different
picture rather than a smaller one. Cropping keeps every pixel exactly as it is
on screen.

Two consequences worth knowing. A hidden or not-yet-laid-out component has zero
size and is reported as an error rather than written as an empty file, which is
usually telling you the state you asked for is not the one you think. And the
name has to exist: containers are unnamed by default in JUCE, so cropping to one
often means adding a `setName` first — worth doing regardless, since that name
is also what accessibility reads.

## Where the files go

`snapshot.sh` writes to **`tmp/` inside the project**, creating it if it is
missing, unless you pass `--dir` or `--out` yourself.

Inside the project rather than the system temp directory, because a path outside
it is gated separately from the permission rules: an agent that has just
rendered its own GUI is then asked whether it may look at the file, on every
render. That is not a feedback loop. `tmp/` is the conventional place for
working files and is usually already in `.gitignore` — check that it is, because
these images are working files and must never be committed.

**Delete a snapshot once you have looked at it.** It has done its job the moment
you have read it; what is left otherwise is a directory of near-identical PNGs
that nobody can tell apart, in which the one that matters is not obvious. Keep
only images someone else has been asked to look at, and only until they have.
The tool also prunes its own directory to the 20 most recent (`--keep`), but
that is a backstop against filling a disk, not a substitute: it cannot know
which of the twenty were still wanted.

`SnapshotTool` on its own — run directly rather than through `snapshot.sh` —
still defaults to `<system temp>/juce-ui-snapshots/`, via
`juce::File::getSpecialLocation (juce::File::tempDirectory)`. It does not rely
on the OS sweeping that directory: macOS only removes files untouched for three
days, and Apple states that behaviour is not API.

## The host window

**By default the editor is placed inside a plain parent component that keeps the
DEFAULT LookAndFeel**, the way a standalone build or a DAW wraps it in a window.
Nothing to pass; `--host-window` is still accepted and means the same thing.

This matters because the editor is never the top-level component in real use, so
anything walking *up* from it — `getTopLevelComponent`, `getLookAndFeel` on an
ancestor, parenting a pop-up — behaves differently once there is a window above
it. The recurring case is pop-up parenting: with the editor as the top level, a
`CallOutBox` parented to `getTopLevelComponent()` inherits your LookAndFeel and
looks perfect; in a host the same call returns a window that has never heard of
it, every custom `ColourId` fails to resolve, and the pop-up comes out styled
like another application.

That is verified rather than argued: the same editor and the same click, with
the wrapper and without, differ exactly where such a bug is — and with it, a
snapshot reproduces what the standalone shows. See
[popups](../references/popups.md#styling-a-pop-ups-contents) for the fix.

It was opt-in once, which is the failure mode worth remembering: the flag was
correct, documented in `--help`, and went unused, so the bug it exists to catch
shipped anyway. A safety net nobody remembers to switch on is not a safety net.

```bash
# Opt out, if you specifically want the editor as the root
snapshot.sh --target X -- --no-host-window
```

What no wrapper can emulate: there is still no window, no peer, and no host. The
editor is never `isShowing()`, which is why a `CallOutBox` logs a
`grabKeyboardFocus` assertion in every headless render, harmlessly.

**`--click` on anything that opens a `PopupMenu` segfaults.** Not your code:
`PopupMenu::getParentArea` dereferences the result of
`Desktop::getDisplays().getDisplayForPoint(...)` without a null check
(`juce_PopupMenu.cpp:920`), and a headless process has no displays, so the call
returns null. Any menu, from any control, in any project. `withParentComponent`
does not avoid it — the dereference happens before the parent is consulted.

So a menu cannot be captured, and clicking the control that opens one takes the
tool down with it. Snapshot the *closed* state, and check the menu itself by
running the standalone. `CallOutBox` is unaffected and does render, which is
worth knowing when choosing between the two for something you will want to look
at often.

## Gotchas

- **Build with `JUCE_MODAL_LOOPS_PERMITTED=1`.** The cmake helper sets this. The
  tool needs `MessageManager::runDispatchLoopUntil` to pump the message loop
  before painting, so timers and APVTS attachments have caught up. Without it
  the tool still runs but warns, and the image can disagree with the running
  plugin. Setting it on this dev-only target has no effect on the plugin.
- **OpenGL is not captured.** `createComponentSnapshot` does not see an attached
  `OpenGLContext`, and snapshotting one has caused crashes because the context
  renders on another thread. If part of the editor is OpenGL it will come out
  blank.
- **A zero-size editor is an error, not a blank PNG.** If the editor constructor
  does not call `setSize`, pass `--size <w>x<h>`.
- **`--scale 2.0` is the default**, matching a Retina display. A 740x430 editor
  produces a 1480x860 image.
- **stdout is only ever the path.** Build output, warnings and the image
  dimensions all go to stderr, so `PNG=$(snapshot.sh ...)` is safe.

## Pre-approving it in settings

To let the agent run this without a prompt, add to `.claude/settings.json`:

```json
{
  "permissions": {
    "allow": ["Bash(.claude/skills/juce-ui/scripts/snapshot.sh:*)"]
  }
}
```

The equivalent inside `SKILL.md` frontmatter, which grants only for the turn
that invokes the skill, is:

```yaml
allowed-tools: Bash(${CLAUDE_SKILL_DIR}/scripts/snapshot.sh *)
```

For that to match, the skill body must also refer to the script as
`${CLAUDE_SKILL_DIR}/scripts/snapshot.sh`; Claude Code substitutes the variable
in both places. Needs Claude Code 2.1.129 or later.

## If linking fails

`juce_gui_add_snapshot_tool` links the plugin's shared-code target, which is the
cheap path — no recompiling the processor. Some projects hit unresolved plug-in
wrapper symbols, because the shared-code target is built for a plug-in rather
than a console app. If that happens, compile the sources into the tool instead
of linking:

```cmake
target_sources(MyPlugin_snapshot PRIVATE
    plugin/PluginProcessor.cpp
    plugin/PluginEditor.cpp)
target_link_libraries(MyPlugin_snapshot PRIVATE
    juce::juce_audio_utils juce::juce_gui_extra)
```

## Adapting the snapshot tool to a GUI app

For a `juce_add_gui_app` project there is no processor or editor. Replace the
processor block in `main()` with a direct instance of the main component:

```cpp
MainComponent component;
component.setSize (800, 600);
settle (o.settleMs);
const auto image = component.createComponentSnapshot (component.getLocalBounds(), true, o.scale);
```

Everything else — output paths, pruning, message pumping — is unchanged.

# Layout lint

```bash
python3 .claude/skills/juce-ui/scripts/layout_lint.py plugin/
```

Needs no build and no JUCE. It finds every `resized()` body in the given files
or directories and checks the three Layout mechanics rules that can be verified
statically:

| rule | what it catches | why it matters |
|------|-----------------|----------------|
| 1 | constants declared inside `resized()` | the same measurement ends up defined twice, on two pages, and drifts apart |
| 2 | more than one `Grid` built in one `resized()` | the columns then exist only inside a row, so anything that must line up *between* rows is arithmetic again — the halves come out ragged |
| 3 | `getWidth() / n` dividing a region by an item count | integer truncation abandons 0..n-1 px at one edge, and the gap changes as the window resizes |
| 6 | unnamed numeric literals greater than 2 | an unnamed number cannot be found or reused, and gives no clue whether it was chosen or left over |

Only rule 6 sets the exit status, because rules 1, 2 and 3 have legitimate
exceptions. Rule 2's is a genuinely independent region — a floating panel laid
out in the same `resized()` as the page behind it — but "two rows of one page"
is not one of them. Use `--max <n>` to hold an existing codebase at its current count
while you bring it down, and `--quiet` for counts without per-line detail.

Suppress a deliberate case with a trailing comment on the same line:

```cpp
logo.setBounds (r.removeFromTop (48));   // layout-lint: allow
```

What it deliberately does not count: numbers inside comments and string
literals, hex constants, array subscripts, loop bounds in a `for` header, and
0, 1 and 2 anywhere.

It is a text scanner, not a compiler, so treat the output as a prompt to look
rather than a verdict. It finds a class or struct name for inline `resized()`
bodies by taking the nearest preceding declaration, which is right in ordinary
code and can mislabel a deeply nested one.

For reference, run against a real editor of ~3,200 lines:

```
rule 6  unnamed literals > 2 in resized():   156
rule 1  constants declared inside resized():  69
rule 3  regions divided by an item count:      6
```

That is what an unchecked layout looks like after a few months. Two of those
rule-3 hits are visible bugs.
