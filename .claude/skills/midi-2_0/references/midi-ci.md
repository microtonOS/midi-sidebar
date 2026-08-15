# MIDI-CI

*M2-101-UM, MIDI-CI Specification v1.2*. **This file is a stub.** The
specification is in `tmp/midi/` and has not been read; nothing here should be
cited as fact beyond the definitions, which come from the UMP spec's glossary.

## What it is, from the UMP spec's definitions

- **MIDI-CI** — "MIDI Capability Inquiry, a specification published by The MIDI
  Association and AMEI" (UMP v1.1.2 §1.6.1, p16).
- **Profile** — "an MA/AMEI specification that includes a set of MIDI messages
  and defined responses to those messages. A Profile is controlled by MIDI-CI
  Profile Configuration Transactions. A Profile may have a defined minimum set of
  mandatory messages and features, along with some optional or recommended
  messages and features" (p17).
- **Property Exchange** — "a set of MIDI-CI Transactions by which one device may
  access properties from another device" (p17).
- **Transaction** — an exchange of associated messages between two devices with a
  bidirectional connection: an inquiry and one or more replies (p17).

The shape to remember: MIDI-CI is how two devices *agree* what they can do,
before either relies on it. It needs a **bidirectional** connection, which is a
real constraint — a plugin receiving MIDI from a host may not have a way to
reply.

## To read and record

- The three MIDI-CI categories (Protocol Negotiation, Profile Configuration,
  Property Exchange) and which are mandatory.
- Muid allocation and collision handling.
- Whether Protocol Negotiation is still current — it was deprecated in later
  revisions in favour of UMP endpoint discovery, and this needs checking against
  v1.2 rather than assumed.
- What a plugin can realistically use when it has no MIDI output path back to
  the controller.
