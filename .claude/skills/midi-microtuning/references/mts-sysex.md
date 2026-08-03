# MTS Sysex

The MIDI Tuning Standard (MTS) is a set of universal system exclusive messages for sharing microtunings
between instruments and switching between them during performance.
It is the only microtuning standard that works over a plain MIDI 1.0 cable.

Sources:
- [MIDI Tuning Updated Specification](https://midi.org/midi-tuning-updated-specification) — the
  original MIDI Tuning messages plus the Bank/Dump Extensions (CA-020) and the Scale/Octave
  Extensions (CA-021/RP-020).
- MMA/AMEI document CA-025, *Master Fine/Coarse Tuning*, incorporated into the Complete MIDI 1.0
  Detailed Specification. Available from [midi.org](https://midi.org/).

The MMA/AMEI documents are copyrighted and may not be redistributed; what follows is a condensed
technical reference, not a reproduction. Consult the specifications for the normative text.

## Design assumptions

- Any of the 128 MIDI key numbers (at least those in the instrument's playable range) shall be
  tunable to any frequency within the proposed playable range.
- Exponential (constant cents) resolution across the frequency range is strongly suggested, not enforced.
- Up to 128 tuning programs, extended by CA-020 to 128 banks × 128 programs. 128 is a maximum,
  not a requirement; an instrument may support fewer.
- Real-time messages shall re-tune sounding notes instantly and without glitching, forced note-offs,
  re-triggering or other audible artifacts.
- Non-real-time messages are intended as setup messages. If one arrives during performance it is
  assumed to be ignored for notes already sounding, unless a specific RP says otherwise.
  Document whatever the device actually does.

## Message index

`08` is sub-ID#1 ("MIDI Tuning Standard") for every message below.
`<device ID>` is the target device, `7F` = all devices.

| sub-ID#2 | Header | Message | Bank | Affects sounding notes |
|----------|--------|---------|------|------------------------|
| `00` | `F0 7E` non-real-time | Bulk Tuning Dump Request | no | — (request) |
| `01` | `F0 7E` non-real-time | Bulk Tuning Dump (reply) | no | no |
| `02` | `F0 7F` real-time | Single Note Tuning Change | no | yes |
| `03` | `F0 7E` non-real-time | Bulk Tuning Dump Request (Bank) | yes | — (request) |
| `04` | `F0 7E` non-real-time | Key-Based Tuning Dump | yes | no |
| `05` | `F0 7E` non-real-time | Scale/Octave Tuning Dump, 1-byte | yes | no |
| `06` | `F0 7E` non-real-time | Scale/Octave Tuning Dump, 2-byte | yes | no |
| `07` | `F0 7F` real-time | Single Note Tuning Change (Bank) | yes | yes |
| `07` | `F0 7E` non-real-time | Single Note Tuning Change (Bank) | yes | no |
| `08` | `F0 7F` real-time | Scale/Octave Tuning, 1-byte form | no | yes |
| `08` | `F0 7E` non-real-time | Scale/Octave Tuning, 1-byte form | no | no |
| `09` | `F0 7F` real-time | Scale/Octave Tuning, 2-byte form | no | yes |
| `09` | `F0 7E` non-real-time | Scale/Octave Tuning, 2-byte form | no | no |

Note that sub-ID#2 `07`, `08` and `09` each denote **two different messages**, distinguished only by
the `7E`/`7F` universal header. Parsers must branch on the header byte, not on sub-ID#2 alone.

The first two messages live in the Universal Non-Real Time area and the third in the Real Time area,
but they deliberately share sub-ID numbering to keep parsing simple.

## Frequency data format (3 bytes)

Used by all key-based messages. Because sysex data bytes carry only 7 bits, three bytes give a
21-bit word:

```
0xxxxxxx 0abcdefg 0hijklmn

xxxxxxx          = semitone: the nearest equal-tempered semitone at or below the frequency
                   (i.e. a MIDI note number, A440 reference)
abcdefghijklmn   = 14-bit fraction of 100 cents above that semitone,
                   in units of 100/2^14 = .0061 cents
```

Range: MIDI note 0 = C = 8.1758 Hz up to `7F 7F 7E` = 13289.73 Hz.
Effective resolution 100/16384 ≈ .0061 cents.

`7F 7F 7F` is reserved and means **no change**, not a frequency. Send it for keys outside the
instrument's range so that receivers do not store bogus data. On reception, leave the stored
frequency for that key untouched.

Examples:

```
00 00 00 =     8.1758 Hz   (C, normal tuning of MIDI key 0)
00 00 01 =     8.2104 Hz
3C 00 00 =   261.6256 Hz   (middle C)
45 00 00 =   440.0000 Hz   (A-440)
45 00 01 =   440.0016 Hz
7F 00 00 = 12543.8800 Hz   (G, normal tuning of MIDI key 127)
7F 7F 7E = 13289.7300 Hz   (top of range)
7F 7F 7F = no change       (reserved)
```

An instrument that does not support the full resolution may discard unneeded low bits on reception,
but storing full resolution internally is preferred so the data can be passed on to instruments that
can use it.

## Checksum calculation

Only the *Dump* messages carry a checksum. Scale/Octave Tuning 1-byte and 2-byte forms
(sub-ID#2 `08`/`09`) and the Single Note Tuning Change messages do **not**.

For all dump messages except sub-ID#2 `01`: XOR every byte of the message excluding `F0`, `F7` and
the checksum field itself, then AND with `7F` to make a 7-bit value.

```c++
uint8_t checksum = 0;
for (size_t i = 1; i < len - 2; ++i)  // skip F0, stop before checksum and F7
    checksum ^= data[i];
checksum &= 0x7F;
```

The original Bulk Tuning Dump (sub-ID#2 `01`) is the exception. Its instructions were ambiguous and
manufacturers implemented it inconsistently, so **receivers are recommended to ignore the checksum in
that message**. When sending sub-ID#2 `01`, compute it the same way as above; that is the most common
interpretation. Prefer sub-ID#2 `04` (Key-Based Tuning Dump) for new work, which has an unambiguous
checksum as well as a bank byte.

## [BULK TUNING DUMP REQUEST]

```
F0 7E <device ID> 08 00 tt F7
```

| Byte | Meaning |
|------|---------|
| `F0 7E` | Universal Non-Real Time SysEx header |
| `<device ID>` | ID of target device |
| `08` | sub-ID#1 (MIDI Tuning) |
| `00` | sub-ID#2 (bulk dump request) |
| `tt` | tuning program number (0–127) |
| `F7` | EOX |

The receiving instrument shall respond with the Bulk Tuning Dump for the requested tuning.

## [BULK TUNING DUMP]

```
F0 7E <device ID> 08 01 tt <tuning name> [xx yy zz] × 128 chksum F7
```

| Byte | Meaning |
|------|---------|
| `tt` | tuning program number (0–127) |
| `<tuning name>` | 16 ASCII characters |
| `[xx yy zz]` | frequency data for one note, repeated 128 times, note 0 first |
| `chksum` | see [checksum calculation](#checksum-calculation) — receivers should ignore it here |

Total message length is 408 bytes. All 128 keys are always sent, in ascending order.
For keys inside the instrument's range, send the pitch the key would actually play if it received a
note-on. For keys outside the range, send `7F 7F 7F`.

## [SINGLE NOTE TUNING CHANGE (REAL-TIME)]

```
F0 7F <device ID> 08 02 tt ll [kk xx yy zz] × ll F7
```

| Byte | Meaning |
|------|---------|
| `F0 7F` | Universal Real Time SysEx header |
| `tt` | tuning program number (0–127) |
| `ll` | number of changes (1 change = 1 set of `[kk xx yy zz]`) |
| `kk` | MIDI key number |
| `[xx yy zz]` | frequency data for that key |

Total length is `8 + (ll × 4)` bytes. Batching several changes into one message saves bandwidth.
This message may target an inactive (background) tuning as well as the current one.

This is the message to use for real-time retuning, e.g. adaptive JI, pitch drift, or dynamically
re-mapping a key while it sounds. The change takes effect immediately and must be artifact-free.

## Changing tuning programs (RPN 03 and 04)

Registered Parameter Number 03 selects the current tuning program; RPN 04 selects the tuning bank.
Instruments that store multiple microtunings shall switch instantly, without notes-off, resets,
re-triggers or glitches, even when notes are sounding.

Tuning Program Select (RPN 03), shown with running status:

```
Bn 64 03 65 00 06 tt      (data entry)
Bn 64 03 65 00 60 7F      (data increment)
Bn 64 03 65 00 61 7F      (data decrement)
```

Tuning Bank Select (RPN 04):

```
Bn 64 04 65 00 06 tt      (data entry)
Bn 64 04 65 00 60 7F      (data increment)
Bn 64 04 65 00 61 7F      (data decrement)
```

`n` is the basic channel number, `tt` the program or bank number (documented as 1–128 in the
specification, transmitted as 0–127).

This tuning Bank Number is deliberately separate from the Program Change Bank Select (CC #00),
though an instrument may link them if a tuning bank is stored alongside a patch bank.
If the instrument has no such program or bank, it should ignore the message.

## Bank/Dump Extensions (CA-020)

These add a bank select byte `bb` (0–127; documented as 1–128) to the existing messages, so that a
tuning message can name the bank it belongs to. RPN 04 could already select a tuning bank, but the
messages had no way to store into one.

### [BULK TUNING DUMP REQUEST (BANK)]

```
F0 7E <device ID> 08 03 bb tt F7
```

The reply is whichever dump (key-based or scale/octave) the instrument supports for that slot.

### [KEY-BASED TUNING DUMP]

```
F0 7E <device ID> 08 04 bb tt <tuning name> [xx yy zz] × 128 chksum F7
```

Identical to the Bulk Tuning Dump plus `bb`. Renamed "key-based" to distinguish it from the
scale/octave dumps. Prefer this over sub-ID#2 `01`.

### [SINGLE NOTE TUNING CHANGE (REAL-TIME) (BANK)]

```
F0 7F <device ID> 08 07 bb tt ll [kk xx yy zz] × ll F7
```

**Will** affect currently sounding notes.

### [SINGLE NOTE TUNING CHANGE (NON REAL-TIME) (BANK)]

```
F0 7E <device ID> 08 07 bb tt ll [kk xx yy zz] × ll F7
```

Same payload, non-real-time header. Will **not** update currently sounding notes. Use this to stage a
tuning change for subsequent notes.

## Scale/Octave Extensions (CA-021 / RP-020)

Scale/octave tuning is micro-tuning that repeats automatically in every octave: it calibrates a single
octave of 12 notes as offsets from equal temperament, rather than defining 128 absolute frequencies.
Far more compact than a key-based dump, and the natural fit for any 12-note-per-octave scale
(meantone, well temperaments, 12-note JI).

Offsets are applied to the **currently selected preset**, not to the modified tuning: each message
offsets from the original preset, so repeated messages do not accumulate. The default assumption is
that the instrument sits at equal temperament.

### Channel bitmap

The three bytes `ff gg hh` let one message update several MIDI channels at once, one bit per channel:

| Byte | Bits | Channels |
|------|------|----------|
| `ff` | 0–1 | channels 15 to 16 |
| `ff` | 2–6 | reserved, **shall be 0** |
| `gg` | 0–6 | channels 8 to 14 |
| `hh` | 0–6 | channels 1 to 7 |

The 5 reserved bits are for future expansion of the tuning messages and shall not be used for
proprietary purposes. Using a channel bitmap in a sysex message is unconventional, but is needed to
change intonation identically on several channels quickly.

### 1-byte form

12 bytes `[ss]`, one per note name C, C#, D … B, in units of 1 cent:

```
00H = -64 cents
40H =   0 cents (equal temperament)
7FH = +63 cents
```

Real-time (updates sounding notes):

```
F0 7F <device ID> 08 08 ff gg hh [ss] × 12 F7
```

Non-real-time (setup message):

```
F0 7E <device ID> 08 08 ff gg hh [ss] × 12 F7
```

### 2-byte form

24 bytes `[ss tt]`, two per note from C to B, 14-bit:

```
00H 00H = -100 cents (8192 steps of .012207 cents)
40H 00H =    0 cents (equal temperament)
7FH 7FH = +100 cents (8191 steps of .012207 cents)
```

Resolution is 200 cents / 16384 = .012207 cents. Minimum and maximum offsets are approximately
±100 cents.

Real-time:

```
F0 7F <device ID> 08 09 ff gg hh [ss tt] × 12 F7
```

Non-real-time:

```
F0 7E <device ID> 08 09 ff gg hh [ss tt] × 12 F7
```

### Scale/octave dumps

The stored, bank-addressed, named counterparts. Both carry a checksum.

```
F0 7E <device ID> 08 05 bb tt <tuning name> [ss] × 12    chksum F7     (1-byte)
F0 7E <device ID> 08 06 bb tt <tuning name> [ss tt] × 12 chksum F7     (2-byte)
```

### RP-020: defaults for scale/octave tuning

If tuning presets are not supported, the initial tuning of the instrument is assumed to be equal
temperament. If presets are supported, the first preset — Bank `0H`, Preset `0H` — should be equal
temperament. Tuning editors should select Bank 0, Preset 0 first in order to start from equal
temperament.

## Master Fine/Coarse Tuning (CA-025)

Two Universal Real Time Device Control messages that shift the tuning of the whole device, like the
pitch control on a tape recorder. Distinct from microtuning proper, but the right message for a global
reference-pitch control (A=442, A=415) and for transposition.

MASTER FINE TUNING:

```
F0 7F <device ID> 04 03 lsb msb F7
```

| Byte | Meaning |
|------|---------|
| `04` | sub-ID#1 (Device Control) |
| `03` | sub-ID#2 (Master Fine Tuning) |
| `lsb msb` | fine tuning value, **LSB first** |

```
lsb msb
00  00   100/8192 * (-8192) = -100 cents
00  40   0 cents
7F  7F   100/8192 * (+8191) ≈ +100 cents
```

MASTER COARSE TUNING:

```
F0 7F <device ID> 04 04 00 msb F7
```

The LSB is always 0.

```
lsb msb
00  00   100 cents * (-64)
00  40   0 cents
00  7F   100 cents * (+63)
```

Total displacement from A440 on a channel is the sum of the Master value (Device Control) and the
corresponding Channel Fine/Coarse Tuning RPN value (RPN 01 and RPN 02, a channel message).
CA-025 renamed those RPNs from "Master Tuning" to "Channel Fine/Coarse Tuning".

For devices with key-based instruments (drum kits in GM1 and DLS1), Master Coarse Tuning shall **not**
result in MIDI note shifting, or a different drum sound would be selected.

## Implementation notes

- The MTS-ESP client library parses **incoming** MTS sysex for you via `MTS_ParseMIDIData` /
  `MTS_ParseMIDIDataU`, and accepts all formats above. It does not construct outgoing messages, so a
  microtuning master has to build the byte streams itself. See [MTS-ESP](mts-esp.md).
- Pick the message class by what changes: scale/octave forms for anything octave-repeating in 12,
  key-based dumps for scales that do not repeat every 12 keys (most EDOs other than 12, non-octave
  periods, large JI mappings), single-note changes for real-time retuning.
- The scale/octave forms cannot express a non-octave period or more than 12 notes per octave.
  Do not try; use a key-based dump.
- A sysex-capable client with no tuning program storage should still accept sub-ID#2 `02`/`07` and
  `08`/`09`, which is the cheapest useful level of support.
- Sysex is bulky: a key-based dump is 408 bytes, roughly 130 ms on a 31.25 kbaud DIN cable. Do not
  send dumps from a UI thread or per note. Prefer single-note changes for live retuning.
- Name every tuning. 16 ASCII characters is all you get; do not send more or fewer.
