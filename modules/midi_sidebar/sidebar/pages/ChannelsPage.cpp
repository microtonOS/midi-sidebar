#include "ChannelsPage.h"

#include "../PopupHost.h"

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

    // Switching zones changes nothing that is heard: both zones are configured
    // at once, and this only says which one a click lands on. It used to *move*
    // the single zone, which is why the two buttons are still a pair rather than
    // two independent toggles.
    const auto pickZone = [this] (channels::Zone zone)
    {
        setup.editing = zone;

        refresh();
        announce();
    };

    lowerZoneButton.onClick = [pickZone] { pickZone (channels::Zone::lower); };
    upperZoneButton.onClick = [pickZone] { pickZone (channels::Zone::upper); };

    bendButton.setName ("bendButton");
    bendButton.setButtonText ("pitchbend sensitivity");
    bendButton.setClickingTogglesState (true);
    bendButton.onClick = [this] { refresh(); };
    addAndMakeVisible (bendButton);

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
    // The bend button takes the click entirely rather than adding to it: a
    // click that both retuned the wheel and moved a zone edge would be one
    // gesture doing two unrelated things.
    if (bendButton.getToggleState())
    {
        showBendEditor (channelIndex);
        return;
    }

    if (showingMpe())
    {
        // A click names the channel the zone should reach to, which is a member
        // *count* once the manager channel is discounted — and clicking the
        // manager channel itself therefore gives zero, deactivating the zone.
        // That is the specification's own rule rather than a shortcut: "If a
        // Zone no longer has any Member Channels, then it shall become
        // deactivated" (§2.2.2).
        const auto channel = channelIndex + 1;   // a channel number, not an index

        const auto members = setup.editing == channels::Zone::lower
                                 ? channel - channels::lowerManagerChannel
                                 : channels::upperManagerChannel - channel;

        setup = channels::withZoneMembers (setup, setup.editing, members);
    }
    else
        setup.omniChannels = channels::withChannel (setup.omniChannels, channelIndex,
                                                    ! channels::isSet (setup.omniChannels, channelIndex));

    refresh();
    announce();
}

void ChannelsPage::showBendEditor (int channelIndex)
{
    const auto target = channels::bendTargetFor (setup, channelIndex, showingMpe());

    if (target.isEmpty())
        return;

    // Inside the editor rather than on the desktop, which is what the volume
    // call-out does and for the same reasons: a desktop call-out has no parent,
    // so `getLookAndFeel` finds nothing, none of this module's ColourIds resolve
    // and `createComponentSnapshot` cannot see it at all.
    auto* host = findPopupHost (*this);

    if (host == nullptr)
        return;

    // The first affected channel's value is what the bubble opens on. Under MPE
    // the members agree, so any of them is *the* value; under omni there is
    // only one anyway.
    auto current = channels::defaultBendCents;

    for (int c = 0; c < channels::numChannels; ++c)
        if (channels::isSet (target.channelsAffected, c))
        {
            current = setup.pitchBendCents[(size_t) c];
            break;
        }

    auto content = std::make_unique<BendBubble>();

    content->describe (target.description, current);

    // Committing closes the bubble: the value is the whole of what it is for,
    // so there is nothing left to look at once it has been given.
    content->onCommit = [this, affected = target.channelsAffected] (int cents)
    {
        for (int c = 0; c < channels::numChannels; ++c)
            if (channels::isSet (affected, c))
                setup.pitchBendCents[(size_t) c] = juce::jlimit (0, channels::highestBendCents, cents);

        refresh();
        announce();
    };

    const auto cell = channelGrid.boundsForChannel (channelIndex);

    // Relative to the host, because that is what the call-out is parented to.
    const auto area = host->getLocalArea (&channelGrid, cell);

    juce::CallOutBox::launchAsynchronously (std::move (content), area, host);
}

void ChannelsPage::refresh()
{
    const auto mpe = showingMpe();

    enabledStrip.setSelectedIndex ((mpe ? setup.mpeOn : setup.omniOn) ? channels::onIndex
                                                                     : channels::onIndex + 1);

    // The grid shows whichever setting is in view — the zone's span under MPE,
    // the free selection under omni. Neither is drawn through the other; see
    // the note on the class.
    // **One zone at a time under MPE**, the one being edited. Both are
    // configured and both can be active, but drawing them together loses the
    // thing the matrix is for: with every channel lit there is no telling one
    // big zone from two abutting ones, nor where the lower ends and the upper
    // begins. So the lower/upper buttons still change what is shown; what they
    // no longer do is move a single zone between the two ends.
    channelGrid.setChannels (mpe ? channels::allChannelsForZone (setup.editing,
                                                                 setup.members (setup.editing))
                                 : setup.omniChannels);

    // Which zone is being looked at, for the same reason the bend latch lights:
    // a pair of buttons where one is the view needs to say which.
    lowerZoneButton.setToggleState (setup.editing == channels::Zone::lower,
                                    juce::dontSendNotification);
    upperZoneButton.setToggleState (setup.editing == channels::Zone::upper,
                                    juce::dontSendNotification);

    // One row, two pairs. Neither pair means anything to the other setting, so
    // only the one belonging to the view in force is there at all.
    lowerZoneButton.setVisible (mpe);
    upperZoneButton.setVisible (mpe);

    selectAllButton.setVisible (! mpe);
    muteAllButton  .setVisible (! mpe);

    // While ranges are being set, a click on the grid no longer selects — so the
    // two buttons that *do* select have nothing to act on and are disabled.
    //
    // The zone buttons are the opposite case and stay live: they choose which
    // zone is in view, and in the lower zone you edit the lower zone's ranges
    // and nothing else. Switching zones is how you reach the other zone's
    // ranges, so disabling it would strand you in one of them.
    const auto settingBend = bendButton.getToggleState();

    selectAllButton.setEnabled (! settingBend);
    muteAllButton  .setEnabled (! settingBend);
}

//==============================================================================
void ChannelsPage::lookAndFeelChanged()
{
    // Guarded like every other lookAndFeelChanged here: it also fires during
    // teardown, when the owner sets its LookAndFeel to nullptr and these ids
    // stop resolving.
    if (! getLookAndFeel().isColourSpecified (pageColours::sectionTitleColourId))
        return;

    // Left to LookAndFeel_V4 a toggled TextButton is drawn *darker* than its
    // neighbours, which on a dark theme reads as the disabled one rather than
    // the chosen one — the same trap ChoiceStrip answers for its chosen button
    // and the presets page for `active`. The latch has to look latched.
    for (auto* button : { &bendButton, &lowerZoneButton, &upperZoneButton })
    {
        button->setColour (juce::TextButton::buttonOnColourId,
                           findColour (ChoiceStrip::selectedColourId));
        button->setColour (juce::TextButton::textColourOnId,
                           findColour (ChoiceStrip::selectedTextColourId));
    }

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
    //  The bend button sits to the right of both, spanning the pair, because it
    //  qualifies what a click on the grid does under either of them.
    constexpr auto switchSpan = metrics::pageColumns - 2;
    constexpr auto bendColumn = 1 + switchSpan;

    const auto settingRow = pageGrid.addRow (metrics::pageRowHeight);
    const auto enabledRow = pageGrid.addRow (metrics::pageRowHeight);

    pageGrid.place (settingStrip, settingRow, 1, switchSpan);
    pageGrid.place (enabledStrip, enabledRow, 1, switchSpan);

    pageGrid.placeSpanning (bendButton, settingRow, enabledRow, bendColumn, 2);

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
