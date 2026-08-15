# Controllers

In Controllers, the end-user can monitor MIDI messages (of any kind).
They can manually add control change (CC), (channel) aftertouch and polyphonic aftertouch (polytouch) as a complement to [MIDI learn](right-click.md).
Regardless of how they were added, properties like parameter, MIDI channel, and CC number can be edited.

![](figures/controllers-sorted.png)


<!-- TODO make sure these examples are followed in the monitor
The monitor shows the last three MIDI messages, e.g.:
- ch 1 CC 80 value 101
- ch 3 pitchbend 2003
- sysex bulk tuning dump
-->
<!-- more examples:
- ch 2 note on 60 velocity 127
- ch 16 polytouch 69 value 50
- ch 15 PC 19
- ch 14 aftertouch 120
-->


The edit section contains a table with columns 'param' (parameter), 'ch' (channel), 'MSB' and 'LSB' (most significant byte and, optionally, least significant byte—two CC messages), 'mode', 'min', and 'max'.
Each column can be ordered alphabetically/numerically or in the reverse order.
By default, no column is ordered which means that rows appear with the latest added entries on top.

The parameter (param) column is always shown.
This is the name of the parameter in the host plugin.
When clicked a menu appears with all available parameters.
Unless, it is a simple host plugin, the parameters are ordered in categories in the menu.

Next is the channel (ch) column.
A channel can be set between 1 and 16.
For omni settings and MPE, see the [Channel](channels.md) menu.

The MSB column is where the CC number, between 0 and 127, is added.
Some controllers send two CC messages per continuous controller to increase precision.
The CC number for the finetuning message can be set in LSB.
Note that not all numbers can be set.
If a CC is already used as an MSB, it cannot also be used as an LSB (the reverse is not true).
Furthermore, the following CC numbers have protected functions:

- 0 and 32 = bank select
- 6 and 38 = data entry
- 7 and 39 = volume
- 88 = high resolution velocity prefix
- 96 to 101 = RPN and NRPN-related messages
- 120 to 127 = channel mode messages

When a disallowed MSB or LSB has been selected, the corresponding table cell is red, and the entire row is ignored.
<!-- In the JUCE dialogs demo, the alert window with warning icon has an icon with a red colour. Not sure if it is part of the look and feel theme, but if it is, use that colour. -->

![](figures/controllers-table-full.png)

Modes:[^korg]
- Jump: When you turn the knob, the parameter value will jump to the value indicated by the knob.
Since this makes it easy to hear the results while editing, we recommend that you use this setting.
- Catch: Turning the knob will not change the parameter value until the knob position matches the
stored value. We recommend that you use this setting when you don’t want the sound to change
abruptly, such as while performing.
- Scale: When you turn the knob, the parameter value will increase or decrease in a relative manner in the direction that it is turned. When you turn the knob and it reaches the full extent of its
motion, it will operate proportionate to the maximum or minimum value of the parameter. Once
the knob position matches the parameter value, the knob position and parameter value will subsequently be linked.


Two more options ignore LSB (LSB values can still be set though):
- Toggle: Whenever a controller emits a value at least 64, the toggle switches. min and max can be swapped for a polarity change.
- Inc(rement): Whenever the controller emits a value of at least 64, it is interpreted as going from CC value x to x+1 (at most 127). min and max can be swapped for decrement.

The 'min' and 'max' columns are for increasing the minimum or decreasing the maximum values the parameter can take.
If 'max' is less than 'min', then the polarity is changed.

The default messages added in the insert section:
- Parameter is the one appearing first in alphabetical order.
- Channel is 1.
- MSB is 0, aftertouch, or polytouch.
- LSB is empty.
- Mode is jump.
- Minimum is the lowest value possible
- Maximum is the highest.


To save a MIDI mapping, the end-user should save the state of the plugin. JUCE allows to do this and many hosts and DAWs do too.
That way other data such as channels, programs, and banks is also saved.


[^korg]: [Korg. *Minilogue XD—Owner's Manual*.](https://www.korg.com/us/support/download/manual/0/811/4277/)




<!-- 

> **Status.** The GUI below is built; see [What is built](#what-is-built) at the
> end. Nothing behind it is — no MIDI is read, and none of the five modes does
> anything yet. They are choices the table can express. -->