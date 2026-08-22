#pragma once

#include <optional>

#include <juce_audio_basics/juce_audio_basics.h>

#include "../pages/ControllersState.h"

namespace microtonos::sidebar
{

//==============================================================================
/** Turning an incoming message into a parameter movement.

    Two questions, kept apart because they fail differently: *does this message
    belong to this mapping* (`matches`), and *where does the parameter go*
    (`valueFor`). The first is a lookup; the second is the five modes of
    docs/controllers.md, four of which need to know where the parameter already
    is.

    Free of the GUI and of `juce::AudioProcessor`: a mapping's target is an index
    and its value a `double` in the parameter's own unit, so the owner does the
    denormalising. That keeps every rule here checkable without a plugin.
*/
namespace midiMapper
{
    /** The value a switch-like controller counts as "on".

        "If a receiver is expecting switch information it should recognize 0-63
        (00H-3FH) as 'OFF' and 64-127 (40H-7FH) as 'ON'" — MIDI 1.0 Detailed
        Specification 4.2.1, p12. So `toggle` and `increment` trigger at 64
        because that is the protocol's threshold, not an arbitrary midpoint. */
    inline constexpr int switchThreshold = 64;

    /** The largest value a 7-bit controller sends. */
    inline constexpr int highestValue = 127;

    /** How close `catch` has to be before it takes over, as a fraction of the
        controller's travel. One step of a 7-bit controller: any tighter and a
        knob swept quickly would step straight over the value and never catch. */
    inline constexpr double catchTolerance = 1.0 / (double) highestValue;

    /** How much of the remaining distance `scale` covers per message. A quarter
        converges in a handful of messages — fast enough to feel connected,
        slow enough that the first message is not a jump, which is the whole
        difference between this mode and `jump`. */
    inline constexpr double scaleRate = 0.25;

    //==========================================================================
    /** Whether this message drives this mapping.

        The channel has to agree, and the *kind* has to agree: an aftertouch
        mapping is not moved by a control change however the numbers line up. A
        `control` mapping with no number set matches nothing, which is what an
        incomplete row should do — `add` leaves one in exactly that state.
    */
    inline bool matches (const controllers::Mapping& mapping, const juce::MidiMessage& m)
    {
        if (m.getChannel() != mapping.channel)
            return false;

        switch (mapping.source)
        {
            case controllers::Source::aftertouch:
                return m.isChannelPressure();

            case controllers::Source::polytouch:
                return m.isAftertouch();

            case controllers::Source::control:
                return m.isController()
                    && mapping.cc.has_value()
                    && m.getControllerNumber() == *mapping.cc;
        }

        return false;
    }

    /** The controller value a message carries, 0..127, whatever kind it is. */
    inline int valueOf (const juce::MidiMessage& m)
    {
        if (m.isController())      return m.getControllerValue();
        if (m.isChannelPressure()) return m.getChannelPressureValue();
        if (m.isAftertouch())      return m.getAfterTouchValue();

        return 0;
    }

    //==========================================================================
    /** Where the parameter should go, or nothing if this message leaves it
        alone — which `catch` does until the controller crosses the current
        value, and which the two switch modes do below the threshold.

        @param current  where the parameter is now, in its own unit. Needed by
                        every mode except `jump`, and harmless there.

        The limits are deliberately *not* ordered: "if 'max' is less than 'min',
        then the polarity is changed" (docs/controllers.md), so a reversed pair
        is a feature and nothing here sorts them.
    */
    inline std::optional<double> valueFor (const controllers::Mapping& mapping,
                                           const juce::MidiMessage& m,
                                           double current)
    {
        const auto raw  = valueOf (m);
        const auto span = mapping.max - mapping.min;

        // Where the controller is, as a fraction of its travel.
        const auto position = (double) raw / (double) highestValue;
        const auto target   = mapping.min + span * position;

        switch (mapping.mode)
        {
            case controllers::Mode::jump:
                return target;

            case controllers::Mode::catchUp:
            {
                // "Turning the knob will not change the parameter value until
                // the knob position matches the stored value." Which side the
                // controller is on decides whether it has arrived, and a span
                // of zero means it always has.
                if (juce::exactlyEqual (span, 0.0))
                    return target;

                const auto currentPosition = (current - mapping.min) / span;

                return std::abs (position - currentPosition) <= catchTolerance
                           ? std::optional<double> (target)
                           : std::nullopt;
            }

            case controllers::Mode::scale:
            {
                // "The parameter value will increase or decrease in a relative
                // manner in the direction that it is turned … once the knob
                // position matches the parameter value, the knob position and
                // parameter value will subsequently be linked." Moving a
                // fraction of the remaining distance is what converges without
                // the jump that `jump` makes.
                return current + (target - current) * scaleRate;
            }

            case controllers::Mode::toggle:
            {
                // "Whenever a controller emits a value at least 64, the toggle
                // switches." Which end it lands on is decided by which it is
                // nearer, so a reversed pair reverses the polarity for free.
                if (raw < switchThreshold)
                    return {};

                const auto midpoint = (mapping.min + mapping.max) * 0.5;

                return current < midpoint ? mapping.max : mapping.min;
            }

            case controllers::Mode::increment:
            {
                // "Interpreted as going from CC value x to x+1 (at most 127)",
                // so one step is one hundred-and-twenty-seventh of the travel —
                // and a reversed pair counts downwards, which is what
                // docs/controllers.md means by decrement.
                if (raw < switchThreshold)
                    return {};

                const auto step = span / (double) highestValue;

                return juce::jlimit (juce::jmin (mapping.min, mapping.max),
                                     juce::jmax (mapping.min, mapping.max),
                                     current + step);
            }
        }

        return {};
    }
}

} // namespace microtonos::sidebar
