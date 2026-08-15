---
name: midi-2_0
description: Reference for MIDI 2.0 — the Universal MIDI Packet, groups and channels, the MIDI 2.0 protocol's higher resolution and per-note controllers, and MIDI-CI. Use when working out how MIDI 2.0 differs from MIDI 1.0, when a claim about groups, channels or resolution needs checking, or when deciding what a plugin should do about MIDI 2.0. For MIDI 1.0 message handling see midi-1_0; for microtuning see midi-microtuning.
allowed-tools: WebFetch(domain:midi.org) WebFetch(domain:www.midi.org)
---

# MIDI 2.0

A repository of what the specifications say, so a claim can be checked. Every
non-obvious statement cites a document and a section within it; where something
has **not** been read from the spec, the file says so rather than guessing.

## Sources

| document | version | date |
|---|---|---|
| *Universal MIDI Packet (UMP) Format and MIDI 2.0 Protocol* | M2-104-UM v1.1.2 | 27 October 2023 |
| *MIDI Capability Inquiry (MIDI-CI)* | M2-101-UM v1.2 | — |
| *MIDI Clip File Specification* | M2-116-U v1.0 | — |
| *MIDI Implementation Chart V2 Instructions* | — | — |

Published by the MIDI Association and AMEI, available from midi.org. Copyrighted
and not redistributable; what follows is condensed technical notes with
quotations, not a reproduction.

## What MIDI 2.0 is not

Three misunderstandings worth clearing first, because each of them has appeared
in this project's own documentation:

- **It does not replace MIDI 1.0.** "MIDI 2.0: The MIDI environment that
  encompasses all of MIDI 1.0, MIDI-CI, Universal MIDI Packet (UMP), MIDI 2.0
  Protocol, MIDI 2.0 messages, and other extensions" (UMP & MIDI 2.0 Protocol
  v1.1.2, §1.6.1, p16). MIDI 1.0 protocol messages are carried inside UMP.
- **There are no "extended channels".** UMP addresses **16 Groups**, each with
  its own 16 channels — 256 in all, and none of them reserved for older devices.
  "Extended channels" is MIDI *1.0* language for multi-port devices, whose ports
  are named A1–A16, B1–B16 (MIDI Implementation Chart V2, p1).
- **MPE is not the MIDI 2.0 answer to per-note control.** MPE v1.1 says so
  itself (§1.3). MIDI 2.0 has per-note controllers natively; MPE remains a MIDI
  1.0 practice.
- **It does not retire the MIDI Tuning Standard.** "The MIDI 1.0 Protocol and
  the MIDI 2.0 Protocol both support the existing MIDI Tuning Standard" (UMP
  §7.4.15.1). MIDI 2.0 adds per-note pitch mechanisms that *override* MTS when
  present, which is not the same as replacing it.

## References

| open | when you are |
|---|---|
| [ump](references/ump.md) | working out how a message is addressed — groups, channels, packet sizes |
| [protocol](references/protocol.md) | comparing resolution or reaching for a per-note controller |
| [midi-ci](references/midi-ci.md) | negotiating capabilities, profiles or property exchange |

## Vocabulary

From §1.6.1 (p16–17), because several of these words mean something narrower
than they look:

- **Group** — a field in a UMP addressing the message to one of 16 Groups.
- **Function Block** — one logical entity on a device, operating on a set of one
  or more Groups.
- **UMP Endpoint** — a MIDI Endpoint using the UMP format.
- **Protocol** — there are exactly two, the MIDI 1.0 Protocol and the MIDI 2.0
  Protocol, and UMP can carry either.
- **Profile** — an agreed set of messages and responses for a kind of instrument,
  configured over MIDI-CI.
- **100-Cent Unit (HCU)** — MIDI 2.0's own name for a twelfth of an octave. The
  spec prefers it to "semitone", "which may refer to various intervals" (p16) —
  a distinction a microtonal application should appreciate.

## Reading the specs

`tmp/midi/` in the midi-sidebar project holds `M2-104-UM` (UMP and protocol),
`M2-101-UM` (MIDI-CI) and `M2-116-U` (Clip File). The UMP spec's printed page
numbers match its PDF pages.
