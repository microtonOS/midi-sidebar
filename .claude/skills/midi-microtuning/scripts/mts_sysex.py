#!/usr/bin/env python3
"""Encode and decode MIDI Tuning Standard system exclusive messages.

No library does both halves of this. ODDSound's ``libMTSClient`` reads every MTS
format but writes none, and prefers a connected MTS-ESP master over its own
sysex table; Surge's tuning-library has no sysex at all; ``tschiemer/midimessage``
carries the message definitions but marks MTS as TODO; ``kosonya/mts_dumper``
generates only. So this is a reference implementation to check an implementation
*against*, and to generate fixtures with.

Sources, all cited in ../references/mts-sysex.md: the *MIDI Tuning Updated
Specification*, CA-020 (bank and dump extensions) and CA-021/RP-020
(scale/octave).

    python3 mts_sysex.py --list
    python3 mts_sysex.py bulk-dump --program 5 --name "Meantone"
    python3 mts_sysex.py single-note --note 69 --hz 442.0
    python3 mts_sysex.py octave-1byte --offsets 0,-14,0,0,0,0,0,0,0,0,0,0
    python3 mts_sysex.py decode "F0 7E 7F 08 01 05 ... F7"

Emitted as hex so the output can be pasted into a C++ test, piped to ``amidi``,
or diffed against a capture.
"""

from __future__ import annotations

import argparse
import math
import sys

NON_REAL_TIME = 0x7E
REAL_TIME = 0x7F
TUNING_SUB_ID = 0x08
ALL_CALL = 0x7F

#: sub-ID#2. Note that 07, 08 and 09 each name *two* messages, told apart only
#: by the 7E/7F header — a decoder must branch on the header, never on sub-ID#2.
KINDS = {
    0x00: "bulk dump request",
    0x01: "bulk dump",
    0x02: "single note tuning change",
    0x03: "bulk dump request (bank)",
    0x04: "key-based tuning dump",
    0x05: "scale/octave dump, 1-byte",
    0x06: "scale/octave dump, 2-byte",
    0x07: "single note tuning change (bank)",
    0x08: "scale/octave tuning, 1-byte",
    0x09: "scale/octave tuning, 2-byte",
}

#: 7F 7F 7F is reserved and means *no change* — send it for keys outside the
#: instrument's range so a receiver does not store a bogus frequency. Decoding
#: it as a pitch is the classic bug.
NO_CHANGE = (0x7F, 0x7F, 0x7F)


# --------------------------------------------------------------------------
#  The 3-byte frequency format
# --------------------------------------------------------------------------

def frequency_to_bytes(hz: float) -> tuple[int, int, int]:
    """``0xxxxxxx 0abcdefg 0hijklmn`` — a semitone, then a 14-bit fraction of
    100 cents in units of 100/2**14 = .0061 cents."""
    if hz <= 0.0:
        return NO_CHANGE

    # The nearest equal-tempered semitone at or below the frequency, A440.
    semitones = 69.0 + 12.0 * math.log2(hz / 440.0)
    semitone = max(0, min(127, math.floor(semitones)))
    fraction = round((semitones - semitone) * 16384)

    if fraction >= 16384:            # rounded up into the next semitone
        semitone, fraction = min(127, semitone + 1), 0

    return semitone, (fraction >> 7) & 0x7F, fraction & 0x7F


def bytes_to_frequency(a: int, b: int, c: int) -> float | None:
    """Returns None for the reserved 'no change' value."""
    if (a, b, c) == NO_CHANGE:
        return None

    semitone = a & 0x7F
    fraction = (((b & 0x7F) << 7) | (c & 0x7F)) / 16384.0

    return 440.0 * 2.0 ** ((semitone + fraction - 69.0) / 12.0)


def checksum(body: list[int]) -> int:
    """XOR of every byte but F0, F7 and the checksum field, masked to 7 bits.

    Carried by the *dump* messages only. The original bulk dump (sub-ID#2 01) is
    the exception the spec makes itself: its instructions were ambiguous, so
    receivers are recommended to **ignore** its checksum. Prefer sub-ID#2 04.
    """
    value = 0
    for byte in body:
        value ^= byte
    return value & 0x7F


# --------------------------------------------------------------------------
#  The channel bitmap, whose bit order is not the obvious one
# --------------------------------------------------------------------------

def channels_to_bitmap(channels: list[int]) -> tuple[int, int, int]:
    """``ff`` bits 0-1 are channels 15-16, ``gg`` bits 0-6 are 8-14, ``hh``
    bits 0-6 are 1-7. The five spare bits of ``ff`` **shall be 0**."""
    ff = gg = hh = 0

    for channel in channels:
        if 1 <= channel <= 7:
            hh |= 1 << (channel - 1)
        elif 8 <= channel <= 14:
            gg |= 1 << (channel - 8)
        elif 15 <= channel <= 16:
            ff |= 1 << (channel - 15)

    return ff, gg, hh


def bitmap_to_channels(ff: int, gg: int, hh: int) -> list[int]:
    channels = [1 + b for b in range(7) if hh >> b & 1]
    channels += [8 + b for b in range(7) if gg >> b & 1]
    channels += [15 + b for b in range(2) if ff >> b & 1]
    return channels


# --------------------------------------------------------------------------
#  Scale/octave offsets
# --------------------------------------------------------------------------

def offset_to_byte(cents: float) -> int:
    """00h = -64 c, 40h = equal temperament, 7Fh = +63 c. One cent a step."""
    return max(0, min(0x7F, round(cents) + 64))


def offset_to_bytes(cents: float) -> tuple[int, int]:
    """00h 00h = -100 c, 40h 00h = equal temperament, 7Fh 7Fh = +100 c.
    200 cents over 14 bits, .012207 cents a step."""
    word = max(0, min(16383, round(cents * 16384.0 / 200.0) + 8192))
    return (word >> 7) & 0x7F, word & 0x7F


# --------------------------------------------------------------------------
#  Message builders. Each returns the whole message, F0 … F7.
# --------------------------------------------------------------------------

def _name_bytes(name: str) -> list[int]:
    padded = (name + " " * 16)[:16]
    return [ord(c) & 0x7F for c in padded]


def bulk_dump(frequencies: list[float], program: int = 0, name: str = "",
              bank: int | None = None, device: int = ALL_CALL) -> list[int]:
    """sub-ID#2 01, or 04 when a bank is given (which is the one to prefer:
    its checksum is unambiguous)."""
    if len(frequencies) != 128:
        raise ValueError("a bulk dump carries exactly 128 frequencies")

    body = [NON_REAL_TIME, device, TUNING_SUB_ID, 0x01 if bank is None else 0x04]

    if bank is not None:
        body.append(bank & 0x7F)

    body.append(program & 0x7F)
    body += _name_bytes(name)

    for hz in frequencies:
        body += list(frequency_to_bytes(hz))

    return [0xF0] + body + [checksum(body), 0xF7]


def single_note(changes: list[tuple[int, float]], program: int = 0,
                bank: int | None = None, real_time: bool = True,
                device: int = ALL_CALL) -> list[int]:
    """sub-ID#2 02, or 07 with a bank. ``changes`` is (key, hz) pairs.

    Real time updates sounding notes; non-real-time stages the change for
    subsequent ones. No checksum on either.
    """
    header = REAL_TIME if real_time else NON_REAL_TIME
    body = [header, device, TUNING_SUB_ID, 0x02 if bank is None else 0x07]

    if bank is not None:
        body.append(bank & 0x7F)

    body += [program & 0x7F, len(changes) & 0x7F]

    for key, hz in changes:
        body.append(key & 0x7F)
        body += list(frequency_to_bytes(hz))

    return [0xF0] + body + [0xF7]


def scale_octave(offsets: list[float], channels: list[int] | None = None,
                 two_byte: bool = False, real_time: bool = True,
                 device: int = ALL_CALL) -> list[int]:
    """sub-ID#2 08 (1-byte) or 09 (2-byte). Twelve offsets, C to B, in cents.

    Offsets apply to the currently selected preset rather than to the modified
    tuning, so repeated messages do not accumulate.
    """
    if len(offsets) != 12:
        raise ValueError("scale/octave tuning carries exactly 12 offsets")

    header = REAL_TIME if real_time else NON_REAL_TIME
    ff, gg, hh = channels_to_bitmap(channels or list(range(1, 17)))

    body = [header, device, TUNING_SUB_ID, 0x09 if two_byte else 0x08, ff, gg, hh]

    for cents in offsets:
        body += list(offset_to_bytes(cents)) if two_byte else [offset_to_byte(cents)]

    return [0xF0] + body + [0xF7]


# --------------------------------------------------------------------------
#  Decoding, enough to describe a captured message
# --------------------------------------------------------------------------

def decode(message: list[int]) -> dict:
    if len(message) < 6 or message[0] != 0xF0 or message[-1] != 0xF7:
        return {"error": "not a system exclusive message"}

    body = message[1:-1]

    if body[0] not in (NON_REAL_TIME, REAL_TIME) or body[2] != TUNING_SUB_ID:
        return {"error": "not MIDI Tuning Standard"}

    kind = body[3]
    out = {
        "kind": KINDS.get(kind, f"unknown sub-ID#2 {kind:#04x}"),
        "real_time": body[0] == REAL_TIME,
        "device": body[1],
        "affects_sounding_notes": body[0] == REAL_TIME,
    }

    at = 4

    if kind in (0x04, 0x05, 0x06, 0x07, 0x03):
        out["bank"] = body[at]
        at += 1

    if kind in (0x00, 0x01, 0x02, 0x04, 0x05, 0x06, 0x07):
        out["program"] = body[at]
        at += 1

    if kind in (0x01, 0x04, 0x05, 0x06):
        out["name"] = "".join(chr(b) for b in body[at:at + 16]).strip()
        at += 16

    if kind in (0x01, 0x04):
        notes = {}
        for note in range(128):
            hz = bytes_to_frequency(*body[at:at + 3])
            if hz is not None:
                notes[note] = hz
            at += 3
        out["notes"] = notes
        out["checksum_ok"] = body[at] == checksum(body[:at]) if at < len(body) else False
        if kind == 0x01:
            out["checksum_note"] = "ignored by recommendation on sub-ID#2 01"

    if kind in (0x02, 0x07):
        count = body[at]
        at += 1
        out["notes"] = {}
        for _ in range(count):
            hz = bytes_to_frequency(*body[at + 1:at + 4])
            if hz is not None:
                out["notes"][body[at]] = hz
            at += 4

    if kind in (0x08, 0x09):
        out["channels"] = bitmap_to_channels(body[4], body[5], body[6])
        at = 7
        two = kind == 0x09
        out["offsets"] = [
            ((((body[at + i * 2] << 7) | body[at + i * 2 + 1]) - 8192) * 200.0 / 16384.0)
            if two else float(body[at + i] - 64)
            for i in range(12)
        ]

    return out


# --------------------------------------------------------------------------

def as_hex(message: list[int]) -> str:
    return " ".join(f"{b:02X}" for b in message)


def parse_hex(text: str) -> list[int]:
    return [int(t, 16) for t in text.replace(",", " ").split()]


def equal_temperament() -> list[float]:
    return [440.0 * 2.0 ** ((n - 69) / 12.0) for n in range(128)]


def main() -> int:
    p = argparse.ArgumentParser(description=__doc__,
                                formatter_class=argparse.RawDescriptionHelpFormatter)
    p.add_argument("command", nargs="?", default="--list")
    p.add_argument("hex", nargs="?", help="for `decode`")
    p.add_argument("--list", action="store_true", help="list the message kinds")
    p.add_argument("--program", type=int, default=0)
    p.add_argument("--bank", type=int, default=None)
    p.add_argument("--name", default="")
    p.add_argument("--note", type=int, default=69)
    p.add_argument("--hz", type=float, default=440.0)
    p.add_argument("--offsets", default=",".join(["0"] * 12))
    p.add_argument("--channels", default="")
    p.add_argument("--non-real-time", action="store_true")

    args = p.parse_args()

    if args.list or args.command == "--list":
        for sub, name in sorted(KINDS.items()):
            print(f"  08 {sub:02X}  {name}")
        return 0

    channels = [int(c) for c in args.channels.split(",") if c.strip()] or None
    offsets = [float(o) for o in args.offsets.split(",")]

    if args.command == "bulk-dump":
        print(as_hex(bulk_dump(equal_temperament(), args.program, args.name, args.bank)))
    elif args.command == "single-note":
        print(as_hex(single_note([(args.note, args.hz)], args.program, args.bank,
                                 not args.non_real_time)))
    elif args.command == "octave-1byte":
        print(as_hex(scale_octave(offsets, channels, False, not args.non_real_time)))
    elif args.command == "octave-2byte":
        print(as_hex(scale_octave(offsets, channels, True, not args.non_real_time)))
    elif args.command == "decode":
        if not args.hex:
            print("decode needs a hex string", file=sys.stderr)
            return 2
        for key, value in decode(parse_hex(args.hex)).items():
            if key == "notes" and isinstance(value, dict) and len(value) > 6:
                shown = list(value.items())[:3]
                print(f"  {key}: {len(value)} notes, e.g. " +
                      ", ".join(f"{n}={hz:.4f} Hz" for n, hz in shown))
            else:
                print(f"  {key}: {value}")
    else:
        print(f"unknown command '{args.command}' (try --list)", file=sys.stderr)
        return 2

    return 0


if __name__ == "__main__":
    sys.exit(main())
