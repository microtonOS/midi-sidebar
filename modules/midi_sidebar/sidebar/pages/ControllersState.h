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

        The same pair, under the same two names, is the first half of the
        channels page's four-way mode — see `channels::Mode`. There it says what
        the whole plugin does; here it says what one mapping does, which is why
        both exist. */
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

        /** A sentence saying what the parameter does, shown by the right-click
            menu's `info` item — see docs/right-click.md. The developer's words:
            this module knows a name and a unit, and nothing about what turning
            the thing actually does. Empty is allowed and disables the item. */
        juce::String info;
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
    /** The channel as the menus and summaries say it. */
    inline juce::String channelName (int channel)
    {
        return channel == omniOnChannel  ? juce::String ("omni on")
             : channel == omniOffChannel ? juce::String ("omni off")
                                         : "channel " + juce::String (channel);
    }

    /** What a parameter's assignment says in one line, for the right-click
        menu — docs/right-click.md gives the wording and every case below is one
        of its examples.

        Nothing about how many mappings there are is worth spelling out beyond
        two: past that the line would be a list rather than a summary, and the
        thing to do with more than one is open the table. */
    inline juce::String assignmentSummary (const juce::Array<Mapping>& mappings, int parameterIndex)
    {
        const Mapping* only = nullptr;

        for (const auto& mapping : mappings)
        {
            if (mapping.parameterIndex != parameterIndex)
                continue;

            if (only != nullptr)
                return "multiple assignments";

            only = &mapping;
        }

        if (only == nullptr)
            return "not assigned";

        auto text = channelName (only->channel);

        if (only->source != Source::control)
            return text + " " + sourceNames[static_cast<int> (only->source)];

        // An incomplete row says so rather than showing "MSB" with nothing
        // after it. `add` leaves one in exactly this state.
        if (! only->msb.has_value())
            return text + " no MSB";

        text << " MSB " << *only->msb;

        // The LSB is only part of the assignment where the mode reads one; the
        // two threshold modes ignore it, and saying otherwise would describe a
        // number that has no effect.
        const auto readsLsb = only->mode != Mode::toggle && only->mode != Mode::increment;

        if (readsLsb && only->lsb.has_value())
            text << " LSB " << *only->lsb;

        return text;
    }

    //==========================================================================
    /** How many recent messages the monitor shows, one to a line.

        Text rather than a table of columns it never scrolls, which is what it
        was: at 260px the columns cost more than they explain. Two lines rather
        than one because a single line is a glimpse rather than a monitor — you
        cannot tell a repeating controller from a stuck one — and because the
        longest line, a continuous controller with both an MSB and an LSB, is
        the one that most wants a neighbour to be read against.

        Three, which is what fits: the block is `metrics::pageTopRows` tall
        because that is what the other pages need, and three lines of body text
        fit inside it. A third line part-hidden would still be worth having —
        it says a message was there even when it cannot quite be read — but at
        these sizes it is not clipped.

        Each line is composed by whoever is reading the MIDI: turning note 69
        into "A4" needs to know how the instrument names its notes, which is
        exactly the knowledge this module does not have and should not
        acquire. */
    inline constexpr int monitorLines = 3;

}

} // namespace microtonos::sidebar
