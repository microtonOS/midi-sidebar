# System exclusive

Source: *Complete MIDI 1.0 Detailed Specification* 4.2.1 (February 1996),
**Table VII, System Exclusive Messages**, and **Table VIIa, Currently Defined
Universal System Exclusive Messages**.

A message that starts `F0` (SOX), ends `F7` (EOX), and carries whatever the
owner of the ID has defined in between.

## The ID byte

The byte after `F0` says whose message this is (Table VII, note 1):

| ID | meaning |
|---|---|
| `00`–`7C` | manufacturer identification |
| `7D` | **non-commercial** — schools, research; "not to be used on any product released to the public" |
| `7E` | Universal **Non**-Real Time |
| `7F` | Universal **Real Time** |

**If the first byte of a manufacturer ID is `00`, the following two bytes extend
it** — the three-byte manufacturer ID form, for the manufacturers who did not get
a one-byte number. A parser that assumes one byte will misread every message from
those.

## Universal system exclusive

```
F0 7E <device ID> <sub-ID#1> <sub-ID#2> …    non-real time
F0 7F <device ID> <sub-ID#1> <sub-ID#2> …    real time
```

`<device ID>` addresses one device; **`7F` means all devices**.

The non-real-time / real-time split is about *when* the message is meant to act.
Non-real-time messages are setup: "if it is NOT sent as a setup message … it is
assumed that the message will be ignored for notes that are already sounding"
(*MIDI Tuning Updated Specification*, Notes). Real-time messages act immediately
on sounding notes.

`sub-ID#1` names the family. From Table VIIa, the non-real-time families include:

| sub-ID#1 | family |
|---|---|
| `01`–`03` | Sample Dump Header / Data Packet / Request |
| `04` | MIDI Time Code |
| `05` | Sample Dump Extensions |
| `06` | General Information — `01` Identity Request, `02` Identity Reply |
| `07` | File Dump |
| `08` | **MIDI Tuning Standard** |
| `09` | General MIDI — `01` System On, `02` System Off |
| `7B`–`7F` | End Of File, Wait, Cancel, NAK, ACK |

`08` is the one this project cares about; the **midi-microtuning** skill has its
messages in full.

## Parsing

Table VII note 2 is the whole rule, and it is worth quoting:

> All bytes between the System Exclusive Status byte and EOX must have zeroes in
> the Most Significant Bit … with the exception of System Real Time Status Bytes
> (`F8H`–`FFH`). Any other Status Byte that appears between the SOX (`F0H`) and
> EOX (`F7H`) will be considered an EOX message, and terminate the System
> Exclusive message.

Three consequences for a parser:

- **Every data byte carries 7 bits.** This is the constraint behind every packing
  scheme in every SysEx family.
- **A System Real Time byte may appear mid-message.** Act on it, remove it, and
  carry on with the SysEx.
- **Any other status byte ends the message**, whether or not `F7` ever arrives.
  So a truncated SysEx is a normal event, not a corruption to be logged.

Length is not declared up front, so a receiver needs a bounded buffer and a
policy for a message that overruns it.

## Checksums

Dump messages carry one. From the *MIDI Tuning Updated Specification*, "Checksum
Calculation":

> successively XOR the bytes in the message, excluding the `F0`, `F7`, and the
> checksum field … the resulting value is then AND'ed with `7F`

One documented exception: the original Bulk Dump (`sub-ID#2 = 01`) was ambiguous
enough that manufacturers implemented its checksum differently, and receivers are
now recommended to **ignore** it on that message alone.

## Device Control: Master Volume and Master Balance

Universal **Real Time**, sub-ID#1 `04` (p57). Both are 14-bit, LSB first:

```
F0 7F <device id> 04 01 vv vv F7     Master Volume;  00 00 = off
F0 7F <device id> 04 02 bb bb F7     Master Balance; 00 00 = hard left,
                                                     7F 7F = hard right
F0 7F <device id> 04 03 ll mm F7     Master Fine Tuning    (CA-025)
F0 7F <device id> 04 04 00 mm F7     Master Coarse Tuning  (CA-025)
```

### Master Fine and Coarse Tuning

Added by **CA-025**, *Master Fine/Coarse Tuning*, 2 March 1999 — "intended to
produce the same effect as the pitch shift control on a tape recorder". The
motivation is in the document: Karaoke needs to retune every channel at once,
and orchestral and piano recordings are often at 442, 443 or 445 Hz, for which
"there is no common message to set overall tuning".

Both are displacements in cents **from A440**, LSB first like the rest of the
family:

| | encoding | centre | range | step |
|---|---|---|---|---|
| Fine `04 03` | 14 bits | `00 40` | −100 c … +99.988 c | 100/8192 ≈ **0.0122 c** |
| Coarse `04 04` | 7 bits; **"the LSB is always 0"** | `00 40` | −64 … +63 semitones | 100 c |

Three consequences worth having:

- **They sum with the RPNs.** "The total displacement in cents from A440 for each
  MIDI channel is summation of the displacement of this Master Fine Tuning and
  the displacement of Fine Tuning using RPN", and likewise for coarse. So a
  channel's total is master fine + master coarse + RPN 01 + RPN 02.
- **CA-025 is why RPN 01 and 02 were renamed.** They were *Master* Fine and
  Coarse Tuning on p18 of the Detailed Specification; this document renamed them
  to **Channel** Fine and Coarse Tuning to free the names for the messages above.
  Table IIIa still uses the old ones. See [rpn-nrpn](rpn-nrpn.md).
- **Coarse tuning must not transpose note numbers.** "For devices which support
  Key-based Instruments … it is important that this message NOT result in MIDI
  note-shifting; otherwise a different drum sound would be selected." Retune the
  pitches, do not renumber the keys.

Note the fine step against MIDI tuning's own resolution: 0.0122 c here against
100/2¹⁴ = 0.0061 c for an MTS frequency field. Retuning by this message is
lossier than rewriting a tuning table, which argues for applying it as an offset
at playback rather than folding it into stored frequencies.

They exist "to produce the same effect as volume and balance controls on a stereo
amplifier … so that one Master Volume control can simultaneously fade out all the
layers in a sound module". The clue to their design is in the first sentence of
the section: **they address *devices*, where CC 7 and CC 8 address *channels***.

### The three scalars

This is the part worth knowing, because it settles a question that looks like a
conflict and is not. A conforming device "must internally track three volume and
two balance scalars":

1. one received on **its own ID** — "which matches its knob on the front panel;
   if no knob or if knob is not scanned then power up default is set at full
   volume";
2. one received on the **broadcast ID `7F`**, the All Call;
3. one from **channel messages**, i.e. CC 7 and CC 8.

They multiply rather than compete: "each virtual/channel-based instrument can be
individually mixed, then a device could be individually scaled, and then all
devices could be brought down together without forgetting their individual
levels." So a plugin answering both a Master Volume SysEx and CC 7 is doing the
right thing — the two are different layers of one mix, and implementing only one
of them loses a layer.

<!-- Not yet read from their own documents: Table VIIb (manufacturer ID
assignments), the Device Inquiry and File Dump message layouts, MIDI Show
Control and MIDI Machine Control. All are listed in the 4.2.1 contents at
pp34-58. -->
