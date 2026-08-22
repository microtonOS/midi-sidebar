#pragma once

#include <filesystem>
#include <optional>

#include <juce_gui_basics/juce_gui_basics.h>

#include <Tunings.h>

#include "TuningTable.h"

namespace microtonos::sidebar
{

//==============================================================================
/** Scala files: a `.scl` scale, and `.kbm` keyboard mappings for it.

    A thin layer over Surge's tuning-library, which does the parsing. What is
    here is the *arrangement* docs/tuning.md:57-62 describes — which file maps
    which channel, and how a directory becomes a bank — because that is this
    project's rule rather than the format's.

    **Modelled on tuneBfree's loader** (`plugin/PluginProcessor.cpp`,
    `loadSCLFile`/`loadKBMFiles`/`rebuildLocalTuning`), which had already worked
    out the same rules. Following it rather than rediscovering it; banks are the
    part it does not do and this does.
*/
namespace scalaFiles
{
    //==========================================================================
    /** One tuning program: a scale, its mappings, and what to call it. */
    struct Program
    {
        juce::String name;
        TuningTable  table;

        /** The `.scl`'s own last tone, in cents — the interval it repeats at.

            A Scala scale **states** its period rather than implying it, so a
            file-based tuning gets `PeriodSource::specified` and never needs
            inference. tuneBfree reads it the same way. */
        std::optional<double> periodCents;

        /** Empty unless something went wrong, in which case it is
            `Tunings::TuningError::what()` and worth showing. */
        juce::String error;

        bool isValid() const { return error.isEmpty() && ! table.isEmpty(); }
    };

    //==========================================================================
    /** The channel a `.kbm` filename names, or nothing for the generic mapping.

        docs/tuning.md:59 — "The suffix `_i.kbm` is the mapping for the ith
        channel." A file without a valid suffix is the default for every channel
        that has no mapping of its own. Lifted from tuneBfree's
        `kbmChannelSuffix`. */
    inline std::optional<int> channelForKbm (const juce::String& baseName)
    {
        const auto underscore = baseName.lastIndexOfChar ('_');

        if (underscore < 0 || underscore == baseName.length() - 1)
            return {};

        const auto digits = baseName.substring (underscore + 1);

        if (! digits.containsOnly ("0123456789"))
            return {};

        const auto channel = digits.getIntValue();

        return juce::isPositiveAndBelow (channel - 1, 16) ? std::optional<int> (channel)
                                                          : std::nullopt;
    }

    inline std::filesystem::path pathOf (const juce::File& file)
    {
        return std::filesystem::path (file.getFullPathName().toStdString());
    }

    //==========================================================================
    /** Fills a table from one scale and whatever mappings were given.

        The rules, all from docs/tuning.md and all matching tuneBfree:

        - a `.kbm` named `_i` maps channel *i*, the last one named winning;
        - an unsuffixed `.kbm` is the generic mapping, used for every channel
          without one of its own;
        - with no `.kbm` at all the bare `.scl` gives tuning-library's own linear
          mapping, which is docs/tuning.md:57's "a single `.scl` file sets the
          tuning for the unspecified channel".

        `isMidiNoteMapped` is how a `.kbm`'s `x` reaches us, and it becomes an
        empty entry — unmapped, not silent. See TuningTable.h.
    */
    inline Program load (const juce::File& sclFile, const juce::Array<juce::File>& kbmFiles)
    {
        Program program;

        try
        {
            const auto scale = Tunings::readSCLFile (pathOf (sclFile));

            // The file's own name line reads better than the filename, which is
            // often a slug. tuneBfree shows the same thing.
            program.name = juce::String (scale.description).trim();

            if (program.name.isEmpty())
                program.name = sclFile.getFileNameWithoutExtension();

            // The last tone of a Scala scale is its repeat interval.
            if (scale.count > 0)
                program.periodCents = scale.tones[(size_t) scale.count - 1].cents;

            std::optional<Tunings::KeyboardMapping> generic;
            std::array<std::optional<Tunings::KeyboardMapping>, 16> perChannel;

            for (const auto& file : kbmFiles)
            {
                auto mapping = Tunings::readKBMFile (pathOf (file));

                if (const auto channel = channelForKbm (file.getFileNameWithoutExtension()))
                    perChannel[(size_t) (*channel - 1)] = std::move (mapping);
                else
                    generic = std::move (mapping);
            }

            const auto fill = [&scale, &program] (TuningTable::Notes& notes,
                                                  const std::optional<Tunings::KeyboardMapping>& mapping)
            {
                const auto tuning = mapping.has_value() ? Tunings::Tuning (scale, *mapping)
                                                        : Tunings::Tuning (scale);

                for (int note = 0; note < 128; ++note)
                    notes[(size_t) note] = tuning.isMidiNoteMapped (note)
                                               ? std::optional<double> (tuning.frequencyForMidiNote (note))
                                               : std::nullopt;
            };

            fill (program.table.unspecified, generic);

            for (size_t i = 0; i < perChannel.size(); ++i)
                if (perChannel[i].has_value())
                {
                    fill (program.table.channels[i], perChannel[i]);
                    program.table.channelUsed[i] = true;
                }
        }
        catch (const Tunings::TuningError& e)
        {
            // tuning-library *throws*, so every entry point needs this. The
            // message is worth keeping: it names the line of the file that
            // failed, which is the only thing that helps an end-user.
            program.error = juce::String (e.what());
        }

        return program;
    }

    //==========================================================================
    /** Splits a selection of files into programs by shared prefix.

        docs/tuning.md:61 — "`.scl` and `.kbm` files with the same prefix are
        taken to belong to the same program." The prefix is the filename with any
        `_i` suffix removed, so `meantone.scl`, `meantone.kbm` and
        `meantone_3.kbm` are one program.

        This is the part tuneBfree does not do: it loads one scale and one batch
        of mappings, which is the single-program case below with `files` all
        sharing a prefix.
    */
    inline juce::String prefixOf (const juce::File& file)
    {
        const auto base = file.getFileNameWithoutExtension();

        if (file.hasFileExtension ("kbm"))
            if (channelForKbm (base).has_value())
                return base.upToLastOccurrenceOf ("_", false, false);

        return base;
    }

    /** Every program a set of files describes, ordered by name.

        A `.scl` with no matching `.kbm` is still a program — the linear mapping
        — and a `.kbm` with no `.scl` is not one at all, since a mapping alone
        tunes nothing. */
    inline juce::Array<Program> loadAll (const juce::Array<juce::File>& files)
    {
        juce::StringArray prefixes;

        for (const auto& file : files)
            if (file.hasFileExtension ("scl"))
                prefixes.addIfNotAlreadyThere (prefixOf (file));

        prefixes.sortNatural();

        juce::Array<Program> programs;

        for (const auto& prefix : prefixes)
        {
            juce::File scl;
            juce::Array<juce::File> kbms;

            for (const auto& file : files)
            {
                if (prefixOf (file) != prefix)
                    continue;

                if (file.hasFileExtension ("scl"))
                    scl = file;
                else if (file.hasFileExtension ("kbm"))
                    kbms.add (file);
            }

            if (scl.existsAsFile())
                programs.add (load (scl, kbms));
        }

        return programs;
    }

    /** A directory read as a bank — docs/tuning.md:60, "Selecting one directory
        creates a bank with all the tuning files in that directory."

        Not recursive: a directory of directories is several banks, which is what
        the next line of the docs describes, and the caller does that by calling
        this once per directory. */
    inline juce::Array<Program> loadBank (const juce::File& directory)
    {
        juce::Array<juce::File> files;

        for (const auto& entry : juce::RangedDirectoryIterator (directory, false, "*.scl;*.kbm",
                                                                juce::File::findFiles))
            files.add (entry.getFile());

        return loadAll (files);
    }
}

} // namespace microtonos::sidebar
