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
The lower zone has 1 as the manager channel and the upper zone has 16.
It is also possible to have two MPE zones with both 1 and 16 as distinct manager channels.
If only the master channel is selcted for a zone, that zone is considered inactive for MPE (and uses the omni settings).
For the remaining member channels, CCs, pitchbend, and channel aftertouch are interpreted as omni off.
Polyphonic aftertouch is ignored unless on a manager channel.
A zone cannot contain gaps.

Omni is set by channel mode messages—CC 124 omni off, CC 125 omni on—on the
receiver's basic channel. An MPE zone is set by the MPE Configuration Message,
which is RPN 0/6: CC 101 value 0 followed by CC 100 value 6 selects it, CC 6
gives the number of member channels, and the channel it is sent to (1 or 16)
chooses the lower or upper zone.
In MIDI Sidebar, only one MPE zone can be active at any time.

Toggling the 'pitchbend sensitivity' knob allows the end-user to set pitchbend settings.
When 'omni' is selected, the end-user can edit the pitchbend of each channel individually by pressing the associated channel.
A popup appears where the end-user can type a value in cents.
When 'MPE' is selected, the end-user can only edit four pitchbend sensitivities: lower zone manager, lower zone member, upper zone manager, and upper zone member.
MPE pitchbend sensitivites are only applied when MPE is on.
RPN 0 can also be used to set pitchbend sensitivities.


MIDI 2.0 messages are kept apart from MIDI 1.0 messages such that no channel clashes occur.