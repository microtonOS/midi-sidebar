#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace microtonos::sidebar
{

//==============================================================================
/** Which MIDI channels the plugin listens to, and how it treats them.

    Two settings that used to live on two different pages and contradict each
    other: omni was part of the tuning page's multichannel call-out, and the MPE
    zone was a section of the controllers page, so a channel could be declared
    an MPE member and separately tuned at the same time. They are one setting
    and one filter now — see docs/channels.md.

    Free of MIDI, like the other pages' state: nothing here reads a message or
    silences a note. A mask is what the end-user asked for, and acting on it is
    somebody else's problem.
*/
namespace channels
{
    //==========================================================================
    /** What the plugin does with the channels it hears, as one four-way choice.

        Omni on and omni off differ in whether the channels are *merged*: on,
        a message moves the instrument; off, it moves only what is sounding on
        the channel it arrived on. The two zones are MPE, which is the same
        per-channel arrangement with the extra rule that the members are
        contiguous from one end of the sixteen and one of them is the master.
    */
    enum class Mode
    {
        omniOn,
        omniOff,
        lowerZone,
        upperZone
    };

    /** The mode is asked as two questions rather than as one four-way choice:
        `MPE on` or `off`, and then which of that answer's two.

        A four-way switch would have to be a two-by-two, and a segmented control
        is a line — but the better reason is that the pair sitting under `MPE
        on` is what says `lower zone` and `upper zone` are MPE's, which nothing
        about the words themselves does. */
    inline const juce::StringArray mpeNames  { "MPE on", "MPE off" };
    inline const juce::StringArray omniNames { "omni on", "omni off" };
    inline const juce::StringArray zoneNames { "lower zone", "upper zone" };

    inline constexpr int mpeOnIndex = 0;

    inline constexpr bool isZone (Mode mode) noexcept
    {
        return mode == Mode::lowerZone || mode == Mode::upperZone;
    }

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

        A zone is anchored at one end of the sixteen and runs contiguously
        towards the other: a lower zone from channel 1 up to `edge`, an upper
        zone from `edge` up to channel 16. So the extent is one number, and
        clicking a channel in the grid is what sets it — which is why the grid
        stays live under a zone instead of being locked. Clicking 8 in a lower
        zone turns 1 to 8 on and the rest off.

        `edge` is a channel number, from 1, not an index. Returns nothing for
        the omni modes, where the mask is the end-user's free choice rather than
        a range. */
    inline constexpr Mask channelsForZone (Mode mode, int edge) noexcept
    {
        Mask covered = 0;

        if (mode == Mode::lowerZone)
            for (int c = 1; c <= edge; ++c)
                covered = (Mask) (covered | (1u << (c - 1)));

        if (mode == Mode::upperZone)
            for (int c = edge; c <= numChannels; ++c)
                covered = (Mask) (covered | (1u << (c - 1)));

        return covered;
    }

    /** Where a zone's edge sits when nothing has been clicked yet: the whole
        sixteen, since a zone that starts off covering half the instrument would
        look like a setting somebody had already made. */
    inline constexpr int fullExtentFor (Mode mode) noexcept
    {
        return mode == Mode::upperZone ? 1 : numChannels;
    }
}

} // namespace microtonos::sidebar
