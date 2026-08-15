# Control change

Source: *Complete MIDI 1.0 Detailed Specification*, document version 4.2.1,
February 1996. The assignment table below is **Table III, Controller Numbers**,
reproduced in full because a partial version is what causes wrong choices.

## The ranges

There are **120** controller numbers, 0 through 119. "Controller 120 was recently
adopted as a Channel Mode Message and is no-longer considered a Control Change"
(p11) — worth knowing when reading old hardware.

| range | classification (p11) |
|---|---|
| 0–31 | MSB of most continuous controller data |
| 32–63 | **LSB for controllers 0–31** |
| 64–95 | additional single-byte controllers |
| 96–101 | increment/decrement and parameter numbers |
| 102–119 | undefined single-byte controllers |
| 120–127 | *not control changes* — see [messages](messages.md#channel-mode-messages) |

## MSB and LSB

**A 14-bit pair is CC *n* and CC *n+32*, and n must be ≤ 31.** This is what
people most often get wrong, because it is tempting to read the LSB as a free
choice of a second controller number. It is not: Table III gives 32–63 as "LSB
for values 0-31", and the text adds that "all controller numbers 64 and above
have single-byte values only, with no corresponding LSB" (p11–12).

Three consequences:

- The LSB may be omitted when 128 steps are enough (Table II note 4).
- Once both have been sent, a fine adjustment needs only the LSB again. A
  *major* adjustment must resend the MSB (p12).
- **On receiving an MSB, a receiver should set its idea of the LSB to zero**
  (p12). Forget this and a fine value from an earlier gesture leaks into the next
  coarse one.

Real hardware does not always comply — see the note in this project's TODO about
the minilogue xd — so a receiver that tolerates a non-conforming pair is being
pragmatic, not wrong.

## Table III in full

| CC | function | LSB |
|---|---|---|
| 0 | Bank Select | 32 |
| 1 | Modulation wheel or lever | 33 |
| 2 | Breath Controller | 34 |
| 3 | *undefined* | 35 |
| 4 | Foot controller | 36 |
| 5 | Portamento time | 37 |
| 6 | **Data entry MSB** | 38 |
| 7 | Channel Volume (formerly Main Volume) | 39 |
| 8 | Balance | 40 |
| 9 | *undefined* | 41 |
| 10 | Pan | 42 |
| 11 | Expression Controller | 43 |
| 12–13 | Effect Control 1, 2 | 44–45 |
| 14–15 | *undefined* | 46–47 |
| 16–19 | General Purpose Controllers 1–4 | 48–51 |
| 20–31 | *undefined* | 52–63 |
| 32–63 | LSB for values 0–31 | — |
| 64 | Damper pedal (sustain) | — |
| 65 | Portamento On/Off | — |
| 66 | Sostenuto | — |
| 67 | Soft pedal | — |
| 68 | Legato Footswitch — 00–3F normal, 40–7F legato | — |
| 69 | Hold 2 | — |
| 70 | Sound Controller 1 (default: Sound Variation) | — |
| 71 | Sound Controller 2 (default: Timbre/Harmonic Intensity) | — |
| 72 | Sound Controller 3 (default: Release Time) | — |
| 73 | Sound Controller 4 (default: Attack Time) | — |
| 74 | Sound Controller 5 (default: Brightness) | — |
| 75–79 | Sound Controllers 6–10 (no defaults) | — |
| 80–83 | General Purpose Controllers 5–8 | — |
| 84 | Portamento Control | — |
| 85–90 | *undefined* | — |
| 91 | Effects 1 Depth (formerly External Effects Depth) | — |
| 92 | Effects 2 Depth (formerly Tremolo Depth) | — |
| 93 | Effects 3 Depth (formerly Chorus Depth) | — |
| 94 | Effects 4 Depth (formerly Celeste/Detune Depth) | — |
| 95 | Effects 5 Depth (formerly Phaser Depth) | — |
| 96 | **Data increment** | — |
| 97 | **Data decrement** | — |
| 98 | **NRPN LSB** | — |
| 99 | **NRPN MSB** | — |
| 100 | **RPN LSB** | — |
| 101 | **RPN MSB** | — |
| 102–119 | *undefined* | — |
| 120–127 | reserved for Channel Mode Messages | — |

The six in bold are one mechanism, not six controls — see
[rpn-nrpn](rpn-nrpn.md).

<!-- CC 88, High Resolution Velocity Prefix, is defined in CA-031 and is NOT in
Table III of the 4.2.1 specification, which lists 85-90 as undefined. Read CA-031
before citing it. Likewise the other CA/RP controller documents. -->

## Values

Transmitters send 0 for minimum and 127 for maximum, and "virtually all
controllers are defined as 0 being no effect and 127 being maximum effect"
(p13). Three are different:

- **Balance** (8): 0 = full volume left/lower, 64 = equal, 127 = full
  right/upper.
- **Pan** (10): 0 = hard left, 64 = centre, 127 = hard right.
- **Expression** (11): "a form of volume accent above the programmed or main
  volume" — a modifier, not a level.

**Switches read at 64.** A receiver expecting a switch treats 0–63 as OFF and
64–127 as ON, "because a receiver has no way of knowing whether the message
information is from a switch or a continuous controller" (p12). This is the
spec's threshold, not an arbitrary midpoint — which is why the sidebar's
`toggle` and `inc` modes trigger there.

## Bank Select

CC 0 and CC 32, and a special case (p13):

- MSB and LSB must be sent **as a pair**, and the Program Change must follow
  **immediately** — a delay risks a merger inserting something between them.
- **Bank Select alone must not change the program.** The receiver remembers the
  bank and applies it when the Program Change arrives, "to assure that multiple
  devices change concurrently".
- The program "need not be changed for a note which is already sounding".
