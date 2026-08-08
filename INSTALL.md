# Building and running

## Requirements

| | |
|---|---|
| CMake | 3.22 or newer |
| Compiler | anything with C++17 (tested on Apple clang 21) |
| JUCE | 9.0.0, at `~/JUCE` by default |

JUCE is **not** vendored in this repository. Sidebar is a JUCE module, so a
project that consumes it will already have brought JUCE in, and adding a second
copy would produce duplicate targets. The top-level `CMakeLists.txt` only pulls
JUCE in when this project is built on its own:

```cmake
if(NOT TARGET juce::juce_gui_basics)
    ...
endif()
```

If your JUCE lives somewhere else, point at it:

```bash
cmake -B build -DJUCE_SIDEBAR_JUCE_DIR=/path/to/JUCE
```

## Build

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build
```

The first configure takes a minute or two because JUCE builds its `juceaide`
helper. Later builds are incremental.

Targets:

| target | what it is |
|---|---|
| `midi_sidebar` | the module itself — a library, nothing to run |
| `SidebarDemo` | the demo's shared code |
| `SidebarDemo_Standalone` | the runnable app |
| `SidebarDemo_snapshot` | headless PNG renderer, see below |

Note that `--target SidebarDemo` builds only the shared code and leaves you with
an empty `.app`. Use the default target, or `--target SidebarDemo_Standalone`.

## Run

```bash
open "build/demo/SidebarDemo_artefacts/Debug/Standalone/Sidebar Demo.app"
```

The demo is a pass-through plugin whose only purpose is to host the sidebar; the
outlined area beside it marks where a real plugin's UI would go.

Resize the window vertically. The arrangement is the same at every height —
pages at the top, volume and all-sound-off at the bottom, one flexible gap
between them — and only the volume control changes:

| window height | volume control |
|---|---|
| below 272 | an icon button; pressing it opens the fader and meter in a pop-up |
| 272 and up | the fader and meter inline in the rail |

Minimum height is 212, the point at which the five controls and their gaps
exactly fit. Both numbers are *derived* from the control sizes rather than
chosen, so changing a button size or a gap moves them — see
[docs/sidebar.md](docs/sidebar.md).

## Screenshot it without opening a window

`SidebarDemo_snapshot` renders the editor to a PNG in software: no window, no
audio device, nothing read off the screen. It works over SSH and inside a
sandbox.

```bash
# builds the target if needed, prints the PNG path as the last line of stdout
.claude/skills/juce-ui/scripts/snapshot.sh --build-dir build --target SidebarDemo_snapshot

# a specific size and name
.claude/skills/juce-ui/scripts/snapshot.sh --build-dir build --target SidebarDemo_snapshot \
    -- --size 368x420 --name tall

# with a page open
.claude/skills/juce-ui/scripts/snapshot.sh --build-dir build --target SidebarDemo_snapshot \
    -- --size 628x420 --name expanded --param page=Tuning

# what can be set?
.claude/skills/juce-ui/scripts/snapshot.sh --build-dir build --target SidebarDemo_snapshot \
    -- --list-params

# click something first, e.g. the volume pop-up at a height where it is a button
.claude/skills/juce-ui/scripts/snapshot.sh --build-dir build --target SidebarDemo_snapshot \
    -- --size 628x240 --click Volume --name volume-popup
```

Images land in `<system temp>/juce-ui-snapshots/` and the directory is pruned to
the twenty most recent. Full options in
[.claude/skills/juce-ui/scripts/README.md](.claude/skills/juce-ui/scripts/README.md).

## Check the layout rules

```bash
python3 .claude/skills/juce-ui/scripts/layout_lint.py modules/ demo/
```

Needs no build and no JUCE. It fails if any `resized()` contains an unnamed
numeric literal greater than 2, and reports constants declared inside `resized()`
and regions divided by an item count. All three should stay at zero.

## Using the module in your own plugin

```cmake
add_subdirectory(path/to/midi-sidebar)

target_link_libraries(MyPlugin PRIVATE microtonos::midi_sidebar)
```

`add_subdirectory` is enough: the module registers itself, the demo target is
skipped because your project is the top-level one, and your existing JUCE is
used rather than a second copy.

Then:

```cpp
#include <midi_sidebar/midi_sidebar.h>

// An alias rather than `using namespace`: the module exports Sidebar,
// LevelMeter, VolumeStrip, SidebarPanel and SidebarLookAndFeel, and at least
// the first three are names a plugin may well already have of its own.
namespace msb = microtonos::sidebar;

class MyEditor : public juce::AudioProcessorEditor
{
    msb::SidebarLookAndFeel lookAndFeel;   // or call
                                           // SidebarLookAndFeel::registerColours()
                                           // on your own LookAndFeel
    msb::Sidebar sidebar;
};
```

Give the sidebar the full height of your editor and a width of
`sidebar.getPreferredWidth()`, and re-lay out when `onPreferredWidthChanged`
fires — `demo/DemoEditor.cpp` is the worked example. Use
`Sidebar::getMinimumHeight()` for your editor's minimum rather than picking a
number.

**Your own layout does not have to react to the sidebar opening.** The panel
lies over your UI rather than pushing it aside, so reserve only
`Sidebar::getRailWidth()` at the chosen edge and lay your controls out in what
is left, once. Add the sidebar to your editor *after* your own components so it
draws on top. What the panel covers while it is open is hidden deliberately;
only the rail is permanently reserved.

If you supply your own `LookAndFeel` rather than `SidebarLookAndFeel`, call
`SidebarLookAndFeel::registerColours (yourLookAndFeel, yourScheme)`. Without it
the sidebar's `ColourId`s are unknown, and JUCE answers colour lookups with
black.

## Known noise

Building on macOS under a sandbox prints one of these per compiler invocation:

```
c++: error: couldn't create cache file '/var/folders/.../xcrun_db-XXXXXXXX' (errno=Operation not permitted)
```

`xcrun` cannot write its tool cache, which lives in the Darwin per-user temp
directory and is located by syscall rather than `$TMPDIR`. It is harmless — the
build succeeds — but it is printed as `error:`, so filter it when scanning
output:

```bash
cmake --build build 2>&1 | grep -v "cache file"
```
