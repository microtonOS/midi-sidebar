#pragma once

#include <juce_audio_basics/juce_audio_basics.h>

#include "../pages/ChannelsState.h"

namespace microtonos::sidebar
{

//==============================================================================
/** What the channels page means, applied to a message.

    A pure function of a `channels::Setup` and a channel number: nothing here
    reads a buffer, holds state or has an opinion about time, which is what lets
    it be checked over all sixteen channels in every configuration rather than
    tried and hoped for. See docs/channels.md.

    **The MPE half is `juce::MPEZoneLayout`'s**, not ours. It already knows a
    zone's manager and member channels, already parses the MPE Configuration
    Message, and already carries the two pitch-bend ranges the tuning page shows.
    Re-deriving any of that from `Setup::zoneEdge` would be a second answer to a
    question JUCE has already answered — so `Setup` is converted *into* a layout
    and the layout is asked.

    One conversion is needed because the two count differently: `Setup` stores
    the channel a zone reaches to, and `MPEZone` stores how many member channels
    it has. `layoutFor` is the only place that arithmetic appears.
*/
namespace midiFilter
{
    /** The zone as JUCE models it.

        A lower zone reaching to channel `e` has manager channel 1 and members
        2…e, so `e - 1` members. An upper zone reaching down to `e` has manager
        16 and members e…15, so `16 - e`. Either can come out as zero — a lower
        zone reaching only to channel 1 is a manager with nobody to manage — and
        `MPEZone::isActive` is false in that case, which is the right answer.

        **Both zones, independently.** MPE allows both at once and
        `channels::Setup` now holds both, so each is set from its own member
        count and a zone with no members is simply not set — which is the same
        thing MPE means by deactivated. `MPEZoneLayout::clearAllZones` first, so
        a zone that has just been emptied cannot survive as a stale layout and
        claim channels the end-user has taken back.
    */
    inline juce::MPEZoneLayout layoutFor (const channels::Setup& setup)
    {
        juce::MPEZoneLayout layout;

        layout.clearAllZones();

        if (! setup.mpeOn)
            return layout;   // both zones inactive, so MPE mode is off

        // JUCE resolves overlap between the two the same way the specification
        // does — the later call wins — but `channels::withZoneMembers` has
        // already resolved it, so these two counts never overlap by the time
        // they arrive here and the order of the calls does not matter.
        if (setup.lowerMembers > 0)
            layout.setLowerZone (setup.lowerMembers);

        if (setup.upperMembers > 0)
            layout.setUpperZone (setup.upperMembers);

        return layout;
    }

    //==========================================================================
    /** Whether the plugin listens to this channel at all.

        The filter section is the first gate: "The plugin listens to channels
        marked in the filter section. Messages and tunings on other channels are
        ignored" (docs/channels.md). A channel inside an active MPE zone is
        listened to whether or not omni selected it, because the zone is a
        statement about those channels that omni does not get to overrule.

        `channel` is 1..16 as MIDI counts it.
    */
    inline bool listensTo (const channels::Setup& setup, int channel)
    {
        if (! juce::isPositiveAndBelow (channel - 1, channels::numChannels))
            return false;

        if (layoutFor (setup).getLowerZone().isUsing (channel)
            || layoutFor (setup).getUpperZone().isUsing (channel))
            return true;

        return channels::isSet (setup.omniChannels, channel - 1);
    }

    /** True when the channel is a member channel of an active zone — the ones
        carrying per-note expression, and the ones on which polyphonic key
        pressure "shall not be sent" (MPE v1.1 §2.2.7). */
    inline bool isMemberChannel (const channels::Setup& setup, int channel)
    {
        const auto layout = layoutFor (setup);

        return layout.getLowerZone().isUsingChannelAsMemberChannel (channel)
            || layout.getUpperZone().isUsingChannelAsMemberChannel (channel);
    }

    /** True when the channel carries a zone's whole-zone messages. JUCE still
        calls this the *master* channel; MPE v1.1 renamed it **manager**. */
    inline bool isManagerChannel (const channels::Setup& setup, int channel)
    {
        const auto layout = layoutFor (setup);

        return (layout.getLowerZone().isActive() && layout.getLowerZone().getMasterChannel() == channel)
            || (layout.getUpperZone().isActive() && layout.getUpperZone().getMasterChannel() == channel);
    }

    //==========================================================================
    /** Whether a message's channel identity is thrown away.

        Omni on "reads messages from multiple channels while ignoring the channel
        number itself"; omni off keeps them apart (docs/channels.md). MPE member
        channels are never merged — the whole point of a zone is that the channel
        *is* the note — so a member channel answers false however omni is set.

        Program change is the documented exception: "program change messages are
        always interpreted as omni".
    */
    inline bool mergesChannel (const channels::Setup& setup, int channel,
                               const juce::MidiMessage& message)
    {
        if (message.isProgramChange())
            return true;

        if (isMemberChannel (setup, channel))
            return false;

        return setup.omniOn;
    }
}

} // namespace microtonos::sidebar
