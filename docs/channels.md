# Channels

In Channels, the end-user can manage channel settings for notes (on and off), program changes, control changes and (channel and polyphonic) aftertouch, and pitchbend.

![](figures/channels-omni.png)

When omni is set to on, the plugin reads messages from multiple channels while ignoring the channel number itself.
This setting overrides the channels set in [Controllers](controllers.md) (or via [MIDI learn](right-click.md)).
Tunings are read from the unspecified channel.
When off, each message only affects its corresponding channel as far as is possible.
Not all parameters can be modulated on a per-channel basis.
Note that program change messages are always interpreted as omni.

The plugin listens to channels marked in the filter section.
Messages and tunings on other channels are ignored.
Use cases. Allow the muted channels to be used for a different plugin or a different instance of this plugin. Reduce the size of the tuning table.
The select all and mute all are convenience buttons to reduce the number of clicks.

![](figures/channels-mpe.png)

When MPE is set to on, selected channels override the settings omni.
Selected channels follow the [MPE specification](https://midi.org/mpe-midi-polyphonic-expression).
The lower zone has 1 as the master channel and the upper zone has 16.
A zone cannot contain gaps.

Channels 1 to 16 are reserved for devices without MIDI 2.0 compatibility.
MIDI 2.0 devices use the extended channels instead.