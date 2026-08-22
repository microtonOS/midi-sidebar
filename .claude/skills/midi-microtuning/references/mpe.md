# MPE

MIDI Polyphonic Expression (MPE) is an MMA/AMEI Recommended Practice for communicating
multidimensional control data over MIDI 1.0. It is not a tuning standard, but it is the most widely
supported way to give every sounding note its own pitch, and therefore the fallback microtuning
transport for any client that has no MTS or MTS-ESP support.

Source: [MIDI Polyphonic Expression](https://midi.org/mpe-midi-polyphonic-expression),
document M1-100-UM, v1.1, 14 April 2022. Download requires a (free) MIDI Association account.
Version 1.1 is an editorial update of rp53 v1.0 (2018): no technical design changes.

The MMA/AMEI document is copyrighted and may not be redistributed; what follows is a condensed
technical reference, not a reproduction. Consult the specification for the normative text.

## Terminology

MPE 1.1 renamed the **Master Channel** of rp53 v1.0 to **Manager Channel**. Older documentation,
libraries and JUCE APIs (`MPEZone`, `getMasterChannel()`) still say "master". They are the same thing.

| Term | Definition |
|------|------------|
| **Zone** | Contiguous MIDI channels comprising a Manager Channel and one or more Member Channels |
| **Manager Channel** | Channel reserved for messages applying to the entire Zone |
| **Member Channel** | Any channel in a Zone that is not the Manager Channel; carries notes |
| **Lower Zone** | Manager Channel 1, Member Channels increasing from Channel 2 |
| **Upper Zone** | Manager Channel 16, Member Channels decreasing from Channel 15 |
| **Active Note** | A note whose Note On has been delivered but whose Note Off has not |
| **Released Note** | A note whose Note Off has been delivered; may still sound (release, pedal) |
| **Sounding Note** | Any Active or Released Note that is still sounding |
| **Occupied Channel** | A Member Channel with at least one Active Note |
| **MCM** | MPE Configuration Message |
| **MPE Mode** | Enabled in a controller or synthesizer when at least one MPE Zone is configured |

Channels are numbered 1–16 throughout this file, matching the specification. MIDI status bytes and
most APIs use 0–15.

## Functional overview

Turning MPE on and configuring it uses: MPE Configuration Message, MIDI Mode, Pitch Bend Sensitivity.

Per-note expressive control uses: Note On/Off, Pitch Bend, Channel Pressure, Control Change #74.

Coordination rules:

- The MCM is an RPN that sets the range of channels over which notes are sent and received.
  The channel space may be divided into two sub-spaces called Zones, so multi-timbral playing is
  still possible over one MIDI Transport.
- Each Zone has Member Channels for notes plus a dedicated Manager Channel conveying information
  common to all Active Notes in that Zone.
- Wherever possible, every note is assigned its own Channel for the lifetime of that note. This is
  what allows channel messages to address one note.
- MPE Devices shall default Pitch Bend Sensitivity to **48 semitones on all Member Channels** and
  **2 semitones on the Manager Channel**. Either may be changed to 0–96 semitones using RPN 0.
- Pressure is sent using Channel Pressure. Polyphonic Key Pressure may be used with Active Notes on
  the Manager Channel but **shall not** be used on Member Channels.
- A third dimension of per-note control may be expressed using Control Change #74.

## MPE Configuration Message (MCM)

MPE Mode shall be enabled when at least one MPE Zone is configured. All MPE-compatible Devices shall
support the MCM, in addition to any optional means (power-up default, on-board selection).
Setting both Zones to use no channels deactivates MPE Mode; what the Device does then is up to the
manufacturer.

The MCM is Registered Parameter Number `00 06`. The MSB of Data Entry is the number of MIDI channels
assigned; the LSB of Data Entry has no function.

```
[REGISTERED PARAMETER NUMBER]
CC#101 (MSB)   CC#100 (LSB)   Function
========================================
00             06             MPE Configuration RPN

Message Format: [0xBn 0x65 0x00] [0xBn 0x64 0x06] [0xBn 0x06 <mm>]

  n = MIDI Channel Number:
      n = 0x0    Lower Zone Manager Channel
      n = 0xF    Upper Zone Manager Channel
      All other values are invalid and should be ignored.

  mm = Number of Member MIDI Channels in the Zone:
      mm = 0x0        MPE is Off (no channels)
      mm = 0x1 – 0xF  Assigns that number of channels to the Zone
```

Rules:

- Each MCM defines exactly one Zone, determined by the channel nibble `n`. Two MCMs configure
  two Zones. **If a Sender intends to use only one Zone, it should send one MCM, not two.**
- The Lower Zone is controlled by Manager Channel 1, Member Channels assigned sequentially from
  Channel 2 upwards. The Upper Zone is controlled by Manager Channel 16, Member Channels assigned
  sequentially from Channel 15 downwards. Either order of MCMs is valid.
- Sending an MCM with the Member Channel count set to zero deactivates that Zone.
- The Manager Channel of an unused Zone may be used as a Member Channel for the other Zone.
  So a single Zone may use up to 15 Member Channels (`mm` = `0xF`).
- No MIDI Channel shall be assigned to more than one Zone at a time. When an MCM claims channels
  already belonging to another Zone, the most recent message takes precedence and steals them.
  If that leaves a Zone with no Member Channels, that Zone shall be deactivated.
- The "lower/upper" naming suggests a split keyboard, but Zones may map to a single physical
  controller in any way the manufacturer chooses.

### Examples

Enable the Lower Zone using 15 Member Channels (2–16):

```
[0xB0 0x65 0x00] [0xB0 0x64 0x06] [0xB0 0x06 0x0F]
```

Enable the Lower Zone using 7 Member Channels (2–8) and turn off the Upper Zone:

```
[0xB0 0x65 0x00] [0xB0 0x64 0x06] [0xB0 0x06 0x07]
[0xBF 0x65 0x00] [0xBF 0x64 0x06] [0xBF 0x06 0x00]
```

Enable the Lower Zone using 7 Channels (2–8) and the Upper Zone using 7 Channels (9–15):

```
[0xB0 0x65 0x00] [0xB0 0x64 0x06] [0xB0 0x06 0x07]
[0xBF 0x65 0x00] [0xBF 0x64 0x06] [0xBF 0x06 0x07]
```

Enable the Upper Zone using 15 Member Channels (1–15). Because the Lower Zone is not allocated,
Channel 1 is used as a Member Channel for the Upper Zone:

```
[0xBF 0x65 0x00] [0xBF 0x64 0x06] [0xBF 0x06 0x0F]
```

Overlap: Lower Zone with 7 Channels (2–8), then Upper Zone with 11 Channels (6–15). The later MCM
steals channels 6–8, leaving the Lower Zone with Member Channels 2–5:

> **The channel numbers on this one line look wrong — check them against the PDF
> before relying on them.** An Upper Zone's `mm` Member Channels run `16-mm`
> through `15`, so `mm = 11` should be channels **5–15**, not 6–15, which is only
> ten channels. It would then steal 5–8 and leave the Lower Zone with 2–4.
>
> The other two examples on this page agree with `16-mm … 15`: `mm = 14` is given
> as 2–15 and `mm = 0x0F` is described as reaching channel 1. So the formula is
> not in doubt; either this line or its `0x0B` is a transcription slip. Left as
> transcribed rather than silently corrected, since the source has not been
> re-checked.

```
[0xB0 0x65 0x00] [0xB0 0x64 0x06] [0xB0 0x06 0x07]
[0xBF 0x65 0x00] [0xBF 0x64 0x06] [0xBF 0x06 0x0B]
```

Deactivation by overlap: Lower Zone with 7 Channels (2–8), then Upper Zone with 14 Channels (2–15).
The Lower Zone is left with no Member Channels and is therefore deactivated:

```
[0xB0 0x65 0x00] [0xB0 0x64 0x06] [0xB0 0x06 0x07]
[0xBF 0x65 0x00] [0xBF 0x64 0x06] [0xBF 0x06 0x0E]
```

### MCM strategies

MPE predates MIDI-CI, so bidirectional connections can produce a MIDI feedback loop or an endless
loop of changing MCMs. Strategies to prevent this:

- Ignore a received MCM if your Device already sent one.
- Adapt to a received MCM and change your internal configuration without sending a new MCM to
  confirm it.
- Ignore a received MCM your Device cannot adapt to, and do not send a new MCM trying to force other
  Devices down to your limitations.

## Power-on default behaviour

A Device may be configured to MPE Mode on power-up. If it defaults to MPE Mode, it should be
configured to use the Lower Zone with Member Channels 2–16 in Poly Mode (MIDI Mode 3), which gives a
good initial experience in monotimbral operation. A Device may override this default, for example
because the user defined a program with a different configuration.

## Receiver behaviour when resetting zones

To avoid a sender leaving a receiver with hanging Sounding Notes when Zone configuration changes,
a receiver changing its Zone configuration shall stop all Sounding Notes and reset all controls to
reasonable default values on each Channel entering or leaving MPE control.

## MIDI modes

The default MIDI Mode for MPE Senders and Receivers shall be MIDI Mode 3.

**MIDI Mode 3 ("Poly Mode", Omni Off / Poly).** A Channel is maximally polyphonic. An MPE controller
shall assign every new note its own Channel until no unoccupied Channels remain. An occupied Channel
becomes unoccupied when all its Active Notes have sent or received Note Off. When there are more
notes than unoccupied Channels, a new note shall share a Channel with an existing note; Control
Change and Pitch Bend then affect all Active Notes on that Channel. How that is implemented is up to
the Device.

**MIDI Mode 4 ("Mono Mode", Omni Off / Mono).** Optional. Each Member Channel plays one note at a
time; starting a note on a Channel that is already playing shall stop the older note, possibly with a
legato transition. Ideal for controllers modelling stringed instruments (one channel per string,
hammer-on and pull-off) and for driving a collection of monophonic synthesizers. When MPE is used
with MIDI Mode 4, the MIDI 1.0 Global Channel for Global Controllers shall not be used. MPE Devices
are not required to support MIDI Mode 4.

Senders wishing to switch receivers between Mode 3 and Mode 4 should send the appropriate Mode
Message to the **lowest numbered Member Channel** of a Zone, not to the Manager Channel.

## Pitch Bend Sensitivity

On receiving an MCM, a receiver shall set Manager Channel Pitch Bend Sensitivity to **2 semitones**
and every Member Channel's Pitch Bend Sensitivity to **48 semitones**. Both may be changed at any
time with RPN 0, per MIDI 1.0.

```
RPN 0 (Pitch Bend Sensitivity)
CC#101 (MSB)   CC#100 (LSB)   Function
========================================
00             00             Pitch Bend Sensitivity RPN

Message Format: [0xBn 0x65 0x00] [0xBn 0x64 0x00] [0xBn 0x06 <sensitivity>]

  sensitivity = the +/- range of Pitch Bend in semitones
```

- Manager Channel sensitivity is set by sending RPN 0 to the Manager Channel.
- Member Channel sensitivity is set by sending RPN 0 to **every Member Channel individually**.
  Sending it to all of them improves compatibility with all MIDI Devices.
- Member Channels within the same Zone shall not have different Pitch Bend Sensitivity values.
  A receiver shall apply the last Pitch Bend Sensitivity received on **any** Member Channel to all
  Member Channels in the Zone.
- RPN 0 has an optional LSB for microtonal fractions of a semitone. It is recommended that MPE
  Devices use an integer number of semitones and either transmit the LSB as zero or not transmit it
  at all, so the receiver infers zero. Devices may still respond to 14-bit values for compatibility.
- Devices may limit communication to a whole number of semitones between 0 and 96. At 96 semitones,
  14-bit Pitch Bend granularity is still smaller than 1.2 cents.

Non-MPE gear can often be configured manually to work well with MPE: a typical 16-part multitimbral
synthesizer, or a collection of monosynths spread across channels.

## Pitch Bend

An MPE Device may send Pitch Bend on both the Manager Channel and Member Channels. On the Manager
Channel it is typically a global control (pitch wheel, tremolo bar); on Member Channels it is
typically the movement of a single finger on the playing surface.

- The pitch of a new note is affected by the Pitch Bend most recently received on **both** the
  Manager Channel and that note's Member Channel **before** the Note On. A receiver shall keep
  tracking Pitch Bend on both even when no note is playing.
- Manager Channel Pitch Bend continues to affect all Sounding Notes even after Note Off, including
  notes sustained only by a pedal or a release envelope.
- A Released Note shall **cease** to be affected by Member Channel Pitch Bend once its Note Off
  occurs. Any feature requiring continual Pitch Bend transmission shall send those messages before
  sending the Note Off.
- Where both are received, the Device shall combine them meaningfully and separately for each
  Sounding Note.

### Pitch Bend calculation

The Manager and Member Channels may have different sensitivities. The total Pitch Bend for a note
should be the sum of the two. Because Pitch Bend may span multiple semitones, it should be linear
across the sensitivity range.

Senders (note the deliberate asymmetry with the receiver equations: the neutral value 8192 makes the
upward range one step shorter than the downward range):

```
pbValMem = min(round(pbMem * 8192 / pbSenseMember) + 8192, 16383)
pbValMan = min(round(pbMan * 8192 / pbSenseManager) + 8192, 16383)
```

Sender example, Member sensitivity 48 and Manager sensitivity 2, Member bend +7 semitones and
Manager bend +2 semitones:

```
pbValMem = min(round(7 * 8192 / 48) + 8192, 16383) = 9387
pbValMan = min(round(2 * 8192 /  2) + 8192, 16383) = 16383
```

Receivers:

```
pbMan   = pbSenseMan    * ((pbValMan - 8192) / 8191)
pbMem   = pbSenseMember * ((pbValMem - 8192) / 8191)
pbTotal = pbMan + pbMem
```

`pbMan` and `pbMem` should be stateful, so `pbTotal` is always the sum of the most recent values.

Receiver example with the values above:

```
pbMan   =  2 * ((16383 - 8192) / 8191) = 2 semitones
pbMem   = 48 * (( 9387 - 8192) / 8191) = 7 semitones
pbTotal = 9 semitones
```

## Channel Pressure and Polyphonic Key Pressure

An MPE Device may send Channel Pressure on both the Manager Channel and Member Channels.
**Polyphonic Key Pressure shall not be sent on Member Channels**; it may be sent for notes on the
Manager Channel at the implementer's discretion, to preserve compatibility with non-MPE-aware
Devices.

A new note is affected by the Channel Pressure most recently received on its Channel before Note On,
which also influences the note's initial state; a receiver shall keep tracking Channel Pressure even
when no note is playing. The note ceases to be affected by Channel Pressure on its Channel after
Note Off. All MPE receivers shall respond to Channel Pressure on both the Manager Channel and each
Member Channel, combining them meaningfully and separately per Sounding Note.

## Control Change #74

The optional third dimension of continuous control, typically finger movement along the length of a
key, mapped to CC #74. The same before-Note-On, keep-tracking, stop-at-Note-Off rules as Channel
Pressure apply, and all MPE receivers shall respond to it on both Manager and Member Channels.

Two transmission schemes:

- **Absolute**: the value at Note On encodes the initial position of the performer's interaction.
  A key played at 75 % from the bottom sends `0x5F` (95), and subsequent motion sends `0x00`–`0x7F`
  proportional to the height of the key.
- **Relative**: the value at Note On is always `0x40` (64) regardless of initial position, and
  subsequent motion sends `0x00`–`0x7F` proportional to the position of the initial touch.

Either scheme, or a mix, is up to the manufacturer.

## Manager Channel and Member Channel messages

An MPE Zone normally represents one polyphonic instrument, so messages such as Damper Pedal are
expected to affect all Sounding Notes across the whole Zone. To reduce MIDI traffic and make event
editing easier, those messages should be sent **only** on a Zone's Manager Channel; if an MPE Device
receives one on a Member Channel, it should ignore it.

Pitch Bend, Channel Pressure and CC #74 are the exceptions: they are both Manager Channel and Member
Channel messages, and a receiver getting both shall combine them meaningfully.

**Program Change** is a special case. In MIDI Mode 3 it shall be applied only at the Manager Channel,
so a Zone plays monotimbrally; a Mode 3 receiver shall ignore Program Change on Member Channels.
In MIDI Mode 4, Program Change may be sent on Member Channels (e.g. a different program per string),
and a Mode 4 receiver should apply Manager and Member Program Changes in the order received.

### MIDI messages used on MPE channels

M = Mandatory, O = Optional, P = Prohibited.

| MIDI Message or Feature | Mgr Tx | Mgr Rx | Mem Tx | Mem Rx |
|-------------------------|--------|--------|--------|--------|
| RPN #6 [MPE Configuration Message] | M | M | P | P |
| RPN #0 [Pitch Bend Sensitivity] | O | M | O | M |
| Pitch Bend | O | M | O | M |
| Channel Pressure | O | M | O | M |
| Control Change #74 [Brightness] | O | M | O | M |
| Polyphonic Key Pressure | O | O | P | P |
| CC #120 [All Sounds Off] | P | P | O | O |
| CC #121 [Reset All Controllers] | O | O | O | O |
| CC #122 [Local Control] | O | O | O | O |
| CC #123 [All Notes Off] | O | O | O | O |
| CC #124 [Omni Off] | O | O | O | O |
| CC #125 [Omni On] | P | P | P | P |
| CC #126 [Mono Mode On] | P | P | O | O |
| CC #127 [Poly Mode On] | P | P | O | O |
| All other Control Change / RPN / NRPN | O | O | O | O |
| Program Change | O | O | P/O | P/O |
| Bank Select CC #0 and CC #32 | O | O | O | O |
| Note On/Off messages | O | M | M | M |
| System Common / Real Time / Exclusive | O | O | O | O |

CC #126 and #127 go to the lowest Member Channel. Program Change on Member Channels is prohibited in
MIDI Mode 3 and optional in MIDI Mode 4.

## Control messages and Note On/Off ordering

Senders using MPE control messages (Pitch Bend, Channel Pressure, CC #74) **should send initial
values for these controls before a Note On message**. The order in which the controllers are sent
does not matter. If the Sender does not do this, the Receiver will play notes with its own current
values, which may not match user intention. For Receivers, MPE control messages shall not affect a
note after its Note Off has been received.

This ordering rule is what makes MPE usable as a microtuning transport: the per-note pitch offset has
to be in place before the note starts, or the note glides into tune.

### Note On setup example

To play a note one quarter tone above middle C with an initial timbre value of 64, using Channel 3:

| Seq | MIDI Bytes | Description | Effect |
|-----|------------|-------------|--------|
| 1 | `0xE2 0x2B 0x41` | Pitch Bend | Quartertone bend upwards, assuming sensitivity is 48 semitones |
| 2 | `0xB2 0x4A 0x40` | Control Change | CC #74 = `0x40` (64 decimal) |
| 3 | `0xD2 0x00` | Channel Pressure | Set to zero |
| 4 | `0x92 0x3C 0x38` | Note On | Note = Middle C, velocity = `0x38` (56 decimal) |

## Allocation of notes to Member Channels

An MPE Sender determines the allocation of each note to a Channel. Simple circular assignment will
not usually give satisfactory results. In the simplest workable implementation, a new note is
assigned to the Channel with the lowest count of Active Notes; all else equal, prefer the Channel
whose last Note Off is oldest.

Controllers can preferentially re-use the Channel most recently used for a given Note Number once the
previous note has entered its Note Off state. This avoids stacking and chorusing identical notes,
which sounds bad in monotimbral applications and affects synthesizers not designed for MPE.

In particular circumstances the same Note Number on two different Channels is appropriate: a note may
start at one pitch and be bent to another before a second note is initiated at the original pitch, or
a guitar-type controller may permit the same pitch on different strings.

When all Channels are occupied, a controller may choose the Channel where the pitch change for the
new note requires the smallest pitch adjustment for other playing notes. Alternatively it may degrade
gently by switching to a mode where notes step discretely from pitch to pitch, letting Pitch Bend
respond only to small vibrato gestures.

## Compatibility and sound quality

Making the MPE workflow transparent presents three challenges:

- **Note editing across channels.** Editing MPE sequences should handle notes across multiple
  Channels without making the user work out Channel assignment per note. In Poly Mode, originating
  Channel numbers do not have to be preserved during editing; Member Channels may be dynamically
  reassigned during playback or retransmission.
- **Mono Mode and standard MIDI behaviour** still requires preserving Channel numbers, which means a
  far more sophisticated note model: a note is no longer a pair of time-stamped Note On/Off messages,
  it is an entity with its own timeline of multidimensional control data that moves across time and
  channel space along with the note.
- **Controller message and note state.** Values for Pitch Bend, Channel Pressure, CC #74 and all
  other control messages from both Manager and Member Channels should be tracked and stored even when
  no note is sounding, to provide an initial state for any future note. To allow rapid reuse of
  unoccupied Member Channels, per-note control should stop after Note Off, regardless of whether
  notes are kept active by a damper pedal or long release envelopes.

**Channel Pressure.** Many synthesizers are designed for controllers that set Channel Pressure to
zero before a note terminates. For full compatibility, Channel Pressure should be set to zero
immediately before a Note On or a Note Off wherever appropriate to the controller design. Not all
controllers can behave this way — simulated hand drums adjust skin pressure independently of note
creation.

**Zones.** Many MPE Devices support only one Zone. Using the Lower Zone by default gives the widest
interoperability. Where interface design permits, an instrument should display the currently selected
Manager Channel and the range of Member Channels.

**Combining Manager and Member values** for Channel Pressure and CC #74 is left to the manufacturer.
Options: **Add** (Manager value acts as a bias or offset, e.g. filter cutoff), **Max** (Manager value
offsets the Sounding Note's current value, e.g. volume swell), or **Contention** — the last is not
recommended.

## Using MPE for microtuning

- Every note gets a Member Channel, and each Member Channel has its own Pitch Bend, so an MPE sender
  can play any tuning into any MPE-capable synth without that synth knowing anything about tuning.
  This is the universal fallback when a client supports neither MTS nor MTS-ESP.
- Send the Pitch Bend for the note's tuning offset **before** the Note On (see the ordering rule
  above), or the note glides into tune.
- **Resolution depends on Pitch Bend Sensitivity.** At the MPE default of 48 semitones, one Pitch
  Bend step is 4800/8192 ≈ 0.586 cents, which is audible on sustained just intervals. If the tuning
  offsets are small — a temperament, a scale near 12-EDO — send RPN 0 to every Member Channel to
  reduce sensitivity: at ±2 semitones a step is 200/8192 ≈ 0.024 cents. Trade this off against how
  much pitch range the tuning and any performance gestures actually need, and remember the receiver
  applies the last RPN 0 seen on any Member Channel to the whole Zone.
- Sensitivity is per Zone, not per note, so the tuning offsets and any expressive bend share one
  budget.
- Polyphony is capped at the number of Member Channels, at most 15. Beyond that, notes share a
  Channel and the shared Pitch Bend detunes them together — audible as wrong pitches, not just
  wrong expression. A microtuning master should treat "more simultaneous notes than Member Channels"
  as a tuning failure, not merely a voice-stealing decision.
- MPE and MTS-ESP overlap awkwardly on the multichannel question: if a plug-in supports MPE and has a
  switch for it, ODDSound recommend **not** supplying a MIDI channel to MTS-ESP queries while MPE is
  enabled, since the MPE channel identifies a voice, not a manual. See [MTS-ESP](mts-esp.md) step 6.
- MPE 1.1 explicitly is not an MPE solution for MIDI 2.0; MIDI 2.0 has per-note pitch of its own.
  See [MIDI 2.0](midi-2_0.md).
