// Scala files: a .scl scale, and .kbm keyboard mappings for it.
//
// The parsing is Surge's tuning-library; what is checked here is the
// *arrangement* docs/tuning.md describes and this project adds — which file maps
// which channel, and how a directory becomes a bank.
//
// The fixtures are written on the fly rather than committed, so this needs no
// data files. The same shapes are emitted by
// .claude/skills/midi-microtuning/scripts/scales.py for use elsewhere.
#include <midi_sidebar/midi_sidebar.h>

#include "Check.h"

using namespace microtonos::sidebar;
using namespace checks;

namespace
{
    juce::File write (const juce::File& dir, const juce::String& name, const juce::String& text)
    {
        auto f = dir.getChildFile (name);
        f.replaceWithText (text);
        return f;
    }

    /** `steps` equal divisions of an octave, as Scala text. */
    juce::String equalDivisionScl (int steps, const juce::String& description)
    {
        juce::String s;

        s << "! generated.scl\n!\n" << description << "\n " << steps << "\n!\n";

        for (int i = 1; i <= steps; ++i)
            s << " " << juce::String (i * 1200.0 / steps, 5) << "\n";

        return s;
    }

    /** A mapping that leaves the black keys out, so `x` can be exercised. */
    const juce::String whiteKeysKbm =
        "! generated.kbm\n12\n0\n127\n60\n69\n440.0\n12\n"
        "0\nx\n2\nx\n4\n5\nx\n7\nx\n9\nx\n11\n";
}

int main()
{
    auto dir = juce::File::getSpecialLocation (juce::File::tempDirectory)
                   .getChildFile ("midi-sidebar-scala-check");

    dir.deleteRecursively();
    dir.createDirectory();

    const auto sclText = equalDivisionScl (12, "Twelve equal divisions");
    const auto sclFile = write (dir, "meantone.scl", sclText);

    //  A bare .scl ---------------------------------------------------------------
    {
        const auto p = scalaFiles::load (sclFile, {});

        check (p.isValid(), "a bare .scl loads");
        check (p.error.isEmpty(), "with no error");
        check (p.name == "Twelve equal divisions",
               "and takes its name from the description line, not the filename");
        near (*p.periodCents, 1200.0, 1e-6,
              "the period is the file's last tone — stated, so never inferred");
        near (*p.table.frequencyFor (69, 1), 440.0, 1e-6, "A-440 on channel 1");
        near (*p.table.frequencyFor (69, 9), 440.0, 1e-6,
              "and on channel 9, from the channel-independent list");
        check (p.table.frequencyFor (61, 1).has_value(),
               "every key is mapped when there is no .kbm");
    }

    //  A generic .kbm leaves keys unmapped ----------------------------------------
    {
        const auto p = scalaFiles::load (sclFile, { write (dir, "meantone.kbm", whiteKeysKbm) });

        check (p.isValid(), "a .kbm with no suffix loads as the generic mapping");
        check (p.table.frequencyFor (60, 1).has_value(), "C is mapped");
        check (! p.table.frequencyFor (61, 1).has_value(),
               "C# is not — 'x' means unmapped, which is not the same as silent");
    }

    //  Per-channel .kbm ------------------------------------------------------------
    {
        juce::Array<juce::File> files;
        files.add (write (dir, "meantone_3.kbm", whiteKeysKbm));

        const auto p = scalaFiles::load (sclFile, files);

        check (! p.table.frequencyFor (61, 3).has_value(),
               "_3.kbm leaves C# unmapped on channel 3");
        check (p.table.frequencyFor (61, 1).has_value(),
               "while channel 1 falls back to the linear mapping");
    }

    //  A channel's own table is authoritative, holes and all ------------------------
    {
        // The bug this catches: an explicitly-unmapped key fell through to the
        // channel-independent list and got quietly mapped again.
        juce::Array<juce::File> files;
        files.add (write (dir, "meantone.kbm", whiteKeysKbm));   // generic: maps nothing black
        files.add (write (dir, "meantone_3.kbm", whiteKeysKbm)); // and channel 3 likewise

        const auto p = scalaFiles::load (sclFile, files);

        check (! p.table.frequencyFor (61, 3).has_value(),
               "a hole in a channel's own mapping stays a hole");
    }

    //  The suffix rule ---------------------------------------------------------------
    {
        check (scalaFiles::channelForKbm ("meantone_3") == 3,  "_3 names channel 3");
        check (scalaFiles::channelForKbm ("meantone_16") == 16, "_16 names channel 16");
        check (! scalaFiles::channelForKbm ("meantone").has_value(),
               "no suffix means the generic mapping");
        check (! scalaFiles::channelForKbm ("meantone_0").has_value(),  "_0 is not a channel");
        check (! scalaFiles::channelForKbm ("meantone_17").has_value(), "_17 is out of range");
        check (! scalaFiles::channelForKbm ("meantone_").has_value(),
               "a bare underscore is not a suffix");
        check (! scalaFiles::channelForKbm ("my_tuning").has_value(),
               "nor is a non-numeric one");
    }

    //  A malformed file raises rather than crashing -----------------------------------
    {
        // tuning-library *throws*, so every entry point needs a try.
        const auto p = scalaFiles::load (write (dir, "broken.scl", "not a scale at all\n"), {});

        check (! p.isValid(), "a malformed .scl does not load");
        check (p.error.isNotEmpty(),
               "and reports why: " + p.error.substring (0, 48).replace ("\n", " "));
    }

    //  A directory is a bank -------------------------------------------------------
    {
        auto bankDir = dir.getChildFile ("bank");
        bankDir.createDirectory();

        write (bankDir, "first.scl", sclText);
        write (bankDir, "second.scl", equalDivisionScl (19, "Nineteen equal divisions"));
        write (bankDir, "second_2.kbm", whiteKeysKbm);

        const auto bank = scalaFiles::loadBank (bankDir);

        eq (bank.size(), 2, "a directory of two scales is two programs");
        check (bank[0].isValid() && bank[1].isValid(), "both load");
        check (bank[0].name != bank[1].name, "and are told apart by name");

        // The _2.kbm shares a prefix with second.scl, so it belongs to it.
        // Copied rather than pointed at: `juce::Array::operator[]` returns by
        // value, so taking its address is an address-of-temporary.
        const auto nineteen = bank[0].name.contains ("Nineteen") ? bank[0] : bank[1];
        check (! nineteen.table.frequencyFor (61, 2).has_value(),
               "a .kbm is grouped with the .scl sharing its prefix");
    }

    dir.deleteRecursively();

    return report ("ScalaCheck");
}
