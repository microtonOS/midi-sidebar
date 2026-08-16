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
Some controllers send two CC messages per continuous controller to increase precision.
The CC number for the finetuning message can be set in LSB.
Note that not all numbers can be set.
If a CC is already used as an MSB, it cannot also be used as an LSB (the reverse is not true).
If so, both the MSB and the LSB are marked in red and both rows are ignored.

<!-- revert this, I think:
Selecting CC 7 for something else frees CC 39 with it, because an LSB alone refines nothing.
-->
Some CCs have special function.
There are three cases.
- Unavailable. These cannot carry a mapping under any setting.
The cell turns red and the row is ignored.
    - 98 to 101 = RPN and NRPN selection
    - 120 to 127 = channel mode messages
- Built in. These have a function the plugin performs itself, and each appears as its own row at the bottom of the table.
    - 0 and 32 = bank select
    - 7 and 39 = volume
    - 88 = velocity prefix
    
    The parameter column names the function until it is pointed at a parameter in the host plugin, and doing so is what makes the plugin give the function up.
    Everything on the row can be edited except the MSB, since the number is what the row is.
    They cannot be deleted; 'delete' reads 'reset' while one of them is selected, and restores the row to what it was.

> Volume here is the plugin's master volume rather than MIDI's per-channel one, which is why the row is called 'volume' and is marked as global.
> Master volume also has a system exclusive message of its own, and the two are meant to coexist: a device is required to keep three separate volume scalars — one for messages addressed to itself, one for the broadcast address, and one for channel messages such as CC 7 — and to multiply them, so that channels can be mixed against each other, then the device scaled, then everything faded together.
> Answering only one of the two would lose a layer of that.
- Data entry. These have no meaning of their own, and act on whatever RPN or NRPN was selected before them.
    - 6 and 38 = data entry
    - 96 and 97 = data increment and decrement
    They can be mapped, and while a registered parameter number the plugin recognises is in force they are read as data entry instead.
    A null RPN releases them.


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

MIDI 2.0 devices communicate parameter names directly rather than through CC messages.

[^korg]: [Korg. *Minilogue XD—Owner's Manual*.](https://www.korg.com/us/support/download/manual/0/811/4277/)

