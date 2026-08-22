# General MIDI

Source: *General MIDI 2*, version 1.2a, MMA/AMEI. References are to its section
numbers.

**Why this file is useful out of proportion to its size.** MIDI 1.0 defines what
messages *mean* and almost never says a device must implement one — see
[rpn-nrpn](rpn-nrpn.md). GM2 is where "required" appears. So when the question is
*can I rely on this being supported*, GM2 is usually the only document with an
answer, even for devices that are not GM2.

Treat that carefully in both directions: GM2's `[required]` binds GM2 devices
only, and a non-GM2 instrument owes you nothing. But GM2 is also where the MMA
wrote down what a sensible baseline looks like, so its list is a good guide to
what is worth implementing first.

## What GM2 is

A profile over MIDI 1.0, not a separate protocol. The device requirements set the
scale: **32-voice polyphony or more**, **all 16 MIDI Channels addressable
simultaneously**, any channel usable as a melody channel. Channel 10 defaults to
a Rhythm Channel and Channel 11 defaults to Melody; both 10 and 11 can be Rhythm
Channels via Bank Select.

That last point matters more than it looks, because several messages below carry
"Rhythm Channels shall not respond to this message" — a pitch-shifting message
must not detune a drum kit, since on a key-based instrument a different pitch
would be a different sound.

GM2 pulls in seven addenda by reference (§1):

| | |
|---|---|
| CA-020 | MIDI Tuning Bank/Dump Extensions |
| CA-021 | Scale/Octave Tuning |
| CA-022 | Controller Destination Setting |
| CA-023 | Key-Based Instrument Controllers |
| CA-024 | Global Parameter Control |
| CA-025 | Master Fine/Coarse Tuning |
| CA-026 | Modulation Depth Range RPN |

## What GM2 requires, for pitch

All of these are `[required]`, and all default to a neutral value — so a
conforming device implements **every one of them at once**, and they have to
compose rather than being alternatives.

| § | message | default | range / resolution |
|---|---|---|---|
| 3.4.1 | Pitch Bend Sensitivity, RPN 00 | `02H/00H` = 2 semitones | must accommodate **at least ±12 semitones**; LSB may be ignored |
| 3.4.2 | Channel Fine Tuning, RPN 01 | `40H/00H` | ±100 c, 100/8192 ≈ 0.0122 c |
| 3.4.3 | Channel Coarse Tuning, RPN 02 | `40H/00H` | −64 … +63 semitones, 100 c |
| 3.4.4 | Modulation Depth Range, RPN 05 | `00H/40H` = ±50 c | CA-026 |
| 4.2 | Master Fine Tuning, sysex `04 03` | `40H/00H` | as RPN 01 |
| 4.3 | Master Coarse Tuning, sysex `04 04` | `40H/00H` | as RPN 02 |
| 4.7 | **Scale/Octave Tuning Adjust**, sysex `08 08` | `40H` | ±64 c per pitch class |

Rhythm Channels are excepted from 3.4.2, 3.4.3 and 4.7.

**§4.7 is the surprising one.** GM2 requires the non-real-time one-byte form of
Scale/Octave Tuning Adjust, and recommends the real-time one-byte form. So
**GM2 includes part of the MIDI Tuning Standard and mandates it** — which is easy
to miss, because MTS is usually filed under microtuning rather than under General
MIDI. The rest of MTS is not required: nothing in the GM2 body asks for the bulk
dump, the key-based dump or single-note changes, though CA-020 is incorporated by
reference.

For the messages themselves see [sysex](sysex.md) and
[rpn-nrpn](rpn-nrpn.md); for what a tuning is and the sources that are not MIDI
messages at all, see the **midi-microtuning** skill.

## What that combination settles

Because scale/octave tuning and the four displacements are all required and all
default to neutral, a GM2 device must do both at once. And they cannot conflict,
because **scale/octave tuning carries no absolute reference**: it is twelve
offsets from equal temperament, one per pitch class, repeating in every octave.
It says what the temperament is; the tuning displacements say where it sits.

§4.2 puts the other half plainly:

> When Master Fine and Coarse Tuning are at their default settings, the tuning of
> Note number 69 will be A440Hz (in the absence of Pitch Bend or other pitch
> altering controllers).

So in GM2's model those messages are precisely *how* an instrument leaves A440.

The four displacements **sum**, which is CA-025's rule rather than GM2's: "the
total displacement in cents from A440 for each MIDI channel is summation of the
displacement of this Master Fine Tuning and the displacement of Fine Tuning using
RPN", and likewise for coarse.

## Volume, and the curve

§3.3.4 gives the law for CC 7, and §4.1 says Master Volume follows the same one:

> Regarding the curve of volume change messages, **the square of the value is
> proportional to the volume.**

Worked through in the document as 127 × 127 = 16129 at 0 dB. In decibels that is
**40·log₁₀(v / v_max)**, not 20 — so halving the *value* is −12.04 dB, not
−6.02 dB. Getting this wrong is a quiet bug: everything works, and every fade is
the wrong shape.

§3.3.6 adds that Expression (CC 11) follows the same square law and is a
*modifier* of CC 7 rather than a second volume: "Channel Volume (cc#7) should be
used to set the overall volume of the Channel prior to music data playback as
well as for mixdown fader-style movements, while Expression (cc#11) should be
used during music data playback to attenuate the programmed MIDI volume."

## The three volume scalars

From the Detailed Specification's Device Control section rather than GM2, but it
is the same subject and the same trap. A conforming device tracks **three**
volume scalars and multiplies them:

1. Master Volume received on its **own device ID**;
2. Master Volume received on the **broadcast ID `7F`**;
3. **channel** messages, i.e. CC 7 and CC 11.

So answering both a Master Volume system exclusive and CC 7 is correct rather
than redundant — they are different layers of one mix, and implementing only one
loses a layer.

<!-- General MIDI 1 (RP-003) and GM Lite have not been read from their own
documents. Nothing here should be attributed to them; GM1 in particular predates
every addendum listed above. -->
