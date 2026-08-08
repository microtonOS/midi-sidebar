# MIDI Sidebar
A sidebar widget for JUCE managing microtunings, program changes, and continuous controllers. (Volume and all notes/sound off are extras.)

## Introduction
This Sidebar is a custom component for JUCE that can be easily added to a JUCE plugin.
Sidebar merges the `juce::SidePanel` and `juce::ToolBar` classes into a new independent `Sidebar` class.
In addition, it adds functionality:
- A presets menu lets you save and load programs and banks. Presets can also be managed by program change and bank select messages (CC0 MSB and CC32 LSB). See [Presets](docs/presets.md).
- A continuous controller menu lets you see what controller controls what parameter. You can edit this manually or via MIDI learn. The overview is in the sidebar and individual parameters can be edited through right-click. See [Right-Click](docs/right-click.md).
- A tuning menu lets you turn the plugin into a microtuning client accepting MIDI 2.0, MTS ESP, MTS Sysex, MPE, and Scala files. See [Tuning](docs/tuning.md).

In addition, it adds simple utilities:
- Volume slider (hardcoded to CC7 MSB and CC29 LSB) and a parallel VU meter.
- All sound off button (hardcoded to CC120).

Throughout the documentation, we distinguish between the *developer* of the plugin and the *end-user*.

For more, see [Sidebar](docs/sidebar.md).

## Motivation
Sidebar makes it easy to set up automation such that you can run a microtunable plugin headlessly.
Settings made in the GUI can be saved and loaded through the CLI. In addition it is a useful tool for developing and debugging microtonal plugins.

## Roadmap
Future work may include tempo-related settings as well.

At the moment Linux/Raspberry Pi and MacOS are supported, but future work may include support for Windows and mobile devices.