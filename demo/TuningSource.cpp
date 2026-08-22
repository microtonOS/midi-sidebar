#include "TuningSource.h"

namespace microtonos::sidebar::demo
{

namespace
{
    /** RPN 0/3 and 0/4. An RPN's number is its MSB times 128 plus its LSB, so
        "RPN 0/3" is parameter 3 — see the midi-1_0 skill on why an RPN is not a
        message. docs/tuning.md:27 names both. */
    constexpr int tuningProgramRpn = 3;
    constexpr int tuningBankRpn    = 4;

    /** "There are 128 tuning banks at most" (docs/tuning.md:28), and the same
        for programs: the dump messages carry a single byte for each. */
    constexpr int highestTuningSlot = 127;

    double centsBetween (double low, double high)
    {
        return 1200.0 * std::log2 (high / low);
    }
}

//==============================================================================
TuningSource::TuningSource()
{
    // Safe with no libMTS installed: the client reports no master and every
    // query falls back to equal temperament, so the demo runs on a clean
    // machine. Registered once for the life of the plugin rather than per
    // query — ODDSound's own instructions.
    client = MTS_RegisterClient();

    refresh();
}

TuningSource::~TuningSource()
{
    if (client != nullptr)
        MTS_DeregisterClient (client);
}

//==============================================================================
void TuningSource::setScheme (tuning::Scheme newScheme)
{
    if (scheme == newScheme)
        return;

    scheme = newScheme;

    // Nothing is discarded: the sysex table and the loaded programs stay where
    // they are, so switching away and back restores what was set up. That is
    // what TuningState.h asks the owner to do, and it is the only reason the
    // stores are separate members rather than one.
    refresh();
}

//==============================================================================
void TuningSource::handleSysex (const juce::MidiMessage& message)
{
    const auto change = mtsSysex::parse (message);

    if (! change.has_value())
        return;

    mtsSysex::apply (*change, sysexTable);

    if (change->name.isNotEmpty())
        sysexName = change->name;

    // A dump names the slot it belongs to; a real-time change names the slot it
    // is editing, which may not be the one in force. Both are recorded, because
    // the status block shows where the tuning came from either way.
    if (change->program.has_value())
        sysexProgram = change->program;

    if (change->bank.has_value())
        sysexBank = change->bank;

    updated = juce::Time::getCurrentTime();

    if (scheme == tuning::Scheme::mtsSysex)
        refresh();
}

bool TuningSource::handleRpn (const juce::MidiRPNMessage& rpn)
{
    if (rpn.isNRPN)
        return false;

    if (rpn.parameterNumber != tuningProgramRpn && rpn.parameterNumber != tuningBankRpn)
        return false;

    // The value is 0-127 on the wire and shown as 1-128, but only a single byte
    // is meaningful: "a sender adding an LSB would be selecting something the
    // dump format has no way to name" (midi-microtuning/references/mts-sysex.md).
    // `MidiRPNDetector` reports 14 bits when a data LSB arrived, so the top
    // seven are taken and the rest ignored rather than trusted.
    const auto slot = juce::jlimit (0, highestTuningSlot,
                                    rpn.is14BitValue ? rpn.value >> 7 : rpn.value);

    if (rpn.parameterNumber == tuningProgramRpn)
        setProgram (slot);
    else
        setBank (slot);

    return true;
}

//==============================================================================
void TuningSource::loadFiles (const juce::Array<juce::File>& files)
{
    auto loaded = scalaFiles::loadAll (files);

    if (loaded.isEmpty())
        return;

    // One selection is one bank, however many programs it holds — which is what
    // makes "a selection of several directories generates a set of banks"
    // (docs/tuning.md:62) work by calling this once per directory.
    bankStarts.add (programs.size());
    programs.addArray (loaded);

    fileProgram = juce::jlimit (0, programs.size() - 1, fileProgram);
    scheme = tuning::Scheme::tuningFile;

    updated = juce::Time::getCurrentTime();
    refresh();
}

void TuningSource::loadBank (const juce::File& directory)
{
    loadFiles ([&directory]
    {
        juce::Array<juce::File> files;

        for (const auto& entry : juce::RangedDirectoryIterator (directory, false, "*.scl;*.kbm",
                                                                juce::File::findFiles))
            files.add (entry.getFile());

        return files;
    }());
}

void TuningSource::clearFiles()
{
    programs.clearQuick();
    bankStarts.clearQuick();
    fileProgram = 0;

    refresh();
}

juce::String TuningSource::loadedSummary() const
{
    if (programs.isEmpty())
        return {};

    const auto banks = juce::jmax (1, bankStarts.size());

    auto text = juce::String (programs.size())
              + (programs.size() == 1 ? " tuning" : " tunings");

    if (banks > 1)
        text << " in " << banks << " banks";

    return text;
}

//==============================================================================
const juce::Array<scalaFiles::Program>* TuningSource::filePrograms() const
{
    return scheme == tuning::Scheme::tuningFile && ! programs.isEmpty() ? &programs : nullptr;
}

void TuningSource::setProgram (std::optional<int> program)
{
    if (! program.has_value())
        return;

    if (scheme == tuning::Scheme::tuningFile)
        fileProgram = juce::jlimit (0, juce::jmax (0, programs.size() - 1), *program);
    else
        sysexProgram = juce::jlimit (0, highestTuningSlot, *program);

    updated = juce::Time::getCurrentTime();
    refresh();
}

void TuningSource::setBank (std::optional<int> bank)
{
    if (! bank.has_value())
        return;

    if (scheme == tuning::Scheme::tuningFile)
    {
        // A file bank is a run of programs, so selecting one means jumping to
        // where it starts rather than storing a number.
        const auto index = juce::jlimit (0, juce::jmax (0, bankStarts.size() - 1), *bank);

        if (! bankStarts.isEmpty())
            fileProgram = bankStarts[index];
    }
    else
    {
        sysexBank = juce::jlimit (0, highestTuningSlot, *bank);
    }

    updated = juce::Time::getCurrentTime();
    refresh();
}

juce::StringArray TuningSource::availableNames() const
{
    juce::StringArray names;

    if (const auto* list = filePrograms())
        for (const auto& program : *list)
            names.add (program.name);

    return names;
}

void TuningSource::chooseName (int index)
{
    if (filePrograms() != nullptr && juce::isPositiveAndBelow (index, programs.size()))
    {
        fileProgram = index;
        refresh();
    }
}

//==============================================================================
void TuningSource::refreshFromMtsEsp()
{
    table = {};

    if (client == nullptr)
        return;

    // Per channel rather than only the unspecified list, because MTS-ESP
    // supports multi-channel tables and asking per channel is how the client
    // reports which. A master with one table answers the same for all sixteen,
    // which the table then stores sixteen times — cheap, and it keeps this from
    // having to guess whether multi-channel is in use.
    for (int channel = 0; channel < 16; ++channel)
    {
        for (int note = 0; note < 128; ++note)
        {
            if (MTS_ShouldFilterNote (client, (char) note, (signed char) channel))
                continue;   // unmapped: left empty, which is not the same as silent

            table.channels[(size_t) channel][(size_t) note] =
                MTS_NoteToFrequency (client, (char) note, (signed char) channel);
        }

        table.channelUsed[(size_t) channel] = true;
    }
}

void TuningSource::refresh()
{
    switch (scheme)
    {
        case tuning::Scheme::mtsEsp:
            refreshFromMtsEsp();
            break;

        case tuning::Scheme::mtsSysex:
            table = sysexTable;
            break;

        case tuning::Scheme::tuningFile:
            if (juce::isPositiveAndBelow (fileProgram, programs.size()))
                table = programs[fileProgram].table;
            else
                table = {};
            break;

        case tuning::Scheme::standard:
            table = standardTuning::table();
            break;

        case tuning::Scheme::midi2:
            // Not implemented: MIDI 2.0 per-note pitch needs UMP, which nothing
            // here reads yet, and finding A in TODO.md is still open on what the
            // plugin should ask of a MIDI 2.0 device. Standard tuning rather
            // than an empty table, so the demo still makes sense.
            table = standardTuning::table();
            break;
    }

    // An empty table is a scheme with nothing in it — no master, no sysex yet,
    // no files loaded. Equal temperament is the honest fallback: the plugin has
    // to play *something*, and docs/tuning.md's own default name says as much.
    if (table.isEmpty())
        table = standardTuning::table();
}

//==============================================================================
bool TuningSource::hasMaster() const
{
    return client != nullptr && MTS_HasMaster (client);
}

tuning::Status TuningSource::getStatus() const
{
    tuning::Status status;

    switch (scheme)
    {
        case tuning::Scheme::mtsEsp:
            if (hasMaster())
                status.name = juce::String (MTS_GetScaleName (client)).trim();

            break;

        case tuning::Scheme::mtsSysex:
            status.name    = sysexName;
            status.program = sysexProgram;
            status.bank    = sysexBank;
            break;

        case tuning::Scheme::tuningFile:
            if (juce::isPositiveAndBelow (fileProgram, programs.size()))
            {
                status.name    = programs[fileProgram].name;
                status.program = fileProgram;

                // Which bank the current program falls in: the last start at or
                // before it.
                for (int i = 0; i < bankStarts.size(); ++i)
                    if (bankStarts[i] <= fileProgram)
                        status.bank = i;
            }
            break;

        case tuning::Scheme::standard:
        case tuning::Scheme::midi2:
            status.name = standardTuning::name;
            break;
    }

    // Empty is "no name", which the page draws as a placeholder rather than as
    // a value — see TuningState.h. Standard tuning is the one scheme that always
    // has one.
    if (status.name.isEmpty() && scheme != tuning::Scheme::mtsEsp)
        status.name = standardTuning::name;

    if (updated != juce::Time())
        status.updated = updated;

    // Under MTS-ESP the clock is the connection: a master answers several times
    // a second, and a stopped clock is how a dropped connection shows.
    if (scheme == tuning::Scheme::mtsEsp && hasMaster())
        status.updated = juce::Time::getCurrentTime();

    return status;
}

tuning::Period TuningSource::getPeriod() const
{
    const auto inferred = periodInference::periodFor (table.sortedFrequencies());

    // A Scala file states its period as its last tone, so it is specified and
    // never guessed at.
    if (scheme == tuning::Scheme::tuningFile
        && juce::isPositiveAndBelow (fileProgram, programs.size()))
    {
        if (const auto stated = programs[fileProgram].periodCents)
        {
            tuning::Period period;

            period.cents      = *stated;
            period.source     = tuning::PeriodSource::specified;
            period.candidates = inferred.candidates;

            return period;
        }
    }

    if (scheme == tuning::Scheme::mtsEsp && hasMaster())
    {
        const auto ratio = MTS_GetPeriodRatio (client);

        // ⚠️ This "returns 2.0 (12 semitones) if not supplied by a master", so
        // an octave and *no answer* are the same value. A master that really did
        // say 2.0 and one that said nothing cannot be told apart — so 2.0 counts
        // as specified only when inference agrees it is an octave, and is
        // treated as unstated otherwise. Any other value is unambiguous.
        const auto statedOctave = std::abs (ratio - 2.0) < 1.0e-9;
        const auto inferredOctave = inferred.cents.has_value()
                                 && std::abs (*inferred.cents - 1200.0) < 1.0e-6;

        if (ratio > 0.0 && (! statedOctave || inferredOctave))
        {
            tuning::Period period;

            period.cents      = 1200.0 * std::log2 (ratio);
            period.source     = tuning::PeriodSource::specified;
            period.candidates = inferred.candidates;

            return period;
        }
    }

    return inferred;
}

//==============================================================================
std::optional<double> TuningSource::intervalFor (int lowestNote, int highestNote,
                                                 int lowestChannel, int highestChannel) const
{
    const auto low  = table.frequencyFor (lowestNote, lowestChannel);
    const auto high = table.frequencyFor (highestNote, highestChannel);

    if (! low.has_value() || ! high.has_value() || *low <= 0.0 || *high <= 0.0)
        return {};

    // Through the table rather than from the note numbers, so this is the
    // interval actually sounding rather than the twelve-tone one — which is the
    // whole point of showing it on a microtuning page.
    return centsBetween (*low, *high);
}

} // namespace microtonos::sidebar::demo
