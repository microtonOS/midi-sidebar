#include "TuningPage.h"
#include "ChannelSelector.h"

namespace microtonos::sidebar
{

using Track = juce::Grid::TrackInfo;
using juce::Grid;
using juce::GridItem;

namespace
{
    /** The scheme names, in the order docs/tuning.md lists them — which is the
        order `Scheme` declares them in, so a menu index is the enum's value and
        nothing has to be converted. */
    const juce::StringArray schemeNames { "MTS ESP", "MTS sysex", "tuning file",
                                          "MPE", "MIDI 2.0", "standard" };

    juce::String centsText (double cents)
    {
        // Two decimal places: the sketch shows "1902.98 c", and a cent is
        // already finer than anyone can hear, so a third would be noise.
        return juce::String (cents, 2) + " c";
    }

    juce::String sourceText (tuning::PeriodSource source)
    {
        // Unchanged by the end-user stepping through candidates: an inferred
        // period is still inferred whichever of them is in force.
        return source == tuning::PeriodSource::specified ? "specified" : "inferred";
    }

    void prepareNumericEditor (juce::TextEditor& editor)
    {
        editor.setMultiLine (false);
        editor.setReturnKeyStartsNewLine (false);
        editor.setJustification (juce::Justification::centredLeft);

        // A cents value can be negative and fractional; nothing else here can
        // be typed at all, so neither field can be left holding something the
        // plugin could not act on.
        editor.setInputRestrictions (0, "-0123456789.");
    }
}

//==============================================================================
TuningPage::TuningPage()
      // No title: the two choices name themselves, and the strip fills the
      // right-hand half of the two rows the load buttons occupy on the left.
    : updateStrip ({}, { "note on", "always" })
{
    updateStrip.setOrientation (ChoiceStrip::Orientation::vertical);

    // Frames first, so everything else is drawn over them. JUCE has no
    // text-transform anywhere in Font, Label or LookAndFeel, so a title is in
    // capitals only because it was typed that way.
    for (auto* group : { &statusGroup, &periodGroup, &settingsGroup })
    {
        group->setTextLabelPosition (juce::Justification::centredLeft);

        // A frame is scenery: it must not take clicks from the widgets sitting
        // over it, and there is nothing in it to click.
        group->setInterceptsMouseClicks (false, false);
        addAndMakeVisible (*group);
    }

    statusGroup  .setText ("STATUS");
    periodGroup  .setText ("PERIOD");
    settingsGroup.setText ("SETTINGS");

    const auto addLabel = [this] (juce::Label& label, const juce::String& text)
    {
        label.setText (text, juce::dontSendNotification);
        label.setJustificationType (juce::Justification::centredLeft);
        label.setInterceptsMouseClicks (false, false);
        addAndMakeVisible (label);
    };

    addLabel (modLabel,     "mod");

    // Laid out exactly like the word labels — same border, same justification —
    // and for the same reason: it shares its column with `bank` on the row
    // below, so anything else puts the "=" and the "b" in different places.
    // Centring it looked reasonable in isolation and was wrong against the rest
    // of the page.
    addLabel (equalsLabel,  "=");
    addLabel (programLabel, "program");
    addLabel (bankLabel,    "bank");
    addLabel (updatedLabel, "updated");

    for (auto* c : std::initializer_list<juce::Component*> {
             &intervalField, &modResultField, &nameField,
             &programField, &bankField, &updatedField, &periodSourceField,
             &schemeButton, &channelsButton, &scaleButton, &mapButton,
             &updateStrip, &modEditor, &periodChooser })
        addAndMakeVisible (*c);

    //  Interval ---------------------------------------------------------------
    prepareNumericEditor (modEditor);
    modEditor.setText (juce::String (interval.modDivisor, 0), juce::dontSendNotification);
    modEditor.onReturnKey  = [this] { applyModDivisor(); };
    modEditor.onFocusLost  = [this] { applyModDivisor(); };

    //  Period -----------------------------------------------------------------
    // A number box stepping through the offered periods, not a free text field:
    // inference gives a set of candidates — 12edo admits 100c, 200c, … past
    // 1200c — and every one of them is a real answer, while anything between
    // them is not. Inc/dec buttons over an index make the invalid values
    // unreachable rather than merely rejected, so the read-out is read-only.
    periodChooser.setSliderStyle (juce::Slider::IncDecButtons);
    periodChooser.setTextBoxStyle (juce::Slider::TextBoxLeft, true,
                                   metrics::periodTextBoxWidth, metrics::pageRowHeight);
    periodChooser.setIncDecButtonsMode (juce::Slider::incDecButtonsDraggable_Vertical);

    periodChooser.textFromValueFunction = [this] (double value)
    {
        const auto index = juce::roundToInt (value);
        return juce::isPositiveAndBelow (index, choices.size()) ? centsText (choices[index])
                                                                : juce::String();
    };

    periodChooser.onValueChange = [this]
    {
        const auto index = juce::roundToInt (periodChooser.getValue());

        if (! juce::isPositiveAndBelow (index, choices.size()))
            return;

        period.cents = choices[index];

        if (onPeriodChosen != nullptr)
            onPeriodChosen (choices[index]);
    };

    //  Settings ---------------------------------------------------------------
    schemeButton.setItems (schemeNames);
    schemeButton.setSelectedIndex (0);

    schemeButton.onChoice = [this] (int index)
    {
        if (onSchemeChanged != nullptr)
            onSchemeChanged (static_cast<tuning::Scheme> (index));
    };

    channelsButton.onClick = [this] { showChannelSelector(); };

    // The buttons say what they hold, and prompt when they hold nothing. A
    // separate label for the filename would need a column the panel has not
    // got at 248px.
    setScaleFileName ({});
    setMappingSummary ({});

    scaleButton.onClick = [this] { if (onScaleFileRequested    != nullptr) onScaleFileRequested(); };
    mapButton  .onClick = [this] { if (onMappingFilesRequested != nullptr) onMappingFilesRequested(); };

    updateStrip.onChoice = [this] (int index)
    {
        if (onUpdateModeChanged != nullptr)
            onUpdateModeChanged (index == 1 ? tuning::UpdateMode::always
                                            : tuning::UpdateMode::noteOn);
    };

    refreshInterval();
    refreshPeriod();
}

TuningPage::~TuningPage() = default;

//==============================================================================
void TuningPage::setInterval (const tuning::Interval& newInterval)
{
    interval = newInterval;

    modEditor.setText (juce::String (interval.modDivisor, 0), juce::dontSendNotification);
    refreshInterval();
}

void TuningPage::setStatus (const tuning::Status& newStatus)
{
    status = newStatus;

    // An empty name leaves the field showing its "Unnamed" placeholder, which
    // is dimmed — so a tuning actually called "Unnamed" still looks different
    // from one with no name at all.
    nameField.setValue (status.name);

    programField.setValue (status.program.has_value() ? juce::String (*status.program) : juce::String());
    bankField   .setValue (status.bank   .has_value() ? juce::String (*status.bank)    : juce::String());

    // Seconds included and a 24-hour clock, as the sketch shows: under MTS ESP
    // this is re-stamped several times a second, and a clock whose smallest
    // digit never moves cannot show that the connection is alive.
    updatedField.setValue (status.updated.has_value()
                               ? status.updated->toString (false, true, true, true)
                               : juce::String());
}

void TuningPage::setPeriod (const tuning::Period& newPeriod)
{
    period = newPeriod;

    // What the chooser may step through. A specified period is a single value
    // with nothing to choose between, and so is an inferred one that came with
    // no alternatives; either way the box still shows the number, it just
    // cannot be moved off it.
    choices = period.source == tuning::PeriodSource::inferred && ! period.candidates.isEmpty()
                  ? period.candidates
                  : (period.cents.has_value() ? juce::Array<double> { *period.cents }
                                              : juce::Array<double>());

    refreshPeriod();
}

void TuningPage::setScheme (tuning::Scheme scheme)
{
    schemeButton.setSelectedIndex (static_cast<int> (scheme));
}

void TuningPage::setUpdateMode (tuning::UpdateMode mode)
{
    updateStrip.setSelectedIndex (mode == tuning::UpdateMode::always ? 1 : 0);
}

void TuningPage::setChannels (bool omniOn, tuning::ChannelMask mask)
{
    omni = omniOn;
    channelMask = mask;
}

void TuningPage::setScaleFileName (const juce::String& name)
{
    scaleButton.setButtonText (name.isNotEmpty() ? name : juce::String ("load scale"));
}

void TuningPage::setMappingSummary (const juce::String& summary)
{
    // Plural: a mapping is a set of `.kbm` files, one per channel, not one file.
    mapButton.setButtonText (summary.isNotEmpty() ? summary : juce::String ("load maps"));
}

//==============================================================================
void TuningPage::refreshInterval()
{
    if (! interval.cents.has_value())
    {
        // Nothing sounding: both boxes empty, per the spec. Not "0.00 c",
        // which would claim that two notes a unison apart are being played.
        intervalField .setValue ({});
        modResultField.setValue ({});
        return;
    }

    intervalField.setValue (centsText (*interval.cents));

    // Guarded because the divisor is typed by the end-user, and 0 is a number
    // they can type. std::fmod by zero is a NaN, which would reach the screen.
    modResultField.setValue (interval.modDivisor > 0.0
                                 ? centsText (std::fmod (*interval.cents, interval.modDivisor))
                                 : juce::String());
}

void TuningPage::refreshPeriod()
{
    periodSourceField.setValue (period.cents.has_value() ? sourceText (period.source)
                                                         : juce::String());

    // The slider indexes the list, so its range is the list's. NormalisableRange
    // asserts on an empty one — start must be below end — so a list of none or
    // one is given a single step it cannot leave, and the buttons are disabled
    // to say so.
    periodChooser.setRange (0.0, juce::jmax (1, choices.size() - 1), 1.0);
    periodChooser.setEnabled (choices.size() > 1);

    const auto index = period.cents.has_value() ? choices.indexOf (*period.cents) : -1;

    // dontSendNotification: this is the owner telling the page what the period
    // is, so reporting it straight back through onPeriodChosen would be an echo
    // — and, if the owner acted on it, a loop.
    periodChooser.setValue (juce::jmax (0, index), juce::dontSendNotification);
    periodChooser.updateText();
}

void TuningPage::applyModDivisor()
{
    const auto typed = modEditor.getText().getDoubleValue();

    // exactlyEqual, not ==: this is a "did anything change" check on a value
    // that came from text, not a tolerance question, and the bare operator
    // trips -Wfloat-equal.
    if (juce::exactlyEqual (typed, interval.modDivisor))
        return;

    interval.modDivisor = typed;
    refreshInterval();

    if (onModDivisorChanged != nullptr)
        onModDivisorChanged (typed);
}


void TuningPage::showChannelSelector()
{
    auto content = std::make_unique<ChannelSelector> (omni, channelMask);
    const auto size = ChannelSelector::getPreferredSize();

    content->setSize (size.x, size.y);

    content->onChanged = [this] (bool omniOn, tuning::ChannelMask mask)
    {
        omni = omniOn;
        channelMask = mask;

        if (onChannelsChanged != nullptr)
            onChannelsChanged (omniOn, mask);
    };

    // findPopupHost, never getTopLevelComponent: in a standalone or a host the
    // top-level component is a window that knows nothing about this module's
    // LookAndFeel, so a call-out parented to it resolves none of the ColourIds
    // the widgets inside ask for — the selected channels come out drawn by
    // LookAndFeel_V4 instead of in the sidebar's accent. It looks correct under
    // the snapshot tool either way, because there the editor *is* the top-level
    // component. See PopupHost.h.
    if (auto* host = findPopupHost (*this))
        juce::CallOutBox::launchAsynchronously (std::move (content),
                                                host->getLocalArea (&channelsButton,
                                                                    channelsButton.getLocalBounds()),
                                                host);
}

//==============================================================================
void TuningPage::lookAndFeelChanged()
{
    // Guarded like every other lookAndFeelChanged in this module: it also fires
    // during teardown, when the owner sets its LookAndFeel to nullptr and these
    // ids stop resolving.
    if (! getLookAndFeel().isColourSpecified (pageColours::sectionTitleColourId))
        return;

    const auto text = findColour (pageColours::sectionTitleColourId);
    const auto font = SidebarLookAndFeel::font (metrics::bodyFontHeight);

    // The groups take the two colours a section header would have used — the
    // frame is the rule, drawn all the way round — rather than JUCE's defaults,
    // whose outline is bright enough to be read before the widgets inside it.
    // Set per instance rather than on the LookAndFeel, so a host's own
    // GroupComponents are left alone.
    for (auto* group : { &statusGroup, &periodGroup, &settingsGroup })
    {
        group->setColour (juce::GroupComponent::textColourId,    text);
        group->setColour (juce::GroupComponent::outlineColourId, findColour (pageColours::sectionOutlineColourId));
    }

    for (auto* label : { &modLabel, &equalsLabel, &programLabel, &bankLabel,
                         &updatedLabel })
    {
        label->setFont (font);
        label->setColour (juce::Label::textColourId, text);
    }

    // TextEditor bakes the text colour into each run of text as it is inserted,
    // and TextEditor::lookAndFeelChanged only rebuilds the caret — so text that
    // was set before this page reached a styled parent keeps the *default*
    // LookAndFeel's colour for ever. That is white, which is invisible against
    // the Light scheme's white field: the value is there, correct, and cannot
    // be read. Re-applying it here is the documented fix (see the note on
    // TextEditor::textColourId), and `true` makes later insertions use it too.
    modEditor.setFont (font);
    modEditor.applyColourToAllText (findColour (juce::TextEditor::textColourId), true);
}

//==============================================================================
void TuningPage::resized()
{
    // ONE grid for the whole page, on the six equal columns the sketch in
    // docs/tuning.md is drawn on — see metrics::pageColumns. Every widget spans
    // a whole number of them, so a row split in half really is halved, and
    // things in different rows that begin at the same column line up because
    // the layout holds them there.
    //
    // A grid per row cannot do this. It aligns only within its own row, and
    // every alignment down the page then rests on fixed widths adding up to the
    // same number by accident — which is how this page was first written, and
    // the halves came out ragged.
    Grid grid;

    grid.columnGap = Grid::Px (metrics::pageColumnGap);
    grid.rowGap    = Grid::Px (metrics::pageRowGap);

    // A gutter, the six content columns, a gutter. The group frames span all
    // eight; everything else spans only the six between them, which is what
    // insets a group's contents from its own outline without taking them out of
    // this grid.
    grid.templateColumns.add (Track (Grid::Px (metrics::pageGroupPadding)));

    for (int i = 0; i < metrics::pageColumns; ++i)
        grid.templateColumns.add (Track (Grid::Fr (1)));

    grid.templateColumns.add (Track (Grid::Px (metrics::pageGroupPadding)));

    // Rows are numbered as they are declared rather than written out, so
    // inserting one does not renumber everything below it. Grid lines are
    // 1-based and a row's end line is the next row's start.
    int nextRow = 1;

    const auto addRow = [&grid, &nextRow] (int height)
    {
        grid.templateRows.add (Track (Grid::Px (height)));
        return nextRow++;
    };

    /** @param firstColumn  1-based within the six *content* columns
        @param columnSpan   how many of them to cover

        The +1s step over the leading gutter, so callers count content columns
        and never have to know the gutters are there. */
    const auto place = [&grid] (juce::Component& component, int row, int firstColumn, int columnSpan)
    {
        grid.items.add (GridItem (component).withArea (row, firstColumn + 1,
                                                       row + 1, firstColumn + columnSpan + 1));
    };

    /** Frames a section, from its title band down to the padding row below its
        last row, and out to both gutters. */
    const auto frame = [&grid] (juce::GroupComponent& group, int titleRow, int paddingRow)
    {
        grid.items.add (GridItem (group).withArea (titleRow, 1,
                                                   paddingRow + 1,
                                                   metrics::pageColumnsWithGutters + 1));
    };

    // Named spans, so a row says what it means: "the left half", "a label".
    constexpr auto full = metrics::pageColumns;
    constexpr auto half = metrics::pageColumns / 2;
    constexpr auto pair = metrics::pageLabelColumns;

    // The line the right-hand half begins on. Grid lines are 1-based, so the
    // left half covers lines 1..rightHalf and everything that has to start at
    // the middle of the page starts here.
    constexpr auto rightHalf = 1 + half;

    //  Interval ---------------------------------------------------------------
    place (intervalField, addRow (metrics::pageRowHeight), 1, full);

    {
        // mod | divisor | = | remainder, which puts the "=" on the half-way
        // line and the remainder in the right-hand third.
        const auto row = addRow (metrics::pageRowHeight);

        place (modLabel,       row, 1, 1);
        place (modEditor,      row, 2, half - 1);
        place (equalsLabel,    row, rightHalf, 1);
        place (modResultField, row, rightHalf + 1, half - 1);
    }

    //  Status -----------------------------------------------------------------
    const auto statusTitle = addRow (metrics::pageGroupTitleHeight);

    place (nameField, addRow (metrics::pageRowHeight), 1, full);

    {
        // Program takes the left half, bank the right, each a label and a
        // narrow field.
        const auto row = addRow (metrics::pageRowHeight);

        place (programLabel, row, 1, pair);
        place (programField, row, 1 + pair, half - pair);
        place (bankLabel,    row, rightHalf, pair);
        place (bankField,    row, rightHalf + pair, half - pair);
    }

    {
        const auto row = addRow (metrics::pageRowHeight);

        place (updatedLabel, row, 1, pair);
        place (updatedField, row, 1 + pair, full - pair);
    }

    frame (statusGroup, statusTitle, addRow (metrics::pageGroupPadding));

    //  Period -----------------------------------------------------------------
    const auto periodTitle = addRow (metrics::pageGroupTitleHeight);

    {
        const auto row = addRow (metrics::pageRowHeight);

        place (periodChooser,     row, 1, half);
        place (periodSourceField, row, rightHalf, half);
    }

    frame (periodGroup, periodTitle, addRow (metrics::pageGroupPadding));

    //  Settings ---------------------------------------------------------------
    const auto settingsTitle = addRow (metrics::pageGroupTitleHeight);

    {
        const auto row = addRow (metrics::pageRowHeight);

        place (schemeButton,   row, 1, half);
        place (channelsButton, row, rightHalf, half);
    }

    {
        // Two rows, two halves: the load buttons stacked on the left, the two
        // update choices stacked on the right. The buttons say what they load,
        // so neither column needs a label — and the strip spans both rows with
        // the same gap between its choices as there is between the rows, which
        // is what lands "note on" beside the scale button and "always" beside
        // the map button.
        const auto firstRow  = addRow (metrics::pageRowHeight);
        const auto secondRow = addRow (metrics::pageRowHeight);

        place (scaleButton, firstRow,  1, half);
        place (mapButton,   secondRow, 1, half);

        grid.items.add (GridItem (updateStrip).withArea (firstRow, rightHalf + 1,
                                                         secondRow + 1, rightHalf + half + 1));
    }

    frame (settingsGroup, settingsTitle, addRow (metrics::pageGroupPadding));

    grid.performLayout (getLocalBounds());
}

} // namespace microtonos::sidebar
