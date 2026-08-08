#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "SidebarLookAndFeel.h"

namespace microtonos::sidebar
{

//==============================================================================
/** The panel revealed beside the rail when a page button is active.

    A placeholder for now: it shows which page is open and nothing else. The
    presets, controllers and tuning pages each get their own file when they are
    built, and this becomes the frame that hosts whichever one is active.
*/
class SidebarPanel final : public juce::Component
{
public:
    enum ColourIds
    {
        backgroundColourId = 0x1a10200,
        titleColourId      = 0x1a10201
    };

    SidebarPanel()
    {
        title.setJustificationType (juce::Justification::centredLeft);
        title.setFont (SidebarLookAndFeel::font (metrics::titleFontHeight, true));
        addAndMakeVisible (title);
    }

    void setTitle (const juce::String& newTitle)
    {
        title.setText (newTitle, juce::dontSendNotification);
    }

    void paint (juce::Graphics& g) override
    {
        g.fillAll (findColour (backgroundColourId));
    }

    void lookAndFeelChanged() override
    {
        // Guarded for the same reason as Sidebar::refreshIcons: this also fires
        // during teardown, when the owner sets its LookAndFeel to nullptr and
        // our ColourIds are no longer resolvable.
        if (getLookAndFeel().isColourSpecified (titleColourId))
            title.setColour (juce::Label::textColourId, findColour (titleColourId));
    }

    void resized() override
    {
        juce::Grid grid;
        using Track = juce::Grid::TrackInfo;

        // Title on its own row at the top; the rest is left for the page
        // content, which is why it is a flexible track rather than empty space.
        grid.templateColumns = { Track (juce::Grid::Fr (1)) };
        grid.templateRows    = { Track (juce::Grid::Px (metrics::railButton)),
                                 Track (juce::Grid::Fr (1)) };

        grid.items = { juce::GridItem (title), juce::GridItem() };

        grid.performLayout (getLocalBounds().reduced (metrics::railPadding));
    }

private:
    juce::Label title;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SidebarPanel)
};

} // namespace microtonos::sidebar
