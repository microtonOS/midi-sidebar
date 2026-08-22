#pragma once

#include <cmath>
#include <optional>

#include <juce_audio_basics/juce_audio_basics.h>

#include "TuningTable.h"

namespace microtonos::sidebar
{

//==============================================================================
/** Reading the MIDI Tuning Standard system exclusive messages.

    Written against `.claude/skills/midi-microtuning/references/mts-sysex.md`,
    which cites the *MIDI Tuning Updated Specification* along with CA-020 (bank
    and dump extensions) and CA-021/RP-020 (scale/octave). Every rule below has a
    line there.

    **Why this exists when ODDSound's client would do it.** `libMTSClient` reads
    every MTS format — but `MTSClient::freq()` opens with `if
    (!global.isOnline()) return localTunings[note].freq`, so a connected MTS-ESP
    master always wins and the sysex table is only a fallback. docs/tuning.md
    says selecting MTS Sysex "ignores the MTS ESP master", and there is no API to
    turn the master off. So the two schemes need two independent sources, and
    this is the one we own. Nothing else off the shelf does the job either: Surge's
    tuning-library has no sysex at all, `tschiemer/midimessage` marks MTS TODO,
    and `kosonya/mts_dumper` is a Python generator.

    Reports a `Change` rather than mutating a table, so which notes and which
    channels a message addresses can be checked without one. Pure and fixed-size,
    like the headers in `sidebar/midi/`.
*/
namespace mtsSysex
{
    //==========================================================================
    inline constexpr int nonRealTime = 0x7e;   ///< `F0 7E` universal header
    inline constexpr int realTime    = 0x7f;   ///< `F0 7F` universal header

    inline constexpr int tuningSubId = 0x08;   ///< sub-ID#1, "MIDI Tuning Standard"

    /** sub-ID#2. Note that `07`, `08` and `09` each name **two** messages,
        told apart only by the `7E`/`7F` header — so a parser must branch on the
        header byte, never on sub-ID#2 alone. */
    enum class Kind
    {
        bulkDumpRequest     = 0x00,
        bulkDump            = 0x01,
        singleNote          = 0x02,
        bulkDumpRequestBank = 0x03,
        keyBasedDump        = 0x04,
        scaleOctaveDump1    = 0x05,
        scaleOctaveDump2    = 0x06,
        singleNoteBank      = 0x07,
        scaleOctave1        = 0x08,
        scaleOctave2        = 0x09
    };

    /** The 16 ASCII characters a dump carries for its name. */
    inline constexpr int nameLength = 16;

    /** `7F 7F 7F` in the three frequency bytes is reserved and means **no
        change** — "send it for keys outside the instrument's range so that
        receivers do not store bogus data. On reception, leave the stored
        frequency for that key untouched." Not a frequency, and storing it as
        one is the classic bug. */
    inline constexpr int noChange = 0x1fffff;

    //==========================================================================
    /** One key's new frequency, or a request to leave it alone. */
    struct NoteChange
    {
        int note = 0;

        /** Empty for `7F 7F 7F`: leave whatever is stored. Distinct from a note
            being *unmapped*, which `TuningTable` also spells as empty — the
            caller must not confuse "do not touch" with "silence". */
        std::optional<double> frequency;
    };

    /** What one message asks for. */
    struct Change
    {
        Kind kind = Kind::singleNote;

        /** True for `F0 7F`. "Will affect currently sounding notes"; the
            non-real-time forms stage a change for subsequent notes instead,
            which is the tuning page's `UpdateMode` distinction arriving over the
            wire rather than being chosen. */
        bool affectsSoundingNotes = false;

        /** `7F` addresses every device. Anything else names one, and a plugin
            has no device id of its own — see MidiDeviceControl.h for the same
            argument about Master Volume. */
        int deviceId = 0x7f;

        std::optional<int> program, bank;

        /** Empty unless the message carried one; only the dumps do. */
        juce::String name;

        /** Absolute frequencies, from the key-based messages. */
        juce::Array<NoteChange> notes;

        /** Offsets from equal temperament in cents, one per pitch class C..B,
            from the scale/octave messages. Empty for key-based messages. */
        juce::Array<double> octaveOffsets;

        /** Which channels the scale/octave messages address, from their bitmap.
            Empty means every channel, which is what the key-based messages
            imply — they carry no bitmap at all. */
        juce::Array<int> channels;

        bool isScaleOctave() const noexcept { return ! octaveOffsets.isEmpty(); }
    };

    //==========================================================================
    /** The frequency three data bytes encode.

        `0xxxxxxx 0abcdefg 0hijklmn` — seven bits of semitone (a MIDI note
        number, A440) then a 14-bit fraction of 100 cents in units of
        100/2^14 = 0.0061 cents. */
    inline std::optional<double> frequencyFromBytes (int a, int b, int c)
    {
        const auto word = ((a & 0x7f) << 14) | ((b & 0x7f) << 7) | (c & 0x7f);

        if (word == noChange)
            return {};

        const auto semitone = (word >> 14) & 0x7f;
        const auto fraction = (double) (word & 0x3fff) / 16384.0;

        return standardTuning::frequencyFor (semitone) * std::pow (2.0, fraction / 12.0);
    }

    /** XOR of every byte but `F0`, `F7` and the checksum itself, masked to 7
        bits. `data` here is `getSysExData()`, which already excludes `F0` and
        `F7`, so this runs to `size - 1`. */
    inline int checksumOf (const juce::uint8* data, int size)
    {
        int sum = 0;

        for (int i = 0; i < size - 1; ++i)
            sum ^= data[i];

        return sum & 0x7f;
    }

    /** Whether the checksum on a dump is right.

        **Tolerated on `bulkDump`.** "Its instructions were ambiguous and
        manufacturers implemented it inconsistently, so receivers are recommended
        to ignore the checksum in that message." Every other dump is checked. */
    inline bool checksumIsAcceptable (Kind kind, const juce::uint8* data, int size)
    {
        if (kind == Kind::bulkDump)
            return true;

        return size >= 2 && data[size - 1] == checksumOf (data, size);
    }

    //==========================================================================
    /** The channels a scale/octave bitmap names.

        Three bytes, and the bit order is not the obvious one: `hh` low bits are
        channels 1-7, `gg` low bits are 8-14, and only the bottom **two** bits of
        `ff` are used, for 15 and 16. The other five "shall be 0". */
    inline juce::Array<int> channelsFromBitmap (int ff, int gg, int hh)
    {
        juce::Array<int> channels;

        for (int bit = 0; bit < 7; ++bit)
            if ((hh >> bit) & 1)
                channels.add (1 + bit);

        for (int bit = 0; bit < 7; ++bit)
            if ((gg >> bit) & 1)
                channels.add (8 + bit);

        for (int bit = 0; bit < 2; ++bit)
            if ((ff >> bit) & 1)
                channels.add (15 + bit);

        return channels;
    }

    /** 1-byte offsets: `00` is -64 cents, `40` is equal temperament, `7F` is
        +63. One cent per step. */
    inline double offsetFromByte (int s) { return (double) ((s & 0x7f) - 64); }

    /** 2-byte offsets: `00 00` is -100 cents, `40 00` is equal temperament,
        `7F 7F` is +100. 200 cents over 14 bits, 0.012207 cents a step. */
    inline double offsetFromBytes (int s, int t)
    {
        const auto word = ((s & 0x7f) << 7) | (t & 0x7f);

        return (double) (word - 8192) * (200.0 / 16384.0);
    }

    //==========================================================================
    /** What this message asks for, or nothing if it is not an MTS message.

        `getSysExData()` omits `F0` and `getSysExDataSize()` counts neither `F0`
        nor `F7`, so `data[0]` is the universal id and the last byte is whatever
        precedes `F7`.
    */
    inline std::optional<Change> parse (const juce::MidiMessage& m)
    {
        if (! m.isSysEx())
            return {};

        const auto* data = m.getSysExData();
        const auto  size = m.getSysExDataSize();

        // id, device, sub-ID#1, sub-ID#2 at the very least.
        if (data == nullptr || size < 4)
            return {};

        const auto header = data[0];

        if (header != nonRealTime && header != realTime)
            return {};

        if (data[2] != tuningSubId)
            return {};

        Change change;

        change.kind                 = static_cast<Kind> (data[3]);
        change.affectsSoundingNotes = header == realTime;
        change.deviceId             = data[1];

        // Where the body starts, after id/device/sub-ID#1/sub-ID#2.
        auto at = 4;

        const auto take = [&at, data, size] (int& out)
        {
            if (at >= size)
                return false;

            out = data[at++];
            return true;
        };

        switch (change.kind)
        {
            // Requests carry no tuning, so there is nothing here to apply. They
            // are recognised rather than parsed: a receiver that answered them
            // would be a tuning *source*, which this plugin is not.
            case Kind::bulkDumpRequest:
            case Kind::bulkDumpRequestBank:
                return {};

            case Kind::bulkDump:
            case Kind::keyBasedDump:
            {
                int value = 0;

                if (change.kind == Kind::keyBasedDump)
                {
                    if (! take (value))
                        return {};

                    change.bank = value;
                }

                if (! take (value))
                    return {};

                change.program = value;

                // 16 ASCII characters, then 128 notes of three bytes, then the
                // checksum. Anything shorter is truncated rather than short.
                if (at + nameLength + 128 * 3 + 1 > size)
                    return {};

                change.name = juce::String::createStringFromData (data + at, nameLength).trim();
                at += nameLength;

                for (int note = 0; note < 128; ++note, at += 3)
                    change.notes.add ({ note, frequencyFromBytes (data[at], data[at + 1], data[at + 2]) });

                if (! checksumIsAcceptable (change.kind, data, size))
                    return {};

                return change;
            }

            case Kind::singleNote:
            case Kind::singleNoteBank:
            {
                int value = 0;

                if (change.kind == Kind::singleNoteBank)
                {
                    if (! take (value))
                        return {};

                    change.bank = value;
                }

                if (! take (value))
                    return {};

                change.program = value;

                int count = 0;

                if (! take (count))
                    return {};

                // `ll` sets of `[kk xx yy zz]`. No checksum on these.
                if (at + count * 4 > size)
                    return {};

                for (int i = 0; i < count; ++i, at += 4)
                    change.notes.add ({ data[at] & 0x7f,
                                        frequencyFromBytes (data[at + 1], data[at + 2], data[at + 3]) });

                return change;
            }

            case Kind::scaleOctave1:
            case Kind::scaleOctave2:
            {
                int ff = 0, gg = 0, hh = 0;

                if (! take (ff) || ! take (gg) || ! take (hh))
                    return {};

                change.channels = channelsFromBitmap (ff, gg, hh);

                const auto wide  = change.kind == Kind::scaleOctave2;
                const auto bytes = wide ? 24 : 12;

                if (at + bytes > size)
                    return {};

                for (int i = 0; i < 12; ++i)
                    change.octaveOffsets.add (wide ? offsetFromBytes (data[at + i * 2], data[at + i * 2 + 1])
                                                   : offsetFromByte  (data[at + i]));

                // No checksum on the real-time or setup forms — only the dumps
                // below carry one.
                return change;
            }

            case Kind::scaleOctaveDump1:
            case Kind::scaleOctaveDump2:
            {
                int value = 0;

                if (! take (value))
                    return {};

                change.bank = value;

                if (! take (value))
                    return {};

                change.program = value;

                const auto wide  = change.kind == Kind::scaleOctaveDump2;
                const auto bytes = wide ? 24 : 12;

                if (at + nameLength + bytes + 1 > size)
                    return {};

                change.name = juce::String::createStringFromData (data + at, nameLength).trim();
                at += nameLength;

                for (int i = 0; i < 12; ++i)
                    change.octaveOffsets.add (wide ? offsetFromBytes (data[at + i * 2], data[at + i * 2 + 1])
                                                   : offsetFromByte  (data[at + i]));

                if (! checksumIsAcceptable (change.kind, data, size))
                    return {};

                // A dump names no channels, so it addresses all of them.
                return change;
            }
        }

        return {};
    }

    //==========================================================================
    /** Applies a change to a table.

        Two rules worth keeping in one place. An empty `frequency` means *leave
        this key alone*, so the table is not written; and a scale/octave message
        "offsets from the original preset, [so] repeated messages do not
        accumulate" — which is why the offsets are applied to equal temperament
        rather than to whatever is currently stored.
    */
    inline void apply (const Change& change, TuningTable& table)
    {
        const auto channels = change.channels.isEmpty()
                                  ? juce::Array<int> { TuningTable::allChannels }
                                  : change.channels;

        if (change.isScaleOctave())
        {
            for (const auto channel : channels)
                for (int note = 0; note < 128; ++note)
                {
                    const auto offset = change.octaveOffsets[note % 12];

                    table.set (note, channel,
                               standardTuning::frequencyFor (note) * std::pow (2.0, offset / 1200.0));
                }

            return;
        }

        for (const auto& note : change.notes)
        {
            if (! note.frequency.has_value())
                continue;   // 7F 7F 7F: leave it as it was

            for (const auto channel : channels)
                table.set (note.note, channel, note.frequency);
        }
    }
}

} // namespace microtonos::sidebar
