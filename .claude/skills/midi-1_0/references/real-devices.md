# Where real devices depart from the specification

The other references in this skill say what MIDI 1.0 *requires*. This one says
what instruments and software actually do, because a receiver written only
against the specification will misread hardware that is on sale today.

Everything here is evidence rather than folklore: a manufacturer's own
implementation chart, or source you can read. Where something is a general
impression rather than a citation, it says so.

**The rule this file exists to teach:** a receiver should decide what a stream
means from **how it behaves**, not from what the numbers are supposed to mean. A
sender is not obliged to be conformant and frequently is not.

## Korg minilogue xd — two breaches, both in the 14-bit rules

The MIDI implementation chart is published with the manual. It breaks
[controllers.md](controllers.md#msb-and-lsb) twice.

**It uses CC 63 as one shared low byte** for every 14-bit control, instead of
pairing each high byte with its own *n*+32. So a receiver that derives the low
byte's number from the high byte's — which is the only thing the specification
describes — never sees the fine data at all, from any control.

**It sends the low byte first.** Note `*1-4` of the chart: "when a 10 bit value
is sent, the lower 3 bits are first sent via a CC #63 (0x3f) message." Table II
note 4 says the opposite — "first the MSB, then the LSB" — so the *first* control
change of every gesture on this instrument is the one that does **not** identify
the control.

That second one is what breaks naive MIDI learn. Take the first controller you
see and you learn CC 63, every time, for every knob.

### It also treats a tuning dump as a *scale*

Worth its own note, because it is a good illustration of something the
specifications leave open: whether an absolute tuning and a pitch reference are
the same thing. On this instrument they are firmly separate.

It receives a **Bulk Tuning Dump** (`08 01`) — nominally 128 absolute
frequencies, with a device id, a tuning set number and a 16-character name — and
its chart says the device id, tuning set and name are **Ignored**, the data being
applied "to the scale being edited". Better still: "when applied to a USER OCTAVE
only notes 60~71 will be used." Twelve of the 128 frequencies survive, as a
repeating octave pattern. The absolute anchor is discarded entirely.

Its pitch reference lives somewhere else: a global **Master Tune** parameter of
−50 to +50 cents, a **Transpose** of ±12 notes, and a per-program tuning of ±50
cents (CC 32). So the scale says the temperament and the tuning parameters say
where it sits — orthogonal, on hardware, whatever the specifications do or do not
say about composing them. See the `midi-microtuning` skill.

Two smaller things from the same chart, both worth knowing:

- The value is **10-bit**, not 14: seven bits of high byte and three of low.
  Nothing requires a device to use all seven bits of a low byte, so the width of
  a "14-bit" controller is not something a receiver can assume either.
- It sends **CC 39, 88 and 96** as ordinary knobs. Those are, respectively, the
  low byte of Channel Volume, the High Resolution Velocity Prefix (CA-031) and
  Data Increment. A receiver with a flat list of "reserved" controllers that
  refuses to map them will refuse three real knobs on a current instrument.

## Mixxx — the convention is not universal, and behaviour is the fix

Mixxx is a DJ application whose whole business is arbitrary MIDI controllers, so
its choices are worth more than most. From `src/controllers/learningutils.cpp`:

> There is an industry convention that a 14-bit CC control is a pair of controls
> offset by 32 (the lower is the MSB, the higher is the LSB). My VCI-400 follows
> this convention, for example. I don't use that convention here because it's not
> universal and we should be able to come up with reasonable heuristics to
> identify an LSB and an MSB.

Its heuristic is the useful part, and it is device-independent. During a sweep a
**low byte wraps** from its maximum back to zero over and over, while the high
byte climbs one step at a time — so the largest difference between two
consecutive values is enormous for the low byte and tiny for the high one.
Comparing that maximum jump identifies which is which without knowing anything
about the numbers involved.

Mixxx pairs the two by **target parameter rather than by number**, and its join
in `src/controllers/midicontroller.cpp` is **order-agnostic** — whichever half
arrives first is queued and the second completes it, with both
`(value << 7) | queued` and `(queued << 7) | value` written out. A minilogue xd
therefore works in Mixxx despite sending its bytes backwards.

The cost of that design is the other half of the story: a queued half that never
gets a partner is stuck, so Mixxx has to warn and flush when a non-14-bit message
turns up mid-pair. The specification's register model — hold a high byte and a
low byte, emit on either — has no such failure, at the price of being
order-sensitive unless you drop the rule that a high byte zeroes the low one.

## Almost nothing implements 14-bit CC at all

A sender cannot assume a receiver joins pairs. Three checked directly:

| software | plain control change |
|---|---|
| **Surge XT** | 7-bit. `SurgeSynthesizer::channelController` opens `float fval = (float)value * (1.f / 127.f)`. RPN/NRPN are parsed properly; ordinary controllers are not paired. |
| **Ardour** (generic MIDI) | 7-bit. `MIDIControllable::midi_sense_controller` uses `msg->value` directly. 14 bits only for bindings explicitly declared RPN or NRPN. |
| **JUCE** | nothing at all — see the `juce-midi` skill. `MidiRPNDetector` is the only multi-message parser it ships. |

Mixxx is the exception rather than the rule, and it needs two mapping rows per
control plus a statistical learn wizard to get there.

**Consequence for a sender:** if fine resolution matters, exposing a coarse and a
fine *parameter* is more likely to work than sending a 14-bit pair and hoping.

## Pianoteq — a timing heuristic, and why it is the wrong one

Reported by a user of this project rather than read from source, so treat it as a
description of observed behaviour: Pianoteq will not MIDI-learn when two control
changes arrive very close together, but will when the same messages are
separated in time.

That is almost certainly an attempt to solve the problem above — reject the
second half of a 14-bit pair — using arrival time as the signal. It misfires in
both directions: a slow 14-bit sender still gets its low byte learned, and a user
wiggling one ordinary knob quickly gets nothing. The wraparound test costs no
more and does not depend on how fast the wire is.

## What to do about all this

- Do not learn a controller from **one** message. Watch a gesture.
- Identify a low byte by **behaviour** — the wrapping test — not by its number.
  Keep the 32–63 rule only as a tie-break for when there is no behaviour to read.
- Do not maintain a flat list of controller numbers that "cannot be mapped"
  beyond the ones that genuinely are not control changes (98–101 select an
  (N)RPN; 120–127 are Channel Mode Messages). Real instruments use the rest.
- If you join pairs at all, join them **in either order**.

<!-- Only these devices and applications have been checked. Nothing here should
be read as a survey: it is what this project happened to run into. Add to it
rather than generalising from it. -->
