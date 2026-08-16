#pragma once

#include <algorithm>
#include <array>
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
    /** How far a controller aimed at this parameter reaches.

        The developer's statement about their own plugin — the module cannot
        work it out — and it is marked beside the name in the table rather than
        explained, because it is the sort of fact you want while scanning a list
        rather than while reading. See docs/controllers.md. */
    enum class Scope
    {
        split,     ///< One side of the keyboard split. Unmarked, being the usual case.
        perNote,   ///< Can modulate individual notes. Marked with the notes glyph.
        global     ///< The whole plugin, both sides of a split. Marked with the globe.
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

        Scope scope = Scope::split;
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

    //==========================================================================
    /** The controllers the sidebar answers itself, shown as rows of the table.

        These are the `CcStatus::reserved` numbers made visible. Each is a real
        row: its channel, LSB, mode and limits can be edited like any other, and
        pointing its `param` at a host parameter is what makes the sidebar give
        up the built-in function — which is the behaviour docs/controllers.md
        describes for CC 7 and 39. What cannot be edited is the **MSB**, because
        the number is what the row *is*.

        They cannot be deleted either, so the delete button reads `reset` when
        one of them is selected. */
    enum class Builtin { none, bankSelect, channelVolume, velocityPrefix };

    /** Shown in the `param` column while the row still does its built-in job.

        Short forms of the specification's names. `Bank Select` and
        `Channel Volume` are Table III's own wording (Complete MIDI 1.0 Detailed
        Specification 4.2.1); CC 88 is not in Table III at all and is named
        *High Resolution Velocity Prefix* by CA-031, which will not fit a column,
        so it is shortened here and given in full in the glossary. */
    inline juce::String builtinName (Builtin b)
    {
        switch (b)
        {
            case Builtin::bankSelect:     return "Bank Select";
            case Builtin::channelVolume:  return "Volume";
            case Builtin::velocityPrefix: return "Velocity Prefix";
            case Builtin::none:           break;
        }

        return {};
    }

    /** How far each built-in reaches, marked in the table like any parameter's.

        Volume is the plugin's **master** volume, not MIDI's per-channel one, so
        it is global — CC 7 drives it because that is the controller a keyboard
        sends, not because the two mean the same thing. Master volume has a
        Universal Real Time SysEx of its own, which should reach the same place;
        see TODO.md.

        Bank select changes which preset the whole plugin is on. The velocity
        prefix is the odd one: CA-031 makes it a prefix to the *next* Note On,
        so it reaches exactly one note. */
    inline Scope builtinScope (Builtin b)
    {
        switch (b)
        {
            case Builtin::bankSelect:     return Scope::global;
            case Builtin::channelVolume:  return Scope::global;
            case Builtin::velocityPrefix: return Scope::perNote;
            case Builtin::none:           break;
        }

        return Scope::split;
    }

    /** `parameterIndex` when a row is not pointed at a host parameter. Only a
        built-in row can be in that state; an ordinary row always targets
        something, because it was created by choosing one. */
    inline constexpr int noParameter = -1;

    /** One controller mapped to one parameter. */
    struct Mapping
    {
        /** Which built-in this row is, if any. Fixed for the life of the row —
            resetting restores the row's defaults, it does not change what the
            row is. */
        Builtin builtin = Builtin::none;

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
    /** A built-in row as it starts out, and as `reset` puts it back.

        The LSB is the specification's partner for the number — CC 0 with 32,
        CC 7 with 39 — because that is what the built-in function actually
        listens to. The velocity prefix has none: CA-031 defines CC 88 as a
        prefix to the next Note On, not as half of a 14-bit pair. */
    inline Mapping defaultsFor (Builtin b)
    {
        Mapping m;

        m.builtin        = b;
        m.parameterIndex = noParameter;

        switch (b)
        {
            case Builtin::bankSelect:
                // 14 bits across the pair: "This allows 16,384 banks to be
                // specified" (Complete MIDI 1.0 Detailed Specification 4.2.1,
                // p13). Counted from 1 here because a bank is shown to the
                // end-user as a number, and the presets page counts from 1.
                m.msb = 0;  m.lsb = 32;
                m.min = 1.0;
                m.max = 128.0 * 128.0;
                break;

            case Builtin::channelVolume:
                // The fader's own scale, so a mapped CC 7 lands on the same
                // range the sidebar already shows in dB.
                m.msb = 7;  m.lsb = 39;
                m.min = metrics::floorDb;
                m.max = 0.0;
                break;

            case Builtin::velocityPrefix:
                // CA-031 makes CC 88 a *prefix* carrying the low 7 bits of the
                // next Note On's velocity, so its own travel is one byte. It is
                // not half of a 14-bit controller pair and has no LSB.
                m.msb = 88;
                m.min = 0.0;
                m.max = (double) metrics::highestCc;
                break;

            case Builtin::none:
                break;
        }

        return m;
    }

    /** The three, in the order the table shows them. */
    inline const std::array<Builtin, 3> builtins { Builtin::bankSelect,
                                                   Builtin::channelVolume,
                                                   Builtin::velocityPrefix };

    /** Puts the three built-in rows at the front of the list, adding any that
        are missing and keeping whatever state an existing one already had.

        Called on the way in rather than left to the owner: these rows are the
        sidebar's own functions, so a consumer that forgot to supply them would
        silently lose bank select and volume.

        **The front, because the table shows newest first.** With no column
        sorted the display is the reverse of this order, so the oldest row is at
        the bottom — and these three were there before anything the end-user
        added. Appending would have put the plugin's own furniture on top of the
        work. */
    inline juce::Array<Mapping> withBuiltins (juce::Array<Mapping> list)
    {
        juce::Array<Mapping> ordered;

        for (auto b : builtins)
        {
            const auto* existing = std::find_if (list.begin(), list.end(),
                                                 [b] (const Mapping& m) { return m.builtin == b; });

            ordered.add (existing != list.end() ? *existing : defaultsFor (b));
        }

        for (const auto& m : list)
            if (m.builtin == Builtin::none)
                ordered.add (m);

        return ordered;
    }

    //==========================================================================
    /** Why a controller number may not carry a mapping.

        Three tiers, because "reserved" turned out to mean three different
        things — see docs/controllers.md and the reasoning in TODO.md. Checked
        against a real device: the minilogue xd sends CC 39, 88 and 96 as
        ordinary knobs, and a single-tier list would have refused all three.
    */
    enum class CcStatus
    {
        free,        ///< Nothing claims it.

        /** The sidebar itself would use it, but gives way to a mapping. Bank
            select, volume, the high-resolution velocity prefix. */
        reserved,

        /** Meaningless on its own: data entry and data increment/decrement act
            on whatever (N)RPN was last selected, so they carry a mapping *and*
            are consumed as data entry while a recognised parameter is live.
            "The basic procedure for altering a parameter value is to first send
            the Registered or Non-Registered Parameter Number … followed by the
            Data Entry, Data Increment, or Data Decrement value" — Complete MIDI
            1.0 Detailed Specification 4.2.1, p17. */
        dataEntry,

        /** Cannot carry a mapping under any setting. 98-101 select a parameter,
            and 120-127 are Channel Mode Messages rather than control changes at
            all (Table III). This is the only tier that turns a cell red. */
        unavailable
    };

    inline constexpr CcStatus statusOfCc (int cc) noexcept
    {
        if (cc < 0 || cc > metrics::highestCc)
            return CcStatus::unavailable;

        // Hard first, so nothing below can accidentally free one.
        if ((cc >= 98 && cc <= 101) || cc >= 120)
            return CcStatus::unavailable;

        if (cc == 6 || cc == 38 || cc == 96 || cc == 97)
            return CcStatus::dataEntry;

        if (cc == 0 || cc == 32 || cc == 7 || cc == 39 || cc == 88)
            return CcStatus::reserved;

        return CcStatus::free;
    }

    /** True when this number cannot be used, whatever else is mapped. The GUI
        colours exactly these cells; `reserved` and `dataEntry` are legal. */
    inline constexpr bool isCcUnavailable (int cc) noexcept
    {
        return statusOfCc (cc) == CcStatus::unavailable;
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
