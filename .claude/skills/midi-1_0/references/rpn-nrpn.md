# RPN and NRPN

Sources: *Complete MIDI 1.0 Detailed Specification* 4.2.1 (February 1996),
Table III and **Table IIIa, Registered Parameter Numbers**; and the later
documents named against each parameter below.

## An RPN is not a message

This is the thing to be clear about first, because the shorthand "RPN 3" hides
it. **There is no RPN status byte.** An RPN is a *parameter number* selected by
sending ordinary Control Change messages, and then written to by sending more
Control Change messages. Every byte on the wire is a `Bn` control change.

Six controllers do the work (Table III):

| CC | hex | role |
|---|---|---|
| 99 | `63` | NRPN MSB — select a non-registered parameter |
| 98 | `62` | NRPN LSB |
| 101 | `65` | RPN MSB — select a registered parameter |
| 100 | `64` | RPN LSB |
| 6 | `06` | Data Entry MSB — write to whatever is selected |
| 38 | `26` | Data Entry LSB |
| 96 | `60` | Data Increment |
| 97 | `61` | Data Decrement |

So "RPN 0" means *the parameter whose MSB is 0 and whose LSB is 0*, and setting
it to two semitones is four control changes on channel `n`:

```
Bn 65 00     CC 101 = 0     RPN MSB
Bn 64 00     CC 100 = 0     RPN LSB      -> parameter 0,0 = Pitch Bend Sensitivity
Bn 06 02     CC 6   = 2     Data Entry MSB -> 2 semitones
Bn 26 00     CC 38  = 0     Data Entry LSB -> 0 cents
```

The selection is **sticky**: the last parameter selected on a channel stays
selected, so a later Data Entry lands on it. RPN 127/127 is conventionally sent
to deselect, so a stray data entry cannot move whatever was last addressed.
<!-- The 127/127 null convention is not in Table IIIa; it is widely implemented
and appears in later MMA documents. Verify before citing it as normative. -->

## The rules, and the defaults they hand you

p17 numbers six. Four decide how a receiver should behave before anyone has
configured anything:

- **NRPN reception "should be disabled on power-up** to avoid confusion between
  different machines", though transmitting them "should be safe at any time"
  (rule 2). RPN reception, being a standardised list, "may be enabled on
  power-up" (rule 5). So the safe default is *answer RPNs, ignore NRPNs until
  told otherwise* — the spec's advice, not a design taste.
- **Wait for both bytes.** A receiver "should wait until it receives both the LSB
  and MSB for a parameter number to ensure that it is operating on the correct
  parameter" (rule 3) — but must also cope with a sender that transmits only one
  (rule 4), which is why senders are told to send both whenever a new parameter
  is selected.
- **A parameter keeps its value.** "Once a new Parameter Number is chosen, that
  parameter retains its old value until a new Data Entry, Data Increment, or Data
  Decrement is received" (rule 6). Selecting is neither reading nor clearing.

`juce::MidiRPNDetector` already implements the first three: it returns on the MSB
once a parameter number is set, returns again with a 14-bit value on each
following LSB, and copes with a sender that never sends one. Reach for it before
writing a parser.

## Registered parameters

Table IIIa lists five, and lists them **LSB first** — the MSB is `00` for all of
them:

| RPN LSB | RPN MSB | function |
|---|---|---|
| `00` | `00` | Pitch Bend Sensitivity |
| `01` | `00` | Fine Tuning |
| `02` | `00` | Coarse Tuning |
| `03` | `00` | Tuning Program Select |
| `04` | `00` | Tuning Bank Select |

Two later additions are **not** in Table IIIa, because they postdate the 4.2.1
document. Cite them to their own specifications:

| RPN LSB | function | source |
|---|---|---|
| `05` | Modulation Depth Range | CA-026, *RPN05 Modulation Depth Range* |
| `06` | MPE Configuration Message | *MIDI Polyphonic Expression*, M1-100-UM v1.1, 14 April 2022, §2.2.1 |

CA-025, *Master Fine & Coarse Tuning*, renamed RPN 01 and 02 to **Channel** Fine
and Coarse Tuning; Table IIIa still uses the older names.

### What 01 and 02 can express

Both are displacements from A440, and p18 gives their arithmetic. Worth having
because the two are often assumed to be a coarse/fine pair over one range, and
they are not — they overlap:

| RPN | resolution | range |
|---|---|---|
| `01` Fine Tuning | 100/8192 cents ≈ 0.0122 c | −8192 … +8191 of those units, so ±100 c |
| `02` Coarse Tuning | 100 cents | −64 … +63 semitones |

So Fine Tuning covers a whole semitone either way, and Coarse Tuning covers more
than five octaves; the two are not a coarse/fine pair over one range but two
ranges that overlap. Centre is `40H` in the MSB for both.

For a microtonal application, note that Fine Tuning's 0.0122 c is **twice as
coarse** as MTS SysEx's key-based 0.0061 c, and it is per *channel* rather than
per key — a different tool for a different job.

### Tuning program and bank

From the *MIDI Tuning Updated Specification*, "Changing Tuning Programs". `tt` is
the tuning program number:

```
Bn 64 03  Bn 65 00  Bn 06 tt      select tuning program tt
Bn 64 03  Bn 65 00  Bn 60 7F      increment it
Bn 64 03  Bn 65 00  Bn 61 7F      decrement it
```

and the same three with `Bn 64 04` for the tuning **bank**. Written with running
status — status byte sent once — that is exactly the form the tuning
specification prints:

```
Bn 64 03 65 00 06 tt
```

Two warts. The tuning specification numbers programs and banks **1–128** while
the later Bank/Dump Extensions describe the same byte as **0–127** — one value,
counted differently in two documents. And the tuning bank is deliberately
*separate* from the Program Change bank select (CC 0/32), though an instrument
may link them. An instrument receiving a tuning program or bank it does not have
should ignore the message.

### Pitch bend sensitivity

Data Entry MSB is semitones and LSB is cents, which is what makes RPN 0 usable
for microtonal work.

MPE gives it a **default**, not a lock. On receiving an MPE Configuration Message
a receiver shall set the Manager Channel to 2 semitones and every Member Channel
to 48 — and "the values may subsequently be changed at any time using Registered
Parameter Number [RPN] 0, in accordance with the MIDI 1.0 Specification" (MPE
v1.1 §2.2.5). Sending it to the members means sending RPN 0 to **each Member
Channel individually**, which the same section recommends for compatibility.

## Non-registered parameters

The same mechanism with CC 99/98 in place of 101/100, and the meaning is the
manufacturer's own. Nothing is portable: the same NRPN number means different
things on different devices, which is the price of not needing anyone's
permission. The specification's advice is to prefer an NRPN over burning a
controller number on a device-specific setting (p11).
