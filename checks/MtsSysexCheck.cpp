// The MIDI Tuning Standard system exclusive messages.
//
// Byte layouts from .claude/skills/midi-microtuning/references/mts-sysex.md,
// which cites the MIDI Tuning Updated Specification along with CA-020 (bank and
// dump extensions) and CA-021/RP-020 (scale/octave). The same fixtures are
// generated language-agnostically by that skill's scripts/mts_sysex.py.
#include <midi_sidebar/midi_sidebar.h>

#include <cstring>

#include "Check.h"

using namespace microtonos::sidebar;
using namespace checks;

namespace
{
    using Bytes = std::vector<juce::uint8>;

    juce::MidiMessage sysex (Bytes b)
    {
        return juce::MidiMessage::createSysExMessage (b.data(), (int) b.size());
    }

    /** XOR of everything but the checksum slot, masked to 7 bits — computed
        here independently of the parser's own version. */
    juce::uint8 checksumOver (const Bytes& body)
    {
        int sum = 0;
        for (auto b : body) sum ^= b;
        return (juce::uint8) (sum & 0x7f);
    }

    /** 16 ASCII characters, space-padded. Stops at the terminator rather than
        indexing past it, which is a real trap: reading name[i] beyond the
        literal's length picks up whatever the linker put next in .rodata. */
    void appendName (Bytes& b, const char* name)
    {
        const auto length = (int) std::strlen (name);

        for (int i = 0; i < 16; ++i)
            b.push_back ((juce::uint8) (i < length ? name[i] : ' '));
    }

    /** The three frequency bytes: semitone, then a 14-bit fraction of 100 c. */
    void appendFrequency (Bytes& b, int semitone, int fraction = 0)
    {
        b.push_back ((juce::uint8) semitone);
        b.push_back ((juce::uint8) ((fraction >> 7) & 0x7f));
        b.push_back ((juce::uint8) (fraction & 0x7f));
    }

    /** One [kk xx yy zz] of a single-note change: key, then frequency. Four
        bytes, not three. */
    void appendChange (Bytes& b, int key, int semitone, int fraction = 0)
    {
        b.push_back ((juce::uint8) key);
        appendFrequency (b, semitone, fraction);
    }
}

int main()
{
    //  The frequency encoding ---------------------------------------------------
    {
        near (*mtsSysex::frequencyFromBytes (0x45, 0x00, 0x00), 440.0,    1e-9, "45 00 00 is A-440");
        near (*mtsSysex::frequencyFromBytes (0x3c, 0x00, 0x00), 261.6256, 1e-3, "3C 00 00 is middle C");
        near (*mtsSysex::frequencyFromBytes (0x00, 0x00, 0x00), 8.1758,   1e-3, "00 00 00 is MIDI key 0");
        near (*mtsSysex::frequencyFromBytes (0x7f, 0x00, 0x00), 12543.88, 1e-1, "7F 00 00 is MIDI key 127");
        near (*mtsSysex::frequencyFromBytes (0x45, 0x00, 0x01), 440.0016, 1e-3, "one step is .0061 cents");
        near (*mtsSysex::frequencyFromBytes (0x7f, 0x7f, 0x7e), 13289.73, 1.0,  "7F 7F 7E is the top");

        check (! mtsSysex::frequencyFromBytes (0x7f, 0x7f, 0x7f).has_value(),
               "7F 7F 7F is reserved and means 'no change', not a frequency");
    }

    //  08 01 bulk tuning dump ---------------------------------------------------
    {
        Bytes b { 0x7e, 0x7f, 0x08, 0x01, 0x05 };
        appendName (b, "Test tuning");
        for (int n = 0; n < 128; ++n) appendFrequency (b, n);
        b.push_back (0x00);   // deliberately WRONG

        const auto change = mtsSysex::parse (sysex (b));

        check (change.has_value(), "a bulk dump parses");
        check (change->kind == mtsSysex::Kind::bulkDump, "and is recognised as one");
        check (change->program == 5, "program 5");
        check (! change->bank.has_value(), "no bank on 08 01");
        check (change->name == "Test tuning", "the name loses its padding");
        eq (change->notes.size(), 128, "128 notes");
        check (! change->affectsSoundingNotes, "7E does not touch sounding notes");
        near (*change->notes[69].frequency, 440.0, 1e-9, "note 69 is A-440");
        check (true, "and its bad checksum is tolerated - the spec recommends ignoring it here");
    }

    //  08 04 key-based dump: the checksum IS enforced ---------------------------
    {
        const auto build = [] (bool goodChecksum)
        {
            Bytes body { 0x7e, 0x7f, 0x08, 0x04, 0x02, 0x05 };
            appendName (body, "Keyed");
            for (int n = 0; n < 128; ++n) appendFrequency (body, n);

            Bytes b = body;
            b.push_back (goodChecksum ? checksumOver (body) : (juce::uint8) 0x01);
            return sysex (b);
        };

        const auto good = mtsSysex::parse (build (true));

        check (good.has_value(), "a key-based dump with a good checksum parses");
        check (good.has_value() && good->bank == 2 && good->program == 5, "bank 2, program 5");
        check (! mtsSysex::parse (build (false)).has_value(),
               "and one with a bad checksum is refused");
    }

    //  08 02 single note tuning change -------------------------------------------
    {
        Bytes b { 0x7f, 0x7f, 0x08, 0x02, 0x00, 0x02 };
        appendChange (b, 60, 60);
        appendChange (b, 69, 0);

        const auto change = mtsSysex::parse (sysex (b));

        check (change.has_value(), "a single note change parses");
        check (change->affectsSoundingNotes, "7F affects sounding notes");
        eq (change->notes.size(), 2, "two changes batched in one message");
        eq (change->notes[0].note, 60, "the first is key 60");
        eq (change->notes[1].note, 69, "the second is key 69");
        near (*change->notes[1].frequency, 8.1758, 1e-3, "which has been dragged right down");
        check (change->channels.isEmpty(), "no channel bitmap, so every channel");
    }

    //  08 07, whose two forms differ only by the header --------------------------
    {
        Bytes rt { 0x7f, 0x7f, 0x08, 0x07, 0x03, 0x04, 0x01 };
        appendChange (rt, 64, 64);

        const auto a = mtsSysex::parse (sysex (rt));
        check (a.has_value() && a->bank == 3 && a->program == 4,
               "08 07 real-time carries a bank and a program");
        check (a.has_value() && a->affectsSoundingNotes, "and updates sounding notes");

        Bytes nrt = rt;
        nrt[0] = 0x7e;

        const auto b = mtsSysex::parse (sysex (nrt));
        check (b.has_value() && ! b->affectsSoundingNotes,
               "08 07 non-real-time is the same payload, staged rather than applied");
    }

    //  08 08 and 08 09, scale/octave ---------------------------------------------
    {
        // Channels 1 and 16 only: hh bit 0, ff bit 1.
        Bytes one { 0x7f, 0x7f, 0x08, 0x08, 0x02, 0x00, 0x01 };
        for (int i = 0; i < 12; ++i)
            one.push_back ((juce::uint8) (i == 0 ? 0x40 - 14 : 0x40));

        const auto a = mtsSysex::parse (sysex (one));

        check (a.has_value() && a->isScaleOctave(), "scale/octave 1-byte parses");
        eq (a->octaveOffsets.size(), 12, "twelve offsets, one per pitch class");
        near (a->octaveOffsets[0], -14.0, 1e-9, "40h less 14 is -14 cents");
        near (a->octaveOffsets[1], 0.0, 1e-9, "40h is equal temperament");
        check (a->channels.size() == 2 && a->channels[0] == 1 && a->channels[1] == 16,
               "the bitmap names channels 1 and 16 - note the unusual bit order");

        Bytes two { 0x7f, 0x7f, 0x08, 0x09, 0x00, 0x00, 0x7f };
        for (int i = 0; i < 12; ++i) { two.push_back (0x40); two.push_back (0x00); }

        const auto b = mtsSysex::parse (sysex (two));

        check (b.has_value() && b->isScaleOctave(), "scale/octave 2-byte parses");
        near (b->octaveOffsets[0], 0.0, 1e-9, "40h 00h is equal temperament");
        eq (b->channels.size(), 7, "hh = 7F names channels 1 to 7");
    }

    //  What must be refused --------------------------------------------------------
    {
        check (! mtsSysex::parse (sysex ({ 0x7e, 0x7f, 0x08, 0x00, 0x05 })).has_value(),
               "a dump *request* carries no tuning, so it is not a change");
        check (! mtsSysex::parse (sysex ({ 0x7d, 0x7f, 0x08, 0x02 })).has_value(),
               "a non-universal id is not MTS");
        check (! mtsSysex::parse (sysex ({ 0x7e, 0x7f, 0x06, 0x02 })).has_value(),
               "sub-ID#1 06 is not MIDI tuning");
        check (! mtsSysex::parse (sysex ({ 0x7f, 0x7f, 0x08 })).has_value(),
               "a truncated header is refused");
        check (! mtsSysex::parse (sysex ({ 0x7f, 0x7f, 0x08, 0x02, 0x00, 0x08, 60 })).has_value(),
               "so is a message claiming more changes than it carries");
        check (! mtsSysex::parse (juce::MidiMessage::noteOn (1, 60, (juce::uint8) 100)).has_value(),
               "a note is not sysex");
    }

    //  Applying to a table -----------------------------------------------------------
    {
        TuningTable table;

        Bytes b { 0x7f, 0x7f, 0x08, 0x02, 0x00, 0x01 };
        appendChange (b, 60, 0x45);

        mtsSysex::apply (*mtsSysex::parse (sysex (b)), table);

        near (*table.frequencyFor (60, 1), 440.0, 1e-9, "apply retunes key 60 on channel 1");
        near (*table.frequencyFor (60, 9), 440.0, 1e-9, "and on channel 9, there being no bitmap");

        Bytes untouched { 0x7f, 0x7f, 0x08, 0x02, 0x00, 0x01 };
        untouched.push_back (60);
        untouched.push_back (0x7f); untouched.push_back (0x7f); untouched.push_back (0x7f);

        mtsSysex::apply (*mtsSysex::parse (sysex (untouched)), table);
        near (*table.frequencyFor (60, 1), 440.0, 1e-9,
              "7F 7F 7F leaves the stored value exactly as it was");

        // Offsets are applied to the preset, not to the modified tuning, so the
        // same message twice must not accumulate.
        TuningTable octave;
        Bytes so { 0x7f, 0x7f, 0x08, 0x08, 0x00, 0x00, 0x01 };
        for (int i = 0; i < 12; ++i) so.push_back ((juce::uint8) (0x40 + 10));

        const auto change = *mtsSysex::parse (sysex (so));

        mtsSysex::apply (change, octave);
        const auto once = *octave.frequencyFor (69, 1);
        mtsSysex::apply (change, octave);

        near (*octave.frequencyFor (69, 1), once, 1e-12,
              "scale/octave offsets do not accumulate when repeated");
        near (once, 440.0 * std::pow (2.0, 10.0 / 1200.0), 1e-9, "+10 cents on A-440");
    }

    //  A partial write must not blank the rest of a channel ---------------------------
    {
        // The bug this catches: seeding a per-channel table from nothing meant a
        // single-note change addressed to one channel left its other 127 notes
        // unmapped.
        auto table = standardTuning::table();

        Bytes b { 0x7f, 0x7f, 0x08, 0x08, 0x00, 0x00, 0x01 };   // channel 1 only
        for (int i = 0; i < 12; ++i) b.push_back (0x40);

        mtsSysex::apply (*mtsSysex::parse (sysex (b)), table);

        check (table.frequencyFor (60, 1).has_value(), "the addressed channel keeps its other notes");
        check (table.frequencyFor (60, 2).has_value(), "and an unaddressed channel is untouched");
    }

    return report ("MtsSysexCheck");
}
