# The inventory

Everything in `juce_audio_basics/midi`, checked against **JUCE 9.0.0**. The point
of a complete list is that the absences are as informative as the entries.

## What is there

| class | what it is for |
|---|---|
| `MidiMessage` | one message, owning its bytes. Small ones are stored inline; sysex heap-allocates. |
| `MidiMessageMetadata` | a message plus its `samplePosition`, which is what iterating a buffer yields |
| `MidiBuffer` | timestamped messages for one block, sorted by sample position |
| `MidiBufferIterator` | forward iterator over the above; `for (const auto m : buffer)` |
| `MidiRPNDetector` / `MidiRPNMessage` | the only multi-message parser JUCE ships |
| `MidiRPNGenerator` | the other direction — builds the four control changes |
| `MidiKeyboardState` | which notes are down, plus a `Listener` |
| `MidiFile` / `MidiMessageSequence` | standard MIDI files and their tracks |
| `MidiDataConcatenator` | reassembles messages from a byte stream; for device drivers, not for plugins |

## What is not there

- **14-bit control changes.** No pairing of CC *n* with CC *n*+32, in either
  direction. This surprises people because `MidiRPNDetector` exists and looks
  adjacent; it is not.
- **A high-resolution velocity prefix** (CC 88, CA-031). Nothing knows about it.
- **MIDI tuning system exclusive.** No MTS parser or generator of any kind — see
  the `midi-microtuning` skill, and note that ODDSound's `libMTSClient` will read
  all MTS formats for you if its behaviour suits (it prefers a connected MTS-ESP
  master over its own sysex table, which is not always what you want).
- **Anything that turns a controller number into a name for the specification's
  *reserved* meanings.** `getControllerName` exists but is a display convenience
  — see below.

## `MidiMessage` details worth knowing

**`getDescription()`** produces a human-readable line — `"Note on C3 Velocity 100
Channel 1"`, `"Program change 5 Channel 1"`, `"Sysex: 6 bytes"`. Useful for a
monitor or a log. It is JUCE's wording, not the specification's, and it is not
stable API in the sense of something to parse.

**`getControllerName (int)`** returns the standard name for a controller number
— `"Modulation Wheel (coarse)"`, `"Channel Volume"`. Fine for a tooltip. Do not
use it to decide *behaviour*: instruments use "reserved" numbers as ordinary
knobs, so a name is a hint about intent, never a fact about the sender.

**Note off is two things.** `isNoteOff (bool returnTrueForNoteOnVelocity0 =
true)` defaults to counting a note-on with velocity zero as a note off, which is
what you want almost always. `isNoteOnOrOff()` covers both. Getting this wrong
gives stuck notes with any sender that uses running status.

**Sysex offsets.** `getSysExData()` returns a pointer past `F0`;
`getSysExDataSize()` excludes both `F0` and `F7`. `getSysExDataSpan()` (JUCE 8+)
gives the same as a `Span<const std::byte>`. To construct one,
`MidiMessage::createSysExMessage (data, size)` where `data` again excludes the
framing bytes — it adds them.

**Pitch bend is raw.** `getPitchWheelValue()` is 0..16383 with no centring; the
centre is 8192 and JUCE will not subtract it for you.

## `MidiBuffer` in `processBlock`

The contract, from `AudioProcessor::processBlock`:

> Any messages left in the MIDI buffer when this method has finished are assumed
> to be the processor's MIDI output. This means that your processor should be
> careful to **clear any incoming messages from the array if it doesn't want them
> to be passed-on**.

So pass-through is free and consuming is work. The usual shape is to read the
buffer `const`, collect the sample positions you acted on, and rebuild:

```cpp
kept.clear();                       // a member, so nothing allocates per block

for (const auto metadata : midiMessages)
    if (! consumed.contains (metadata.samplePosition))
        kept.addEvent (metadata.getMessage(), metadata.samplePosition);

midiMessages.swapWith (kept);
```

`addEvent` is cheapest in ascending sample order, which iterating the source
gives you for free.

**Nothing on the audio thread should build a `juce::String`.** That rules out
`getDescription()` in `processBlock` — collect the messages and format them on
the message thread.

## `MidiRPNDetector`

`tryParse (channel, controllerNumber, controllerValue)` returns an
`optional<MidiRPNMessage>`. Feed it **every** control change; it decides. Two
behaviours worth knowing before trusting it:

- it returns a result on the **MSB** when a parameter number is selected, and
  again with a full 14-bit value when the LSB follows — so one gesture can yield
  two results, and the second supersedes the first;
- it copes with senders that never send a data LSB at all, which the
  specification permits.

`reset()` forgets a half-received parameter number. Worth calling when transport
stops or the plugin resets, or a stale selection will collect whatever data entry
comes along next.
