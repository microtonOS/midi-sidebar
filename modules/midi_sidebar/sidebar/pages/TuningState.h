#pragma once

#include <optional>

#include <juce_gui_basics/juce_gui_basics.h>

namespace microtonos::sidebar
{

//==============================================================================
/** Everything the tuning page displays and everything it can be asked for.

    Deliberately free of MIDI: no MTS ESP handle, no sysex, no `.scl` parser. A
    tuning arrives here as text and numbers that have already been worked out
    somewhere else, which is what lets the page be built, looked at and changed
    before any of that exists — and lets it stay unchanged when it does.

    See docs/tuning.md for what each field means and which standards can supply
    it. Where a value is `std::optional`, "not known" is a state the page has to
    draw: the spec asks for an empty box rather than a zero.
*/
namespace tuning
{
    //==========================================================================
    /** How the plugin is being told what to play. One at a time; the page
        remembers nothing else about the others, but the owner is expected to,
        so that toggling back restores what was set up. */
    enum class Scheme
    {
        mtsEsp,
        mtsSysex,
        tuningFile,
        mpe,
        midi2,
        standard
    };

    /** When a sounding note may change pitch. MTS ESP's distinction, applied to
        the other schemes as well. */
    enum class UpdateMode
    {
        noteOn,     ///< Pitches are fixed when the note starts.
        always      ///< A ringing note follows the tuning.
    };

    /** Where the period came from. Read-only to the end-user: typing in the
        period field is what makes it `edited`, and the two things that can put
        it back are a tuning that states its period (`specified`) and the
        plugin working one out (`inferred`). */
    enum class PeriodSource
    {
        inferred,
        specified,
        edited
    };

    /** MIDI channels, one bit per channel, bit 0 being channel 1. A mask rather
        than a set of 16 flags because that is how it will have to be handed to
        anything that acts on it, and because "no channels selected" — legal,
        if unadvisable — then needs no special case. */
    using ChannelMask = juce::uint16;

    inline constexpr int numChannels = 16;
    inline constexpr ChannelMask allChannels  = 0xffff;
    inline constexpr ChannelMask noChannels   = 0x0000;

    inline constexpr bool isChannelSet (ChannelMask mask, int channelIndex) noexcept
    {
        return (mask & (ChannelMask) (1u << channelIndex)) != 0;
    }

    inline constexpr ChannelMask withChannel (ChannelMask mask, int channelIndex, bool shouldBeSet) noexcept
    {
        const auto bit = (ChannelMask) (1u << channelIndex);
        return (ChannelMask) (shouldBeSet ? (mask | bit) : (mask & ~bit));
    }

    //==========================================================================
    /** What the status block shows.

        Every field is optional in practice, because which of them a standard
        can supply varies: MPE and pitchbend carry no name at all, only some MTS
        sysex messages carry one, and program and bank exist only where tuning
        programs do.
    */
    struct Status
    {
        /** Empty means unknown, which the page draws as "Unnamed" — the word
            the spec asks for, kept here as a placeholder rather than as a value
            so that a tuning genuinely called "Unnamed" is not indistinguishable
            from one with no name. */
        juce::String name;

        std::optional<int> program;
        std::optional<int> bank;

        /** When the tuning was last touched: an MTS ESP query, a sysex message
            arriving, a pitchbend, or a file being loaded. Under MTS ESP this
            ticks several times a second, which is the point — a moving clock is
            how you see that the connection is alive. */
        std::optional<juce::Time> updated;
    };

    /** An octave. The default divisor, not a limit — the end-user can set any
        other, which is the whole reason the field is editable. */
    inline constexpr double defaultModDivisor = 1200.0;

    /** The interval between the lowest and the highest sounding note, in cents,
        and its remainder over the divisor the end-user chose. Empty when
        nothing is sounding. */
    struct Interval
    {
        std::optional<double> cents;
        double modDivisor = defaultModDivisor;
    };

    /** The scale's period, in cents, and where that number came from. */
    struct Period
    {
        std::optional<double> cents;
        PeriodSource source = PeriodSource::inferred;
    };
}

} // namespace microtonos::sidebar
