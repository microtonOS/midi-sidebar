// Learning a control from a gesture.
//
// The case that matters is the last one: an instrument that sends its low byte
// *before* its high byte must still be learned as the high byte. Taking the
// first message that arrives — the obvious implementation — gets that wrong
// every time. See MidiLearner.h and
// .claude/skills/midi-1_0/references/real-devices.md.
#include <midi_sidebar/midi_sidebar.h>

#include "Check.h"

using namespace microtonos::sidebar;
using namespace checks;

namespace
{
    juce::MidiMessage cc (int channel, int number, int value)
    {
        return juce::MidiMessage::controllerEvent (channel, number, value);
    }

    /** A plain 7-bit sweep on one controller. */
    void sweep7 (MidiLearner& l, int channel, int number, int steps)
    {
        for (int i = 0; i < steps; ++i)
            l.observe (cc (channel, number, juce::jmin (127, i * 127 / juce::jmax (1, steps - 1))));
    }

    /** A 14-bit sweep. `lowByteFirst` reproduces the minilogue xd's order. */
    void sweep14 (MidiLearner& l, int channel, int high, int low, int steps, bool lowByteFirst)
    {
        for (int i = 0; i < steps; ++i)
        {
            const auto value = i * 16383 / juce::jmax (1, steps - 1);
            const auto hi = (value >> 7) & 0x7f;
            const auto lo = value & 0x7f;

            if (lowByteFirst) { l.observe (cc (channel, low, lo)); l.observe (cc (channel, high, hi)); }
            else              { l.observe (cc (channel, high, hi)); l.observe (cc (channel, low, lo)); }
        }
    }
}

int main()
{
    //  What can name a control at all ------------------------------------------
    {
        check (! MidiLearner::isLearnable (juce::MidiMessage::noteOn (1, 60, (juce::uint8) 100)),
               "a note is not learnable");
        check (! MidiLearner::isLearnable (juce::MidiMessage::pitchWheel (1, 4000)),
               "pitch bend is not learnable");
        check (! MidiLearner::isLearnable (cc (1, 0, 64)),
               "CC 0 is not learnable - bank select is the plugin's");
        check (! MidiLearner::isLearnable (cc (1, 120, 0)),
               "CC 120 is not learnable - a channel mode message is not a control change");
        check (MidiLearner::isLearnable (cc (1, 7, 64)),   "CC 7 is learnable");
        check (MidiLearner::isLearnable (cc (1, 88, 64)),  "CC 88 is learnable");
        check (MidiLearner::isLearnable (cc (1, 96, 64)),  "CC 96 is learnable");
    }

    //  Nothing at all ----------------------------------------------------------
    {
        MidiLearner l;
        l.begin (4);

        check (l.isActive(), "still active with nothing seen");
        check (! l.suggestion().has_value(), "no messages suggests nothing");
    }

    //  A plain 7-bit sweep -----------------------------------------------------
    {
        MidiLearner l;
        l.begin (4);
        sweep7 (l, 2, 74, 40);

        const auto s = l.suggestion();

        check (s.has_value() && s->msb == 74 && s->channel == 2
                   && s->source == controllers::Source::control
                   && s->parameterIndex == 4,
               "a 7-bit sweep learns CC 74 on channel 2, for parameter 4");
        check (s.has_value() && ! s->lsb.has_value(),
               "and learns no LSB - that is the end-user's to state");
    }

    //  A compliant 14-bit sweep: the low byte must be discarded ----------------
    {
        MidiLearner l;
        l.begin (0);
        sweep14 (l, 1, 11, 43, 60, false);

        check (l.suggestion().has_value() && l.suggestion()->msb == 11,
               "an MSB-first pair learns 11, not 43");
    }

    //  The minilogue xd's order: the low byte arrives FIRST, every time --------
    {
        MidiLearner l;
        l.begin (0);
        sweep14 (l, 1, 43, 63, 60, true);

        check (l.suggestion().has_value() && l.suggestion()->msb == 43,
               "an LSB-first pair still learns 43, not 63");
    }

    //  A lone message has no behaviour to read ---------------------------------
    {
        MidiLearner refused, accepted;

        refused.begin (0);
        refused.observe (cc (1, 40, 127));
        check (! refused.suggestion().has_value(),
               "one lone message on CC 40 is refused - 32-63 are reserved as low bytes");

        accepted.begin (0);
        accepted.observe (cc (1, 80, 127));
        check (accepted.suggestion().has_value() && accepted.suggestion()->msb == 80,
               "but one lone message on CC 80 is learned");
    }

    //  A control that *sweeps* in 32-63 is learned like any other --------------
    {
        MidiLearner l;
        l.begin (0);
        sweep7 (l, 1, 40, 40);

        check (l.suggestion().has_value() && l.suggestion()->msb == 40,
               "a sweep on CC 40 is learned - behaviour outranks the reserved range");
    }

    //  The two touch messages --------------------------------------------------
    {
        MidiLearner pressure, poly;

        pressure.begin (7);
        for (int i = 0; i < 20; ++i)
            pressure.observe (juce::MidiMessage::channelPressureChange (9, i * 6));

        const auto a = pressure.suggestion();
        check (a.has_value() && a->source == controllers::Source::aftertouch
                   && a->channel == 9 && ! a->msb.has_value(),
               "channel pressure on channel 9, with no controller number");

        poly.begin (7);
        for (int i = 0; i < 20; ++i)
            poly.observe (juce::MidiMessage::aftertouchChange (3, 60, i * 6));

        check (poly.suggestion().has_value()
                   && poly.suggestion()->source == controllers::Source::polytouch,
               "polyphonic key pressure");
    }

    //  Somebody playing on another channel must not derail it ------------------
    {
        MidiLearner l;
        l.begin (0);
        sweep7 (l, 5, 74, 30);

        for (int i = 0; i < 50; ++i)
            l.observe (cc (10, 1, i));

        const auto s = l.suggestion();
        check (s.has_value() && s->channel == 5 && s->msb == 74,
               "a second channel is ignored - a gesture is one control");
    }

    //  A dominant controller wins outright, without the pair rule firing -------
    {
        MidiLearner l;
        l.begin (0);
        sweep7 (l, 1, 74, 60);
        l.observe (cc (1, 1, 5));
        l.observe (cc (1, 1, 6));

        check (l.suggestion().has_value() && l.suggestion()->msb == 74,
               "a much-moved controller beats a barely-moved one");
    }

    //  Cancelling ---------------------------------------------------------------
    {
        MidiLearner l;
        l.begin (0);
        sweep7 (l, 1, 74, 20);
        l.cancel();

        check (! l.isActive() && ! l.suggestion().has_value(), "cancel forgets everything");
    }

    return report ("MidiLearnerCheck");
}
