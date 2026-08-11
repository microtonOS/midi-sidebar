#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "../SidebarLookAndFeel.h"
#include "../widgets/ChoiceStrip.h"
#include "TuningState.h"

namespace microtonos::sidebar
{

//==============================================================================
/** Which MIDI channels are handled separately, shown in a call-out.

    Omni off — the default — means each selected channel is tuned in its own
    right, from its own `_<i>.kbm` file or its own MTS ESP channel, and
    everything unselected falls back to the generic channel. Omni on means every
    channel is the generic channel, so the individual selections stop meaning
    anything and are disabled rather than hidden: they are still what you go
    back to.

    Selecting no channels at all is legal and is what the spec calls
    unadvisable, so nothing here prevents it.

    A third mask marks channels that cannot be tuned separately at all — MPE's,
    which carry one voice each and take their tuning from the generic channel.
    Those are disabled for the same reason omni disables all sixteen: the
    selection underneath is still what comes back when MPE is switched off.
*/
class ChannelSelector final : public juce::Component
{
public:
    ChannelSelector (bool startOmni, tuning::ChannelMask startMask,
                     tuning::ChannelMask unavailableMask = tuning::noChannels)
        : omni (startOmni), mask (startMask), unavailable (unavailableMask)
    {
        // The same connected pair the tuning page uses for note on / always:
        // omni is one either-or setting, and it should not look like two
        // unrelated switches that happen to be next to each other.
        omniStrip.setOrientation (ChoiceStrip::Orientation::vertical);
        omniStrip.onChoice = [this] (int index) { setOmni (index == omniOnIndex); };
        addAndMakeVisible (omniStrip);

        for (int i = 0; i < tuning::numChannels; ++i)
        {
            // Numbered as musicians count them, from 1 — the mask underneath is
            // zero-based, and this is the only place the two meet.
            auto* toggle = channels.add (new juce::TextButton (juce::String (i + 1)));

            toggle->setClickingTogglesState (true);
            toggle->setToggleState (tuning::isChannelSet (mask, i), juce::dontSendNotification);

            toggle->onClick = [this, i]
            {
                mask = tuning::withChannel (mask, i, channels[i]->getToggleState());
                notify();
            };

            addAndMakeVisible (toggle);
        }

        // "All" means all the ones that are available: ticking a channel MPE has
        // taken would put the mask somewhere no click could have put it.
        selectAll  .onClick = [this] { setMask ((tuning::ChannelMask) (tuning::allChannels & ~unavailable)); };
        deselectAll.onClick = [this] { setMask (tuning::noChannels); };

        addAndMakeVisible (selectAll);
        addAndMakeVisible (deselectAll);

        refresh();
    }

    /** Called whenever the selection changes, for every change: the call-out
        has no OK button to commit on, and dismissing it must not lose what was
        just clicked. */
    std::function<void (bool omniOn, tuning::ChannelMask)> onChanged;

    /** The size a call-out should be given before launching it. Derived from
        the grid inside, so it cannot drift from what is actually drawn. */
    static juce::Point<int> getPreferredSize()
    {
        const auto width = metrics::channelButton * channelColumns
                         + metrics::pageColumnGap * (channelColumns - 1)
                         + metrics::channelSideWidth
                         + metrics::pageColumnGap
                         + metrics::readOutPadding * 2;

        const auto height = metrics::pageRowHeight * channelRows
                          + metrics::pageRowGap * (channelRows - 1)
                          + metrics::readOutPadding * 2;

        return { width, height };
    }

    void resized() override
    {
        juce::Grid grid;
        using Track = juce::Grid::TrackInfo;

        // One grid, placed by line number rather than in flow order, because
        // the omni pair spans two of the four rows. The sketch in
        // docs/tuning.md draws exactly this: the side controls down the left,
        // sixteen channels in a block to their right.
        grid.templateColumns = { Track (juce::Grid::Px (metrics::channelSideWidth)) };

        for (int i = 0; i < channelColumns; ++i)
            grid.templateColumns.add (Track (juce::Grid::Px (metrics::channelButton)));

        for (int i = 0; i < channelRows; ++i)
            grid.templateRows.add (Track (juce::Grid::Px (metrics::pageRowHeight)));

        grid.columnGap = juce::Grid::Px (metrics::pageColumnGap);
        grid.rowGap    = juce::Grid::Px (metrics::pageRowGap);

        const auto place = [&grid] (juce::Component& c, int row, int column, int rowSpan = 1)
        {
            grid.items.add (juce::GridItem (c).withArea (row, column, row + rowSpan, column + 1));
        };

        // Rows and columns are 1-based grid lines. The omni pair covers the
        // first rows and divides itself on the gap between them; the two
        // buttons follow it, so their rows are derived from its span rather
        // than counted out by hand.
        place (omniStrip,   1, 1, omniRowSpan);
        place (selectAll,   1 + omniRowSpan, 1);
        place (deselectAll, 2 + omniRowSpan, 1);

        for (int row = 0; row < channelRows; ++row)
            for (int column = 0; column < channelColumns; ++column)
                place (*channels[row * channelColumns + column], row + 1, column + 2);

        grid.performLayout (getLocalBounds().reduced (metrics::readOutPadding));
    }

    /** See the note on ChoiceStrip: this whole component is built parentless
        inside a call-out, so being attached is the only moment its real
        LookAndFeel becomes reachable — and that moment sends
        `parentHierarchyChanged`, not `lookAndFeelChanged`. */
    void parentHierarchyChanged() override { lookAndFeelChanged(); }

    void lookAndFeelChanged() override
    {
        auto& lf = getLookAndFeel();

        if (! lf.isColourSpecified (ChoiceStrip::selectedColourId))
            return;

        // A selected channel is drawn the way every other "this one is on"
        // in the sidebar is drawn. Without this the call-out inherits
        // LookAndFeel_V4's toggled TextButton, which is *darker* than its
        // neighbours — so on a dark theme the selected channels read as the
        // disabled ones, which is precisely the complaint the rest of the
        // module already answers.
        for (auto* toggle : channels)
        {
            toggle->setColour (juce::TextButton::buttonOnColourId, lf.findColour (ChoiceStrip::selectedColourId));
            toggle->setColour (juce::TextButton::textColourOnId,   lf.findColour (ChoiceStrip::selectedTextColourId));
        }
    }

private:
    void setOmni (bool shouldBeOmni)
    {
        omni = shouldBeOmni;
        refresh();
        notify();
    }

    void setMask (tuning::ChannelMask newMask)
    {
        mask = newMask;
        refresh();
        notify();
    }

    void refresh()
    {
        omniStrip.setSelectedIndex (omni ? omniOnIndex : omniOffIndex);

        for (int i = 0; i < channels.size(); ++i)
        {
            channels[i]->setToggleState (tuning::isChannelSet (mask, i), juce::dontSendNotification);

            // Disabled rather than hidden, under omni and under MPE alike: the
            // selection still exists and is what comes back when either is
            // switched off, so removing it from view would misrepresent the
            // state.
            channels[i]->setEnabled (! omni && ! tuning::isChannelSet (unavailable, i));
        }

        selectAll  .setEnabled (! omni);
        deselectAll.setEnabled (! omni);
    }

    void notify()
    {
        if (onChanged != nullptr)
            onChanged (omni, mask);
    }

    static constexpr int channelColumns = 4;
    static constexpr int channelRows    = tuning::numChannels / channelColumns;

    /** Which entry of the omni strip is which. Omni *on* is first, matching the
        sketch, even though off is the default. */
    static constexpr int omniOnIndex  = 0;
    static constexpr int omniOffIndex = 1;

    /** Rows the omni pair covers, which is one per choice — and what the two
        buttons below it derive their own rows from. */
    static constexpr int omniRowSpan = 2;

    bool omni;
    tuning::ChannelMask mask;
    tuning::ChannelMask unavailable;

    ChoiceStrip omniStrip { {}, { "omni on", "omni off" } };
    juce::TextButton selectAll { "select all" }, deselectAll { "deselect all" };
    juce::OwnedArray<juce::TextButton> channels;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ChannelSelector)
};

} // namespace microtonos::sidebar
