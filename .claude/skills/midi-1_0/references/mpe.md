# MPE

Source: *MIDI Polyphonic Expression*, M1-100-UM v1.1, 14 April 2022, published by
the MIDI Association and AMEI. It is a MIDI **1.0** recommended practice: §1.3
says the revision "is not intended to be an MPE solution for MIDI 2.0".

The problem it solves: Pitch Bend and Control Change are *channel* messages, so
they move every note on the channel. MPE gives each note a channel of its own so
that channel messages become per-note messages (§1.2).

## Vocabulary

v1.1 renamed the central term. **Manager Channel**, not master channel (§1.5.1,
Definitions):

- **Manager Channel** — the channel carrying messages that apply to the whole
  zone.
- **Member Channel** — any channel in the zone that is not the Manager Channel.
- **Zone** — a Manager Channel and its Member Channels.
- **Occupied Channel** — a Member Channel with at least one Active Note.
- **MPE Mode** — in force when at least one zone is configured.

## Zones

| zone | Manager | Members |
|---|---|---|
| Lower | channel 1 | 2 upwards |
| Upper | channel 16 | 15 downwards |

A device has a Lower Zone, an Upper Zone, or both (§2.2.1). Members run
contiguously from the Manager, so a zone has no gaps. Channels not in any zone
stay available for conventional use.

Three rules that are easy to miss:

- **No channel belongs to two zones.** A later MCM reassigns any overlap, and a
  zone left with no Member Channels is deactivated.
- **The Manager Channel of an unused zone may be a Member of the other**, so a
  single zone can have up to 15 Member Channels.
- **Changing zones stops notes.** A receiver "shall stop all Sounding Notes and
  reset all controls to reasonable default values on each Channel entering or
  leaving MPE control" (§2.2.3), so that a reconfiguration cannot leave notes
  hanging.

## The MPE Configuration Message

The MCM is **not a message type of its own** — it is Registered Parameter Number
`00 06`, which means three ordinary control changes (§2.2.1, and see
[rpn-nrpn](rpn-nrpn.md#an-rpn-is-not-a-message)). The spec prints it as:

```
[0xBn 0x65 0x00]  [0xBn 0x64 0x06]  [0xBn 0x06 <mm>]
```

which is, byte by byte:

| bytes | control change | meaning |
|---|---|---|
| `Bn 65 00` | CC 101 = 0 | RPN MSB |
| `Bn 64 06` | CC 100 = 6 | RPN LSB — together, parameter `00 06` |
| `Bn 06 mm` | CC 6 = mm | Data Entry MSB — the payload |

One MCM defines one zone, and **which zone is decided by the channel nibble**:

- `n = 0x0` — Lower Zone Manager Channel (channel 1).
- `n = 0xF` — Upper Zone Manager Channel (channel 16).
- "All other values are invalid and should be ignored."

`mm` is the number of Member Channels: `0x0` means MPE off for that zone, and
`0x1`–`0xF` assigns that many. Data Entry **LSB has no function** here — the spec
says so explicitly.

Setting both zones to zero channels deactivates MPE Mode. A sender intending one
zone should send **one** MCM, not two.

## What MPE does about pitch bend

It sets a **default**, which remains editable. Configuring a zone does not take
pitch-bend sensitivity away from you.

On receiving an MCM a receiver shall set Pitch Bend Sensitivity to 2 semitones on
the Manager Channel and 48 on every Member Channel — and, in the same sentence,
"the values may subsequently be changed at any time using Registered Parameter
Number [RPN] 0, in accordance with the MIDI 1.0 Specification" (§2.2.5). The
`shall` governs what happens *on receiving the MCM*, not what may happen
afterwards.

Setting them afterwards is asymmetric (§2.2.5):

- the Manager Channel's value is set by sending RPN 0 to the Manager Channel;
- the Member Channels' value is set by sending RPN 0 to **every Member Channel
  individually**, which the spec recommends because it "improves compatibility
  with all MIDI Devices".

**But the members must all agree**, and this is a `shall`:

> Member Channels within the same Zone **shall not** have different Pitch Bend
> Sensitivity values. A receiver **shall** apply the last Pitch Bend Sensitivity
> message received on any Member Channel to all Member Channels in the Zone.

So sending to every member individually is a *compatibility* measure, not a way
of giving them different ranges. Two consequences worth designing around:

- **A per-member-channel control is wrong.** An interface offering one range per
  member channel lets the end-user build a state the specification forbids. One
  value per zone is the correct model, not a simplification of it.
- **It is per Zone, and there can be two.** A lower and an upper zone may hold
  *different* member sensitivities, so the full picture is up to four numbers:
  lower manager, lower members, upper manager, upper members. A single
  "member" setting quietly assumes one zone.

Two further notes from the same section. MPE devices may limit themselves to a
whole number of semitones between 0 and 96 — and even at 96, 14-bit pitch bend
still resolves finer than 1.2 cents. And RPN 0's LSB carries the microtonal
fraction of the range; the spec recommends MPE devices use whole semitones and
send the LSB as zero, while still *responding* to 14-bit values from other
equipment.

Pitch bend on the two kinds of channel combines: a note's pitch is affected by
the most recent bend on both its Member Channel and the Manager Channel. Manager
bends keep affecting sounding notes after Note Off; Member bends stop affecting a
released note (§2.2.6).

## Two prohibitions

- **Polyphonic Key Pressure shall not be sent on Member Channels** (§2.2.7).
  Channel Pressure is the per-note pressure message in MPE. Polytouch may be sent
  on the Manager Channel "at the discretion of the implementer, to preserve
  compatibility with non-MPE-aware Devices".
- **In MIDI Mode 4, the MIDI 1.0 Global Channel shall not be used** (§2.2.4.2).

## What MPE says about everything else: nothing

Appendix E, Table 5, *MIDI Messages Used on MPE Channels*, is the place to check
before assuming MPE has an opinion. It has a single row reading **"All other RPN
messages"**, marked **O — Optional** on Manager and Member channels, both Tx and
Rx. NRPN is the same.

So MPE mandates RPN 6, gives RPN 0 its own section, and defines **no zone
semantics at all** for the rest. Channel Fine and Coarse Tuning (RPN 01 and 02),
in particular, are neither required nor described — there is no specification
answer to borrow about how they interact with zones, and a design decision there
is genuinely yours to make.

## Modes

Default is MIDI Mode 3, Omni Off / Poly (§2.2.4). In Mode 3 a controller
assigns each new note an unoccupied channel; when notes outnumber channels a new
note shares one, and from that moment control messages affect both — which is the
audible failure mode of a zone that is too small. Mode 4 (Mono) is optional and
suits one-channel-per-string models.
