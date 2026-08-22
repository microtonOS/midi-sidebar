# Messages

Source: *Complete MIDI 1.0 Detailed Specification*, document version 4.2.1,
revised February 1996, published by the MIDI Manufacturers Association.
References are to its **printed** page and table numbers.

## Channel voice messages

Table II, *Channel Voice Messages*. `n` is the voice channel nibble, and Table
II's note 1 gives it as "Voice Channel number (1-16)" — the nibble 0x0 is
channel 1. Off-by-one here is the commonest MIDI bug.

| status | message | data bytes |
|---|---|---|
| `8n` | Note Off | key, note off velocity |
| `9n` | Note On | key, velocity — **velocity 0 means note off** |
| `An` | Polyphonic Key Pressure (Aftertouch) | key, pressure |
| `Bn` | Control Change | control # **0–119**, value |
| `Cn` | Program Change | program 0–127 |
| `Dn` | Channel Pressure (Aftertouch) | pressure |
| `En` | Pitch Bend Change | **LSB first, then MSB** |

Table II reserves control numbers 120–127 for Channel Mode Messages, so a
control change is 0–119 and nothing else.

**Velocity.** Table II note 3: "key velocity. A logarithmic scale is
recommended." 64 (0x40) is mezzo-forte and is what a device without velocity
sensitivity should send (p10).

**Note numbers.** Middle C is 60, and the spec is careful that this is a
*reference*: it "need not be physically located in the center of a keyboard"
(p10).

**Three ways to end a note**, all of which a receiver must accept (p10):

1. `9n key 00` — Note On, velocity zero. The common one, because it keeps
   running status going.
2. `8n key 40` — a real Note Off, where release velocity is not measured; 64 is
   the recommended value.
3. `8n key vel` — a Note Off with a real release velocity.

**Pitch bend** is 14 bits sent LSB first, and is the one 14-bit message that is
**always sent with both data bytes** — "in contrast to other MIDI functions,
which may send either the LSB or MSB" (p19). So the optional-LSB rule that
governs continuous controllers does not apply here.

Its centre is **8192**, given as data bytes `00 40H`: "the maximum negative swing
is achieved with data byte values of 00, 00. The center (no effect) position is
achieved with data byte values of 00, 64 (00H, 40H). The maximum positive swing
is achieved with data byte values of 127, 127" (p19). Signed, that is
**−8192 to +8191** — not symmetric, because 16384 values cannot sit evenly either
side of a centre, and the spare one falls on the negative side.

Its range in semitones is not part of the message: that is RPN 0, whose
sensitivity "is selected in the receiver" (p19). See [rpn-nrpn](rpn-nrpn.md).

**MSB before LSB.** Table II note 4: send the MSB alone when seven bits are
enough; send MSB then LSB when they are not; and "if only the LSB has changed in
value, the LSB may be sent without re-sending the MSB."

## Channel mode messages

Table IV, *Channel Mode Messages*. Status is `Bn` with a controller number of
120–127, recognised **only on the receiver's Basic Channel**, whatever mode it is
currently in (p7).

| CC | message | value | also does |
|---|---|---|---|
| 120 | All Sound Off | 0 | |
| 121 | Reset All Controllers | 0 | |
| 122 | Local Control | 0 off, 127 on | |
| 123 | All Notes Off | 0 | |
| 124 | Omni Mode Off | 0 | **All Notes Off** |
| 125 | Omni Mode On | 0 | **All Notes Off** |
| 126 | Mono Mode On (Poly Mode Off) | M | **All Notes Off** |
| 127 | Poly Mode On (Mono Mode Off) | 0 | **All Notes Off** |

Two details from Table IV that are easy to lose:

- The four mode messages **also perform All Notes Off**. Changing mode is not a
  quiet operation.
- On CC 126, `M` is "the number of **channels**", not of voices, and `M = 0` is
  special: "the number of channels equals the number of voices in the receiver."

Table IV's own note 2 says "Controller number (121 - 127)", which contradicts the
table listing 120. CC 120 was adopted later than the note (see
[controllers](controllers.md)); the table body is the current reading.

### The four channel modes

Omni and Mono/Poly are independent, so there are four modes (p7). For a receiver
on Basic Channel N:

| mode | omni | | behaviour |
|---|---|---|---|
| 1 | On | Poly | voice messages received from **all** channels, played polyphonically |
| 2 | On | Mono | received from all channels, controlling one voice monophonically |
| 3 | Off | Poly | received on channel N only, played polyphonically |
| 4 | Off | Mono | received on channels N … N+M−1, one voice each |

MPE builds on Mode 3, and optionally works with Mode 4 — see [mpe](mpe.md).
Power-up default is recommended to be Basic Channel 1, Mode 1 (p8).

**Multi Mode** is not a MIDI mode. It names an instrument acting as several
receivers, each with its own Basic Channel and its own mode (p7).

**Global Controllers.** In Mode 4, controllers sent on the channel *one below*
the Basic Channel affect all voices regardless of channel; if the Basic Channel
is 1 the global channel wraps to 16. "Not all receivers may provide this
function" (p12).

## System common messages

Table V. Note that two of the eight are undefined.

| status | message |
|---|---|
| `F1` | MIDI Time Code Quarter Frame |
| `F2` | Song Position Pointer (LSB then MSB) |
| `F3` | Song Select |
| `F4` | **Undefined** |
| `F5` | **Undefined** |
| `F6` | Tune Request |
| `F7` | EOX — end of system exclusive |

## System real time messages

Table VI. Single bytes, and again two are undefined.

| status | message |
|---|---|
| `F8` | Timing Clock |
| `F9` | **Undefined** |
| `FA` | Start |
| `FB` | Continue |
| `FC` | Stop |
| `FD` | **Undefined** |
| `FE` | Active Sensing |
| `FF` | System Reset |

These may appear **between the bytes of another message**, including inside a
system exclusive — Table VII note 2 makes the exception explicit. That is the
detail naive parsers get wrong.

## Running status

A status byte may be omitted when it would repeat the previous one, so a stream
of note-ons can be sent as `9n` followed by key/velocity pairs indefinitely (p5,
and Appendix A-4). This is most of why Note On with velocity 0 exists: note-offs
can then ride the same running status as note-ons.

<!-- Whether System Real Time bytes cancel running status is treated in Appendix
A ("Running Status", A-4), which has not been read. Do not assert either way
from this file. -->
