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
    /** A mapping listens on one channel, 1 to 16, and that is the whole of it.

        There used to be `omni on` and `omni off` entries above the numbers here.
        They are gone: the channels page already says which channels the plugin
        listens to at all, and having the same question answered twice — once for
        the plugin and once per mapping — meant two settings that could disagree
        with no rule saying which won.

        So this column is now what it looks like: a channel number, typed like an
        MSB or an LSB. **The channels page may still override it functionally** —
        a mapping on a channel the page has muted hears nothing — but that is the
        page filtering, not this value changing. The number stays 1 to 16
        whatever the page is set to, so nothing here has to be recomputed when
        the page changes. */
    inline constexpr int firstChannel = 1;
    inline constexpr int lastChannel  = 16;

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
        int channel = firstChannel;

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
    /** The channel as the summaries say it. Spelled out rather than abbreviated:
        the table's column header is `ch` because it has 44px, but a summary line
        is prose and reads better with the word. */
    inline juce::String channelName (int channel)
    {
        return "channel " + juce::String (channel);
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
