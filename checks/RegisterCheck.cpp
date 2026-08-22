// The MSB/LSB register, end to end through MidiRouter.
//
// The rule under test (docs/controllers.md, and MIDI 1.0 Detailed Specification
// 4.2.1 p12): an LSB refines the last MSB, and a new MSB resets the LSB to
// zero. Because the two write to different places, nothing else about order
// matters — which is what several of these assert directly.
#include <midi_sidebar/midi_sidebar.h>

#include "Check.h"

using namespace microtonos::sidebar;
using namespace checks;

namespace
{

    controllers::Mapping pair (int msb, int lsb, int channel = 1)
    { controllers::Mapping m; m.channel = channel; m.msb = msb; m.lsb = lsb; return m; }

    controllers::Mapping lone (int msb, int channel = 1)
    { controllers::Mapping m; m.channel = channel; m.msb = msb; return m; }

    /** Runs messages through the router and returns the last match's value. */
    struct Rig
    {
        MidiRouter router;
        MidiRouter::Result result;

        explicit Rig (juce::Array<controllers::Mapping> maps) { router.setMappings (std::move (maps)); }

        std::optional<MidiRouter::Result::Match> send (std::initializer_list<juce::MidiMessage> ms)
        {
            std::optional<MidiRouter::Result::Match> last;
            for (const auto& m : ms)
            {
                juce::MidiBuffer b; b.addEvent (m, 0);
                router.process (b, result);
                if (! result.matches.isEmpty()) last = result.matches.getLast();
            }
            return last;
        }
    };

    juce::MidiMessage cc (int number, int value, int channel = 1)
    { return juce::MidiMessage::controllerEvent (channel, number, value); }
}

int main()
{
    // --- MSB then LSB, the spec's order -----------------------------------
    {
        Rig rig { { pair (11, 43) } };
        const auto m = rig.send ({ cc (11, 100), cc (43, 64) });

        check (m.has_value(), "an MSB/LSB pair matches");
        eq (m->highest, 16383, "a row with an LSB is 14-bit");
        eq (m->value, (100 << 7) | 64, "the two bytes combine");
    }

    // --- the reset: a new MSB clears the LSB ------------------------------
    {
        Rig rig { { pair (11, 43) } };
        rig.send ({ cc (11, 100), cc (43, 127) });
        const auto m = rig.send ({ cc (11, 101) });

        eq (m->value, 101 << 7, "a new MSB resets the LSB to zero");
    }

    // --- an LSB refines the last MSB without it being resent ---------------
    {
        Rig rig { { pair (11, 43) } };
        rig.send ({ cc (11, 100) });
        const auto a = rig.send ({ cc (43, 1) });
        const auto b = rig.send ({ cc (43, 2) });

        eq (a->value, (100 << 7) | 1, "the first fine step");
        eq (b->value, (100 << 7) | 2, "and the next, with no MSB in between");
    }

    // --- an LSB before any MSB does nothing --------------------------------
    {
        Rig rig { { pair (11, 43) } };
        const auto m = rig.send ({ cc (43, 64) });

        check (! m.has_value(), "an LSB with no MSB yet drives nothing");
    }

    // --- LSB-first, the minilogue's order, is the case that cannot work -----
    {
        Rig rig { { pair (43, 63) } };
        const auto m = rig.send ({ cc (63, 7), cc (43, 100) });

        eq (m->value, 100 << 7, "an LSB sent *before* its MSB is wiped by the reset");
    }

    // --- a row with no LSB stays 7-bit and still reaches the top ------------
    {
        Rig rig { { lone (74) } };
        const auto m = rig.send ({ cc (74, 127) });

        eq (m->highest, 127, "a row with no LSB is 7-bit");
        eq (m->value, 127, "and reaches its maximum");

        const auto target = midiMapper::valueFor (lone (74), m->value, m->highest, 0.0);
        check (target.has_value() && std::abs (*target - 100.0) < 1e-9,
               "which maps to the top of a 0..100 range");
    }

    // --- any number may be an LSB, not only n+32 ---------------------------
    {
        Rig rig { { pair (11, 90) } };   // 90 is not 11+32, and is a legal MSB elsewhere
        const auto m = rig.send ({ cc (11, 64), cc (90, 3) });

        eq (m->value, (64 << 7) | 3, "an arbitrary number works as an LSB");
    }

    // --- the threshold modes read the coarse byte only ---------------------
    {
        auto toggle = pair (11, 43);
        toggle.mode = controllers::Mode::toggle;
        toggle.min = 0.0; toggle.max = 1.0;

        // MSB 63 with a full LSB is still below the switch threshold.
        const auto low = midiMapper::valueFor (toggle, (63 << 7) | 127, 16383, 0.0);
        check (! low.has_value(), "MSB 63 does not trip the switch, whatever the LSB");

        const auto high = midiMapper::valueFor (toggle, 64 << 7, 16383, 0.0);
        check (high.has_value(), "MSB 64 does");
    }

    // --- two rows may share an MSB ----------------------------------------
    {
        Rig rig { { lone (74), lone (74) } };
        juce::MidiBuffer b; b.addEvent (cc (74, 10), 0);
        rig.router.process (b, rig.result);

        eq (rig.result.matches.size(), 2, "one controller can drive two rows");
    }

    // --- channels are honoured --------------------------------------------
    {
        Rig rig { { pair (11, 43, 5) } };
        check (! rig.send ({ cc (11, 100, 6) }).has_value(), "another channel does not match");
        check (rig.send ({ cc (11, 100, 5) }).has_value(), "its own channel does");
    }

    // --- the MPE Configuration Message reaches the channel setup ------------
    //
    // RPN 6 on channel 1 or 16. Read *before* the channel filter, because a
    // plugin configured for one zone is generally not listening to the other's
    // manager channel and could otherwise never be reconfigured onto it.
    {
        const auto mcm = [] (int channel, int members)
        {
            return std::vector<juce::MidiMessage> {
                cc (101, 0, channel), cc (100, 6, channel), cc (6, members, channel) };
        };

        const auto send = [&mcm] (MidiRouter& router, MidiRouter::Result& result,
                                  int channel, int members)
        {
            std::optional<MidiRouter::Result::MpeConfiguration> seen;

            for (const auto& m : mcm (channel, members))
            {
                juce::MidiBuffer b; b.addEvent (m, 0);
                router.process (b, result);
                if (result.mpeConfiguration.has_value()) seen = result.mpeConfiguration;
            }

            return seen;
        };

        MidiRouter router;
        MidiRouter::Result result;

        // Listening to a lower zone of 4, so channel 16 is filtered out for
        // everything else — and the MCM must still arrive.
        auto setup = channels::Setup {};
        setup.mpeOn = true;
        setup.zone = channels::Zone::lower;
        setup.zoneEdge = 5;
        router.setChannels (setup);

        const auto lower = send (router, result, 1, 4);
        check (lower.has_value() && lower->zone == channels::Zone::lower
                   && lower->memberChannels == 4,
               "an MCM on channel 1 names the lower zone and its member count");

        const auto upper = send (router, result, 16, 3);
        check (upper.has_value() && upper->zone == channels::Zone::upper
                   && upper->memberChannels == 3,
               "an MCM on channel 16 arrives even though the filter excludes it");

        check (! send (router, result, 5, 4).has_value(),
               "RPN 6 on any other channel is not an MCM");

        // The count-to-edge conversion, which is the page's arithmetic.
        const auto asLower = channels::withMpeConfiguration (setup, channels::Zone::lower, 4);
        eq (asLower.zoneEdge, 5, "4 members on the lower zone reach channel 5");
        check (asLower.mpeOn, "and MPE is on");

        const auto asUpper = channels::withMpeConfiguration (setup, channels::Zone::upper, 3);
        eq (asUpper.zoneEdge, 13, "3 members on the upper zone reach down to channel 13");

        const auto off = channels::withMpeConfiguration (asUpper, channels::Zone::upper, 0);
        check (! off.mpeOn, "mm = 0 deactivates the zone");
        eq (off.zoneEdge, 13, "and keeps the edge, so turning it back on restores it");

        // Exactly one zone is ever active, whichever way the setup was reached.
        {
            auto asLowerSetup = channels::withMpeConfiguration (setup, channels::Zone::lower, 4);
            const auto lowerLayout = midiFilter::layoutFor (asLowerSetup);

            check (lowerLayout.getLowerZone().isActive(), "a lower setup activates the lower zone");
            check (! lowerLayout.getUpperZone().isActive(), "and leaves the upper zone off");

            auto asUpperSetup = channels::withMpeConfiguration (setup, channels::Zone::upper, 3);
            const auto upperLayout = midiFilter::layoutFor (asUpperSetup);

            check (upperLayout.getUpperZone().isActive(), "an upper setup activates the upper zone");
            check (! upperLayout.getLowerZone().isActive(), "and leaves the lower zone off");

            // Which is what keeps channels the end-user never gave to MPE out of
            // it: with an upper zone of 3, channels 1 to 12 belong to nobody.
            check (! midiFilter::isMemberChannel (asUpperSetup, 2),
                   "channel 2 is not an MPE member under an upper zone");
            check (midiFilter::isMemberChannel (asUpperSetup, 13),
                   "channel 13 is");

            auto off = channels::withMpeConfiguration (asUpperSetup, channels::Zone::upper, 0);
            const auto offLayout = midiFilter::layoutFor (off);

            check (! offLayout.getLowerZone().isActive() && ! offLayout.getUpperZone().isActive(),
                   "mm = 0 leaves both zones off");
        }

        // The full round trip: what the wire says, through to what the page holds.
        const auto fromWire = channels::withMpeConfiguration (setup, upper->zone,
                                                              upper->memberChannels);
        check (fromWire.zone == channels::Zone::upper && fromWire.zoneEdge == 13,
               "an MCM for the upper zone moves the zone rather than adding one");
    }

    return report ("RegisterCheck");
}
