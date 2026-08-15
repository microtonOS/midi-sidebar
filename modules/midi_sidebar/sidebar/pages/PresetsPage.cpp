#include "PresetsPage.h"

namespace microtonos::sidebar
{

using namespace presets;

namespace
{
    /** Two decimals, as the sketch shows: enough to tell two tunings apart at
        the bottom of the range, where a cent is a fraction of a hertz. */
    juce::String hertzText (const std::optional<double>& frequency)
    {
        return frequency.has_value() ? juce::String (*frequency, 2) + " Hz" : juce::String();
    }

}

//==============================================================================
PresetsPage::PresetsPage()
      // No title on the strip: "lower" and "upper" name themselves, and it
      // fills the row beside the split button.
    : layerStrip ({}, layerNames)
{
    // Frames first, so everything else draws over them.
    for (auto* group : { &statusGroup, &filesGroup, &metaGroup })
    {
        group->setTextLabelPosition (juce::Justification::centredLeft);
        group->setInterceptsMouseClicks (false, false);
        addAndMakeVisible (*group);
    }

    statusGroup.setText ("STATUS");
    filesGroup .setText ("FILE");
    metaGroup  .setText ("META");

    const auto addLabel = [this] (juce::Label& label, const juce::String& text)
    {
        label.setText (text, juce::dontSendNotification);
        label.setJustificationType (juce::Justification::centredLeft);
        label.setInterceptsMouseClicks (false, false);
        addAndMakeVisible (label);
    };

    addLabel (programLabel, "program");
    addLabel (bankLabel,    "bank");

    // See the same two lines on the tuning page: JUCE positions these above
    // their steppers, so the grid reserves their row but does not place them.
    programLabel.attachToComponent (&programStepper, false);
    bankLabel   .attachToComponent (&bankStepper,    false);
    addLabel (authorLabel,  "author");
    addLabel (commentLabel, "comment");

    // Its row grows with the panel, so a vertically centred label drifts into
    // the middle of a tall box and stops reading as that box's name. Anchored
    // to the top, it stays beside the first line of the comment.
    commentLabel.setJustificationType (juce::Justification::topLeft);

    for (auto* c : std::initializer_list<juce::Component*> {
             &lowField, &highField, &splitButton, &activeButton, &layerStrip,
             &nameButton, &programStepper, &bankStepper,
             &openButton, &saveButton,
             &authorEditor, &commentEditor })
        addAndMakeVisible (*c);

    // Momentary: pressing it sets the split point. What it means depends on what
    // it is showing, which is why the label goes out with the call rather than
    // the owner having to remember what it pushed in.
    splitButton.onClick = [this]
    {
        if (onSplitPointRequested != nullptr)
            onSplitPointRequested (notesActive);
    };

    // Latching: this button *is* the split's state, not a command to toggle it.
    activeButton.setClickingTogglesState (true);
    activeButton.onClick = [this]
    {
        if (onSplitToggled != nullptr)
            onSplitToggled (activeButton.getToggleState());
    };

    layerStrip.onChoice = [this] (int index)
    {
        if (onLayerChanged != nullptr)
            onLayerChanged (index == 1 ? Layer::upper : Layer::lower);
    };

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

    openButton.onClick = [this] { if (onOpenRequested != nullptr) onOpenRequested(); };
    saveButton.onClick = [this] { if (onSaveRequested != nullptr) onSaveRequested(); };

    authorEditor.setMultiLine (false);
    authorEditor.setReturnKeyStartsNewLine (false);

    // The one place on any page with room for prose: usage notes, a licence.
    // Return puts in a line rather than committing, so the commit happens when
    // focus leaves.
    commentEditor.setMultiLine (true, true);
    commentEditor.setReturnKeyStartsNewLine (true);

    authorEditor .onFocusLost = [this] { commitMeta(); };
    commentEditor.onFocusLost = [this] { commitMeta(); };
    authorEditor .onReturnKey = [this] { commitMeta(); };
}

PresetsPage::~PresetsPage() = default;

//==============================================================================
void PresetsPage::setFrequencies (presets::Frequencies frequencies)
{
    lowField .setValue (hertzText (frequencies.low));
    highField.setValue (hertzText (frequencies.high));
}

void PresetsPage::setAvailableNames (juce::StringArray names)
{
    availableNames = std::move (names);
    refreshNames();
}

void PresetsPage::setStatus (presets::Status newStatus)
{
    status = std::move (newStatus);

    refreshNames();

    programStepper.setNumber (status.program);
    bankStepper   .setNumber (status.bank);
}

void PresetsPage::refreshNames()
{
    // The menu offers what it was given, plus the current preset if that is not
    // among them — otherwise the button would show nothing before anything has
    // supplied a list.
    auto items = availableNames;

    if (status.name.isNotEmpty() && ! items.contains (status.name))
        items.insert (0, status.name);

    nameButton.setItems (items);
    nameButton.setSelectedIndex (items.indexOf (status.name));
}

void PresetsPage::setMeta (presets::Meta meta)
{
    authorEditor .setText (meta.author,  juce::dontSendNotification);
    commentEditor.setText (meta.comment, juce::dontSendNotification);
}

void PresetsPage::setSplitActive (bool isActive)
{
    activeButton.setToggleState (isActive, juce::dontSendNotification);
}

void PresetsPage::setNotesActive (bool anyNotesActive)
{
    if (anyNotesActive == notesActive)
        return;

    notesActive = anyNotesActive;

    // The question mark is the point: with notes held, pressing this would take
    // the split from them rather than from the boxes, and the label is the only
    // warning that the same button now does a different thing.
    splitButton.setButtonText (notesActive ? "update?" : "split");
}

void PresetsPage::setLayer (presets::Layer layer)
{
    layerStrip.setSelectedIndex (layer == Layer::upper ? 1 : 0);
}

void PresetsPage::commitMeta()
{
    if (onMetaEdited != nullptr)
        onMetaEdited ({ authorEditor.getText(), commentEditor.getText() });
}

//==============================================================================
void PresetsPage::lookAndFeelChanged()
{
    if (! getLookAndFeel().isColourSpecified (pageColours::sectionTitleColourId))
        return;

    const auto text = findColour (pageColours::sectionTitleColourId);
    const auto font = SidebarLookAndFeel::font (metrics::bodyFontHeight);

    for (auto* group : { &statusGroup, &filesGroup, &metaGroup })
    {
        group->setColour (juce::GroupComponent::textColourId,    text);
        group->setColour (juce::GroupComponent::outlineColourId, findColour (pageColours::sectionOutlineColourId));
    }

    // "Split is on" has to read the same as every other on-state in the
    // sidebar. Left to LookAndFeel_V4 a toggled TextButton is drawn *darker*
    // than its neighbours, which on a dark theme looks like the disabled one —
    // the same trap ChoiceStrip answers for its chosen button.
    // Only the latching one has an on-state to colour. `split` is momentary and
    // stays an ordinary button.
    activeButton.setColour (juce::TextButton::buttonOnColourId, findColour (ChoiceStrip::selectedColourId));
    activeButton.setColour (juce::TextButton::textColourOnId,   findColour (ChoiceStrip::selectedTextColourId));

    for (auto* label : { &programLabel, &bankLabel, &authorLabel, &commentLabel })
    {
        label->setFont (font);
        label->setColour (juce::Label::textColourId, text);
    }

    // TextEditor bakes the colour into each run as it is inserted and
    // lookAndFeelChanged only rebuilds the caret, so text set before this page
    // reached a styled parent would keep the default LookAndFeel's white — see
    // the juce-ui skill's note on TextEditor.
    for (auto* editor : { &authorEditor, &commentEditor })
    {
        editor->setFont (font);
        editor->applyColourToAllText (findColour (juce::TextEditor::textColourId), true);
    }
}

//==============================================================================
void PresetsPage::resized()
{
    PageGrid grid;

    constexpr auto full = metrics::pageColumns;
    constexpr auto half = metrics::pageColumns / 2;
    constexpr auto third = metrics::pageThirdColumns;
    constexpr auto pair = metrics::pageLabelColumns;
    constexpr auto rightHalf = 1 + half;

    //  The split ---------------------------------------------------------------
    //  Two rows of thirds, and the first column of each is a button: `split`
    //  over the two frequencies that are the split point, `on` over the pair
    //  that says which side of it is live. The sketch draws both rows as three
    //  equal cells, so the frequencies are thirds rather than halves and line up
    //  under the buttons beside them.
    {
        const auto row = grid.addRow (metrics::pageRowHeight);

        grid.place (splitButton, row, 1, third);
        grid.place (lowField,    row, 1 + third, third);
        grid.place (highField,   row, 1 + third * 2, third);
    }

    {
        // `lower`/`upper` is one strip spanning the last two thirds rather than
        // two buttons, so it reads as the single either-or it is — the same
        // control the tuning page's update mode uses.
        const auto row = grid.addRow (metrics::pageRowHeight);

        grid.place (activeButton,   row, 1, third);
        grid.place (layerStrip, row, 1 + third, full - third);
    }

    //  Status -----------------------------------------------------------------
    const auto statusTitle = grid.addRow (metrics::pageGroupTitleHeight);

    grid.place (nameButton, grid.addRow (metrics::pageRowHeight), 1, full);

    {
        // The tuning page's status block, row for row, so the two pages agree
        // about where a program number lives and what it looks like.
        const auto labelRow   = grid.addRow (metrics::pageRowHeight);
        const auto stepperRow = grid.addRow (metrics::pageRowHeight);

        juce::ignoreUnused (labelRow);   // the attached labels place themselves

        grid.place (programStepper, stepperRow, 1, half);
        grid.place (bankStepper,    stepperRow, rightHalf, half);
    }

    grid.frame (statusGroup, statusTitle, grid.addRow (metrics::pageGroupPadding));

    //  Files ------------------------------------------------------------------
    const auto filesTitle = grid.addRow (metrics::pageGroupTitleHeight);

    {
        const auto row = grid.addRow (metrics::pageRowHeight);

        grid.place (openButton, row, 1, half);
        grid.place (saveButton, row, rightHalf, half);
    }

    grid.frame (filesGroup, filesTitle, grid.addRow (metrics::pageGroupPadding));

    //  Meta -------------------------------------------------------------------
    const auto metaTitle = grid.addRow (metrics::pageGroupTitleHeight);

    {
        const auto row = grid.addRow (metrics::pageRowHeight);

        grid.place (authorLabel,  row, 1, pair);
        grid.place (authorEditor, row, 1 + pair, full - pair);
    }

    {
        // The page's one flexible row. The label stays at the top of it rather
        // than floating in the middle of a tall box, which is why it is placed
        // as its own item in the same row rather than beside a centred field.
        const auto row = grid.addFlexibleRow();

        grid.place (commentLabel,  row, 1, pair);
        grid.place (commentEditor, row, 1 + pair, full - pair);
    }

    grid.frame (metaGroup, metaTitle, grid.addRow (metrics::pageGroupPadding));

    grid.performLayout (getLocalBounds(), getMinimumHeight());
}

} // namespace microtonos::sidebar
