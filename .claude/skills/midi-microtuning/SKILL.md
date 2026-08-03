---
name: midi-microtuning
description: Different standards for implementing microtuning in digital instruments. For use in microtonal or xenharmonic applications. Standards include MIDI 2.0, the MIDI tuning standard (MTS), tuning files (e.g. .scl and .kbm), and MIDI polyphonic expression (MPE).
allowed-tools: WebFetch(domain:midi.org)
---

# MIDI Microtuning

Make digital instruments microtunable.

## Establish the scope first

Which standards to implement is a property of the project, not of this skill.
Determine the following from the project itself—its README, build files, dependencies, target format, and existing MIDI code—and ask the user only what cannot be found there.

- **Role.** Master (sends tuning), client (receives tuning), or both?
A master owns the scale and must construct messages; a client only has to interpret them and is usually far less work.
- **Platform.** Desktop application or plugin on MacOS, Linux, or Windows? Embedded or hardware? Web? Mobile?
- **Transport.** Is the other end reachable over UMP/MIDI 2.0, or MIDI 1.0 only? Is it a DIN cable with 31.25 kbaud of bandwidth, or USB, or in-process?
- **Control of both ends.** If the project talks only to itself, pick one standard and do it well. If it talks to arbitrary third-party instruments, breadth matters more than depth.
- **Real-time.** Must a sounding note be able to change pitch, or is tuning fixed at note-on?

Support several standards where the project allows.
Consider the following cross-standards features:
- Multichannel support. Add if possible.
In general MIDI, channels are usually intended for different timbres but in microtuning it can be a way for using more than 128 keys—either from one specialized controller or several generic controllers used as different manuals with different tunings.
- Microtuning presets and tuning program change. Add if possible.
- Naming. Display or send the names of tunings for standards that support it.
- Real-time. At least add support for non-real-time tuning change. Real-time depends on the digital instrument. Realtime means that a currently sounding note may change its pitch.
- Tuning period. Display or send a tuning period when applicable. A tuning period is when an uneven tuning starts to repeat its pattern of distances between adjacent notes. For equal divisions, any interval could be a period.


## MIDI 2.0

Implement MIDI 2.0 on top of MIDI 1.0 for backwards compatibility.
All other standards here are compatible with MIDI 1.0.
Microtuning is implemented with per-note tuning.
See [references](references/midi-2_0.md).

## MTS Sysex

Implement MIDI tuning standard sysex messages using the MTS ESP library (see [below](#mts-esp)) as far as is possible.
Note that the library does not produce outgoing MTS sysex messages, it only parses incoming.

There are three kinds of tuning messages
- Bulk Tuning Dump Request (non-real-time)
- Bulk Tuning Dump (non-real-time)
- Single-note Tuning Change (real-time)

Note that the bulk tuning dump can also contain a name with 16 ASCII characters.
They contain tuning program number and an extension added tuning banks as well.

A later extension added
- 1-byte scale/octave
- 2-bytes scale/octave

for real-time and non-real-time respectively.
Both scale/octave forms are limited to 12 notes repeating every octave, so use the key-based dumps for anything else.

Separately, the master fine and coarse tuning sysex messages (CA-025) shift the tuning of the whole device.
Use them for a global reference pitch (A=442, A=415) and transposition rather than for microtuning proper.

Registered parameter numbers 03 and 04 are used to select any of the instrument's stored tunings as the "current" or active
tuning. Instruments which permit the storage of multiple microtunings should respond to these messages by instantly
changing the "current" tuning to the specified stored tuning. This change takes effect immediately and must occur without
audible artifacts (notes-off, resets, re-triggers, glitches, etc.) if any affected notes are sounding when the message is
received.

See [references](references/mts-sysex.md) for further information.

## MTS ESP

Implement MIDI tuning standard extrasensory perception if the digital instrument is an application on MacOS, Linux, or Windows.
MTS ESP uses a shared object to write and read (multichannel) tuning data with additional support for naming the tuning, muting certain notes, and noting the period of the tuning.
Use the official library ([GitHub](https://github.com/ODDSound/MTS-ESP)) for C/C++ and mtsespy ([GitHub](https://github.com/narenratan/mtsespy)) for Python.
See [references](references/mts-esp.md) for further information.

## Tuning Files

At least implement support for Scala tuning files (`.scl` and `.kbm`).
Use the Surge Synthesizer Team's library ([GitHub](https://github.com/surge-synthesizer/tuning-library)) for handling `.scl`and `.kbm` files as far as possible.
See [references](references/scala.md) for more information.

Other formats include:
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
- Soniccouture tuning (`.nka`)
- [SonicWeave Interchange (`.swi`)](https://github.com/xenharmonic-devs/sonic-weave/tree/main)

More information is available at [Scale Workshop](https://scaleworkshop.plainsound.org/) ([GitHub](https://github.com/xenharmonic-devs)).

## MPE

Implement support for microtuning via MIDI polyphonic expression if the digital instrument allows for real-time retuning.
In MPE, channel 0 (1) is used as a manager channel (called a master channel before MPE v1.1, and still so named in JUCE).
The remaining channels are member channels used for notes, with voice stealing if the number of notes exceeds the number of member channels—at most 15.
Each channel has its own pitchbend which enables microtuning if the pitchbend message is sent ahead of the note.
(Optionally, MPE can be bitimbral with a second zone whose manager channel is 15 (16) and whose member channels descend from 14 (15).)
Zones are not implicit: they are configured by sending the MPE configuration message (RPN 6) to the manager channel of each zone.
Note that the default pitchbend sensitivity of 48 semitones only resolves to about 0.6 cents, so reduce it for fine tunings.
See [references](references/mpe.md) for further information.

**General MIDI**.
If the different MIDI channels use the same timbre, general MIDI behaves like MPE with two exceptions.
There are no master channels.
MIDI channel 9 (10) is a percussion channel and MIDI channel 10 (11) is optionally a percussion channel.

**Other multitimbrals**.
In general, an omni-off device or plugin can work similarly to MPE depending on exactly how the channels are used.

## Monophonic Pitchbend

Only relevant for microtuning masters.
(It is trivial for clients.)
If a client has no MIDI tuning capabilities, it probably still accepts pitchbend.
To avoid artifacts such as undesired note glides, a master should cut off a currently ringing note, send an appropriate pitchbend message, and then send the new note.
Furthermore, it should keep track of pressed keys such that if a key is released then the last note and associated pitch among the still pressed keys is resounded—typical of monophonic synthesizers.
