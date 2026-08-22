# Tuning

In Tuning, the end-user can monitor tunings of individual notes, see the name, program number, and bank number, as well as the period.
The end-user can edit which tuning standard to use.
~~The end-user can choose between MTS ESP, MTS Sysex, tuning files, or standard.~~
> The end-user can choose between MTS-ESP, MIDI 1.0, MIDI 2.0, Scala, or standard.
> These name where the tuning comes from, which is what actually separates them: an inter-process master, the MIDI stream, a file, or nothing.
> 'MIDI 1.0' covers MTS system exclusive and the tuning RPNs together — the RPNs are MTS but are not system exclusive, so the old name was too narrow — and it is also where master and channel tuning are read.
The user can set associated parameters as well as pitchbend sensitivity.
> Under MIDI 1.0 the overall pitch is set by master and channel tuning messages rather than here; see the settings section.

MIDI Sidebar saves a table of frequencies per note per channel.
In addition, there is a list frequencies per note for an *unspecified channel*.
There can be multiple table+list pairs arranged in tuning programs and tuning banks.
The precision is at least 0.0061 c—the limit for MTS Sysex.[^precision]

![](figures/tuning.png)

A value in cents (two decimals) indicating the interval between the lowest and highest currently active notes.
0 c if a single note and 'all notes off' if none.
For large values a modulo over 1200 is handy to quickly identify the interval.
1200 is the default value but can be edited.
(The post-modulo indicator is empty if all notes off.)

The status section shows the name of the tuning.
~~Not all tuning standards allow naming (MTS Sysex has only partial support) and, if so, it says 'no name' (standard is '12edo A4=440 Hz').~~
> Not all tuning standards allow naming (MTS Sysex has only partial support).
> Where there is no name, '12edo' is shown — the fallback for every standard, MTS ESP included, since a plugin with no tuning and a master with no scale name are both playing equal temperament.
> The reference pitch is not part of that name: master and channel tuning displace the whole instrument from A440, and the presets page shows the frequencies actually sounding.
~~For tuning standards that allow tuning programs (and tuning banks) are MTS Sysex and tuning files.~~
> The tuning standards that allow tuning programs (and tuning banks) are MIDI 1.0 and Scala.
Tunings files can be arranged in a directory to form a bank, and several such directories can be opened together.
For these tuning standards the name is clickable and other tuning programs (and banks) are selectable.
If so, tuning programs and banks can also be explored numerically.
Tuning program and bank also respond to registered parameter numbers. Tuning Program Select is RPN 0/3 (CC 101 value 0 followed by CC 100 value 3, CC 6 set the value while CCs 96 and 97 step it) and Tuning Bank Select is RPN 0/4 (4 instead of 3).
There are 128 tuning banks at most.


A time stamp when the tuning was last updated is useful for seeing whether the plugin is connected to a tuning master of one sort or another.

The period section shows the period of a tuning.
For an equal division tuning, it is trivial—the step between two notes count as one period.
So does the distance between three, and four, and so on.
Therefore, the period indicator can be incremented or decremented among acceptable choices.
For tuning with uneven step sizes that are nonetheless arranged in a pattern, e.g. repeating every 12th note, the period is the interval across the pattern, e.g. 1200 c.
MTS ESP and some tuning files can specify the period.
If they do, 'specified' is indicated (and the period cannot be incremented/decremented).
Otherwise, the period is 'inferred'.
Period inference merges all the channels and sorts the frequencies.
~~By default the smallest possible period is shown.~~
> By default whichever acceptable period is closest to an octave is shown.
> For an equal division of the octave every step size is a period, so this shows the octave itself.
> Where a scale divides something else — 13 equal divisions of 3/1, say — the frequencies alone do not say which multiple was meant, so this is a guess and the octave is simply the easier rule. It is a default, editable here, and affects nothing that sounds.
> Distance is measured in cents, so the octave below is nearer than the octave above: a period is more usefully small than large.
> A tuning that states its own period is not inferred at all — a Scala file gives its last tone and MTS ESP reports one — and shows 'specified'.
If no period is found the entire set of frequencies is taken as the period.
The precision is the same as MTS Sysex (0.0061 c)
Use cases:
A tonewheel organ has its drawbars tuned according to an underlying scale. To get the correct pitches for the higher notes, a period has to be inferred.
Similar ideas could be applied to any synthesizer with numerous oscillators.

In the settings page, the end-user sets up what tuning standard to use.
The name of the tuning standard is selected from a menu.
~~MTS ESP is used by default and then the plugin acts as an MTS ESP client and ignores other tuning data.
If MTS Sysex is selected the plugin listens to the relevant Sysex messages but ignores the MTS ESP master.
If tuning files or standard is selected data of either kind is ignored.~~
> MTS-ESP is used by default and then the plugin acts as an MTS-ESP client and ignores other tuning data.
> If MIDI 1.0 is selected the plugin listens to the relevant messages but ignores the MTS-ESP master.
> If Scala or standard is selected data of either kind is ignored.
> MIDI 2.0 is not implemented yet and behaves as standard.

> Under MIDI 1.0 the overall pitch is set separately from the scale, by four messages that all displace the instrument from A440 and are added together.
> Master Fine Tuning and Master Coarse Tuning are system exclusive and apply to the whole plugin; Channel Fine Tuning (RPN 1) and Channel Coarse Tuning (RPN 2) apply to one channel.
> Fine covers ±100 c in steps of about 0.0122 c, coarse covers −64 to +63 semitones.
> They shift pitch without transposing note numbers, so a key-based instrument keeps its sounds.

> These apply **only** under MIDI 1.0 — the standard that reads its tuning from the MIDI stream, which is where these messages also arrive. Each of the other three is left alone for its own reason.
> Under MTS-ESP the master is the authority on absolute pitch and every other client is asking that same master, so displacing our copy would put the plugin out of tune with all of them; a user wanting A=442 sets it on the master instead.
> A tuning file states its own reference — a `.kbm` gives a reference note and its frequency — so a displacement would override what the file said.
> And 'standard' means 12edo at A440, which a displacement would make untrue.

~~The 'open files' button is active for the tuning files.
(It can also be used in MTS Sysex for `.syx` files.)~~
> The 'open files' button is active for Scala.
> (It can also be used in MIDI 1.0 for `.syx` files.)
A single `.scl` file sets the tuning for the unspecified channel.
The end-user can select one `.scl` file and one or several `.kbm` files at the same time.
The suffix `_i.kbm` is the mapping for the ith channel.
Selecting one directory creates a bank with all the tuning files in that directory.
`.scl` and `.kbm` files with the same prefix are taken to belong to the same program.
As mentioned, a selection of several directories generates a set of banks.
If the end-user want to check what files are selected, they can press 'open files' and see them marked.

Tuning can be changed for a currently sounding note (always) or only applied to the next note on.
This is a relevant setting for MTS-ESP.
~~For MTS Sysex messages the switch cannot be set but works as an indicator.~~
> For MIDI 1.0 the switch cannot be set but works as an indicator, since the message itself says whether it is real time.
Otherwise, it has no effect.

In the pitchbend sensitivity section, the user can set the global and MPE member pitchbend sensitivities.
The global pitchbend sensitivity applies to the MPE manager channel and adds pitchbend to all MPE members.
The global pitchbend sensitivity also apply to all non-MPE channels.
Pitchbend messages are never ignored, but sensitivity can be set, and a sensitivity of 0 is effectively ignoring them.

MIDI 2.0 messages are kept apart from MIDI 1.0 messages such that no channel clashes occur.

[^precision]: MIDI 2.0 can be finer, and is requested by MIDI Sidebar to be, but the minimum precision a MIDI 2.0 device is required to send is around 0.2 c.