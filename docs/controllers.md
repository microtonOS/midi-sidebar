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
Parameters marked with <svg xmlns="http://www.w3.org/2000/svg" width="1em" height="1em" viewBox="0 0 256 256">
	<path d="M0 0h256v256H0z" fill="none" />
	<path fill="currentColor" fill-rule="evenodd" d="M24.22 67.796a3.995 3.995 0 0 1 4.008-3.991h85.498c8.834 0 19.732 6.112 24.345 13.657l53.76 87.936c3.46 5.66 11.628 10.247 18.256 10.247h16.718a3.996 3.996 0 0 1 3.994 4.007v8.985a4.007 4.007 0 0 1-4.007 4.008h-24.7c-8.835 0-19.709-6.13-24.283-13.683l-52.324-86.4c-3.43-5.665-11.577-10.257-18.202-10.257H28.214a3.995 3.995 0 0 1-3.993-3.992V67.796z" />
</svg>
only affect the lower-frequencies split.
Parameters marked with
<svg xmlns="http://www.w3.org/2000/svg" width="1em" height="1em" viewBox="0 0 256 256">
	<path d="M0 0h256v256H0z" fill="none" />
	<path fill="currentColor" fill-rule="evenodd" d="M231.007 68.729c0-2.206-1.787-4.995-4.007-4.995h-85.499c-6.466 0-19.531 7.705-22.66 15.97l-55.92 85.647c-3.624 5.55-11.93 10.05-18.559 10.05H28.167c-2.206 0-3.994 2.787-3.994 5.007v8.985a4.005 4.005 0 0 0 3.998 4.007h22.713c8.832 0 20.495-8.703 23.588-16.987l56.167-84.189c3.68-5.517 12.04-9.99 18.668-9.99h77.695c2.212 0 4.005-2.797 4.005-4.994v-8.51z" />
</svg>
only affect the higher-frequencies split.
See [Presets](presets.md).
Unmarked affect the entire host plugin.
<!-- remove this:
Parameters marked with
<svg xmlns="http://www.w3.org/2000/svg" width="1em" height="1em" viewBox="0 0 48 48">
	<path d="M0 0h48v48H0z" fill="none" />
	<path fill="none" stroke="currentColor" stroke-linecap="round" stroke-linejoin="round" d="M24 2.5v43c-6.573 0-11.902-9.626-11.902-21.5S17.427 2.5 24 2.5S35.902 12.126 35.902 24S30.573 45.5 24 45.5M45.5 24h-43m40.12-10.75H5.38m37.24 21.5H5.38" />
	<path fill="none" stroke="currentColor" stroke-linecap="round" stroke-linejoin="round" d="M2.5 24c0 11.874 9.626 21.5 21.5 21.5S45.5 35.874 45.5 24S35.874 2.5 24 2.5S2.5 12.126 2.5 24" />
</svg>
affect the entire plugin.
Unmarked affect the entire keyboard split.
-->



Next is the channel (ch) column.
A channel can be set between 1 and 16.
For omni settings and MPE, see the [Channels](channels.md) page.

The MSB column is where the CC number, between 0 and 127, is added.
Rows reading 'aftertouch' or 'polytouch' across those two columns have no number.
Some controllers send two CC messages per continuous controller to increase precision.

The CC number for the finetuning message can be set in LSB.
MIDI Sidebar follows the official specification in that CCs 0 to 31 are paired with CCs 32 to 63 respectively, so that CC i is the MSB and CC i+32 is the LSB.
A new MSB resets the paired LSB.
MIDI Sidebar extends the official specification by allowing the end-user to define arbitrary MSB–LSB pairs.
One LSB can only be paired with one other MSB within the same channel though, and an LSB cannot also be an MSB.
In [MIDI learn](right-click.md),
only pairs according to the official specification are learned.[^highPrecision]
<!-- I believe this is not quite the current implementation, fix that! -->

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

[^highPrecision]: Some devices may use one LSB for several MSBs. This is not supported. Some devices may send an LSB before the MSB and this will have no effect on MIDI sidebar.
[^korg]: [Korg. *Minilogue XD—Owner's Manual*.](https://www.korg.com/us/support/download/manual/0/811/4277/)

