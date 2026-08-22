#pragma once

#include <array>

#include <juce_gui_basics/juce_gui_basics.h>

namespace microtonos::sidebar
{

//==============================================================================
/** Which MIDI channels the plugin listens to, and how it treats them.

    Two settings that used to live on two different pages and contradict each
    other: omni was part of the tuning page's multichannel call-out, and the MPE
    zone was a section of the controllers page. See docs/channels.md.

    **They are independent.** An MPE zone over channels 1 to 9 leaves 10 to 16
    for omni to do as it likes with, and turning MPE off must not throw away the
    omni selection underneath — nor the other way round. So this holds both at
    once and the page shows one of them at a time.

    Free of MIDI, like the other pages' state: nothing here reads a message or
    silences a note. A mask is what the end-user asked for, and acting on it is
    somebody else's problem.
*/
namespace channels
{
    //==========================================================================
    /** Which end of the sixteen an MPE zone is anchored at. A lower zone runs
        up from channel 1, an upper zone down from channel 16 — one master and
        its members either way, and which end carries the zone-wide messages is
        the whole of the difference. */
    enum class Zone { lower, upper };

    /** Where each zone's manager channel is. Not a preference: MPE fixes them —
        "The Lower Zone is controlled by Manager Channel 1 … The Upper Zone is
        controlled by Manager Channel 16" (M1-100-UM v1.1, §2.2.1) — and they are
        also the only two channels an MPE Configuration Message is ever sent on,
        which is how the message says which zone it means. */
    inline constexpr int lowerManagerChannel = 1;
    inline constexpr int upperManagerChannel = 16;

    /** The MPE Configuration Message is RPN 6 — four control changes, not a
        message of its own. Sent on a manager channel, its data entry MSB is the
        number of **member** channels for that zone, and zero deactivates it. */
    inline constexpr int mpeConfigurationRpn = 6;

    /** The page asks two questions rather than showing one four-way switch:
        *which* of the two settings you are looking at, and whether it is on.
        The buttons under the channels then belong to whichever is showing. */
    inline const juce::StringArray settingNames { "omni", "MPE" };
    inline const juce::StringArray enabledNames { "on", "off" };
    inline const juce::StringArray zoneNames    { "lower zone", "upper zone" };
    inline const juce::StringArray selectNames  { "select all", "mute all" };

    /** `on` first in `enabledNames`, so an index is not a truth value. */
    inline constexpr int onIndex = 0;

    //==========================================================================
    //==========================================================================
    /** Pitch-bend sensitivity, in cents.

        Here rather than on the tuning page, where it used to be as a single
        global value: RPN 0 is addressed *per channel*, and once both MPE zones
        can be active there is no one number to show. It is still a statement
        about pitch and still in cents, which is the unit the interval, the
        period and the modulo divisor already use.

        **Cents, not semitones.** RPN 0's Data Entry MSB is semitones and its LSB
        is cents, which is what makes it usable microtonally at all. General MIDI
        2 §3.4.1 permits a receiver to ignore the LSB — which is why most synths
        appear to be semitone-only — but that is a conformance floor for GM2
        devices, not a limit in MIDI 1.0, and there is no reason to inherit it
        here. */
    inline constexpr int defaultBendCents = 200;

    /** What an MPE Configuration Message sets, per §2.2.5: 2 semitones on the
        manager channel and 48 on every member channel.

        Not a choice of ours, and a default rather than a lock — the same section
        allows RPN 0 to change either at any time afterwards, which is why these
        channels stay editable once an MCM has arrived. `juce::MPEZone` uses the
        same two numbers. */
    inline constexpr int mpeManagerBendCents = 2 * 100;
    inline constexpr int mpeMemberBendCents  = 48 * 100;

    /** RPN 0's semitone count is a 7-bit field, so 127 semitones is the largest
        range the message can express — the protocol's ceiling rather than a
        judgement about what is musical. */
    inline constexpr int highestBendCents = 127 * 100;

    //==========================================================================
    /** One bit per channel, bit 0 being channel 1. A mask rather than sixteen
        flags because that is how it will have to be handed to anything acting
        on it, and because "none selected" then needs no special case. */
    using Mask = juce::uint16;

    inline constexpr int numChannels = 16;

    inline constexpr Mask allChannels = 0xffff;
    inline constexpr Mask noChannels  = 0x0000;

    inline constexpr bool isSet (Mask mask, int channelIndex) noexcept
    {
        return (mask & (Mask) (1u << channelIndex)) != 0;
    }

    inline constexpr Mask withChannel (Mask mask, int channelIndex, bool shouldBeSet) noexcept
    {
        const auto bit = (Mask) (1u << channelIndex);
        return (Mask) (shouldBeSet ? (mask | bit) : (mask & ~bit));
    }

    /** The channels a zone covers, given the channel its far edge sits on.

        A zone runs contiguously from its anchor towards the other end, so its
        extent is one number — and clicking a channel is what sets it. Clicking
        8 in a lower zone gives channels 1 to 8.

        `edge` is a channel number, from 1, not an index. */
    inline constexpr Mask channelsForZone (Zone zone, int edge) noexcept
    {
        Mask covered = 0;

        if (zone == Zone::lower)
            for (int c = 1; c <= edge; ++c)
                covered = (Mask) (covered | (1u << (c - 1)));
        else
            for (int c = edge; c <= numChannels; ++c)
                covered = (Mask) (covered | (1u << (c - 1)));

        return covered;
    }

    inline constexpr int managerChannelFor (Zone zone) noexcept
    {
        return zone == Zone::lower ? lowerManagerChannel : upperManagerChannel;
    }

    /** The most member channels a zone can have: fifteen, which leaves only its
        own manager channel.

        Not a rounding of sixteen. The MPE specification's own example sends
        `mm = 0x0F` to the upper zone and describes the result as channel 1 being
        used as a member channel of that zone — so a zone really may reach across
        the other's manager channel, and the limit is what the arithmetic gives
        rather than a politeness we impose. */
    inline constexpr int maxMemberChannels = numChannels - 1;

    /** A zone's member channels — its manager channel excluded.

        A lower zone with `mm` members holds 2…1+mm; an upper zone holds
        16-mm…15. Both are `mm` channels wide, which is the check worth making
        when reading this: the manager is *not* one of them. */
    inline constexpr Mask memberChannelsForZone (Zone zone, int members) noexcept
    {
        Mask covered = 0;

        if (zone == Zone::lower)
            for (int c = 2; c <= 1 + members; ++c)
                covered = (Mask) (covered | (1u << (c - 1)));
        else
            for (int c = numChannels - members; c <= numChannels - 1; ++c)
                covered = (Mask) (covered | (1u << (c - 1)));

        return members > 0 ? covered : noChannels;
    }

    /** Everything a zone occupies: its manager channel and its members.

        Empty when the zone has no members, because a zone with no members is
        not a zone — "If a Zone no longer has any Member Channels, then it shall
        become deactivated" (M1-100-UM v1.1, §2.2.2). Deactivation is therefore
        not a flag that could disagree with the count; it *is* the count. */
    inline constexpr Mask allChannelsForZone (Zone zone, int members) noexcept
    {
        if (members <= 0)
            return noChannels;

        const auto manager = (Mask) (1u << (managerChannelFor (zone) - 1));

        return (Mask) (manager | memberChannelsForZone (zone, members));
    }

    //==========================================================================
    /** Everything the page holds, and everything an owner has to act on.

        One struct rather than four setters, because the two settings are read
        together — what a channel actually does depends on whether the zone has
        claimed it — and handing them over separately invites an owner to act on
        half an answer.
    */
    /** Sixteen channels at MIDI's own default of two semitones. A function
        because a defaulted `std::array` member would otherwise be sixteen
        zeroes, and zero is a real value here — a channel that does not bend. */
    inline std::array<int, numChannels> defaultBendArray()
    {
        std::array<int, numChannels> cents { };
        cents.fill (defaultBendCents);
        return cents;
    }

    struct Setup
    {
        //  Omni: which channels are listened to, and whether that is in force.
        bool omniOn = false;
        Mask omniChannels = allChannels;

        //  MPE, and whether that is in force.
        bool mpeOn = false;

        /** How many member channels each zone has, zero meaning deactivated.

            **Both zones exist at once**, which is what MPE actually describes:
            the two are independent, and any channels neither has claimed are
            "available for conventional use" (§2.2.1). The page used to hold one
            zone and a `Zone` saying which, so an MCM for the other end *moved*
            the zone; now an MCM configures its own zone and leaves the other
            alone except where they overlap. */
        int lowerMembers = maxMemberChannels;
        int upperMembers = 0;

        /** Which zone the strip is editing — a question about the view, not
            about the configuration. Switching it changes nothing that is sent
            or heard; it only decides which zone a click lands on. */
        Zone editing = Zone::lower;

        /** How far the pitch-bend wheel reaches on each channel, in cents,
            indexed from zero for channel 1.

            Per channel because that is how RPN 0 is addressed, and because MPE
            gives a zone's manager and its members *different* defaults — 2
            semitones and 48 — which a single value could not hold. The page
            edits them a zone at a time; the wire has always addressed them
            singly. */
        std::array<int, numChannels> pitchBendCents = defaultBendArray();

        int members (Zone zone) const noexcept
        {
            return zone == Zone::lower ? lowerMembers : upperMembers;
        }

        bool isActive (Zone zone) const noexcept  { return members (zone) > 0; }

        /** Every channel the two zones hold, or none when MPE is off. What an
            owner needs in order to know which channels the omni selection no
            longer governs — the two overlap, and MPE wins on the ones it has
            claimed. */
        Mask mpeChannels() const noexcept
        {
            if (! mpeOn)
                return noChannels;

            return (Mask) (allChannelsForZone (Zone::lower, lowerMembers)
                         | allChannelsForZone (Zone::upper, upperMembers));
        }
    };

    //==========================================================================
    /** Which channels a pitch-bend click sets, and what to call them.

        Empty when the click sets nothing, which is a real answer rather than a
        failure: under MPE a channel in neither zone is not a manager and not a
        member, so this view has no range for it. Its plain range belongs to the
        omni view, and offering it here would be offering to set something these
        buttons do not govern.

        A free function rather than a method on the page because it is the rule,
        not the widget — and a rule that can be checked without building a GUI.
    */
    struct BendTarget
    {
        Mask channelsAffected = noChannels;
        juce::String description;

        bool isEmpty() const noexcept { return channelsAffected == noChannels; }
    };

    inline BendTarget bendTargetFor (const Setup& setup, int channelIndex, bool mpeView)
    {
        if (! juce::isPositiveAndBelow (channelIndex, numChannels))
            return {};

        const auto channel = channelIndex + 1;
        const auto single  = (Mask) (1u << channelIndex);

        if (! mpeView)
            return { single, "channel " + juce::String (channel) };

        for (const auto zone : { Zone::lower, Zone::upper })
        {
            if (! setup.isActive (zone))
                continue;

            const auto name = juce::String (zone == Zone::lower ? "lower" : "upper");

            // No channel number: this is the *zone's* manager range, and naming
            // the channel would suggest it was that channel's plain setting,
            // which is a different value reached from the omni view.
            if (channel == managerChannelFor (zone))
                return { single, name + " zone manager" };

            // Every member together: §2.2.5 sets them as a group, and a per-note
            // range that differed between members would be a different
            // instrument on every note.
            const auto members = memberChannelsForZone (zone, setup.members (zone));

            if (isSet (members, channelIndex))
                return { members, name + " zone members" };
        }

        return {};
    }

    //==========================================================================
    /** Sets one zone's member count, letting the other zone yield what overlaps.

        **The later message wins, and this is the same rule the wire has.** MPE
        §2.2.1 gives precedence to the most recent MCM, and the specification's
        worked example is exactly this function: a lower zone of 7 members
        (channels 2–8) followed by an upper zone of 14 (channels 2–15) leaves the
        lower zone with nothing at all, and therefore deactivated. Doing it here
        rather than in the page means a click and a received MCM resolve overlap
        identically — which is what makes the display trustworthy.

        The other zone is shrunk rather than moved: its members run from its own
        manager channel outwards, so yielding means stopping earlier. If the
        claim reaches its manager channel there is nowhere to stop, and it
        deactivates.
    */
    inline Setup withZoneMembers (Setup setup, Zone zone, int members)
    {
        members = juce::jlimit (0, maxMemberChannels, members);

        (zone == Zone::lower ? setup.lowerMembers : setup.upperMembers) = members;

        const auto other  = zone == Zone::lower ? Zone::upper : Zone::lower;
        const auto claimed = allChannelsForZone (zone, members);

        // The other zone keeps the longest run that does not touch the claim.
        // Counted down rather than solved for, because the two zones grow from
        // opposite ends and an arithmetic shortcut here would have to be redone
        // the moment either anchor changed.
        auto& otherMembers = other == Zone::lower ? setup.lowerMembers : setup.upperMembers;

        if ((claimed & (Mask) (1u << (managerChannelFor (other) - 1))) != 0)
        {
            otherMembers = 0;
            return setup;
        }

        while (otherMembers > 0 && (memberChannelsForZone (other, otherMembers) & claimed) != 0)
            --otherMembers;

        return setup;
    }

    //==========================================================================
    /** This setup with an MPE Configuration Message applied.

        The message names a zone by the channel it arrived on and a **count** of
        member channels; the page holds an *edge*, which is the channel the zone
        reaches to. A lower zone with `mm` members occupies 1…1+mm, an upper zone
        16-mm…16 — so the conversion is the inverse of `midiFilter::layoutFor`'s,
        and both live beside their own type rather than being done at call sites.

        `mm = 0` deactivates the zone. The edge is *kept* rather than zeroed, for
        the same reason `zoneEdge` survives the zone being switched off by hand:
        turning it back on should restore what was set up, not a default.

        **Both zones.** MPE allows both at once, with any channels left over
        "available for conventional use" (§2.2.1), and that is now what is
        modelled: an MCM configures the zone it arrived on and leaves the other
        one alone, except where the two would overlap — which `withZoneMembers`
        resolves the way §2.2.1 says, in favour of the later message.

        `mm = 0` deactivates that zone and only that zone.
    */
    inline Setup withMpeConfiguration (Setup setup, Zone zone, int memberChannels)
    {
        setup = withZoneMembers (std::move (setup), zone, memberChannels);

        // MPE is in force while *either* zone has members. An MCM that empties
        // one zone does not turn the setting off, because the other may still be
        // playing.
        setup.mpeOn = setup.isActive (Zone::lower) || setup.isActive (Zone::upper);

        // "On receiving an MPE Configuration Message, the receiver shall set the
        // Manager Channel to 2 semitones and every Member Channel to 48"
        // (§2.2.5). A default, not a lock — the same section allows RPN 0 to
        // change either afterwards, which is why these are only applied here,
        // when the message arrives, and not maintained.
        if (memberChannels > 0)
        {
            setup.pitchBendCents[(size_t) (managerChannelFor (zone) - 1)] = mpeManagerBendCents;

            const auto members = memberChannelsForZone (zone, memberChannels);

            for (int c = 0; c < numChannels; ++c)
                if (isSet (members, c))
                    setup.pitchBendCents[(size_t) c] = mpeMemberBendCents;
        }

        return setup;
    }
}

} // namespace microtonos::sidebar
