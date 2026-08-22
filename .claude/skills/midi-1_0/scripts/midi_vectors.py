#!/usr/bin/env python3
"""Byte sequences for the MIDI 1.0 messages that are awkward to hand-write.

Every one of these is several messages that mean one thing, or a *gesture* whose
shape is the point — the cases where writing the bytes by hand is where the bug
gets introduced. Emitted as hex so the output can be pasted into a test, piped
to ``amidi``, or diffed against a capture.

    python3 midi_vectors.py --list
    python3 midi_vectors.py rpn --channel 1 --number 3 --value 5
    python3 midi_vectors.py mpe-lower --members 4
    python3 midi_vectors.py sweep14 --msb 11 --lsb 43 --steps 8
    python3 midi_vectors.py sweep14 --msb 43 --lsb 63 --steps 8 --low-byte-first

The last of those is the one worth knowing about: an instrument that sends its
low byte *before* its high byte. See ../references/real-devices.md — it is why a
receiver must not learn a controller from the first message it sees.
"""

from __future__ import annotations

import argparse
import sys


def cc(channel: int, number: int, value: int) -> list[int]:
    """A control change. `channel` is 1-16; the nibble on the wire is one less,
    which is the commonest off-by-one in MIDI."""
    return [0xB0 | ((channel - 1) & 0x0F), number & 0x7F, value & 0x7F]


# --------------------------------------------------------------------------
#  RPN and NRPN — four control changes that mean one thing
# --------------------------------------------------------------------------

def rpn(channel: int, number: int, value: int, fourteen_bit: bool = False,
        nrpn: bool = False) -> list[int]:
    """**An RPN is not a message.** There is no RPN status byte: it is a
    parameter *selected* by CC 101 and 100, then written by CC 6 (and CC 38 for
    a low byte). "RPN 0/3" means four `Bn` messages on the wire.

    NRPN uses CC 99 and 98 for the same job.
    """
    msb_cc, lsb_cc = (99, 98) if nrpn else (101, 100)

    out = cc(channel, msb_cc, (number >> 7) & 0x7F)
    out += cc(channel, lsb_cc, number & 0x7F)

    if fourteen_bit:
        out += cc(channel, 6, (value >> 7) & 0x7F)
        out += cc(channel, 38, value & 0x7F)
    else:
        out += cc(channel, 6, value & 0x7F)

    return out


def rpn_null(channel: int) -> list[int]:
    """RPN 127/127, which deselects — so a stray data entry afterwards writes
    nothing. Worth sending after any RPN, and worth testing that a receiver
    honours."""
    return cc(channel, 101, 127) + cc(channel, 100, 127)


# --------------------------------------------------------------------------
#  MPE — the configuration message is an RPN too
# --------------------------------------------------------------------------

def mpe_zone(manager_channel: int, members: int) -> list[int]:
    """The MPE Configuration Message: RPN 6, sent on the zone's manager channel,
    whose value is the number of **member** channels.

    A lower zone has manager channel 1 and members counting up; an upper zone has
    manager channel 16 and members counting down. `members = 0` clears the zone.
    """
    return rpn(manager_channel, 6, members)


def pitch_bend_range(channel: int, semitones: int, cents: int = 0) -> list[int]:
    """RPN 0. MPE defaults this to 2 semitones on a manager channel and 48 on a
    member channel, but §2.2.5 allows it to be changed at any time — the
    defaults are starting points, not fixed values."""
    return cc(channel, 101, 0) + cc(channel, 100, 0) \
        + cc(channel, 6, semitones & 0x7F) + cc(channel, 38, cents & 0x7F)


# --------------------------------------------------------------------------
#  Device control
# --------------------------------------------------------------------------

def master_volume(value: int, device: int = 0x7F) -> list[int]:
    """``F0 7F <device> 04 01 vv vv F7``, 14 bits **LSB first**.

    Device `7F` is the All Call broadcast, which is the only one a plugin can
    sensibly answer: it has no device id of its own.
    """
    return [0xF0, 0x7F, device & 0x7F, 0x04, 0x01,
            value & 0x7F, (value >> 7) & 0x7F, 0xF7]


# --------------------------------------------------------------------------
#  Gestures — a sweep, which is what learning and 14-bit handling see
# --------------------------------------------------------------------------

def sweep7(channel: int, number: int, steps: int) -> list[list[int]]:
    """One controller swept across its range."""
    return [cc(channel, number, min(127, i * 127 // max(1, steps - 1)))
            for i in range(steps)]


def sweep14(channel: int, msb: int, lsb: int, steps: int,
            low_byte_first: bool = False) -> list[list[int]]:
    """A 14-bit controller swept across its range.

    ``low_byte_first`` reproduces the Korg minilogue xd, whose implementation
    chart says "when a 10 bit value is sent, the lower 3 bits are first sent via
    a CC #63 (0x3f) message" — the reverse of Table II note 4. Two things break
    on it: learning by first-message-wins catches the low byte every time, and a
    receiver following the spec's rule that a new MSB zeroes the LSB discards the
    fine data on every gesture.
    """
    out = []

    for i in range(steps):
        value = i * 16383 // max(1, steps - 1)
        high = cc(channel, msb, (value >> 7) & 0x7F)
        low = cc(channel, lsb, value & 0x7F)
        out += [low, high] if low_byte_first else [high, low]

    return out


# --------------------------------------------------------------------------

def as_hex(message: list[int]) -> str:
    return " ".join(f"{b:02X}" for b in message)


VECTORS = {
    "rpn": "an RPN, as its four control changes",
    "nrpn": "the same, as a non-registered parameter",
    "rpn-null": "RPN 127/127, which deselects",
    "mpe-lower": "MPE configuration for a lower zone",
    "mpe-upper": "MPE configuration for an upper zone",
    "pitch-bend-range": "RPN 0, in semitones and cents",
    "master-volume": "the Master Volume system exclusive",
    "sweep7": "a 7-bit controller swept",
    "sweep14": "a 14-bit controller swept (--low-byte-first for the minilogue's order)",
}


def main() -> int:
    p = argparse.ArgumentParser(description=__doc__,
                                formatter_class=argparse.RawDescriptionHelpFormatter)
    p.add_argument("vector", nargs="?", default="--list")
    p.add_argument("--list", action="store_true")
    p.add_argument("--channel", type=int, default=1)
    p.add_argument("--number", type=int, default=0)
    p.add_argument("--value", type=int, default=0)
    p.add_argument("--msb", type=int, default=11)
    p.add_argument("--lsb", type=int, default=43)
    p.add_argument("--steps", type=int, default=8)
    p.add_argument("--members", type=int, default=4)
    p.add_argument("--semitones", type=int, default=2)
    p.add_argument("--cents", type=int, default=0)
    p.add_argument("--low-byte-first", action="store_true")
    p.add_argument("--fourteen-bit", action="store_true")

    args = p.parse_args()

    if args.list or args.vector == "--list":
        for name, description in VECTORS.items():
            print(f"  {name:<18} {description}")
        return 0

    v = args.vector

    if v == "rpn":
        print(as_hex(rpn(args.channel, args.number, args.value, args.fourteen_bit)))
    elif v == "nrpn":
        print(as_hex(rpn(args.channel, args.number, args.value, args.fourteen_bit, nrpn=True)))
    elif v == "rpn-null":
        print(as_hex(rpn_null(args.channel)))
    elif v == "mpe-lower":
        print(as_hex(mpe_zone(1, args.members)))
    elif v == "mpe-upper":
        print(as_hex(mpe_zone(16, args.members)))
    elif v == "pitch-bend-range":
        print(as_hex(pitch_bend_range(args.channel, args.semitones, args.cents)))
    elif v == "master-volume":
        print(as_hex(master_volume(args.value or 16383)))
    elif v == "sweep7":
        for m in sweep7(args.channel, args.number or 74, args.steps):
            print(as_hex(m))
    elif v == "sweep14":
        for m in sweep14(args.channel, args.msb, args.lsb, args.steps, args.low_byte_first):
            print(as_hex(m))
    else:
        print(f"unknown vector '{v}' (try --list)", file=sys.stderr)
        return 2

    return 0


if __name__ == "__main__":
    sys.exit(main())
