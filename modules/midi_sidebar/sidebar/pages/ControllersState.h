#pragma once

#include <optional>

#include <juce_gui_basics/juce_gui_basics.h>

// `metrics::` is used below. The module is a unity build, so this happens to
// resolve without the include — SidebarLookAndFeel.h is concatenated first —
// but a header that only compiles in one include order is a header that cannot
// be used anywhere else, including from a test.
#include "../SidebarLookAndFeel.h"

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
        docs/controllers.md; the last two are this project's, and both act on a
        single threshold rather than on the controller's position.
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

    /** Where the menu is divided: the modes below it read a threshold rather
        than a position. */
    inline constexpr int modesBeforeSeparator = 3;

    //==========================================================================
    /** A mapping listens on one channel, 1 to 16, and that is the whole of it.

        There used to be `omni on` and `omni off` entries above the numbers here.
        They are gone: the channels page already says which channels the plugin
        listens to at all, and having the same question answered twice — once for
        the plugin and once per mapping — meant two settings that could disagree
        with no rule saying which won.

        So this column is now what it looks like: a channel number, typed like a
        controller number. **The channels page may still override it
        functionally** —
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
    /** How far a controller aimed at this parameter reaches.

        The developer's statement about their own plugin — the module cannot
        work it out — and it is marked beside the name in the table rather than
        explained, because it is the sort of fact you want while scanning a list
        rather than while reading. See docs/controllers.md.

        **One value, not a side plus a flag.** `perNote` and the two split sides
        are mutually exclusive rather than independent: per-note *is* a finer
        division than the split, so a controller that reaches individual notes
        does not also need to say which half of the keyboard it reaches. The
        combination has no meaning to express.

        There used to be a `global` here, marked with a globe, for a parameter
        affecting the whole plugin. It is gone: with the split named by
        *frequency* the interesting statement is which side a parameter belongs
        to, and "both" is the unmarked default rather than a third icon. */
    enum class Scope
    {
        both,      ///< Both sides of the keyboard split. Unmarked, being the usual case.
        perNote,   ///< Can modulate individual notes. Marked with the notes glyph.
        lower,     ///< Only the lower-frequencies split. Marked with the `/` glyph.
        upper      ///< Only the higher-frequencies split. Marked with the `\` glyph.
    };

    /** What values a parameter can actually take.

        Without this the `min` and `max` columns could only be believed, and a
        mapping typed with a minimum below what the parameter has would drive it
        somewhere it cannot go. The columns **restrict** the travel; they do not
        extend it, so they are clamped to this on the way in.

        Mirrors `juce::NormalisableRange`'s three fields deliberately, because
        that is where a host parameter's range comes from and a conversion with
        different names is a conversion that can be got wrong.
    */
    struct Range
    {
        double lowest   = 0.0;
        double highest  = 1.0;

        /** The step between usable values, or 0 for continuous. A bank number,
            a program number and a choice index are all `1`: typing 3.7 into one
            of those should land on 4, not on 3.7. */
        double interval = 0.0;

        /** Inside the range, and on a step if there is one. Rounds to nearest
            rather than truncating, so 3.7 becomes 4 — truncation would make
            every typed value drift downwards. */
        double snap (double value) const noexcept
        {
            const auto low  = juce::jmin (lowest, highest);
            const auto high = juce::jmax (lowest, highest);

            value = juce::jlimit (low, high, value);

            if (interval <= 0.0)
                return value;

            // Rounded relative to `low`, not to zero: a range starting at 1
            // steps 1, 2, 3, and one starting at 0.5 steps 0.5, 1.5, 2.5.
            const auto steps = std::round ((value - low) / interval);

            return juce::jlimit (low, high, low + steps * interval);
        }
    };

    struct Parameter
    {
        juce::String name;
        juce::String unit;     ///< "%", "st", or empty for a bare count.

        /** A sentence saying what the parameter does, shown by the right-click
            menu's `info` item — see docs/right-click.md. The developer's words:
            this module knows a name and a unit, and nothing about what turning
            the thing actually does. Empty is allowed and disables the item. */
        juce::String info;

        Scope scope = Scope::both;

        /** What the parameter can take. The `min` and `max` columns are clamped
            and snapped to this. */
        Range range;
    };

    /** What kind of message drives a mapping.

        Only `control` has a controller number. The other two are message types
        in their own right, so the `CC` cell holds their name instead of a
        number. They are added by their own buttons rather than chosen from a
        menu, because the choice decides what the rest of the row means. */
    enum class Source
    {
        control,       ///< A continuous controller.
        aftertouch,    ///< Channel pressure: one value for the whole channel.
        polytouch      ///< Polyphonic key pressure, per sounding note.
    };

    /** What the two spanning cells say. Indexed by `Source`, so `control` has an
        entry it never uses — the alternative is a switch that has to be kept in
        step with the enum.

        Spelled out rather than abbreviated: the word is drawn across the MSB and
        LSB columns together, which is room enough for it, and these are the
        specification's own names. */
    inline const juce::StringArray sourceNames { "control", "aftertouch", "polytouch" };

    //==========================================================================
    /** No mapping targets this parameter. Not a state an ordinary row can be
        in — a row is created by choosing a parameter — so this is the sentinel
        for *nothing selected*: the argument that cancels MIDI learn, and what
        `MidiLearner` reports when it has nothing to suggest. */
    inline constexpr int noParameter = -1;

    /** One controller mapped to one parameter. */
    struct Mapping
    {
        int parameterIndex = 0;
        int channel = firstChannel;

        Source source = Source::control;

        /** Controller numbers, and meaningless unless `source` is `control`.

            The MSB is optional because `add` makes a row before there is a
            number to put in it; the LSB because most controllers do not send
            one, and the two threshold modes ignore it even where they do.

            **The pairing is this row's to state, not the specification's.**
            MIDI pairs CC *n* with CC *n*+32 and allows nothing else. Here any
            available number can be the LSB for any MSB, so Table III's
            assignments are a suggestion and this row is where the truth is —
            which also makes the table the one place you can see which
            controllers have a fine byte and which message carries it.

            What is kept from the specification is the *behaviour*: an LSB
            refines the last MSB, and a new MSB resets the LSB to zero. See
            `midiMapper::Register`. */
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
    /** Why a controller number may not carry a mapping.

        Two tiers. There was a third, `reserved`, for numbers the sidebar would
        use itself but would give up when mapped — CC 7 and 39 for volume, CC 88
        for the velocity prefix. It is gone, and so are the table rows that made
        it visible: **those controllers are not special.** Master volume is set
        by its own SysEx (see MidiDeviceControl.h), and a plugin that wants CC 7
        or CC 88 to reach a parameter maps it like any other number. Checked
        against a real device: the minilogue xd sends CC 39, 88 and 96 as
        ordinary knobs, and every one of them is now free.
    */
    enum class CcStatus
    {
        free,        ///< Nothing claims it.

        /** Meaningless on its own: data entry and data increment/decrement act
            on whatever (N)RPN was last selected, so they carry a mapping *and*
            are consumed as data entry while a recognised parameter is live.
            "The basic procedure for altering a parameter value is to first send
            the Registered or Non-Registered Parameter Number … followed by the
            Data Entry, Data Increment, or Data Decrement value" — Complete MIDI
            1.0 Detailed Specification 4.2.1, p17. */
        dataEntry,

        /** Cannot carry a mapping under any setting, and the only tier that
            turns a cell red. Three groups: 0 and 32 are bank select, which the
            plugin performs itself and which is 14-bit by definition, so neither
            half is available to a mapping; 98-101 select an (N)RPN; and 120-127
            are Channel Mode Messages rather than control changes at all
            (Table III). */
        unavailable
    };

    inline constexpr CcStatus statusOfCc (int cc) noexcept
    {
        if (cc < 0 || cc > metrics::highestCc)
            return CcStatus::unavailable;

        // Hard first, so nothing below can accidentally free one.
        if (cc == 0 || cc == 32 || (cc >= 98 && cc <= 101) || cc >= 120)
            return CcStatus::unavailable;

        if (cc == 6 || cc == 38 || cc == 96 || cc == 97)
            return CcStatus::dataEntry;

        return CcStatus::free;
    }

    /** True when this number cannot be used, whatever else is mapped. The GUI
        colours exactly these cells; `dataEntry` is legal. */
    inline constexpr bool isCcUnavailable (int cc) noexcept
    {
        return statusOfCc (cc) == CcStatus::unavailable;
    }

    //==========================================================================
    /** How many recent messages the monitor shows, one to a line.

        Text rather than a table of columns it never scrolls, which is what it
        was: at 260px the columns cost more than they explain. Two lines rather
        than one because a single line is a glimpse rather than a monitor — you
        cannot tell a repeating controller from a stuck one.

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

    //==========================================================================
    /** How long MIDI learn waits for the first message before giving up.

        Long, because the end-user has to get from the menu to the hardware.
        Mixxx uses the same seven seconds for the same reason. */
    inline constexpr int learnTimeoutMs = 7000;

    /** How long after the *last* message learning decides.

        Learning observes a gesture rather than taking the first message it
        sees, so it needs to know when the gesture is over — see MidiLearner.h
        for why observation beats first-past-the-post. A second and a half is
        long enough to survive the pause in the middle of a slow sweep and short
        enough not to feel stuck. */
    inline constexpr int learnSettleMs = 1500;
}

} // namespace microtonos::sidebar
