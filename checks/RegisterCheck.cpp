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
        setup = channels::withZoneMembers (setup, channels::Zone::lower, 4);
        setup = channels::withZoneMembers (setup, channels::Zone::upper, 0);
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

        //  Both zones at once ---------------------------------------------------
        //  The point of the change: an MCM for one zone configures that zone and
        //  leaves the other alone, rather than moving a single zone about.
        {
            const auto both = channels::withMpeConfiguration (
                                  channels::withMpeConfiguration (setup, channels::Zone::lower, 4),
                                  channels::Zone::upper, 3);

            eq (both.members (channels::Zone::lower), 4, "the lower zone keeps its 4 members");
            eq (both.members (channels::Zone::upper), 3, "while the upper zone gains 3");
            check (both.isActive (channels::Zone::lower) && both.isActive (channels::Zone::upper),
                   "and both zones are active at the same time");

            const auto layout = midiFilter::layoutFor (both);

            check (layout.getLowerZone().isActive() && layout.getUpperZone().isActive(),
                   "which is what juce::MPEZoneLayout is given");

            // Channels neither zone claimed are "available for conventional
            // use" (MPE v1.1 §2.2.1), and must not be swept into either.
            check (! midiFilter::isMemberChannel (both, 8),
                   "a channel between the two zones belongs to neither");

            const auto justLower = channels::withMpeConfiguration (both, channels::Zone::upper, 0);

            check (! justLower.isActive (channels::Zone::upper), "mm = 0 deactivates that zone");
            check (justLower.isActive (channels::Zone::lower), "and leaves the other one alone");
            check (justLower.mpeOn, "so MPE is still in force while either zone has members");

            const auto neither = channels::withMpeConfiguration (justLower, channels::Zone::lower, 0);
            check (! neither.mpeOn, "MPE goes off only when neither zone has members");
        }

        //  Overlap, from the specification's own worked examples ----------------
        //  MPE v1.1 §2.2.1 gives precedence to the most recent MCM. These are the
        //  cases the specification spells out, which is why they are the ones
        //  checked rather than cases of our own devising.
        {
            auto seven = channels::withMpeConfiguration (channels::Setup {}, channels::Zone::lower, 7);
            eq (seven.members (channels::Zone::lower), 7, "a lower zone of 7 holds channels 2-8");

            // "Lower Zone with 7 Channels (2-8), then Upper Zone with 14 Channels
            // (2-15). The Lower Zone is left with no Member Channels and is
            // therefore deactivated."
            const auto stolen = channels::withMpeConfiguration (seven, channels::Zone::upper, 14);

            eq (stolen.members (channels::Zone::upper), 14, "an upper zone of 14 reaches down to channel 2");
            eq (stolen.members (channels::Zone::lower), 0, "which leaves the lower zone nothing");
            check (! stolen.isActive (channels::Zone::lower),
                   "and a zone with no member channels shall become deactivated");

            // A partial steal: an upper zone of 11 holds 5-15, so the lower zone
            // keeps only 2-4.
            const auto partial = channels::withMpeConfiguration (seven, channels::Zone::upper, 11);

            eq (partial.members (channels::Zone::lower), 3,
                "an upper zone of 11 takes channels 5-8 back, leaving the lower zone 3");
            check (partial.isActive (channels::Zone::lower), "which is still a zone");

            check ((channels::memberChannelsForZone (channels::Zone::lower, partial.lowerMembers)
                      & channels::memberChannelsForZone (channels::Zone::upper, partial.upperMembers)) == 0,
                   "and after any overlap the two zones share no channel");

            // §2.2.1's example of an upper zone reaching across channel 1, which
            // is the lower zone's manager channel: the lower zone cannot survive
            // losing that, whatever its member count was.
            const auto acrossManager = channels::withMpeConfiguration (seven, channels::Zone::upper,
                                                                       channels::maxMemberChannels);

            eq (acrossManager.members (channels::Zone::lower), 0,
                "an upper zone of 15 claims channel 1, so the lower zone is gone");
        }

        //  RPN 0 over the wire, in cents ---------------------------------------
        //  The check that matters: a sender asking for two semitones and fifty
        //  cents gets 250, not 200. MSB alone would silently drop the fifty.
        {
            MidiRouter bendRouter;
            MidiRouter::Result bendResult;

            auto listening = channels::Setup {};
            listening.omniOn = true;
            bendRouter.setChannels (listening);

            const auto sendCc = [&] (int channel, int number, int value)
            {
                juce::MidiBuffer b;
                b.addEvent (juce::MidiMessage::controllerEvent (channel, number, value), 0);
                bendRouter.process (b, bendResult);
            };

            sendCc (3, 101, 0);    // RPN MSB = 0
            sendCc (3, 100, 0);    // RPN LSB = 0, so parameter 0: bend sensitivity
            sendCc (3, 6, 2);      // data entry MSB: 2 semitones

            check (bendResult.bendSensitivity.has_value()
                       && bendResult.bendSensitivity->cents == 200,
                   "RPN 0 with only an MSB is whole semitones");

            sendCc (3, 38, 50);    // data entry LSB: 50 cents

            check (bendResult.bendSensitivity.has_value()
                       && bendResult.bendSensitivity->cents == 250,
                   "and the LSB adds cents, which is what makes RPN 0 microtonal");
            eq (bendResult.bendSensitivity->channel, 3, "on the channel it was sent on");

            // NRPN 0 is somebody else's parameter entirely.
            sendCc (4, 99, 0);
            sendCc (4, 98, 0);
            sendCc (4, 6, 12);

            check (! bendResult.bendSensitivity.has_value(),
                   "NRPN 0 is not pitch-bend sensitivity");
        }

        //  What a pitch-bend click actually sets ------------------------------
        //  The case worth checking is the one that has no answer: under MPE a
        //  channel in neither zone must set nothing at all, rather than falling
        //  through to its plain range, which belongs to the omni view.
        {
            // Lower zone of 4: manager 1, members 2-5. Upper zone of 2: manager
            // 16, members 14-15. So 6-13 belong to neither.
            auto zoned = channels::withMpeConfiguration (channels::Setup {}, channels::Zone::lower, 4);
            zoned = channels::withMpeConfiguration (zoned, channels::Zone::upper, 2);

            const auto manager = channels::bendTargetFor (zoned, 0, true);
            check (manager.description == "lower zone manager",
                   "clicking channel 1 under MPE sets the lower zone manager");
            eq ((int) manager.channelsAffected, 1 << 0, "and only that channel");

            const auto member = channels::bendTargetFor (zoned, 2, true);
            check (member.description == "lower zone members",
                   "clicking a member sets the whole zone's members");
            eq ((int) member.channelsAffected,
                (int) channels::memberChannelsForZone (channels::Zone::lower, 4),
                "which is every member of that zone and no manager");

            const auto upper = channels::bendTargetFor (zoned, 15, true);
            check (upper.description == "upper zone manager",
                   "and the upper zone has its own manager");

            for (const auto between : { 6, 7, 8, 12 })
                check (channels::bendTargetFor (zoned, between - 1, true).isEmpty(),
                       "channel " + juce::String (between) + " is in neither zone, so it sets nothing");

            // The same channel does have a range under omni, where it is just a
            // channel — which is the reason the MPE view refuses it rather than
            // pretending it has none.
            const auto asOmni = channels::bendTargetFor (zoned, 6, false);
            check (! asOmni.isEmpty() && asOmni.description == "channel 7",
                   "the same channel under omni is its own plain range");

            // A zone that is not active offers nothing, manager included.
            const auto noUpper = channels::withMpeConfiguration (zoned, channels::Zone::upper, 0);
            check (channels::bendTargetFor (noUpper, 15, true).isEmpty(),
                   "a deactivated zone has no manager range either");
        }

        //  The two pitch-bend defaults §2.2.5 requires -------------------------
        {
            const auto configured = channels::withMpeConfiguration (channels::Setup {},
                                                                    channels::Zone::lower, 4);

            eq (configured.pitchBendCents[0], channels::mpeManagerBendCents,
                "an MCM sets the manager channel to 2 semitones");
            eq (configured.pitchBendCents[1], channels::mpeMemberBendCents,
                "and every member channel to 48");
            eq (configured.pitchBendCents[5], channels::defaultBendCents,
                "leaving channels outside the zone as they were");
        }
    }

    return report ("RegisterCheck");
}
