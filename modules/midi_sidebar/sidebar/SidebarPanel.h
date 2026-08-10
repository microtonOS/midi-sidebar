#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "SidebarLookAndFeel.h"
#include "pages/ControllersPage.h"
#include "pages/PresetsPage.h"
#include "pages/TuningPage.h"

namespace microtonos::sidebar
{

//==============================================================================
/** The panel revealed beside the rail when a page button is active.

    The frame that hosts the pages: it draws the title and gives whatever page
    is showing the rest of its bounds. It owns the pages rather than the sidebar
    doing so, because the panel is what knows how much room there is.

    All three pages live in the same track; only one is visible at a time.
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

        // The page's own section titles are what this has to line up with, not
        // the panel's edge, so the label's own border is taken off and the
        // indent applied in `resized` instead — where it can use the one number
        // that says where a GroupComponent draws its title.
        title.setBorderSize ({});
        addAndMakeVisible (title);

        addChildComponent (presetsPage);
        addChildComponent (controllersPage);
        addChildComponent (tuningPage);
    }

    void setTitle (const juce::String& newTitle)
    {
        title.setText (newTitle, juce::dontSendNotification);
    }

    /** Shows one page and hides the others. */
    void showTuningPage (bool shouldShow)      { tuningPage.setVisible (shouldShow); }
    void showControllersPage (bool shouldShow) { controllersPage.setVisible (shouldShow); }
    void showPresetsPage (bool shouldShow)     { presetsPage.setVisible (shouldShow); }

    TuningPage&      getTuningPage()      noexcept { return tuningPage; }
    ControllersPage& getControllersPage() noexcept { return controllersPage; }
    PresetsPage&     getPresetsPage()     noexcept { return presetsPage; }

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

        // Title on its own row at the top; the rest goes to whichever page is
        // showing. Every page occupies the same track, so only one can be
        // visible and none of them has to know about the title.
        grid.templateColumns = { Track (juce::Grid::Fr (1)) };
        grid.templateRows    = { Track (juce::Grid::Px (metrics::railButton)),
                                 Track (juce::Grid::Fr (1)) };

        // Every page is placed in the *same* cell, explicitly. Left to
        // auto-placement, the second one would be given an implicit new row
        // below the first and laid out off the bottom of the panel — where it
        // is invisible, and looks exactly like a page that was never shown.
        // The title is indented to where the page's content starts, so "Tuning"
        // begins directly above the field below it rather than above the
        // panel's edge.
        grid.items = { juce::GridItem (title).withArea (1, 1, 2, 2)
                           .withMargin ({ 0.0f, 0.0f, 0.0f, (float) metrics::pageContentIndent }),
                       juce::GridItem (tuningPage)     .withArea (pageRow, 1, pageRow + 1, 2),
                       juce::GridItem (controllersPage).withArea (pageRow, 1, pageRow + 1, 2),
                       juce::GridItem (presetsPage)    .withArea (pageRow, 1, pageRow + 1, 2) };

        grid.performLayout (getLocalBounds().reduced (metrics::railPadding));
    }

private:
    /** The grid row every page shares, under the title. A line number rather
        than a measurement, which is why it lives here and not in `metrics`. */
    static constexpr int pageRow = 2;

    juce::Label title;
    TuningPage tuningPage;
    ControllersPage controllersPage;
    PresetsPage presetsPage;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SidebarPanel)
};

} // namespace microtonos::sidebar
