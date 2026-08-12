#include "ChannelsPage.h"

namespace microtonos::sidebar
{

// **No `using namespace channels;`.** This module is a unity build — every
// page's .cpp is included into midi_sidebar.cpp — so a file-scope
// using-directive in one of them is still in force in the next. ControllersPage
// has one for `controllers`, whose names would then be ambiguous against these.

//==============================================================================
ChannelsPage::ChannelsPage()
    : settingStrip ({}, channels::settingNames),
      enabledStrip ({}, channels::enabledNames)
{
    // The frame first, so everything drawn over it stays legible — the
    // arrangement every page in this module uses, and the reason a
    // GroupComponent is a frame rather than a container.
    filterGroup.setTextLabelPosition (juce::Justification::centredLeft);
    filterGroup.setInterceptsMouseClicks (false, false);
    filterGroup.setText ("FILTER");
    addAndMakeVisible (filterGroup);

    for (auto* strip : { &settingStrip, &enabledStrip })
        addAndMakeVisible (*strip);

    // Choosing a view changes nothing about either setting — it only decides
    // which of them the rest of the page is showing.
    settingStrip.onChoice = [this] (int) { refresh(); };

    enabledStrip.onChoice = [this] (int index)
    {
        (showingMpe() ? setup.mpeOn : setup.omniOn) = index == channels::onIndex;

        refresh();
        announce();
    };

    // The edge is kept across a change of zone, so switching from a lower zone
    // reaching to 9 gives an upper zone from 9 — the channel last clicked stays
    // the boundary rather than the setting resetting.
    const auto pickZone = [this] (channels::Zone zone)
    {
        setup.zone = zone;

        refresh();
        announce();
    };

    lowerZoneButton.onClick = [pickZone] { pickZone (channels::Zone::lower); };
    upperZoneButton.onClick = [pickZone] { pickZone (channels::Zone::upper); };

    channelGrid.onChannelClicked = [this] (int index) { channelClicked (index); };
    addAndMakeVisible (channelGrid);

    selectAllButton.onClick = [this] { setup.omniChannels = channels::allChannels; refresh(); announce(); };
    muteAllButton  .onClick = [this] { setup.omniChannels = channels::noChannels;  refresh(); announce(); };

    // The name as well as the text. `TextButton (name)` sets both, but these
    // take their labels from the arrays above at construction time, so the name
    // has to be given separately — and it is what identifies the button in a
    // component tree and to the snapshot tool's `--click`.
    const auto label = [] (juce::TextButton& button, const juce::String& text)
    {
        button.setButtonText (text);
        button.setName (text);
    };

    label (selectAllButton, channels::selectNames[0]);
    label (muteAllButton,   channels::selectNames[1]);
    label (lowerZoneButton, channels::zoneNames[0]);
    label (upperZoneButton, channels::zoneNames[1]);

    for (auto* b : { &selectAllButton, &muteAllButton, &lowerZoneButton, &upperZoneButton })
        addAndMakeVisible (*b);

    refresh();
}

//==============================================================================
bool ChannelsPage::showingMpe() const noexcept
{
    return settingStrip.getSelectedIndex() == 1;
}

void ChannelsPage::setSetup (channels::Setup newSetup)
{
    setup = newSetup;
    refresh();
}

void ChannelsPage::announce()
{
    if (onSetupChanged != nullptr)
        onSetupChanged (setup);
}

void ChannelsPage::channelClicked (int channelIndex)
{
    if (showingMpe())
        setup.zoneEdge = channelIndex + 1;   // a channel number, not an index
    else
        setup.omniChannels = channels::withChannel (setup.omniChannels, channelIndex,
                                                    ! channels::isSet (setup.omniChannels, channelIndex));

    refresh();
    announce();
}

void ChannelsPage::refresh()
{
    const auto mpe = showingMpe();

    enabledStrip.setSelectedIndex ((mpe ? setup.mpeOn : setup.omniOn) ? channels::onIndex
                                                                     : channels::onIndex + 1);

    // The grid shows whichever setting is in view — the zone's span under MPE,
    // the free selection under omni. Neither is drawn through the other; see
    // the note on the class.
    channelGrid.setChannels (mpe ? channels::channelsForZone (setup.zone, setup.zoneEdge)
                                 : setup.omniChannels);

    // One row, two pairs. Neither pair means anything to the other setting, so
    // only the one belonging to the view in force is there at all.
    lowerZoneButton.setVisible (mpe);
    upperZoneButton.setVisible (mpe);

    selectAllButton.setVisible (! mpe);
    muteAllButton  .setVisible (! mpe);
}

//==============================================================================
void ChannelsPage::lookAndFeelChanged()
{
    // Guarded like every other lookAndFeelChanged here: it also fires during
    // teardown, when the owner sets its LookAndFeel to nullptr and these ids
    // stop resolving.
    if (! getLookAndFeel().isColourSpecified (pageColours::sectionTitleColourId))
        return;

    filterGroup.setColour (juce::GroupComponent::textColourId,
                           findColour (pageColours::sectionTitleColourId));
    filterGroup.setColour (juce::GroupComponent::outlineColourId,
                           findColour (pageColours::sectionOutlineColourId));
}

//==============================================================================
void ChannelsPage::resized()
{
    // The same six columns as the other pages, so all four read as one plugin.
    PageGrid pageGrid;

    constexpr auto full = metrics::pageColumns;
    constexpr auto half = metrics::pageColumns / 2;
    constexpr auto rightHalf = 1 + half;

    //  The two switches, unframed at the top: the sketch gives them no title,
    //  and on these pages that is what says "not a named section". Two rows,
    //  which is every page's opening block — see metrics::pageTopRows.
    pageGrid.place (settingStrip, pageGrid.addRow (metrics::pageRowHeight), 1, full);
    pageGrid.place (enabledStrip, pageGrid.addRow (metrics::pageRowHeight), 1, full);

    //  Filter -------------------------------------------------------------
    const auto filterTitle = pageGrid.addRow (metrics::pageGroupTitleHeight);

    // One item spanning all six columns; its own four columns are its business.
    pageGrid.place (channelGrid, pageGrid.addRow (metrics::channelGridHeight), 1, full);

    {
        // All four share the row, explicitly. Left to auto-placement the
        // hidden ones would be given implicit rows below the page and would
        // come back invisible when they were shown.
        const auto row = pageGrid.addRow (metrics::pageRowHeight);

        pageGrid.place (selectAllButton, row, 1, half);
        pageGrid.place (muteAllButton,   row, rightHalf, half);

        pageGrid.place (lowerZoneButton, row, 1, half);
        pageGrid.place (upperZoneButton, row, rightHalf, half);
    }

    pageGrid.frame (filterGroup, filterTitle, pageGrid.addRow (metrics::pageGroupPadding));

    // No flexible track, so the page needs its natural height or it is clipped
    // — the tuning page's arrangement rather than the controllers page's.
    pageGrid.performLayout (getLocalBounds(), getNaturalHeight());
}

} // namespace microtonos::sidebar
