---
name: juce-midi
description: What JUCE's MIDI classes actually provide, what they leave to you, and where their names or offsets differ from the specification. Use when handling MIDI in a JUCE plugin — reading a MidiBuffer in processBlock, parsing sysex or RPN, or working with MPE. For what the protocol requires see midi-1_0; for UMP see midi-2_0; for tuning see midi-microtuning.
---

# JUCE and MIDI

JUCE gives you a good message class, a good buffer, one multi-message parser, and
a complete MPE model. It gives you nothing for several things people assume are
there. This skill is about the boundary, so that time is not spent looking for a
helper that does not exist or reimplementing one that does.

Checked against **JUCE 9.0.0**. Where a version matters the file says so; JUCE's
MIDI classes have been stable for a long time, but the MPE naming below has been
wrong for the whole of that time and shows no sign of changing.

## References

| open | when you are |
|---|---|
| [classes](references/classes.md) | asking whether JUCE already does this — the full inventory of `juce_audio_basics/midi`, and the things it deliberately does not do |
| [mpe](references/mpe.md) | using `MPEZoneLayout`, `MPEInstrument` or `MPESynthesiser`, or reading an MPE Configuration Message |

## The four that cost the most time

- **`processBlock` passes MIDI through by default.** "Any messages left in the
  MIDI buffer when this method has finished are assumed to be the processor's
  MIDI output … your processor should be careful to clear any incoming messages
  from the array if it doesn't want them to be passed-on." Consuming is the
  deliberate act, not forwarding. A plugin that reads a `MidiBuffer` and forgets
  this emits everything it received.

- **`getSysExData()` omits `F0`, and `getSysExDataSize()` counts neither `F0` nor
  `F7`.** So `F0 7F 7F 04 01 vv vv F7` — eight bytes on the wire — is a size of
  **6** starting at the manufacturer/universal id. Off-by-two here produces a
  parser that rejects every valid message.

- **There is no 14-bit control change helper anywhere.** Nine classes live in
  `juce_audio_basics/midi` and none of them pairs CC *n* with CC *n*+32. If you
  want that, you write it — and before you do, read
  [midi-1_0/real-devices](../midi-1_0/references/real-devices.md), because
  hardware does not follow the pairing rule either.

- **JUCE says "master" where MPE v1.1 says "manager".** `getMasterChannel()`,
  `masterPitchbendRange`, `setLowerZoneMasterPitchbendRange`. The MIDI
  Association renamed it in the 2018 revision; JUCE did not follow. Code that
  reads the specification and the API side by side will look inconsistent
  whichever word it picks — pick the spec's for your own names and expect the
  seam.

## Checking any of this

None of JUCE's MIDI classes needs a plugin, a host or a window, so a parser
built on them can be exercised in a console app.
`juce-guide/scripts/add_check_app.cmake` makes that a CTest target;
`juce-guide/scripts/check.sh` runs one ad hoc. Byte sequences to feed them —
RPN and NRPN, the MPE configuration message, Master Volume, and a 14-bit sweep
in both byte orders — come from
[midi-1_0/scripts/midi_vectors.py](../midi-1_0/scripts/midi_vectors.py).

## What JUCE is not

`MidiRPNDetector` is a parser for the (N)RPN *idiom*, not a general
multi-message parser: nothing else in JUCE reassembles a sequence of control
changes into one event.

`MPEInstrument` and `MPESynthesiser` model **a synthesiser's voices**. If you are
writing a MIDI effect — something that reads a stream and passes it on — they are
the wrong tool, and `MPEZoneLayout` on its own is usually what you wanted.
