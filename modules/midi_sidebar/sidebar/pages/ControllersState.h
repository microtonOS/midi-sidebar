#pragma once

#include <optional>

#include <juce_gui_basics/juce_gui_basics.h>

namespace microtonos::sidebar
{

//==============================================================================
/** Everything the controllers page displays and everything it can be asked for.

    Deliberately free of MIDI, like `tuning`: nothing here parses a message or
    acts on a mode. A mapping is a row the end-user filled in, and what it means
    is somebody else's problem — which is what lets the page be built and looked
    at before any of that exists.

    See docs/controllers.md for the specification.
*/
namespace controllers
{
    //==========================================================================
    /** How a controller moves the parameter it is mapped to.

        The first three are the Korg minilogue xd's knob modes, cited in
        docs/controllers.md; the last two are this project's, and both ignore
        the LSB — they act on a single threshold rather than on a value.
    */
    enum class Mode
    {
        jump,       ///< The parameter jumps to wherever the controller is.
        catchUp,    ///< Nothing happens until the controller passes the current value.
        scale,      ///< Relative movement, converging on the controller's position.
        toggle,     ///< A value of 64 or more flips between min and max.
        increment   ///< A value of 64 or more steps one towards max.
    };

    /** `catch` is a keyword, so the enum cannot spell it the way the GUI does.
        Both names live here rather than at the two call sites that need them. */
    inline const juce::StringArray modeNames { "jump", "catch", "scale", "toggle", "inc" };

    /** Where the menu is divided: the modes below it ignore the LSB. */
    inline constexpr int modesBeforeSeparator = 3;

    //==========================================================================
    /** The two answers that listen to everything, and differ in what they then
        do with it. Neither is a valid MIDI channel number, so neither can be
        confused with one.

        **Omni on merges.** A controller arriving on any channel moves the
        parameter, once, for the whole instrument — the ordinary meaning of a
        knob on a front panel.

        **Omni off keeps the channels apart.** The mapping still listens to all
        sixteen, but a message on channel 5 moves only what is sounding on
        channel 5. That is what MPE does with its member channels, and it is
        deliberately *not* called MPE here: a controller can be channel-specific
        without a zone being declared — a partial MPE — and naming the value
        after the protocol would claim more than the setting says.

        The same pair, under the same two names, is what the tuning page's
        multichannel call-out offers; see `ChannelSelector`. */
    inline constexpr int omniOnChannel  = -1;
    inline constexpr int omniOffChannel =  0;

    inline constexpr int firstChannel = 1;
    inline constexpr int lastChannel  = 16;

    /** Where the rule goes in the channel menu: the two omni answers above it,
        the sixteen numbered channels below. */
    inline constexpr int channelsBeforeSeparator = 2;

    /** Menu index ↔ channel value.

        The two are numbered so that the menu — omni on, omni off, 1 … 16 — is a
        contiguous run, which makes the conversion one offset rather than a
        lookup table. A stray ±1 in any of the four places that convert would be
        silent, so both directions are written once here. */
    inline constexpr int channelForIndex (int index)   noexcept { return index + omniOnChannel; }
    inline constexpr int indexForChannel (int channel) noexcept { return channel - omniOnChannel; }

    //==========================================================================
    /** An MPE zone: which channels carry one voice each.

        `master` is 1 or 16 — a lower zone's master channel is the first, an
        upper zone's the last — and `last` is how far the members reach from it.
        Held as two channel numbers rather than as a zone-and-count because that
        is what the sketch asks the end-user for: "ch 1 to 16".

        What it costs the tuning page is in docs/controllers.md: the channels in
        this span cannot be tuned separately, since each is carrying a single
        voice, so their tuning is read from the generic channel instead. */
    struct Mpe
    {
        bool on = false;
        int master = 1;
        int last   = 16;
    };

    /** The channels an `Mpe` covers, inclusive and whichever way round it runs,
        as a bit per channel with bit 0 being channel 1 — the same shape as
        `tuning::ChannelMask`, which is where it is going. Empty when MPE is
        off, so a caller does not have to ask twice. */
    inline juce::uint16 channelsCoveredBy (const Mpe& mpe) noexcept
    {
        if (! mpe.on)
            return 0;

        juce::uint16 covered = 0;

        for (auto c = juce::jmin (mpe.master, mpe.last); c <= juce::jmax (mpe.master, mpe.last); ++c)
            covered = (juce::uint16) (covered | (1u << (c - firstChannel)));

        return covered;
    }

    //==========================================================================
    /** One of the plugin's parameters, as the *developer* describes it.

        The unit belongs here rather than on a mapping: it is a property of the
        thing being controlled, not of the controller. So a row that changes
        which parameter it targets changes the unit its limits are shown in,
        with nothing to keep in step.
    */
    struct Parameter
    {
        juce::String name;
        juce::String unit;     ///< "%", "st", or empty for a bare count.
    };

    /** What kind of message drives a mapping.

        Only `control` has controller numbers. The other two are message types
        in their own right — there is no CC number to put in the MSB column —
        which is why the sketch draws the word across both of those columns
        instead. They are added by their own buttons rather than chosen from a
        menu, because the choice decides what the rest of the row means. */
    enum class Source
    {
        control,       ///< A continuous controller, with an MSB and maybe an LSB.
        aftertouch,    ///< Channel pressure: one value for the whole channel.
        polytouch      ///< Polyphonic key pressure, per sounding note.
    };

    /** What the two spanning cells say. Indexed by `Source`, so `control` has an
        entry it never uses — the alternative is a switch that has to be kept in
        step with the enum. */
    inline const juce::StringArray sourceNames { "control", "aftertouch", "polytouch" };

    /** One controller mapped to one parameter. */
    struct Mapping
    {
        int parameterIndex = 0;
        int channel = omniOnChannel;

        Source source = Source::control;

        /** Continuous controller numbers, and meaningless unless `source` is
            `control`. The LSB is optional because most mappings do not have
            one, and the modes past the separator ignore it even when they do. */
        std::optional<int> msb, lsb;

        Mode mode = Mode::jump;

        /** The ends of the parameter's travel, in whatever unit its `Parameter`
            names. Swapping them reverses the mapping — for `toggle` that is a
            polarity change, for `increment` it counts downwards. */
        double min = 0.0;
        double max = 100.0;
    };

    //==========================================================================
    /** A line of the monitor: what arrived, already formatted.

        Four strings rather than a decoded message, because the page's whole job
        is to show what came in. Turning note 69 into "A4" needs to know how the
        instrument names notes, which is exactly the knowledge this module does
        not have and should not acquire. Fields are blank where they do not
        apply — a sysex message has no channel.
    */
    struct Message
    {
        juce::String type;
        juce::String channel;
        juce::String noteOrCc;
        juce::String value;
    };

    /** How many the page shows: the newest, and only that. The monitor is a
        glance — "something is arriving, and it looks like this" — and a glance
        does not need three rows of a page that is 260px wide. */
    inline constexpr int monitorRows = 1;

    /** How many are kept, and how many the monitor shows when it is clicked.
        A tail: the oldest is pushed out rather than the newest dropped. */
    inline constexpr int monitorHistoryRows = 5;

    //==========================================================================
    /** Pitch-bend sensitivity, in cents.

        Not a row in the table: pitch bend has its own 14-bit message and a
        range of its own, so there is nothing to map it *to* — it always bends
        pitch. It sits with the monitor at the top of the page for that reason.

        Cents rather than semitones because this is a microtonal plugin and the
        rest of it already speaks cents: the tuning page's interval, its period
        and its modulo divisor are all in cents, and a sensitivity that had to
        be converted before it could be compared with them would be the only
        pitch on the sidebar that was not. RPN 0 carries semitones and cents
        separately; this is the pair added up.

        200 c is two semitones, which is MIDI's own default. */
    inline constexpr int defaultPitchBendCents = 200;

    /** RPN 0's semitone count is a 7-bit field, so 127 semitones is the largest
        range the message can express. The ceiling is the protocol's rather than
        a judgement about what is musical. */
    inline constexpr int highestPitchBendCents = 127 * 100;
}

} // namespace microtonos::sidebar
