#pragma once

#include <optional>

#include <juce_gui_basics/juce_gui_basics.h>

namespace microtonos::sidebar
{

//==============================================================================
/** Everything the tuning page displays and everything it can be asked for.

    Deliberately free of MIDI: no MTS-ESP handle, no sysex, no `.scl` parser. A
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
        so that toggling back restores what was set up.

        **MPE is not one of them.** MPE is not a way of being told what to play
        but a way of laying voices across channels, and it applies whichever of
        these is in force — so it lives on the channels page. See
        docs/channels.md.

        Named for **where the tuning comes from**, which is what actually
        distinguishes them: an inter-process master, the MIDI stream, a file, or
        nothing at all.

        `midi1` was `mtsSysex`, which was too narrow twice over — the tuning
        RPNs (0/3 program, 0/4 bank) are MTS but are not system exclusive, and
        master and channel tuning arrive on the same route and are not MTS at
        all, being core MIDI and CA-025 Device Control. `scala` was
        `tuningFile`, and says which format rather than only that there is one.

        The order is the sketch's, and a menu index is the enum's value — so
        this list and `schemeNames` in TuningPage.cpp have to move together. */
    enum class Scheme
    {
        mtsEsp,     ///< An MTS-ESP master, over inter-process shared memory.
        midi1,      ///< The MIDI stream: MTS system exclusive, and the tuning RPNs.
        midi2,      ///< Per-note pitch over UMP. Not implemented; see TODO.md.
        scala,      ///< `.scl` and `.kbm` files.
        standard    ///< 12edo, and nothing listening.
    };

    /** When a sounding note may change pitch. MTS-ESP's distinction, applied to
        the other schemes as well. */
    enum class UpdateMode
    {
        noteOn,     ///< Pitches are fixed when the note starts.
        always      ///< A ringing note follows the tuning.
    };

    /** Where the period came from, and nothing else: either the tuning stated
        it or the plugin worked it out.

        Choosing between inferred candidates does **not** change this. An
        inferred period stays inferred however many of its candidates the
        end-user steps past — the plugin is still the one that worked out what
        the possibilities were, and picking one of them is not the same as
        stating a period the tuning did not have. */
    enum class PeriodSource
    {
        inferred,
        specified
    };

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

        /** When the tuning was last touched: an MTS-ESP query, a sysex message
            arriving, a pitchbend, or a file being loaded. Under MTS-ESP this
            ticks several times a second, which is the point — a moving clock is
            how you see that the connection is alive. */
        std::optional<juce::Time> updated;
    };

    //==========================================================================
    /** Pitch-bend sensitivity, in cents.

        Here rather than with the controllers because it is a statement about
        pitch: how far the wheel bends, in the same unit as the interval, the
        period and the modulo divisor. RPN 0 carries semitones and cents
        separately; this is the pair added up, and 200 c is two semitones, which
        is MIDI's own default. */
    inline constexpr int defaultPitchBendCents = 200;

    /** The default for MPE member channels: 48 semitones.

        Not a choice of ours. On receiving an MPE Configuration Message a
        receiver sets the manager channel to 2 semitones and every member
        channel to 48 — MIDI Polyphonic Expression, M1-100-UM v1.1, §2.2.5. The
        same section then allows both to be changed with RPN 0 at any time, so
        these are starting points rather than fixed values, which is why they are
        editable at all. `juce::MPEZone` uses the same two defaults. */
    inline constexpr int defaultMemberPitchBendCents = 48 * 100;

    /** RPN 0's semitone count is a 7-bit field, so 127 semitones is the largest
        range the message can express — the protocol's ceiling rather than a
        judgement about what is musical. */
    inline constexpr int highestPitchBendCents = 127 * 100;

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

    /** The scale's period, in cents, where that number came from, and — when it
        was inferred — what else it could have been.

        Inference rarely has one answer: any multiple of the repeating interval
        is a period, so 12edo admits 100c, 200c, and so on past 1200c. The
        plugin narrows that to a plausible set and the end-user picks from it;
        `cents` is the one currently in force and should be one of the
        candidates. A `specified` period needs no candidates — the tuning said
        what it was — and an empty list means there is nothing to choose.
    */
    struct Period
    {
        std::optional<double> cents;
        PeriodSource source = PeriodSource::inferred;
        juce::Array<double> candidates;
    };
}

} // namespace microtonos::sidebar
