# System exclusive

Source: *Complete MIDI 1.0 Detailed Specification* 4.2.1 (February 1996),
**Table VII, System Exclusive Messages**, and **Table VIIa, Currently Defined
Universal System Exclusive Messages**.

A message that starts `F0` (SOX), ends `F7` (EOX), and carries whatever the
owner of the ID has defined in between.

## The ID byte

The byte after `F0` says whose message this is (Table VII, note 1):

| ID | meaning |
|---|---|
| `00`–`7C` | manufacturer identification |
| `7D` | **non-commercial** — schools, research; "not to be used on any product released to the public" |
| `7E` | Universal **Non**-Real Time |
| `7F` | Universal **Real Time** |

**If the first byte of a manufacturer ID is `00`, the following two bytes extend
it** — the three-byte manufacturer ID form, for the manufacturers who did not get
a one-byte number. A parser that assumes one byte will misread every message from
those.

## Universal system exclusive

```
F0 7E <device ID> <sub-ID#1> <sub-ID#2> …    non-real time
F0 7F <device ID> <sub-ID#1> <sub-ID#2> …    real time
```

`<device ID>` addresses one device; **`7F` means all devices**.

The non-real-time / real-time split is about *when* the message is meant to act.
Non-real-time messages are setup: "if it is NOT sent as a setup message … it is
assumed that the message will be ignored for notes that are already sounding"
(*MIDI Tuning Updated Specification*, Notes). Real-time messages act immediately
on sounding notes.

`sub-ID#1` names the family. From Table VIIa, the non-real-time families include:

| sub-ID#1 | family |
|---|---|
| `01`–`03` | Sample Dump Header / Data Packet / Request |
| `04` | MIDI Time Code |
| `05` | Sample Dump Extensions |
| `06` | General Information — `01` Identity Request, `02` Identity Reply |
| `07` | File Dump |
| `08` | **MIDI Tuning Standard** |
| `09` | General MIDI — `01` System On, `02` System Off |
| `7B`–`7F` | End Of File, Wait, Cancel, NAK, ACK |

`08` is the one this project cares about; the **midi-microtuning** skill has its
messages in full.

## Parsing

Table VII note 2 is the whole rule, and it is worth quoting:

> All bytes between the System Exclusive Status byte and EOX must have zeroes in
> the Most Significant Bit … with the exception of System Real Time Status Bytes
> (`F8H`–`FFH`). Any other Status Byte that appears between the SOX (`F0H`) and
> EOX (`F7H`) will be considered an EOX message, and terminate the System
> Exclusive message.

Three consequences for a parser:

- **Every data byte carries 7 bits.** This is the constraint behind every packing
  scheme in every SysEx family.
- **A System Real Time byte may appear mid-message.** Act on it, remove it, and
  carry on with the SysEx.
- **Any other status byte ends the message**, whether or not `F7` ever arrives.
  So a truncated SysEx is a normal event, not a corruption to be logged.

Length is not declared up front, so a receiver needs a bounded buffer and a
policy for a message that overruns it.

## Checksums

Dump messages carry one. From the *MIDI Tuning Updated Specification*, "Checksum
Calculation":

> successively XOR the bytes in the message, excluding the `F0`, `F7`, and the
> checksum field … the resulting value is then AND'ed with `7F`

One documented exception: the original Bulk Dump (`sub-ID#2 = 01`) was ambiguous
enough that manufacturers implemented its checksum differently, and receivers are
now recommended to **ignore** it on that message alone.

<!-- Not yet read from their own documents: Table VIIb (manufacturer ID
assignments), the Device Inquiry and File Dump message layouts, MIDI Show
Control, MIDI Machine Control, and Device Control (Master Volume and Balance).
All are listed in the 4.2.1 contents at pp34-58. -->
