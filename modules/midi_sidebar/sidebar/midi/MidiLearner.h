#pragma once

#include <array>
#include <optional>

#include <juce_audio_basics/juce_audio_basics.h>

#include "../pages/ControllersState.h"

namespace microtonos::sidebar
{

//==============================================================================
/** MIDI learn: watch a gesture, then say what was moved.

    **Not first-past-the-post.** Taking the first message that arrives is the
    obvious implementation and it fails on real hardware. The Korg minilogue xd
    sends the *low* byte of a 14-bit control first — "when a 10 bit value is
    sent, the lower 3 bits are first sent via a CC #63 (0x3f) message"
    (minilogue xd MIDI Implementation, note `*1-4`), the reverse of Table II
    note 4 of the **MIDI 1.0 Detailed Specification** — so the first control
    change of every gesture on that instrument is CC 63, and every parameter
    learned would be driven by three bits of somebody else's knob.

    This watches instead, and identifies the low byte by how it *behaves*:
    during a sweep a low byte wraps 127 to 0 over and over, so the largest step
    between two consecutive messages is enormous, while the high byte crawls
    upward one at a time. Mixxx reaches the same conclusion, and says why it
    does not simply assume the specification's CC *n* / CC *n*+32 pairing:

    > There is an industry convention that a 14-bit CC control is a pair of
    > controls offset by 32 … I don't use that convention here because it's not
    > universal and we should be able to come up with reasonable heuristics to
    > identify an LSB and an MSB.
    > — `mixxxdj/mixxx`, `src/controllers/learningutils.cpp`

    A behavioural test also lets the blanket refusal go. There used to be a rule
    here rejecting any controller numbered 32 to 63, on the grounds that
    Table III reserves them as low bytes. It was right about the minilogue by
    accident and wrong about any device that uses CC 40 as an ordinary knob; it
    survives only as the tie-break below, where behaviour has said nothing.

    Message thread only, and free of the GUI and of `juce::AudioProcessor`: the
    audio side does no more than copy learnable messages out of the block. Fixed
    size, so nothing here allocates either.
*/
class MidiLearner
{
public:
    //==========================================================================
    /** Whether this message could name a control at all.

        Notes are deliberately excluded: a keyboard sends one whenever it is
        played, so learning would catch the first key pressed rather than the
        control the end-user reached for. Pitch bend is excluded for the same
        reason and because the tuning page owns it. A controller the plugin
        cannot use is excluded because a row that was red the moment it appeared
        would be worse than waiting. See docs/right-click.md. */
    static bool isLearnable (const juce::MidiMessage& m)
    {
        if (m.isController())
            return ! controllers::isCcUnavailable (m.getControllerNumber());

        return m.isChannelPressure() || m.isAftertouch();
    }

    //==========================================================================
    /** Starts watching, for this parameter. Forgets whatever came before. */
    void begin (int parameterIndex)
    {
        *this = {};
        learning = parameterIndex;
    }

    void cancel() { *this = {}; }

    bool isActive() const noexcept { return learning != controllers::noParameter; }

    /** How many learnable messages have arrived, for the monitor to show. */
    int messagesSeen() const noexcept { return total; }

    //==========================================================================
    /** Adds one message to what is known. Ignores anything on a channel other
        than the first one seen: a learn gesture is one control, so a second
        channel is somebody else playing. */
    void observe (const juce::MidiMessage& m)
    {
        if (! isActive() || ! isLearnable (m))
            return;

        if (channel == 0)
            channel = m.getChannel();
        else if (m.getChannel() != channel)
            return;

        ++total;

        if (m.isController())
            record (controls[(size_t) m.getControllerNumber()], m.getControllerValue());
        else if (m.isChannelPressure())
            record (pressure, m.getChannelPressureValue());
        else
            record (polyPressure, m.getAfterTouchValue());
    }

    //==========================================================================
    /** The mapping the gesture describes, or nothing if it does not describe
        one yet.

        Safe to call at any point, not only at the end: during a sweep it names
        the current best guess, which is what the monitor shows while the
        end-user is still moving something. */
    std::optional<controllers::Mapping> suggestion() const
    {
        if (total == 0)
            return {};

        controllers::Mapping mapping;

        mapping.parameterIndex = learning;
        mapping.channel        = channel;

        // The two touch messages first. They do not mix with control changes in
        // practice, so whichever has been seen most often wins outright.
        const auto best = bestControl();
        const auto touch = juce::jmax (pressure.count, polyPressure.count);

        if (touch > 0 && (best < 0 || touch > controls[(size_t) best].count))
        {
            mapping.source = pressure.count >= polyPressure.count
                                 ? controllers::Source::aftertouch
                                 : controllers::Source::polytouch;
            return mapping;
        }

        if (best < 0)
            return {};

        mapping.source = controllers::Source::control;
        mapping.cc     = chooseControl (best);

        if (! mapping.cc.has_value())
            return {};

        return mapping;
    }

private:
    //==========================================================================
    /** One controller number's history. `maxDelta` is the whole heuristic: a
        low byte wraps and so jumps, a high byte steps. */
    struct Stats
    {
        int count     = 0;
        int lastValue = -1;
        int maxDelta  = 0;
    };

    static void record (Stats& s, int value)
    {
        if (s.lastValue >= 0)
            s.maxDelta = juce::jmax (s.maxDelta, std::abs (value - s.lastValue));

        s.lastValue = value;
        ++s.count;
    }

    /** The most-used controller number, or -1 if none was seen. */
    int bestControl() const
    {
        auto found = -1;

        for (int i = 0; i < (int) controls.size(); ++i)
            if (controls[(size_t) i].count > 0
                && (found < 0 || controls[(size_t) i].count > controls[(size_t) found].count))
                found = i;

        return found;
    }

    /** The second most-used controller number, or -1. */
    int runnerUp (int winner) const
    {
        auto found = -1;

        for (int i = 0; i < (int) controls.size(); ++i)
            if (i != winner && controls[(size_t) i].count > 0
                && (found < 0 || controls[(size_t) i].count > controls[(size_t) found].count))
                found = i;

        return found;
    }

    /** Which of the numbers seen is the control itself rather than its low
        byte. */
    std::optional<int> chooseControl (int winner) const
    {
        const auto second = runnerUp (winner);

        // Only one number moved, so there is no pair to disentangle — unless it
        // is the *single message* case, where nothing has behaved at all and
        // the reserved range is the only evidence there is. A lone control
        // change on 32 to 63 is much more likely to be somebody's low byte than
        // a knob of its own, so it is refused and learning keeps waiting.
        if (second < 0)
        {
            if (controls[(size_t) winner].count == 1 && winner >= 32 && winner <= 63)
                return {};

            return winner;
        }

        const auto& a = controls[(size_t) winner];
        const auto& b = controls[(size_t) second];

        // Comparable counts mean the two arrived together, which is what a
        // 14-bit control looks like. Mixxx requires them to be exactly equal;
        // that is too strict for a window which may open or close part-way
        // through a gesture, so a quarter's slack is allowed.
        if (b.count * 4 < a.count * 3)
            return winner;

        // The wraparound test. Whichever jumps further is the low byte.
        if (a.maxDelta != b.maxDelta)
            return a.maxDelta > b.maxDelta ? second : winner;

        // Neither behaved differently from the other — too few messages, most
        // likely. Only now is the specification's own convention worth
        // guessing from: Table III puts high bytes at 0 to 31 and their low
        // bytes at 32 to 63, so the lower number is the better bet.
        return juce::jmin (winner, second);
    }

    //==========================================================================
    int learning = controllers::noParameter;

    /** Locked to the first message seen; 0 until then. */
    int channel = 0;
    int total   = 0;

    std::array<Stats, (size_t) metrics::highestCc + 1> controls {};
    Stats pressure, polyPressure;

    JUCE_LEAK_DETECTOR (MidiLearner)
};

} // namespace microtonos::sidebar
