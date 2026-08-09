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
    /** Any channel at all. Not a valid MIDI channel number, so it cannot be
        confused with one. */
    inline constexpr int omniChannel = 0;

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
    };

    /** One controller mapped to one parameter. */
    struct Mapping
    {
        int parameterIndex = 0;
        int channel = omniChannel;

        /** Continuous controller numbers. The LSB is optional because most
            mappings do not have one, and the modes past the separator ignore it
            even when they do. */
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

    /** How many of them are kept. The spec asks for the last three; a fourth
        pushes the oldest out rather than making the table taller. */
    inline constexpr int monitorRows = 3;
}

} // namespace microtonos::sidebar
