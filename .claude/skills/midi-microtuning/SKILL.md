---
name: midi-microtuning
description: Different standards for implementing microtuning in digital instruments. For use in microtonal or xenharmonic applications. Standards include MIDI 2.0, the MIDI tuning standard (MTS), tuning files (e.g. .scl and .kbm), and MIDI polyphonic expression (MPE).
allowed-tools: WebFetch(domain:midi.org)
---

# MIDI Microtuning

Make digital instruments microtunable.
Ask the user whether the device or plugin should be a tuning client, a tuning master, or both (peer-to-peer).

| standard | master | client |
|----------|--------|--------|
| MIDI 2.0 | ideally | ideally |
| MTS Sysex | yes | yes |
| MTS ESP | if Linux, MacOS, Windows | if Linux, MacOS, Windows |
| tuning files | yes | if uploading possible |
| MPE | yes | if pitchbend possible |
| monophonic pitchbend | yes | no, workaround for clients without microtuning |

Regardless of being a tuning client or tuning master, the software must include a local list plus a local table.
The list contains frequencies for each MIDI note.
The table contains frequencies for each MIDI note and channel.
The former is used for unspecified channels.
One fixed list+table is mandatory to read and write data to, but several can be used and assigned dynamically for several tuning programs and tuning banks.
Other data that should also be tracked include:
- Retuning on note on or always.
- The name of the tuning.
- Tuning period.

## Client
A tuning client listens to tuning updates and/or can open tuning files.

If the tuning client is on Linux, MacOS, or Windows, it should implement MTS ESP.
(As of writing, August 15 2026, MTS ESP does not support mobile devices.)
It should implement all available features, including
- Unspecified channel (-1) support and multichannel support.
- Retuning on note on or always.
- Showing the name of the tuning.
- Indicating whether the connection to the master works.
- Reading the tuning period.[^tuningPeriod]

Use the official library ([GitHub](https://github.com/ODDSound/MTS-ESP)) for C/C++ and mtsespy ([GitHub](https://github.com/narenratan/mtsespy)) for Python.
For more info, see the [MTS ESP reference](references/mts-esp.md).

The tuning client must implement support for MTS Sysex messages.
Reading MTS Sysex included in the MTS ESP library.
The features of MTS Sysex are more or less the same as for MTS ESP except that tuning programs and tuning banks are included.
(Note that names can only be 16 ASCII characters long.)
For more information, see the [MTS Sysex reference](references/mts-sysex.md).

If file uploading is possible, the tuning client should have support for at least Scala files (`.scl` and `.kbm`) as well as `.syx` for MTS Sysex via file.
Use the Surge Synthesizer Team's library ([GitHub](https://github.com/surge-synthesizer/tuning-library)) for handling `.scl`and `.kbm` files as far as possible.
See [Scala reference](references/scala.md) and other tuning files below.
A directory of scala files may be used to form a tuning bank.

Ideally, there should be MIDI 2.0 support for tuning.
MIDI 2.0 build upon older frameworks, so build the MTS standards first.
See the [MIDI 2.0 reference](references/midi-2_0.md) for details on MIDI 2.0 in the context of tuning.

Optionally, a client may implement MPE and pitchbend, but that is orthogonal to microtuning for clients (!).

# Master

A master broadcasts tuning messages both multilaterally and bilaterally.
A master should also have a thru functionality to modulate incoming MIDI messages.

As with clients, a master should implement MTS ESP.
An additional MTS ESP feature it should have is to reinitialize.
Requires no MIDI connection with clients.

As with clients, a master should include support for MTS Sysex.
Note that MTS ESP has no tools for writing MTS Sysex messages.
Requires MIDI connections with clients, but no thru option.
Should preferably still include a thru with entirely unmodulated notes.

As with clients, a tuning master should include support for at least `.scl`, `.kbm`, and `.syx` tuning files.
Some more advanced tuning master may also be able to edit and save tuning files.
The data from the files should be braodcasted using the other frameworks.

Ideally, masters should also have MIDI 2.0 support for tuning.

MPE should be used as a workaround for clients that support MPE but not (other forms of) microtuning.
If so, MIDI notes (note on, note off, and polytouch)arriving to the thru port should be split across the MPE channels, have their pitch bends set so that standard tuning is tranformed to the currently active tuning, and sent through a (separate) MPE outport to the client.
When new notes arrive through thru, the output should be in the order note off -> pitchbend -> note on (same channel).
When incoming pitchbend messages arrive they should be adapted to be relative to the tuning and need of course not follow the ordering above.
Various forms of multitimbral devices can also rely on this method when the same timbre is replicated across all channels.
General MIDI is an example where all channels except drum channels 9 (10) (and optionally 10 \[11\]) can be used.

The workaround for devices that don't support MPE either is monophonic pitchbend.
It works similar to MPE with the difference that a single channel is used.
Because of this reason, the thru must make the messages monophonic.
Otherwise, there will be pitch glide artifacts on unrelated notes.
If several notes are active and one is released, the last pressed note should resound.
Look up how monophonic synthesizers usually handle this.
Most devices and plugins will support this at least.
Monophonic pitchbend should be sent on yet another thru channel different from the Sysex one and the MPE one.

## Tuning Files
Formats include:
- Ableton scale (`.ascl`)
- AnaMark v1 and v2 tuning (`.tun`)
- Deflemask reference (`.txt`)
- Image-Line Harmor and Sytrus pitch maps (`.fnv`)
- Kontakt tuning script (`.txt`)
- Korg Sound Librarian scale (e.g. `.mnlgtuns`)
- Max/MSP coll tuning (`.txt`)
- MTS Sysex Bulk Tuning Dump (`.syx`)
- PureData text tuning (`.txt`)
- Reaper note name map (`.txt`)
- Scala scale files (`.scl`) and keyboard mapping files (`.kbm`)
- Soniccouture tuning (`.nka`)
- [SonicWeave Interchange (`.swi`)](https://github.com/xenharmonic-devs/sonic-weave/tree/main)

More information is available at [Scale Workshop](https://scaleworkshop.plainsound.org/) ([GitHub](https://github.com/xenharmonic-devs)).


[^tuningPeriod]: A tuning period is when an uneven tuning starts to repeat its pattern of distances between adjacent notes. For equal divisions, any interval could be a period.




