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
This means that channel 1 or 16 is a manager channel.
For the remaining member channels, CCs, pitchbend, and channel aftertouch are interpreted as omni off.
Polyphonic aftertouch is ignored unless on a manager channel.
The lower zone has 1 as the manager channel and the upper zone has 16.
A zone cannot contain gaps.

Omni is set by channel mode messages — CC 124 omni off, CC 125 omni on — on the
receiver's basic channel. An MPE zone is set by the MPE Configuration Message,
which is RPN 0/6: CC 101 value 0 followed by CC 100 value 6 selects it, CC 6
gives the number of member channels, and the channel it is sent to (1 or 16)
chooses the lower or upper zone.

> Receiving one updates this page: the channel it arrived on selects the zone, and its member count sets how far the zone reaches, so the matrix redraws to show the new layout.
> It is read even on a channel the filter is muting, since a plugin set to one zone is usually not listening to the other's manager channel and could otherwise never be reconfigured onto it.
> A count of zero switches MPE off while keeping the edge, so turning it back on restores what was there.

> Only one zone is active at a time here, and an incoming message for the other zone moves the zone rather than adding a second.
> The MPE specification does allow both at once, with any channels left over "available for conventional use" — but it also says many devices support one zone and may let the user choose which, which is what this page does.
> Two zones at once is a low-priority item; what it would add is the second device's manager channel, whose pitchbend and aftertouch apply to all of its members.

MIDI 2.0 messages are kept apart from MIDI 1.0 messages such that no channel clashes occur.