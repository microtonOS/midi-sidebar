#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include "../SidebarLookAndFeel.h"
#include "../pages/ChannelsState.h"
#include "ChoiceStrip.h"

namespace microtonos::sidebar
{

//==============================================================================
/** The sixteen MIDI channels as a block of toggles, four across.

    One widget rather than sixteen placed by the page, so the page's grid holds
    a single item and the block's own four columns cannot be confused with the
    page's six — four does not divide six, and trying to express one in the
    other is how the numbers would stop lining up with each other.

    A toggle per channel rather than a `ToggleButton` with a tick: at this size
    the number *is* the label, and a tick box beside it would double the width
    of every cell to say what the button's own state already says.

    **It reports clicks and never changes itself.** What a click means depends
    on the mode: under omni it toggles one channel, under a zone it sets the
    zone's extent and therefore moves several at once. That is the page's
    business, so this widget says which button was pressed and waits to be told
    what to show — the same "values in, intent out" the pages use.
*/
class ChannelGrid final : public juce::Component
{
public:
    ChannelGrid()
    {
        for (int i = 0; i < channels::numChannels; ++i)
        {
            // Numbered as musicians count them, from 1. The mask underneath is
            // zero-based, and this is the only place the two meet.
            auto* toggle = buttons.add (new juce::TextButton (juce::String (i + 1)));

            // Deliberately *not* `setClickingTogglesState`: a button that lit
            // itself would be showing a state the page had not agreed to, and
            // under a zone a click changes several channels rather than the one
            // that was pressed.
            toggle->onClick = [this, i]
            {
                if (onChannelClicked != nullptr)
                    onChannelClicked (i);
            };

            addAndMakeVisible (toggle);
        }
    }

    /** What to show. The only thing that lights a button. */
    void setChannels (channels::Mask mask)
    {
        for (int i = 0; i < buttons.size(); ++i)
            buttons[i]->setToggleState (channels::isSet (mask, i), juce::dontSendNotification);
    }

    /** Which channel was pressed, zero-based. What that means is the page's to
        decide. */
    std::function<void (int channelIndex)> onChannelClicked;

    void resized() override
    {
        juce::Grid grid;
        using Track = juce::Grid::TrackInfo;

        // Flexible columns, fixed rows: the block is as wide as the page gives
        // it and the buttons share that width, while a channel button's height
        // is its own so the block does not stretch when the panel grows.
        for (int i = 0; i < metrics::channelColumns; ++i)
            grid.templateColumns.add (Track (juce::Grid::Fr (1)));

        for (int i = 0; i < metrics::channelRows; ++i)
            grid.templateRows.add (Track (juce::Grid::Px (metrics::channelButton)));

        grid.columnGap = juce::Grid::Px (metrics::pageColumnGap);
        grid.rowGap    = juce::Grid::Px (metrics::pageRowGap);

        for (auto* button : buttons)
            grid.items.add (juce::GridItem (*button));

        grid.performLayout (getLocalBounds());
    }

    /** Both hooks, for the reason given on ChoiceStrip: being added to an
        already-styled parent sends `parentHierarchyChanged` but not
        `lookAndFeelChanged`. */
    void parentHierarchyChanged() override { lookAndFeelChanged(); }

    void lookAndFeelChanged() override
    {
        auto& lf = getLookAndFeel();

        if (! lf.isColourSpecified (ChoiceStrip::selectedColourId))
            return;

        // A selected channel is drawn the way every other "this one is on" in
        // the sidebar is drawn. Without this the grid inherits LookAndFeel_V4's
        // toggled TextButton, which is *darker* than its neighbours — so on a
        // dark theme the selected channels read as the disabled ones.
        for (auto* button : buttons)
        {
            button->setColour (juce::TextButton::buttonOnColourId, lf.findColour (ChoiceStrip::selectedColourId));
            button->setColour (juce::TextButton::textColourOnId,   lf.findColour (ChoiceStrip::selectedTextColourId));
        }
    }

private:
    juce::OwnedArray<juce::TextButton> buttons;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ChannelGrid)
};

} // namespace microtonos::sidebar
