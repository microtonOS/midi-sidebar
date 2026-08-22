# MPE in JUCE

Nine headers in `juce_audio_basics/mpe`. Checked against **JUCE 9.0.0**. For what
MPE itself requires see [midi-1_0/mpe](../../midi-1_0/references/mpe.md); this is
about JUCE's model of it.

## Master, not manager

JUCE says **master** everywhere MPE v1.1 says **manager**:
`MPEZone::getMasterChannel()`, `MPEZone::masterPitchbendRange`,
`MPEMessages::setLowerZoneMasterPitchbendRange`,
`MPEZoneLayout::setLowerZone`'s documentation.

*MIDI Polyphonic Expression* (M1-100-UM v1.1) renamed the concept; JUCE has not
followed, and the API is public so it probably will not. Use the specification's
word in your own names and accept the seam at the boundary — a comment at the
conversion point is cheaper than a codebase that is half-and-half.

## `MPEZoneLayout` — more than a data holder

The one to reach for, and it does more than people expect:

| member | note |
|---|---|
| `setLowerZone (numMemberChannels, perNotePitchbend, masterPitchbend)` | **member channel count**, not a channel number — see the conversion trap below |
| `setUpperZone (...)` | upper zone counts *downward* from channel 16 |
| `getLowerZone() / getUpperZone()` | returns an `MPEZone`; check `isActive()` |
| `isUsing (channel)` | any role in any zone |
| `isUsingChannelAsMemberChannel (channel)` | member specifically |
| `processNextMidiEvent (const MidiMessage&)` | **parses the MPE Configuration Message and RPN 0 for you** |
| `Listener::zoneLayoutChanged()` | fires when the above changes something |

`MPEZone` carries `perNotePitchbendRange` defaulting to **48** and
`masterPitchbendRange` defaulting to **2** — the specification's own defaults
(§2.2.5), so JUCE is right here and you do not need your own constants.

**The conversion trap.** A zone is parameterised by *how many member channels it
has*, while a user interface usually asks for *where the zone ends*. Lower zone
with 4 members occupies channels 1–5, master 1, members 2–5. Upper zone with 4
occupies 12–16, master 16, members 12–15. Converting an edge to a count is
arithmetic that is easy to get off by one in exactly one direction, so do it once
in a named function and test it over all sixteen channels.

**`processNextMidiEvent` means you rarely parse the MCM yourself.** The MPE
Configuration Message is RPN 6 — four control changes, not a message — and this
already recognises it, along with RPN 0 pitch-bend ranges. Feed it every message
and read the layout afterwards.

## `MPEMessages` — the other direction

Static factories returning a `MidiBuffer` of the control changes that make up
each configuration message: `setLowerZone`, `setUpperZone`,
`setLowerZonePerNotePitchbendRange`, `clearAllZones`, `setZoneLayout (layout)`.

Useful when *sending* a configuration. Note they return a whole `MidiBuffer`
rather than a message, because an MCM is four control changes — which is the same
point the `midi-1_0` skill makes about RPNs generally.

## The ones that are usually the wrong tool

`MPEInstrument`, `MPESynthesiser`, `MPESynthesiserBase`,
`MPESynthesiserVoice`, `MPENote`, `MPEValue` model **a synthesiser playing
notes**: per-note pressure, timbre and pitch bend, voice allocation, note
lifetimes.

If you are writing a MIDI *effect* — reading a stream, deciding something, and
passing it on — none of that is yours to own, and adopting `MPEInstrument` means
maintaining a voice model that nothing plays. `MPEZoneLayout` alone answers "is
this channel a member channel", which is usually the whole question.

`MPEChannelAssigner` (in `juce_MPEUtils.h`) allocates notes to member channels.
That one *is* useful to an effect, but only once it re-emits notes rather than
observing them.
