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

> **Both zones can now be active at once**, which is what the MPE specification
> describes. The lower/upper buttons still choose which zone the matrix shows —
> drawing both together would lose the thing the matrix is for, since with every
> channel lit there is no telling one big zone from two abutting ones, nor where
> the lower ends and the upper begins. What the buttons no longer do is *move* a
> single zone between the two ends: the zone you are not looking at keeps its
> channels. Clicking a channel sets
> that zone's edge, so clicking a zone's own manager channel leaves it with no
> member channels and thereby deactivates it — §2.2.2, "If a Zone no longer has
> any Member Channels, then it shall become deactivated".
>
> Where the two zones would overlap, the zone just edited keeps the channels and
> the other yields them, which is §2.2.1's rule that the most recent MPE
> Configuration Message takes precedence. A received MCM and a click go through
> the same code, so the display cannot disagree with the wire. Channels neither
> zone has claimed are available for conventional use.
>
> **Pitchbend sensitivity** is set here rather than on the tuning page. The
> `pitchbend sensitivity` button to the right of the two switches is a latch:
> while it is on, clicking a channel opens its range instead of selecting it.
> Under omni that is one channel; under MPE, clicking a member channel sets
> every member of that zone together, while a manager channel is set on its own.
> A channel in neither zone opens nothing at all: it is not a manager and not a
> member, so the MPE view has no range for it, and its plain range belongs to
> the omni view.
>
> `select all` and `mute all` are disabled while the latch is on, since a click
> on the matrix no longer selects anything. The two zone buttons stay live —
> they choose which zone is in view, and in the lower zone you edit the lower
> zone's ranges and nothing else, so switching zones is how the other zone's
> ranges are reached at all.
>
> The range is in **cents**, not semitones. RPN 0 carries semitones in the Data
> Entry MSB and cents in the LSB, so this is what the message can already say;
> General MIDI 2 §3.4.1 lets a *receiver* ignore the LSB, which is why most
> synths look semitone-only, but that is their conformance floor and not a limit
> worth inheriting.

Omni is set by channel mode messages—CC 124 omni off, CC 125 omni on—on the
receiver's basic channel. An MPE zone is set by the MPE Configuration Message,
which is RPN 0/6: CC 101 value 0 followed by CC 100 value 6 selects it, CC 6
gives the number of member channels, and the channel it is sent to (1 or 16)
chooses the lower or upper zone.
In MIDI Sidebar, only one MPE zone can be active at any time.

MIDI 2.0 messages are kept apart from MIDI 1.0 messages such that no channel clashes occur.