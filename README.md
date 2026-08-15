# MIDI Sidebar
A sidebar widget for JUCE managing microtunings, program changes, and continuous controllers. (Volume and all notes/sound off are extras.)

## Introduction
*Throughout the documentation, we distinguish between the* developer *of the plugin and the* end-user*.*

<!-- TODO: crop image -->
![upper buttons of the sidebar when collapsed.](docs/figures/rail.png)

MIDI Sidebar is a custom component for JUCE that can be easily added to a JUCE plugin.
Sidebar merges the `juce::SidePanel` and `juce::ToolBar` classes into a new independent `Sidebar` class.
In addition, it adds functionality:
- A presets page lets the end-user save and load programs and banks. A preset is a collection of synthesizer parameter values, and the sidebar allows doubling of the parameters for bitimbral functionality with keyboard split. Presets can also be managed by program change and bank select messages (CC0 MSB and CC32 LSB). See [Presets](docs/presets.md).
- A controllers page lets the end-user see what controller controls what parameter. It handles CCs and (channel and polyphonic) aftertouch. You can edit this manually or via MIDI learn.
MIDI learn is accessed through right-clicking.
See [Controllers](docs/controllers.md) and [Right-Click](docs/right-click.md).
- A tuning page lets you turn the plugin into a microtuning client accepting MTS ESP, MTS Sysex, MIDI 2.0, and Scala files. In addition, the pitchbend sensitivity can be set. *Multichannel* tuning is supported! See [Tuning](docs/tuning.md).
- A channels page lets the end-user control MPE and standard MIDI settings.
Channels can be muted, set to omni on/off, or used for an MPE zone.
Channel settings override settings from the presets, controllers, and tuning pages.
See [Channels](docs/channels.md).

In addition, MIDI Sidebar adds simple utilities:
- Volume fader (hardcoded to CC7 MSB and CC29 LSB) and a parallel stereo meter. The meter is post-fader and the two share a common dB scale.
- All sound off button (hardcoded to CC120).
Also turns all notes off (CC123).

<!-- TODO: crop image and add -->
![Volume and allsounds off button.]()

Developers can edit the look and feel of MIDI Sidebar as well as various other settings such as left or right placement.
These are visible in the accompanying demo host plugin, which should be replaced by the proper host.
See [Demo](docs/demo.md).

For further details, see [Appendices](docs/appendices.md)

## Motivation
Sidebar makes it easy to set up automation such that you can run a microtunable plugin headlessly.
Settings made in the GUI can be saved and loaded through the CLI. In addition it is a useful tool for developing and debugging microtonal plugins.

## Roadmap
Future work may include tempo-related settings as well.

At the moment Linux/Raspberry Pi and MacOS are supported, but future work may include support for Windows and mobile devices.