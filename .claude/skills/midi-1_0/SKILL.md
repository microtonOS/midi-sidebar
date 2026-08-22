---
name: midi-1_0
description: Reference for the MIDI 1.0 protocol — channel voice and channel mode messages, the control change assignments, RPN and NRPN, system exclusive, MPE, and where real devices depart from it. Use when implementing or debugging MIDI 1.0 message handling, when choosing controller numbers, when checking a claim about what MIDI does, or when conformant code fails against real hardware. For microtuning specifically see midi-microtuning; for UMP and the MIDI 2.0 protocol see midi-2_0.
allowed-tools: WebFetch(domain:midi.org) WebFetch(domain:www.midi.org)
---

# MIDI 1.0

A repository of what the specification actually says, so a claim can be checked
rather than remembered. Every non-obvious statement in the references cites a
document, and a table or section within it; where something has **not** been read
from the spec, the file says so rather than guessing.

MIDI 1.0 is not superseded. The MIDI 2.0 environment includes it, UMP carries
MIDI 1.0 protocol messages, and MPE — which is where most expressive controllers
live — is a MIDI 1.0 recommended practice, explicitly "not intended to be an MPE
solution for MIDI 2.0" (*MIDI Polyphonic Expression*, M1-100-UM v1.1, §1.3).

## Sources

| document | version | date |
|---|---|---|
| *Complete MIDI 1.0 Detailed Specification* | 4.2.1 | February 1996 |
| *MIDI Polyphonic Expression* | M1-100-UM v1.1 | 14 April 2022 |
| *MIDI Tuning Updated Specification* | incl. CA-020, CA-021/RP-020 | — |
| *MIDI Implementation Chart V2 Instructions* | RP-004 series | — |

All are published by the MIDI Manufacturers Association / MIDI Association and
AMEI, and are available from midi.org. They are copyrighted and may not be
redistributed; the references here are condensed technical notes with quotations,
not reproductions.

## References

| open | when you are |
|---|---|
| [messages](references/messages.md) | working out what a status byte means — channel voice, channel mode, system common, system real time |
| [controllers](references/controllers.md) | choosing or validating a CC number, or pairing an MSB with an LSB |
| [rpn-nrpn](references/rpn-nrpn.md) | setting pitch-bend sensitivity, MPE zones, or tuning program and bank |
| [sysex](references/sysex.md) | reading or writing system exclusive, including the universal ones |
| [mpe](references/mpe.md) | supporting per-note expression: zones, Manager and Member channels |
| [real-devices](references/real-devices.md) | writing a *receiver*, or wondering why conformant code fails on real hardware |

## The five things most often got wrong

- **An RPN is not a message.** There is no RPN status byte: it is a parameter
  *selected* by control changes 101 and 100, then written by control change 6.
  "RPN 0" is shorthand for four `Bn` bytes on the wire. See
  [rpn-nrpn](references/rpn-nrpn.md#an-rpn-is-not-a-message) — the same is true
  of the MPE Configuration Message.
- **Control Change is CC 0–119.** 120–127 are Channel Mode Messages and are not
  control changes at all. CC 120 moved into that range, so old hardware may still
  send it as a control change (Detailed Specification, Table III and p11).
- **A 14-bit controller pair is CC *n* and CC *n+32*, with n ≤ 31.** Not any two
  numbers. Controllers 64 and above have single-byte values only and no LSB at
  all (Table III; p11–12). See
  [controllers](references/controllers.md#msb-and-lsb) — and then
  [real-devices](references/real-devices.md), because instruments break this rule
  routinely and a receiver that trusts it will not read them.
- **Note On with velocity 0 is a Note Off**, and is the common way to send one.
  A receiver must treat both identically (Table II; p10).
- **Channel Mode Messages are recognised only on the Basic Channel**, whatever
  mode the receiver is currently in — and the four mode messages also perform
  All Notes Off (Table IV; p7).
