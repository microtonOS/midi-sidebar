# The Universal MIDI Packet

*M2-104-UM, UMP Format & MIDI 2.0 Protocol v1.1.2*, 27 October 2023. Printed page
numbers match the PDF's.

UMP is "a data container which defines the data format for all MIDI 1.0 Protocol
messages and all MIDI 2.0 Protocol messages" (§1.6.1, p17). It is a container,
not a protocol: the same packet format carries either protocol, which is what
lets a MIDI 2.0 system speak to MIDI 1.0 devices without translation at the
transport.

## Groups and channels

A UMP carries a **Group** field addressing the message to one of **16 Groups**
(§1.6.1, p16). Each Group carries the familiar 16 channels.

```
16 Groups  x  16 Channels  =  256 channels
```

**Nothing is reserved.** There is no rule setting aside Groups or channels for
devices that do not speak MIDI 2.0, and no concept of "channels beyond the first
16". A device that speaks only MIDI 1.0 is reached by sending MIDI 1.0 Protocol
messages — which UMP carries — not by confining it to a particular Group.

A **Function Block** is one logical entity on a device and "operates on a set of
one or more Groups" (§1.6.1, p16). So a Group is the addressing unit and a
Function Block is what claims one or more of them — the closest MIDI 2.0 gets to
the old idea of a port.

<!-- Packet sizes and the Message Type field: the UMP spec defines 32-, 64-,
     96- and 128-bit packets selected by the first nibble (Message Type). The
     detailed per-type tables in §4 have not been read yet and should be, before
     anything here is used to write a parser. -->

## Conformance vocabulary

Worth knowing when reading any MA specification, since the words are reserved
(§1.6.2, p18):

| word | means |
|---|---|
| **shall** | mandatory for conformance |
| **should** | recommended, not mandatory |
| **may** | permitted, optional |
| must | unavoidable — *not* a conformance requirement |
| will | a statement of fact |
| can | a statement of capability |
| might | a statement of possibility |

The last three are never used for conformance, so "must" in an MA spec is
deliberately *weaker* than "shall" — the reverse of most engineering prose.
