# Controllers

In Controllers, the end-user can monitor MIDI messages (of any kind).
They can manually add control change (CC), (channel) aftertouch and polyphonic aftertouch (polytouch) as a complement to [MIDI learn](right-click.md).
Regardless of how they were added, properties like parameter, MIDI channel, and CC number can be edited.

![](figures/controllers-sorted.png)

The monitor shows the last three MIDI messages, for example:

- ch 1 CC 80 value 101
- ch 3 pitchbend 2003
- Sysex bulk tuning dump
- ch 2 note on 60 velocity 127
- ch 16 polytouch 69 value 50
- ch 15 PC 19
- ch 14 aftertouch 120

The edit section contains a table with columns 'param' (parameter), 'ch' (channel), 'MSB' and 'LSB' (most significant byte and, optionally, least significant byte—two CC messages), 'mode', 'min', and 'max'.
Each column can be ordered alphabetically/numerically or in the reverse order.
By default, no column is ordered which means that rows appear with the latest added entries on top.

The parameter (param) column is always shown.
This is the name of the parameter in the host plugin.
When clicked a menu appears with all available parameters.
Unless, it is a simple host plugin, the parameters are ordered in categories in the menu.
Parameters that can modulate individual notes are marked with
<svg xmlns="http://www.w3.org/2000/svg" width="1em" height="1em" viewBox="0 0 48 48">
	<path d="M0 0h48v48H0z" fill="none" />
	<path fill="none" stroke="currentColor" stroke-linecap="round" stroke-linejoin="round" d="M42.5 5.795h-4.747l.301 22.711m-8.83-12.992h-4.747l.301 22.712m-6.88-32.431h-4.746l.3 22.711" />
	<path fill="none" stroke="currentColor" stroke-linecap="round" stroke-linejoin="round" d="M5.5 28.506a3.977 3.977 0 1 0 7.953.004v-.004a3.977 3.977 0 1 0-7.953-.004zm11.326 9.72a3.977 3.977 0 1 0 7.952.005v-.005a3.977 3.977 0 1 0-7.952-.002zm13.276-9.72a3.977 3.977 0 1 0 7.953.004v-.004a3.977 3.977 0 1 0-7.953-.004z" />
</svg>
to the right.
Parameters marked with
<svg xmlns="http://www.w3.org/2000/svg" width="1em" height="1em" viewBox="0 0 48 48">
	<path d="M0 0h48v48H0z" fill="none" />
	<path fill="none" stroke="currentColor" stroke-linecap="round" stroke-linejoin="round" d="M24 2.5v43c-6.573 0-11.902-9.626-11.902-21.5S17.427 2.5 24 2.5S35.902 12.126 35.902 24S30.573 45.5 24 45.5M45.5 24h-43m40.12-10.75H5.38m37.24 21.5H5.38" />
	<path fill="none" stroke="currentColor" stroke-linecap="round" stroke-linejoin="round" d="M2.5 24c0 11.874 9.626 21.5 21.5 21.5S45.5 35.874 45.5 24S35.874 2.5 24 2.5S2.5 12.126 2.5 24" />
</svg>
affect the entire plugin.
Unmarked affect the entire keyboard split.



Next is the channel (ch) column.
A channel can be set between 1 and 16.
For omni settings and MPE, see the [Channels](channels.md) page.

The MSB column is where the CC number, between 0 and 127, is added.
Rows reading 'aftertouch' or 'polytouch' across those two columns have no number.
Some controllers send two CC messages per continuous controller to increase precision.
The CC number for the finetuning message can be set in LSB.

An LSB refines the MSB that came before it, and a new MSB resets the LSB to zero, as in the MIDI spec.
Because the two write to different places, order does not otherwise matter—only the reset makes it matter at all.

> The MIDI spec pairs CC *n* with CC *n*+32 and allows nothing else.
> Here any available number can be the LSB for any MSB, so those assignments are taken as a suggestion rather than a rule, and the table is where the truth is—it is also the one place you can see which controllers have a fine byte and which message carries it.
> What is kept from the spec is the behaviour above.
> A device that sends its low byte *first* cannot work under it: the MSB that follows wipes the LSB every time.

If a CC is already used as an MSB, it cannot also be used as an LSB (the reverse is not true).
If so, both the MSB and the LSB are marked in red and both rows are ignored.
The same MSB may be used by several rows, which is how one controller drives two parameters.

Note that not all numbers can be set.
There are two cases.
- Unavailable. These cannot carry a mapping under any setting.
The cell turns red and the row is ignored.
    - 0 and 32 = bank select, which the plugin performs itself
    - 98 to 101 = RPN and NRPN selection
    - 120 to 127 = channel mode messages
- Data entry. These have no meaning of their own, and act on whatever RPN or NRPN was selected before them.
    - 6 and 38 = data entry
    - 96 and 97 = data increment and decrement
    They can be mapped, and while a registered parameter number the plugin recognises is in force they are read as data entry instead.
    A null RPN releases them.

Everything else is free, including 7, 39 and 88.
Those three once had built-in functions and rows of their own at the bottom of the table; they no longer do.
The plugin's volume is set by a system exclusive message instead, and high-resolution velocity is a parameter like any other, so a controller aimed at either is an ordinary mapping.

> Volume here is the plugin's master volume rather than MIDI's per-channel one.
> It is set by the Universal Real Time Device Control message, `F0 7F 7F 04 01 vv vv F7`, whose square-law curve the fader shares.
> Two more of that family are read and shown in the monitor but belong to the [tuning](tuning.md) page: Master Fine Tuning (`04 03`) and Master Coarse Tuning (`04 04`).
> Only the broadcast address `7F` is answered: a plugin has no device ID of its own, and answering every ID would have two instances in one session fight over the same message.
> The message is passed on rather than swallowed, since a broadcast is addressed to everything downstream as well.


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
- MSB is empty, aftertouch, or polytouch.
- LSB is empty.
- Mode is jump.
- Minimum is the lowest value possible
- Maximum is the highest.

To save a MIDI mapping, the end-user should save the state of the plugin. JUCE allows to do this and many hosts and DAWs do too.
That way other data such as channels, programs, and banks is also saved.

MIDI 2.0 devices communicate parameter names directly rather than through CC messages.

[^korg]: [Korg. *Minilogue XD—Owner's Manual*.](https://www.korg.com/us/support/download/manual/0/811/4277/)

