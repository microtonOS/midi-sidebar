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

# Check the layout at an awkward window size
snapshot.sh --target MyPlugin_snapshot -- --size 640x360 --name narrow

# Keep a series instead of overwriting, to compare before and after
snapshot.sh --target MyPlugin_snapshot -- --timestamp
```

`--param` is the reason this tool does not need environment-variable hooks
baked into the editor. Drive the GUI through its parameters, and the shipped
code stays free of dev-only branches.

## Where the files go

By default `<system temp>/juce-ui-snapshots/`, via
`juce::File::getSpecialLocation (juce::File::tempDirectory)`. Under Claude Code
that resolves to the session temp directory, which is writable inside the
sandbox by default and is discarded with the session. On Linux and the RPi it
resolves to `$TMPDIR` or `/tmp`.

The tool prunes its own directory to the 20 most recent PNGs (`--keep`). It does
not rely on the OS sweeping temp files: macOS only removes files untouched for
three days, and Apple states that behaviour is not API.

Use `--dir` or `--out` to write elsewhere. Writing into the repository is
usually a mistake — the images are build output, not source.

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
| 3 | `getWidth() / n` dividing a region by an item count | integer truncation abandons 0..n-1 px at one edge, and the gap changes as the window resizes |
| 6 | unnamed numeric literals greater than 2 | an unnamed number cannot be found or reused, and gives no clue whether it was chosen or left over |

Only rule 6 sets the exit status, because rules 1 and 3 have legitimate
exceptions. Use `--max <n>` to hold an existing codebase at its current count
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
