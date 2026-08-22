#include "TuningPage.h"

namespace microtonos::sidebar
{

namespace
{
    /** The scheme names, in the order docs/tuning.md lists them — which is the
        order `Scheme` declares them in, so a menu index is the enum's value and
        nothing has to be converted. */
    const juce::StringArray schemeNames { "MTS-ESP", "MIDI 1.0", "MIDI 2.0",
                                          "Scala", "standard" };

    juce::String centsText (double cents)
    {
        // Two decimal places: the sketch shows "1902.98 c", and a cent is
        // already finer than anyone can hear, so a third would be noise.
        return juce::String (cents, 2) + " c";
    }

    /** Which entry of the update strip is which. `always` is first because it
        is drawn at the top of a vertical strip and reading order is downwards —
        the enum's own order is the other way round, so the two are converted
        rather than assumed to agree. */
    constexpr int alwaysIndex = 0;

    juce::String sourceText (tuning::PeriodSource source)
    {
        // Unchanged by the end-user stepping through candidates: an inferred
        // period is still inferred whichever of them is in force.
        return source == tuning::PeriodSource::specified ? "specified" : "inferred";
    }

}

//==============================================================================
TuningPage::TuningPage()
      // No title: the two choices name themselves. `always` first, so that in
      // the vertical strip it sits at the top.
    : updateStrip ({}, { "always", "note on" })
{
    // Vertical, spanning the two rows the scheme and load buttons occupy on the
    // left — so the strip is as tall as the pair beside it and the section
    // reads as two columns rather than as three unrelated rows.
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

    // JUCE's own labelling, for the two that sit above their control:
    // attachToComponent positions the label directly over its owner and follows
    // it, so these two are NOT placed in the page's grid — the grid places the
    // steppers and reserves the row above, and the labels find it themselves.
    programLabel.attachToComponent (&programStepper, false);
    bankLabel   .attachToComponent (&bankStepper,    false);

    // The pitch-bend pair the same way: two columns of one editor each, named
    // above rather than beside, because half a page is not wide enough for a
    // label and a value side by side once the label says "MPE member".

    for (auto* c : std::initializer_list<juce::Component*> {
             &intervalField, &modResultField, &nameButton,
             &programStepper, &bankStepper, &updatedField, &periodSourceField,
             &schemeButton, &openButton,
             &updateStrip, &modEditor, &periodChooser,
           })
        addAndMakeVisible (*c);

    //  Interval ---------------------------------------------------------------
    prepareNumericEditor (modEditor);
    modEditor.setText (juce::String (interval.modDivisor, 0), juce::dontSendNotification);
    modEditor.onReturnKey  = [this] { applyModDivisor(); };
    modEditor.onFocusLost  = [this] { applyModDivisor(); };

    //  Pitch bend -------------------------------------------------------------

    // No sign and no fraction, unlike the modulo divisor: a bend range runs one
    // way and a fraction of a cent is past what RPN 0 can address.



    // Stated rather than inherited. A ChoiceStrip selects its first entry on
    // construction, and reordering the two put `always` there — which would
    // have shown a mode the enum does not default to, and quietly, since
    // nothing else in the page disagrees with it.
    setUpdateMode (tuning::UpdateMode::noteOn);

    //  Period -----------------------------------------------------------------
    // A number box stepping through the offered periods, not a free text field:
    // inference gives a set of candidates — 12edo admits 100c, 200c, … past
    // 1200c — and every one of them is a real answer, while anything between
    // them is not. Inc/dec buttons over an index make the invalid values
    // unreachable rather than merely rejected, so the read-out is read-only.
    periodChooser.setSliderStyle (juce::Slider::IncDecButtons);
    periodChooser.setTextBoxStyle (juce::Slider::TextBoxLeft, true,
                                   metrics::incDecTextBoxWidth, metrics::pageRowHeight);
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

    //  Status -----------------------------------------------------------------
    nameButton.onChoice = [this] (int index)
    {
        if (onNameChosen != nullptr)
            onNameChosen (index);
    };

    programStepper.onNumberChosen = [this] (std::optional<int> program)
    {
        if (onProgramChosen != nullptr)
            onProgramChosen (program);
    };

    bankStepper.onNumberChosen = [this] (std::optional<int> bank)
    {
        if (onBankChosen != nullptr)
            onBankChosen (bank);
    };

    //  Settings ---------------------------------------------------------------
    schemeButton.setItems (schemeNames);
    schemeButton.setSelectedIndex (0);

    schemeButton.onChoice = [this] (int index)
    {
        if (onSchemeChanged != nullptr)
            onSchemeChanged (static_cast<tuning::Scheme> (index));
    };

    openButton.onClick = [this] { if (onFilesRequested != nullptr) onFilesRequested(); };

    // The button says what it holds, and prompts when it holds nothing. A
    // separate label for the filename would need a column the panel has not
    // got at 248px.
    setLoadedSummary ({});

    updateStrip.onChoice = [this] (int index)
    {
        if (onUpdateModeChanged != nullptr)
            onUpdateModeChanged (index == alwaysIndex ? tuning::UpdateMode::always
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

void TuningPage::setAvailableNames (juce::StringArray names)
{
    availableNames = std::move (names);
    refreshNames();
}

void TuningPage::setStatus (const tuning::Status& newStatus)
{
    status = newStatus;

    refreshNames();

    programStepper.setNumber (status.program);
    bankStepper   .setNumber (status.bank);

    // Seconds included and a 24-hour clock, as the sketch shows: under MTS-ESP
    // this is re-stamped several times a second, and a clock whose smallest
    // digit never moves cannot show that the connection is alive.
    updatedField.setValue (status.updated.has_value()
                               ? status.updated->toString (false, true, true, true)
                               : juce::String());
}

void TuningPage::refreshNames()
{
    // The menu offers whatever it was given, plus the current tuning if that is
    // not among them — otherwise the button would show nothing at all before
    // anything has supplied a list, which is most of the time so far. An unnamed
    // tuning falls back to the word the spec asks for.
    auto items = availableNames;
    const auto current = status.name.isNotEmpty() ? status.name : juce::String ("Unnamed");

    if (! items.contains (current))
        items.insert (0, current);

    nameButton.setItems (items);
    nameButton.setSelectedIndex (items.indexOf (current));
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
    updateStrip.setSelectedIndex (mode == tuning::UpdateMode::always ? alwaysIndex
                                                                     : alwaysIndex + 1);
}

void TuningPage::setLoadedSummary (const juce::String& summary)
{
    // One line for however many files were chosen: the owner decides how to say
    // "a scale and four mappings" in the width of half a page.
    openButton.setButtonText (summary.isNotEmpty() ? summary : juce::String ("open files"));
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
    for (auto* editor : { &modEditor })
    {
        editor->setFont (font);
        editor->applyColourToAllText (findColour (juce::TextEditor::textColourId), true);
    }
}

//==============================================================================
void TuningPage::resized()
{
    // One PageGrid for the whole page, on the six columns the sketch in
    // docs/tuning.md is drawn on. See PageGrid for why it is one grid and not
    // one per row, and why `place` counts content columns.
    PageGrid grid;

    // Named spans, so a row says what it means: "the left half", "a label".
    constexpr auto full = metrics::pageColumns;
    constexpr auto half = metrics::pageColumns / 2;
    constexpr auto pair = metrics::pageLabelColumns;

    // The line the right-hand half begins on. Grid lines are 1-based, so the
    // left half covers lines 1..rightHalf and everything that has to start at
    // the middle of the page starts here.
    constexpr auto rightHalf = 1 + half;

    //  Interval ---------------------------------------------------------------
    grid.place (intervalField, grid.addRow (metrics::pageRowHeight), 1, full);

    {
        // mod | divisor | = | remainder, which puts the "=" on the half-way
        // line and the remainder in the right-hand third.
        const auto row = grid.addRow (metrics::pageRowHeight);

        grid.place (modLabel,       row, 1, 1);
        grid.place (modEditor,      row, 2, half - 1);
        grid.place (equalsLabel,    row, rightHalf, 1);
        grid.place (modResultField, row, rightHalf + 1, half - 1);
    }

    //  Status -----------------------------------------------------------------
    const auto statusTitle = grid.addRow (metrics::pageGroupTitleHeight);

    grid.place (nameButton, grid.addRow (metrics::pageRowHeight), 1, full);

    {
        // Labels above rather than beside: a stepper is a read-out plus two
        // buttons, and with a label next to it none of the three has room in
        // half a page. Stacked, each stepper gets the whole half.
        const auto labelRow   = grid.addRow (metrics::pageRowHeight);
        const auto stepperRow = grid.addRow (metrics::pageRowHeight);

        juce::ignoreUnused (labelRow);   // the attached labels place themselves

        grid.place (programStepper, stepperRow, 1, half);
        grid.place (bankStepper,    stepperRow, rightHalf, half);
    }

    {
        const auto row = grid.addRow (metrics::pageRowHeight);

        grid.place (updatedLabel, row, 1, pair);
        grid.place (updatedField, row, 1 + pair, full - pair);
    }

    grid.frame (statusGroup, statusTitle, grid.addRow (metrics::pageGroupPadding));

    //  Period -----------------------------------------------------------------
    const auto periodTitle = grid.addRow (metrics::pageGroupTitleHeight);

    {
        const auto row = grid.addRow (metrics::pageRowHeight);

        grid.place (periodChooser,     row, 1, half);
        grid.place (periodSourceField, row, rightHalf, half);
    }

    grid.frame (periodGroup, periodTitle, grid.addRow (metrics::pageGroupPadding));

    //  Settings ---------------------------------------------------------------
    const auto settingsTitle = grid.addRow (metrics::pageGroupTitleHeight);

    {
        // Two rows, two columns: the scheme and the load button stacked on the
        // left, the update choices stacked beside them. The strip spans both
        // rows with the same gap between its choices as there is between the
        // rows, which is what lands `always` beside the scheme and `note on`
        // beside the load button.
        const auto firstRow  = grid.addRow (metrics::pageRowHeight);
        const auto secondRow = grid.addRow (metrics::pageRowHeight);

        grid.place (schemeButton, firstRow,  1, half);
        grid.place (openButton,   secondRow, 1, half);

        grid.placeSpanning (updateStrip, firstRow, secondRow, rightHalf, half);
    }

    grid.frame (settingsGroup, settingsTitle, grid.addRow (metrics::pageGroupPadding));

    // The page has no flexible track, so it needs its natural height or it is
    // clipped; see docs/tuning.md on small heights.
    grid.performLayout (getLocalBounds(), getNaturalHeight());
}

} // namespace microtonos::sidebar
