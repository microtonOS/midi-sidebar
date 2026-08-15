# The MIDI 2.0 protocol

Source: *Universal MIDI Packet (UMP) Format and MIDI 2.0 Protocol*, M2-104-UM
v1.1.2, 27 October 2023.

## The unit of pitch

MIDI 2.0 defines a **100-Cent Unit (HCU)**: "a unit of measure for musical
intervals, corresponding to one-twelfth of an octave measured logarithmically.
This term is preferred over 'semitone' which may refer to various intervals"
(§1.6.1).

For a microtonal application this is the more useful word, and worth adopting: a
*semitone* in a non-12 tuning is whatever the tuning says it is, an HCU is always
100 cents.

## Pitch is decided by a stack, not by one message

§7.4.15 is the section to read. "The MIDI 2.0 Protocol preserves all the tuning
definitions of the MIDI 1.0 Protocol, including Note Number, MIDI Tuning
Standard, Master Tuning RPN 01 and RPN 02, and Pitch Bend. In addition, the MIDI
2.0 Protocol adds new mechanisms for Per-Note Tuning and Pitch control."

Pitch comes from a combination of the following, later entries overriding
earlier ones:

| what | scope | persists? |
|---|---|---|
| Note On with Note Number | default, "only roughly defined" | — |
| **MIDI Tuning Standard** (SysEx) | sets pitch, overriding the default | yes, for subsequent Note Ons |
| **Registered Per-Note Controller #3: Pitch 7.25** | same | yes, for subsequent Note Ons |
| **Note On with Attribute #3: Pitch 7.9** | one note only | no |
| Channel Tuning RPN 01/02, Per-Note Pitch Bend, Pitch Bend | relative offset from whatever the above settled | — |

Two overrides are stated explicitly and are worth knowing in that order:

- Pitch 7.25 "**overrides the pitch set by previous MIDI Tuning Standard (MTS)
  messages**" (§7.4.15.2).
- Pitch 7.9 "overrides the pitch previously set or implied by other mechanisms
  such as Registered Per-Note Controller #3: Pitch 7.25 and the MIDI Tuning
  Standard (MTS) … valid only for the one Note containing the Attribute"
  (§7.4.15.3).

**MTS still works.** §7.4.15.1: "The MIDI 1.0 Protocol and the MIDI 2.0 Protocol
both support the existing MIDI Tuning Standard, which is formatted as a System
Exclusive message." MIDI 2.0 does not replace it; it adds mechanisms that take
priority over it.

## Resolution, and the floor that actually matters

| mechanism | format | resolution |
|---|---|---|
| Registered Per-Note Controller #3 | Q7.25 — 7 bits HCU, **25 bits fraction** | 100/2²⁵ ≈ 0.000003 c |
| Note On Attribute #3 | Q7.9 — 7 bits HCU, **9 bits fraction** | 1/512 HCU ≈ **0.2 c** (the spec's own figure) |
| MTS SysEx, for comparison | 14 bits fraction | 100/2¹⁴ ≈ 0.0061 c |

**The 25 bits are not guaranteed.** §7.4.15.2: "A Receiver … is free to interpret
and respond to any number of bits of tuning resolution that the Receiver can
support. Support for all 25 bits of fractional pitch resolution is not mandated.
However, at least 9 bits should be supported (strongly recommended)."

So the practical floor for MIDI 2.0 per-note pitch is **9 fractional bits, about
0.2 cents** — *coarser* than MTS SysEx — and the ceiling is far finer. A
microtonal application cannot assume either; it can only assume 9 bits and hope
for more.

## Two ways to send a whole tuning

§7.4.15.2 names both uses of Pitch 7.25:

- "A set of these messages for multiple Note Numbers can be used to **define a
  complete tuning table** for any and all 128 Note Numbers."
- It "can also be used to control pitch in real time throughout the life cycle of
  a note."

That is the answer to whether MIDI 2.0 has per-note *tuning* as opposed to
per-note *expression*: it has one mechanism that does both, distinguished only by
how it is used. Unlike MTS it carries no name, program or bank — see the open
question in this project's TODO about naming tunings.

## Addressing

Appendix H, Table 35. MIDI 2.0 Channel Voice Messages (Message Type `0x4`) are
addressed to a **Channel**, within a Group. The four destinations a message can
have are UMP Stream, Function Block, UMP Group, and Channel — there is no fifth
kind and no "extended channel". See [ump](ump.md#groups-and-channels).

MIDI-CI messages address a Function Block, a Group, or a Channel depending on
their source/destination field, which is the machinery for *negotiating* what a
device will do — but it needs a bidirectional connection. See
[midi-ci](midi-ci.md).

<!-- Still to read: §7.4.1-7.4.14 (the per-message bit layouts), §7.5 Flex Data,
§7.9 Mixed Data Set, and Appendix D (MIDI 1.0 <-> 2.0 translation, including the
Min-Center-Max upscaling algorithm, which is what decides how a 7-bit CC becomes
a 32-bit one). -->
