#include "ChannelsPage.h"

namespace microtonos::sidebar
{

// **No `using namespace channels;`.** This module is a unity build — every
// page's .cpp is included into midi_sidebar.cpp — so a file-scope
// using-directive in one of them is still in force in the next. ControllersPage
// has one for `controllers`, whose `Mode` and `modeNames` would then be
// ambiguous against these. Qualified names cost a word and cannot collide.

//==============================================================================
ChannelsPage::ChannelsPage()
    : mpeStrip  ({}, channels::mpeNames),
      omniStrip ({}, channels::omniNames),
      zoneStrip ({}, channels::zoneNames)
{
    // The frame first, so everything drawn over it stays legible — the
    // arrangement every page in this module uses, and the reason a
    // GroupComponent is a frame rather than a container.
    filterGroup.setTextLabelPosition (juce::Justification::centredLeft);
    filterGroup.setInterceptsMouseClicks (false, false);
    filterGroup.setText ("FILTER");
    addAndMakeVisible (filterGroup);

    for (auto* strip : { &mpeStrip, &omniStrip, &zoneStrip })
        addAndMakeVisible (*strip);

    mpeStrip.onChoice = [this] (int index)
    {
        // The second question keeps its own last answer, so turning MPE off and
        // on again finds the zone that was left rather than resetting it.
        modePicked (index == channels::mpeOnIndex ? lastZone : lastOmni);
    };

    omniStrip.onChoice = [this] (int index)
    {
        modePicked (index == 0 ? channels::Mode::omniOn : channels::Mode::omniOff);
    };

    zoneStrip.onChoice = [this] (int index)
    {
        modePicked (index == 0 ? channels::Mode::lowerZone : channels::Mode::upperZone);
    };

    grid.onChannelClicked = [this] (int index) { channelClicked (index); };
    addAndMakeVisible (grid);

    selectAllButton.onClick = [this] { chosen = channels::allChannels; refresh(); announceChannels(); };
    muteAllButton  .onClick = [this] { chosen = channels::noChannels;  refresh(); announceChannels(); };

    for (auto* b : { &selectAllButton, &muteAllButton })
        addAndMakeVisible (*b);

    refresh();
}

//==============================================================================
void ChannelsPage::setMode (channels::Mode newMode)
{
    // A zone arriving from outside starts at its full extent rather than at
    // whatever edge a previous zone happened to leave behind.
    if (channels::isZone (newMode) && ! channels::isZone (mode))
        zoneEdge = channels::fullExtentFor (newMode);

    mode = newMode;
    refresh();
}

void ChannelsPage::setChannels (channels::Mask mask)
{
    chosen = mask;
    refresh();
}

channels::Mask ChannelsPage::getChannels() const noexcept
{
    return channels::isZone (mode) ? channels::channelsForZone (mode, zoneEdge) : chosen;
}

void ChannelsPage::modePicked (channels::Mode picked)
{
    setMode (picked);

    if (onModeChanged != nullptr)
        onModeChanged (mode);

    // The mask changes with the mode even though nothing in the grid was
    // touched — omni's free selection and a zone's span are different answers.
    announceChannels();
}

void ChannelsPage::channelClicked (int channelIndex)
{
    if (channels::isZone (mode))
        zoneEdge = channelIndex + 1;   // a channel number, not an index
    else
        chosen = channels::withChannel (chosen, channelIndex,
                                        ! channels::isSet (chosen, channelIndex));

    refresh();
    announceChannels();
}

void ChannelsPage::announceChannels()
{
    if (onChannelsChanged != nullptr)
        onChannelsChanged (getChannels());
}

void ChannelsPage::refresh()
{
    const auto zone = channels::isZone (mode);

    (zone ? lastZone : lastOmni) = mode;

    mpeStrip.setSelectedIndex (zone ? channels::mpeOnIndex : channels::mpeOnIndex + 1);

    // One of the two occupies the cell; the other is not merely disabled but
    // absent, because a question that does not apply is not a question with a
    // greyed answer.
    omniStrip.setVisible (! zone);
    zoneStrip.setVisible (zone);

    omniStrip.setSelectedIndex (mode == channels::Mode::omniOn    ? 0 : 1);
    zoneStrip.setSelectedIndex (mode == channels::Mode::lowerZone ? 0 : 1);

    grid.setChannels (getChannels());

    // The grid stays live under a zone — a click there sets the zone's edge.
    // These two do not: "all" and "none" are not extents, and a zone with
    // nothing in it is not a zone. They belong to the free selection, and
    // under a zone there is little to save anyway.
    selectAllButton.setEnabled (! zone);
    muteAllButton  .setEnabled (! zone);
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

    //  The mode, unframed at the top: the sketch gives it no title, and on
    //  these pages that is what says "not a named section". Two rows, which is
    //  the height every page's opening block has — see metrics::pageTopRows.
    pageGrid.place (mpeStrip, pageGrid.addRow (metrics::pageRowHeight), 1, full);

    {
        // Both second-row strips are given the same cell, explicitly. Left to
        // auto-placement the hidden one would be handed an implicit row below
        // the page and would come back invisible when it was shown.
        const auto row = pageGrid.addRow (metrics::pageRowHeight);

        pageGrid.place (omniStrip, row, 1, full);
        pageGrid.place (zoneStrip, row, 1, full);
    }

    //  Filter -------------------------------------------------------------
    const auto filterTitle = pageGrid.addRow (metrics::pageGroupTitleHeight);

    // One item spanning all six columns; its own four columns are its business.
    pageGrid.place (grid, pageGrid.addRow (metrics::channelGridHeight), 1, full);

    {
        const auto row = pageGrid.addRow (metrics::pageRowHeight);

        pageGrid.place (selectAllButton, row, 1, half);
        pageGrid.place (muteAllButton,   row, rightHalf, half);
    }

    pageGrid.frame (filterGroup, filterTitle, pageGrid.addRow (metrics::pageGroupPadding));

    // No flexible track, so the page needs its natural height or it is clipped
    // — the tuning page's arrangement rather than the controllers page's.
    pageGrid.performLayout (getLocalBounds(), getNaturalHeight());
}

} // namespace microtonos::sidebar
