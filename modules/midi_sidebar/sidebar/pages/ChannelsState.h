#pragma once

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

    //==========================================================================
    /** Everything the page holds, and everything an owner has to act on.

        One struct rather than four setters, because the two settings are read
        together — what a channel actually does depends on whether the zone has
        claimed it — and handing them over separately invites an owner to act on
        half an answer.
    */
    struct Setup
    {
        //  Omni: which channels are listened to, and whether that is in force.
        bool omniOn = false;
        Mask omniChannels = allChannels;

        //  MPE: a zone, and whether that is in force.
        bool mpeOn = false;
        Zone zone = Zone::lower;

        /** The channel the zone reaches to, from 1. Kept when the zone is
            switched or turned off, so neither throws away what was set. */
        int zoneEdge = numChannels;

        /** The zone's channels, or none when MPE is off. What an owner needs in
            order to know which channels the omni selection no longer governs —
            the two overlap, and MPE wins on the ones it has claimed. */
        Mask mpeChannels() const noexcept
        {
            return mpeOn ? channelsForZone (zone, zoneEdge) : noChannels;
        }
    };
}

} // namespace microtonos::sidebar
