#pragma once

#include <array>
#include <optional>

#include <juce_audio_basics/juce_audio_basics.h>

#include "../pages/TuningState.h"

namespace microtonos::sidebar
{

//==============================================================================
/** What every tuning scheme produces: a frequency for each note, per channel.

    docs/tuning.md:8-9 — "MIDI Sidebar saves a table of frequencies per note per
    channel. In addition, there is a list frequencies per note for an
    *unspecified channel*." Both live here, and `frequencyFor` is what resolves
    one against the other.

    **An empty entry is not zero.** A note can be *unmapped*, which is a
    different thing from being tuned to nothing: `.kbm` writes `x` for it, MTS
    sysex has a reserved "no change" frequency, and MTS-ESP answers
    `MTS_ShouldFilterNote`. A receiver that stores 0 Hz cannot tell those from a
    note it simply has not been told about, so every entry is optional and the
    owner decides what silence means.

    Fixed size and free of MIDI, files and third-party code, like the headers in
    `sidebar/midi/` — so it can be built and checked without a plugin.
*/
struct TuningTable
{
    //==========================================================================
    /** The frequency for one note, or nothing when it is unmapped. */
    using Notes = std::array<std::optional<double>, 128>;

    /** The channel-independent list. Used for any channel that has no table of
        its own, which is the usual case: a single `.scl`, or a tuning sysex
        addressed to every channel, fills this and nothing else. */
    Notes unspecified;

    /** Per-channel tables, indexed 0..15 for channels 1..16. A channel with
        nothing in it falls back to `unspecified`, so filling this is the
        exception rather than the rule. */
    std::array<Notes, 16> channels;

    /** True where `channels[i]` has been given anything at all. Kept separately
        because "every note of this channel is unmapped" is a legitimate state
        and is not the same as "this channel was never addressed". */
    std::array<bool, 16> channelUsed {};

    //==========================================================================
    /** The frequency this table gives note `note` on channel `channel` (1-16),
        or nothing if it is unmapped or unknown.

        **A channel with its own table is answered by it alone**, gaps included.
        That matters: `_3.kbm` writing `x` for a key means that key is unmapped
        on channel 3, and falling through to the channel-independent list would
        quietly map it again. `channelUsed` is what separates "this channel has a
        table with a hole in it" from "this channel has no table", and `set`
        seeds a new channel from `unspecified` so a *partial* write — one note of
        a tuning sysex — does not blank the rest. */
    std::optional<double> frequencyFor (int note, int channel) const
    {
        if (! juce::isPositiveAndBelow (note, 128))
            return {};

        const auto index = channel - 1;

        if (juce::isPositiveAndBelow (index, 16) && channelUsed[(size_t) index])
            return channels[(size_t) index][(size_t) note];

        return unspecified[(size_t) note];
    }

    /** Whether anything at all has been filled in. An empty table is what the
        page draws as "no tuning", rather than as a table of silence. */
    bool isEmpty() const
    {
        for (const auto& f : unspecified)
            if (f.has_value())
                return false;

        for (size_t i = 0; i < channels.size(); ++i)
            if (channelUsed[i])
                for (const auto& f : channels[i])
                    if (f.has_value())
                        return false;

        return true;
    }

    /** Sets one note on one channel, or on every channel when `channel` is
        `allChannels` — which is what a tuning sysex addressed to the whole
        instrument means. */
    static constexpr int allChannels = 0;

    void set (int note, int channel, std::optional<double> frequency)
    {
        if (! juce::isPositiveAndBelow (note, 128))
            return;

        if (channel == allChannels)
        {
            unspecified[(size_t) note] = frequency;
            return;
        }

        const auto index = channel - 1;

        if (! juce::isPositiveAndBelow (index, 16))
            return;

        // Seeded from the channel-independent list the first time this channel
        // is written to, because a caller may be editing **one note** — a single
        // note tuning change addressed to a few channels — and the other 127
        // should keep whatever they had rather than becoming unmapped. A caller
        // that means to define the whole channel, as a `.kbm` does, overwrites
        // all 128 anyway and never sees this.
        if (! channelUsed[(size_t) index])
        {
            channels[(size_t) index] = unspecified;
            channelUsed[(size_t) index] = true;
        }

        channels[(size_t) index][(size_t) note] = frequency;
    }

    //==========================================================================
    /** Every frequency this table gives, ascending, with unmapped notes left
        out and duplicates removed.

        What period inference is run over. Merging the channels is
        docs/tuning.md:41 — "Period inference merges all the channels and sorts
        the frequencies" — and it is the right thing: a split tuning across two
        channels is still one scale, and inferring separately would find the
        period of each half rather than of the whole. */
    juce::Array<double> sortedFrequencies() const
    {
        juce::Array<double> all;

        const auto collect = [&all] (const Notes& notes)
        {
            for (const auto& f : notes)
                if (f.has_value() && *f > 0.0)
                    all.add (*f);
        };

        collect (unspecified);

        for (size_t i = 0; i < channels.size(); ++i)
            if (channelUsed[i])
                collect (channels[i]);

        all.sort();

        // Exact duplicates only. A near-duplicate is a genuinely distinct pitch
        // as far as this is concerned, and deciding how near is too near is a
        // judgement that belongs to whoever is comparing, not here.
        for (int i = all.size() - 1; i > 0; --i)
            if (juce::exactlyEqual (all[i], all[i - 1]))
                all.remove (i);

        return all;
    }
};

//==============================================================================
/** Twelve-tone equal temperament, A4 = 440 Hz — the `standard` scheme, and the
    fallback whenever nothing else has anything to say.

    Note 69 is A4 by definition (Complete MIDI 1.0 Detailed Specification 4.2.1,
    p10 gives middle C as 60; A4 is nine semitones above). */
namespace standardTuning
{
    inline constexpr double referenceHz   = 440.0;
    inline constexpr int    referenceNote = 69;

    /** The name docs/tuning.md:22 asks for when a scheme carries none. */
    inline const juce::String name { "12edo A4=440 Hz" };

    inline double frequencyFor (int note)
    {
        return referenceHz * std::pow (2.0, (note - referenceNote) / 12.0);
    }

    inline TuningTable table()
    {
        TuningTable t;

        for (int note = 0; note < 128; ++note)
            t.unspecified[(size_t) note] = frequencyFor (note);

        return t;
    }
}

} // namespace microtonos::sidebar
