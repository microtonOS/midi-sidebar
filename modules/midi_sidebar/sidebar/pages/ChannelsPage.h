#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include "../SidebarLookAndFeel.h"
#include "../widgets/ChannelGrid.h"
#include "../widgets/ChoiceStrip.h"
#include "ChannelsState.h"
#include "PageGrid.h"

namespace microtonos::sidebar
{

//==============================================================================
/** The channels page: what the plugin listens to, and how it treats it.

    Implements docs/channels.md. The mode at the top, then a `FILTER` section
    holding the sixteen channels and the two buttons that set them all at once.

    **The mode is two questions, not one four-way switch.** `MPE on` or `off`
    first; the row below it then offers that answer's two — `lower zone` /
    `upper zone` under on, `omni on` / `omni off` under off. Two reasons, and
    the second is the better one: a segmented control is a line rather than a
    two-by-two, and putting the zones *under* `MPE on` is what says they are
    MPE's, which nothing about the words "lower zone" does on its own.

    **The two used to be separate and disagree.** Omni lived in a call-out on
    the tuning page and chose which channels were tuned in their own right; the
    MPE zone was a section of the controllers page. A channel could be declared
    an MPE member and separately tuned at the same time, which is why the owner
    had to be told to grey one out from the other. One mode and one filter
    cannot contradict each other, so nothing has to.

    **Under a zone the grid sets its extent.** A zone runs contiguously from one
    end of the sixteen, so it is described by a single edge — and clicking a
    channel is how that edge is set: in a lower zone, clicking 8 turns 1 to 8 on
    and the rest off. The grid therefore stays live rather than being locked;
    what changes is what a click *means*.

    Holds no MIDI and silences nothing: values in, intent out, like every other
    page.
*/
class ChannelsPage final : public juce::Component
{
public:
    ChannelsPage();

    //==========================================================================
    //  Values in.

    void setMode (channels::Mode mode);
    channels::Mode getMode() const noexcept { return mode; }

    /** The channels the end-user has chosen. Under a zone this is remembered
        rather than shown — the grid shows the zone — and comes back when the
        mode leaves it. */
    void setChannels (channels::Mask mask);

    /** What is actually being listened to: the free selection under omni, the
        zone's span under a zone. This is the one an owner acts on. */
    channels::Mask getChannels() const noexcept;

    //==========================================================================
    //  Intent out. Neither changes the page; the owner acts and pushes the
    //  result back, so what is drawn is always what the plugin has.

    std::function<void (channels::Mode)> onModeChanged;
    std::function<void (channels::Mask)> onChannelsChanged;

    //==========================================================================
    /** The height this page needs. Summed from the rows rather than measured
        once and written down, so a changed row height cannot leave it stale. */
    static constexpr int getNaturalHeight() noexcept
    {
        constexpr int tracks = 4;   // the top block, the title, the grid, the padding

        return metrics::pageTopHeight (metrics::pageTopRows)
             + metrics::pageGroupTitleHeight
             + metrics::channelGridHeight
             + metrics::pageRowHeight              // select all | mute all
             + metrics::pageGroupPadding
             + tracks * metrics::pageRowGap;
    }

    void resized() override;
    void lookAndFeelChanged() override;

private:
    //==========================================================================
    /** Shows the mode, and whatever it implies about the rest of the page. */
    void refresh();

    void modePicked (channels::Mode picked);

    /** What a click on channel `index` means, which depends on the mode. */
    void channelClicked (int channelIndex);

    /** Announces whatever `getChannels` now returns. */
    void announceChannels();

    channels::Mode mode = channels::Mode::omniOff;

    /** The last answer given to each of the two second-row questions, so that
        switching MPE off and on again finds the zone that was left rather than
        resetting it — and the same the other way. */
    channels::Mode lastOmni = channels::Mode::omniOff;
    channels::Mode lastZone = channels::Mode::lowerZone;

    /** What the end-user selected under omni, which is not what the grid shows
        while a zone is in force. */
    channels::Mask chosen = channels::allChannels;

    /** The channel a zone reaches to — its far end from the anchor. Kept across
        a change of zone, so switching lower to upper keeps the extent the
        end-user last clicked rather than resetting it. */
    int zoneEdge = channels::numChannels;

    juce::GroupComponent filterGroup;

    /** The two questions. `omniStrip` and `zoneStrip` share one cell and take
        turns being visible — `ChoiceStrip` is built with its choices, so the
        second question is two widgets rather than one that is relabelled. */
    ChoiceStrip mpeStrip, omniStrip, zoneStrip;

    ChannelGrid grid;
    juce::TextButton selectAllButton { "select all" }, muteAllButton { "mute all" };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ChannelsPage)
};

} // namespace microtonos::sidebar
