# MIDI 2.0

MIDI 2.0 carries microtuning natively as **per-note pitch**, at far higher resolution than anything in
MIDI 1.0 and without sysex, tuning tables or channel juggling. It is the cleanest target of all the
standards here, and the least widely supported.

Sources:
- [UMP and MIDI 2.0 Protocol Specification](https://midi.org/universal-midi-packet-ump-and-midi-2-0-protocol-specification),
  document M2-104-UM, v1.1.2, 27 October 2023. Section numbers below refer to it.
- [MIDI-CI Specification](https://midi.org/midi-ci), document M2-101-UM — how two devices negotiate
  the MIDI 2.0 Protocol in the first place.

Download requires a (free) MIDI Association account. The documents are copyrighted and may not be
redistributed; what follows is a condensed technical reference, not a reproduction.

## Terminology

**100-Cent Unit (HCU)** — a unit of measure for musical intervals, corresponding to one twelfth of an
octave measured logarithmically. The spec prefers this term over "semitone", which may refer to
various intervals. All MIDI 2.0 pitch fields are in HCUs.

**UMP (Universal MIDI Packet)** — the data container for all MIDI 1.0 and MIDI 2.0 messages.
Each UMP is one, two, three or four 32-bit words.

**Group** — a 4-bit field addressing a UMP message to one of 16 Groups, each with its own independent
set of 16 MIDI Channels. So a UMP connection carries up to 256 channels. Group field values 0–15 are
presented to users as Groups 1–16.

**Profile** — an MA/AMEI specification defining a set of messages and responses, configured over
MIDI-CI.

## Where microtuning lives

MIDI 2.0 preserves all the tuning definitions of MIDI 1.0 — Note Number, MIDI Tuning Standard,
Master Tuning RPN 01 and RPN 02, and Pitch Bend — and adds new per-note mechanisms.
Pitch of a note is determined by any combination of the following, some of which override others
(§7.4.15):

- **Set the Default Pitch**, as in MIDI 1.0 (pitch only roughly defined):
  - Note On with Note Number
- **Set Pitch (override Default) with persistent state for subsequent Note Ons:**
  - MIDI Tuning Standard
  - Registered Per-Note Controller #3: Pitch 7.25
- **Set Pitch (override Default) for one note only:**
  - Note On With Attribute #3: Pitch 7.9
- **Modify Pitch relatively from any existing pitch state:**
  - Channel Tuning RPN 01 and RPN 02
  - Per-Note Pitch Bend
  - Pitch Bend

Other messages, or mechanisms defined by MMA/AMEI in future revisions, MIDI-CI Profile
specifications, Articulation Types or other expansions, might also determine pitch.

**Choose Pitch 7.25 or Pitch 7.9 as the primary tuning mechanism**, and treat MTS sysex as the MIDI
1.0 fallback path rather than something to send in parallel — Pitch 7.25 overrides MTS, so sending
both means the MTS table is silently ignored on any MIDI 2.0 receiver.

## Message format

All MIDI 2.0 Channel Voice Messages are 64-bit (two 32-bit words), Message Type `0x4`:

- 4 bits Message Type, value `0x4`
- 4 bits Group
- 8 bits Status: a 4-bit opcode and a 4-bit Channel number
- 16 bits Index
- 32 bits Data

```
word 0:  | mt=4 | group | status(opcode|channel) |        index        |
word 1:  |                          data                               |
```

A device that uses MIDI 2.0 Channel Voice Messages (MT `0x4`) in a Group **shall not** use MIDI 1.0
Channel Voice Messages (MT `0x2`) within that same Group.

Bits marked `r` are reserved, shall be set to zero, and shall not be used for any purpose. Receivers
shall not depend on reserved fields containing zero.

### Message Type allocation

| MT | UMP size | Description |
|----|----------|-------------|
| `0x0` | 32 bits | Utility Messages |
| `0x1` | 32 bits | System Real Time and System Common (except System Exclusive) |
| `0x2` | 32 bits | MIDI 1.0 Channel Voice Messages |
| `0x3` | 64 bits | Data Messages (including System Exclusive) |
| `0x4` | 64 bits | MIDI 2.0 Channel Voice Messages |

## Registered Per-Note Controller #3: Pitch 7.25

The primary mechanism for continuous per-note tuning. Opcode `0000`.

```
word 0:  | mt=4 | group | 0 0 0 0 | channel | r | note number | controller index |
word 1:  |                        data (32 bits)                                  |
```

For Pitch 7.25 the controller index is `3` and the 32-bit data field contains:

- 7 bits: Pitch in HCUs, based on the default Note Number equal temperament scale
- 25 bits: Fractional Pitch above Note Number (a fraction of one HCU)

Pitch is a Q7.25 fixed-point unsigned integer specifying a pitch in HCUs. The integer part is
interpreted as the pitch implied by the MIDI 1.0 Note Number in a 12-tone equal tempered scale with
A=440 (note number 69, `0x45`). The fractional part is a fraction of one HCU.

A receiver may respond to any number of bits of tuning resolution it can support; all 25 bits are not
mandated, but **at least 9 bits should be supported (strongly recommended)**.

Pitch Bend and Per-Note Pitch Bend act as offsets from the pitch set by this controller.

**Important:** the pitch set here overrides the pitch set by previous MTS messages. Controllers create
persistent state, so all following notes use this tuning unless they carry other tuning information
in the Note On.

Two typical uses:

- Define a complete tuning table: send one message per Note Number, for any or all 128 Note Numbers.
- Control pitch in real time throughout the life cycle of a note.

Registered Per-Note Controller numbers with no definition are Reserved and shall not be used.
The defined ones relevant here:

| RPNC | Name | Default |
|------|------|---------|
| 1 | Modulation | — |
| 2 | Breath | — |
| **3** | **Pitch 7.25** | — |
| 7 | Volume | — |
| 8 | Balance | — |
| 10 | Pan | — |
| 11 | Expression | — |
| 70–79 | Sound Controller 1–10 | Sound Variation, Timbre/Harmonic Intensity, Release Time, Attack Time, Brightness, Decay Time, Vibrato Rate, Vibrato Depth, Vibrato Delay, undefined |
| 91, 93 | Effects 1 and 3 Depth | Reverb Send Level, Chorus Send Level |

Assignable Per-Note Controllers (opcode `0001`, same layout) have no predefined function.

Per-Note Controllers and Per-Note Pitch Bend are **not** reset by Reset All Controllers (CC #121).

## Note On With Attribute #3: Pitch 7.9

The one-shot alternative: pitch fixed for the life of the note, carried in the Note On itself.
Note On opcode `1001`, Note Off `1000`.

```
word 0:  | mt=4 | group | 1 0 0 1 | channel | r | note number | attribute type |
word 1:  |        velocity (16 bits)        |    attribute value (16 bits)      |
```

Defined Attribute Types:

| Attribute Type | Definition | Notes |
|----------------|------------|-------|
| `0x00` | No Attribute Data | Sender shall set Attribute Value to `0x0000`; Receiver shall ignore it |
| `0x01` | Manufacturer Specific | Interpretation determined by manufacturer |
| `0x02` | Profile Specific | Interpretation determined by the MIDI-CI Profile in use |
| `0x03` | Pitch 7.9 | See below |
| `0x04`–`0xFF` | Reserved | Do not use |

With Attribute Type `0x03`, the 16-bit Attribute Value contains:

- 7 bits: Pitch in HCUs, based on the default Note Number equal temperament scale
- 9 bits: Fractional pitch above Note Number (a fraction of one HCU)

**When using this Attribute Type, the Note Number should be treated as a note index only; it does not
imply any scale or pitch.** Attribute Pitch is a Q7.9 fixed-point unsigned integer specifying a pitch
in HCUs, with the integer part interpreted as in Pitch 7.25. Resolution is 1/512 HCU, an accuracy of
approximately **0.2 cents**.

Pitch Bend and Per-Note Pitch Bend act as offsets from Attribute #3: Pitch 7.9.

**Important:** this overrides pitch previously set or implied by other mechanisms including Registered
Per-Note Controller #3: Pitch 7.25 and MTS. The override is valid only for the one note carrying the
attribute; it does not apply to subsequent notes.

Velocity range for a MIDI 2.0 Note On is `0x0000`–`0xFFFF`; unlike MIDI 1.0, velocity zero does **not**
function as a Note Off. When translating to MIDI 1.0, a translator shall replace a translated
velocity of zero with 1.

Receivers that select samples based on Note Number might choose instead to select samples based on
the first 7 bits of the pitch data in the last valid Pitch 7.25 or in the Note On Attribute Pitch 7.9.

### Choosing between the two

| | Pitch 7.25 (PNCC#3) | Pitch 7.9 (Attribute) |
|---|---|---|
| Resolution | 25 fractional bits, ~0.0000003 cents | 9 fractional bits, ~0.2 cents |
| Scope | Persistent state, applies to subsequent notes | This note only |
| Continuous change | Yes — send during the note | No |
| Cost | An extra message before each Note On | Free, rides in the Note On |
| Use when | The controller gives continuous pitch control for the whole note life cycle | Pitch is constant for the note's life cycle, and you don't need the Attribute field for anything else |

Both methods are standard; §C.3 says receivers shall respond to both.

## Per-Note Pitch Bend

Opcode `0110`. Acts like Pitch Bend in every way except that it applies to individual Note Numbers.
The data field is an unsigned bipolar value **centred at `0x80000000`**.

```
word 0:  | mt=4 | group | 0 1 1 0 | channel | r | note number | reserved |
word 1:  |                        data (32 bits)                         |
```

### RPN #00/07: Sensitivity of Per-Note Pitch Bend

Registered Controller Bank 0, Index 7 sets the controllable pitch range up and down from the current
sounding pitch of a note. **The sensitivity is common to all Note Numbers on the selected MIDI
Channel** — "Per-Note" in the name refers to the Per-Note Pitch Bend, not the sensitivity.

```
word 0:  | mt=4 | group | 0 0 1 0 | channel | r 0000000 (bank=0) | r 0000111 (index=7) |
word 1:  |  units of 100 cents  |          fraction of 100 cents                        |
```

The value is a **7.25 fixed-point unsigned value** in units of 100 cents: integer part = number of
100-cent units, fractional part = fraction of 100 cents. This matches the data format of Pitch 7.25,
and differs from RPN #00/00 (Channel Pitch Bend Sensitivity), whose MSB is 7 bits of HCU and LSB is a
value in cents between 0 and 99.

Sensitivity sets the range of bend down (Per-Note Pitch Bend `0x80000000` → `0x00000000`) and an equal
range up (`0x80000000` → `0xFFFFFFFF`).

- **Supported resolution:** 32 bits is more than most devices need. A receiver shall recognise the
  7 bits of integer precision, subject to the supported range, and may respond to any number of
  fractional bits it supports (e.g. 7 integer + 8 fractional = 100/256 cents).
- **Supported range:** up to almost ±12800 cents. Devices may support a subset, e.g. `0x00000000`
  to `0x0C000000` (±0 to ±1200 cents).
- This Registered Controller translates to an equivalent RPN in MIDI 1.0, but **that RPN has no
  function within the MIDI 1.0 Protocol**. Per-note bend does not survive translation.

**Per-note range per Note Number** (§7.4.13.4, informational): sensitivity is equal for all Note
Numbers, but if you need a different amount per note, set the sensitivity to what the widest-range
note requires, then for notes needing less use only a subset of the available Per-Note Pitch Bend
values. The 32-bit resolution is ample for smooth pitch change even across a subset.

## Per-Note Management Message

Opcode `1111`. Enables independent control from Per-Note Controllers to multiple notes on the same
Note Number — the mechanism that makes note-number rotation work.

```
word 0:  | mt=4 | group | 1 1 1 1 | channel | r | note number | option flags | D | S |
word 1:  |                          reserved                                        |
```

Option flags, active when set high:

- **D: Detach Per-Note Controllers** from previously received note(s). All currently playing and
  previous notes on the referenced Note Number shall no longer respond to any Per-Note Controllers.
  Currently playing notes maintain their current Per-Note Controller values until end of life cycle.
- **S: Reset (Set) Per-Note Controllers** to default values on the referenced Note Number.

With D=1 and S=1, the device should process Detach first, then Reset: currently playing notes on that
Note Number keep their values for the rest of their life cycle, and the defaults plus any further
changes apply to future notes only. D=0 and S=0 has no defined function.

These default responses apply to all Per-Note Controllers; future MMA/AMEI specifications or Profiles
might define other responses for specific Per-Note Controllers.

## Tuning-related Registered Controllers

MIDI 2.0 replaces MIDI 1.0's compound RPN/NRPN CC sequences with single unified messages.
Registered Controller opcode is `0010`; the index field is split into a 7-bit bank (RPN MSB) and a
7-bit index (RPN LSB). There are 128 banks × 128 controllers.

```
word 0:  | mt=4 | group | 0 0 1 0 | channel | r | bank | r | index |
word 1:  |                        data (32 bits)                   |
```

| Bank/Index | Name | Data layout |
|------------|------|-------------|
| `0x00`/`0x00` | Pitch Bend Range | HCUs in the most significant 7 bits, cents in the next 7 bits; least significant 18 bits undefined and shall be ignored |
| `0x00`/`0x02` | Coarse Tuning | Coarse tuning in the most significant 7 bits; least significant 25 bits undefined |
| `0x00`/`0x03` | Tuning Program Change | Tuning program number in the most significant 7 bits; least significant 25 bits undefined |
| `0x00`/`0x04` | Tuning Bank Select | Tuning bank number in the most significant 7 bits; least significant 25 bits undefined |
| `0x00`/`0x06` | MPE MCM | Number of channels in the most significant 7 bits; least significant 25 bits undefined |
| `0x00`/`0x07` | Sensitivity of Per-Note Pitch Bend | 7.25 fixed point in units of 100 cents (see above) |

So the MTS tuning program and bank selects (RPN 03/04 — see [MTS Sysex](mts-sysex.md)) and the MPE
configuration message (RPN 6 — see [MPE](mpe.md)) both exist in MIDI 2.0 as single messages.

Devices sending MIDI 2.0 **should not** transmit Control Change messages with indexes 0, 6, 32, 38,
98, 99, 100 or 101; use the Registered/Assignable Controller and Program Change messages instead.
Devices receiving MIDI 2.0 should ignore Control Change with those indexes.

Relative Registered Controller (opcode `0100`) and Relative Assignable Controller (`0101`) messages
act on the same address space with a two's complement data value for relative increase or decrease.
They **cannot** be translated to MIDI 1.0.

## Note number rotation: replacing MPE's channel rotation

§C.3 is the key section for a microtuning implementer, and the argument for preferring MIDI 2.0.

MPE uses a **channel** rotation mechanism to get flexible per-note control, capped at 16 notes of
polyphony. In MIDI 2.0, a **note number** rotation mechanism can replace it: this improves on MPE by
using only a single MIDI Channel while providing polyphony of up to 128 notes.

The sender plays notes with added Pitch data. The added Pitch data overrides any notion of pitch
implied by the Note Number field in Note On, Note Off and Per-Note Controllers. **Note Number loses
any implication of pitch and functions only as a note index.**

The sender assigns a Note Number to each note in a rotating fashion. It might use the same value for
Note Number as in the Pitch data whenever feasible, to serve translation to MIDI 1.0; or rotate
through all 128 Note Numbers least-recently-used to more robustly avoid Per-Note Controller overlap;
or any other scheme.

Because Note Numbers are reused for notes of various pitch, **the sender sends a Per-Note Management
message before every Note On** to guarantee a new note adopts no state from controllers previously
addressed to that Note Number.

Receivers do not need to know a rotation scheme is in use. They shall respond to both standard
methods of Pitch control, and shall also implement the Per-Note Management message.

### Method 1: sender using Pitch 7.25

For performance interfaces giving continuous pitch control over every note for the note's whole life
cycle. Two successive notes both playing a Middle C:

```
Per-Note Management @Note Number 00
PNCC#3 @Note Number 00 Set Pitch 60.0
Note On #00 (Pitch sounds as 60.0)
Several other Per-Note Controllers @Note Number 00
Note Off #00

Per-Note Management @Note Number 01
PNCC#3 @Note Number 01 Set Pitch 60.0
Note On #01 (Pitch sounds as 60.0)
Several other Per-Note Controllers @Note Number 01
Note Off #01
```

Because the two notes of the same pitch use different Note Numbers they can overlap in time.
Multiple notes can sound simultaneously on the pitch of Middle C, each with its own dedicated set of
Per-Note Controllers.

### Method 2: sender using Note On With Attribute #3 Pitch 7.9

For interfaces where pitch is generally constant for the note's whole life cycle. Only suited to
applications that do not need the Note On Attribute field for anything else.

```
Per-Note Management @Note Number 00
Note On #00 with AttrPitch7.9 = 60.0
Several Per-Note Controllers @Note Number 00
Note Off #00

Per-Note Management @Note Number 01
Note On #01 with AttrPitch7.9 = 60.0
Several Per-Note Controllers @Note Number 01
Note Off #01
```

## Implementation notes

- **Implement MIDI 2.0 on top of MIDI 1.0 for backwards compatibility.** A device negotiates the
  MIDI 2.0 Protocol over MIDI-CI or UMP Stream messages; until it does, everything falls back to
  MIDI 1.0 and the tuning must go over MTS, MPE or pitchbend.
- Per-note pitch is **absolute**, not an offset. Pitch 7.25 and Pitch 7.9 both specify a pitch in
  HCUs from scratch, so a microtuning master computes the pitch directly from the scale and never
  needs to work out a bend amount. This is the main practical advantage over MPE.
- **Resolution.** Pitch 7.9 is ~0.2 cents, comparable to MPE at a reduced bend range and coarser than
  MTS sysex (0.0061 cents). Pitch 7.25 is far finer than anything else here. If the tuning has
  audible commas, prefer Pitch 7.25.
- **Multichannel** works differently: MIDI 2.0 has 16 Groups × 16 Channels = 256 channels, so the
  128-key limit that motivates multichannel tuning elsewhere is better solved by note number rotation
  on one channel. See the cross-standard multichannel note in the skill.
- Per-note tuning does not survive translation to MIDI 1.0. Neither does Per-Note Pitch Bend or its
  sensitivity RPN. Any device claiming MIDI 1.0 backwards compatibility needs a separate tuning path.
- JUCE has UMP support (`juce::universal_midi_packets`), but check the version in use: per-note
  controller helpers and MIDI-CI coverage have lagged the specification.