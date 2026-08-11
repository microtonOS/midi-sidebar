#include "ControllersPage.h"

namespace microtonos::sidebar
{

using namespace controllers;

//==============================================================================
namespace
{
    /** Which entry of the MPE strip is which. On is first, as the sketch draws
        it, even though off is the default. */
    constexpr int mpeOnIndex  = 0;
    constexpr int mpeOffIndex = 1;
}

ControllersPage::ControllersPage()
    // No title: the frame around the section is already called MPE, and a
    // strip titled "on / off" inside a group titled "MPE" says it twice.
    : mpeStrip ({}, { "on", "off" })
{
    // Frames first, so everything else draws over them — the arrangement the
    // tuning page uses and for the same reason.
    for (auto* group : { &filesGroup, &mpeGroup, &editingGroup })
    {
        group->setTextLabelPosition (juce::Justification::centredLeft);
        group->setInterceptsMouseClicks (false, false);
        addAndMakeVisible (*group);
    }

    filesGroup  .setText ("FILES");
    mpeGroup    .setText ("MPE");
    editingGroup.setText ("EDITING");

    addAndMakeVisible (monitor);

    for (auto* b : { &loadButton, &saveButton, &addButton, &removeButton,
                     &aftertouchButton, &polytouchButton })
        addAndMakeVisible (*b);

    addAndMakeVisible (table);

    const auto addLabel = [this] (juce::Label& label, const juce::String& text)
    {
        label.setText (text, juce::dontSendNotification);
        label.setJustificationType (juce::Justification::centredLeft);
        label.setInterceptsMouseClicks (false, false);
        addAndMakeVisible (label);
    };

    //  Pitch bend -------------------------------------------------------------
    addLabel (pitchBendLabel, "PB sensitivity");
    addAndMakeVisible (pitchBendEditor);

    pitchBendEditor.setMultiLine (false);
    pitchBendEditor.setReturnKeyStartsNewLine (false);
    pitchBendEditor.setJustification (juce::Justification::centredLeft);

    // No sign and no fraction: a negative bend range is not a thing RPN 0 can
    // carry, and a fraction of a cent is past what any of it can address.
    pitchBendEditor.setInputRestrictions (0, "0123456789");

    pitchBendEditor.onReturnKey = [this] { applyPitchBendCents(); };
    pitchBendEditor.onFocusLost = [this] { applyPitchBendCents(); };

    setPitchBendCents (pitchBendCents);

    //  The MPE zone -----------------------------------------------------------
    // A strip, not two toggles: on and off are one either-or setting, and the
    // connected pair is what the rest of the sidebar uses to say so.
    mpeStrip.onChoice = [this] (int index)
    {
        mpe.on = index == mpeOnIndex;
        mpeChanged();
    };

    addAndMakeVisible (mpeStrip);

    addLabel (fromLabel, "ch");
    addLabel (toLabel,   "to");

    // The master is 1 or 16 and nothing between: a zone is anchored at one end
    // of the sixteen channels or the other, and offering the fourteen in the
    // middle would offer states MPE does not have.
    masterButton.setItems ({ juce::String (controllers::firstChannel),
                             juce::String (controllers::lastChannel) });

    juce::StringArray channelNames;

    for (int c = controllers::firstChannel; c <= controllers::lastChannel; ++c)
        channelNames.add (juce::String (c));

    lastButton.setItems (channelNames);

    masterButton.onChoice = [this] (int index)
    {
        mpe.master = index == 0 ? controllers::firstChannel : controllers::lastChannel;
        mpeChanged();
    };

    lastButton.onChoice = [this] (int index)
    {
        mpe.last = index + controllers::firstChannel;
        mpeChanged();
    };

    for (auto* c : { &masterButton, &lastButton })
        addAndMakeVisible (*c);

    setMpe (mpe);

    //  Buttons ----------------------------------------------------------------
    loadButton.onClick = [this] { if (onLoadRequested != nullptr) onLoadRequested(); };
    saveButton.onClick = [this] { if (onSaveRequested != nullptr) onSaveRequested(); };

    addButton   .onClick = [this] { table.addMapping(); };
    removeButton.onClick = [this] { table.removeSelectedMapping(); };

    // Each adds a row of its own kind. The mapping is otherwise blank, exactly
    // as `add` leaves one, so the only thing these decide is what the row's two
    // controller-number cells say instead of holding a number.
    aftertouchButton.onClick = [this] { table.addMapping (controllers::Source::aftertouch); };
    polytouchButton .onClick = [this] { table.addMapping (controllers::Source::polytouch); };

    table.onMappingsChanged = [this] { if (onMappingsChanged != nullptr) onMappingsChanged(); };
}

ControllersPage::~ControllersPage() = default;

//==============================================================================
void ControllersPage::setParameters (juce::Array<controllers::Parameter> parameters)
{
    table.setParameters (std::move (parameters));
}

void ControllersPage::setPitchBendCents (int cents)
{
    pitchBendCents = juce::jlimit (0, controllers::highestPitchBendCents, cents);

    // Shown with its unit, as the sketch draws it, and typed without one — the
    // same arrangement the table's limits use. The restriction on the editor is
    // what keeps the two from disagreeing: a "c" cannot be typed back in.
    pitchBendEditor.setText (juce::String (pitchBendCents) + " c", juce::dontSendNotification);
}

void ControllersPage::applyPitchBendCents()
{
    const auto typed = juce::jlimit (0, controllers::highestPitchBendCents,
                                     pitchBendEditor.getText().getIntValue());

    // Written back either way: an out-of-range or empty entry has to be
    // corrected on screen, or the box goes on showing something the plugin does
    // not have. Only a real change is reported.
    const auto changed = typed != pitchBendCents;

    setPitchBendCents (typed);

    if (changed && onPitchBendCentsChosen != nullptr)
        onPitchBendCentsChosen (pitchBendCents);
}

void ControllersPage::setMpe (controllers::Mpe newMpe)
{
    mpe = newMpe;

    mpeStrip.setSelectedIndex (mpe.on ? mpeOnIndex : mpeOffIndex);
    masterButton.setSelectedIndex (mpe.master == controllers::lastChannel ? 1 : 0);
    lastButton  .setSelectedIndex (juce::jlimit (controllers::firstChannel, controllers::lastChannel, mpe.last)
                                       - controllers::firstChannel);

    // The channels are still shown while the zone is off — they are what comes
    // back when it is switched on, the same reasoning the multichannel call-out
    // uses — but nothing can be chosen that would not take effect.
    for (auto* c : { &masterButton, &lastButton })
        c->setEnabled (mpe.on);

    fromLabel.setEnabled (mpe.on);
    toLabel  .setEnabled (mpe.on);
}

void ControllersPage::mpeChanged()
{
    setMpe (mpe);

    if (onMpeChanged != nullptr)
        onMpeChanged (mpe);
}

void ControllersPage::setMappings (juce::Array<controllers::Mapping> mappings)
{
    table.setMappings (std::move (mappings));
}

const juce::Array<controllers::Parameter>& ControllersPage::getParameters() const noexcept
{
    return table.getParameters();
}

const juce::Array<controllers::Mapping>& ControllersPage::getMappings() const noexcept
{
    return table.getMappings();
}

void ControllersPage::showMappingsFor (int parameterIndex)
{
    table.selectMappingsFor (parameterIndex);
}

void ControllersPage::removeLatestMappingFor (int parameterIndex)
{
    table.removeLatestMappingFor (parameterIndex);
}

void ControllersPage::setMessage (const juce::String& message)
{
    monitor.setValue (message);
}

//==============================================================================
void ControllersPage::lookAndFeelChanged()
{
    if (! getLookAndFeel().isColourSpecified (pageColours::sectionTitleColourId))
        return;

    const auto text = findColour (pageColours::sectionTitleColourId);
    const auto font = SidebarLookAndFeel::font (metrics::bodyFontHeight);

    for (auto* group : { &filesGroup, &mpeGroup, &editingGroup })
    {
        group->setColour (juce::GroupComponent::textColourId,    text);
        group->setColour (juce::GroupComponent::outlineColourId, findColour (pageColours::sectionOutlineColourId));
    }

    for (auto* label : { &pitchBendLabel, &fromLabel, &toLabel })
    {
        label->setFont (font);
        label->setColour (juce::Label::textColourId, text);
    }

    // A TextEditor bakes the colour into each run as it is inserted, so text set
    // before this page reached a styled parent keeps the default LookAndFeel's
    // white for ever — invisible on the Light scheme. See the same note on the
    // tuning page's modulo editor.
    pitchBendEditor.setFont (font);
    pitchBendEditor.applyColourToAllText (findColour (juce::TextEditor::textColourId), true);
}

//==============================================================================
void ControllersPage::resized()
{
    // The same six columns as the other pages, so all three read as one plugin;
    // see PageGrid.
    PageGrid grid;

    constexpr auto full = metrics::pageColumns;
    constexpr auto half = metrics::pageColumns / 2;
    constexpr auto rightHalf = 1 + half;

    //  Monitor and pitch bend, unframed at the top like the tuning page's
    //  interval block: the sketch gives neither a title.
    grid.place (monitor, grid.addRow (metrics::pageRowHeight), 1, full);

    {
        const auto row = grid.addRow (metrics::pageRowHeight);

        grid.place (pitchBendLabel,  row, 1, half);
        grid.place (pitchBendEditor, row, rightHalf, half);
    }

    //  MPE --------------------------------------------------------------------
    const auto mpeTitle = grid.addRow (metrics::pageGroupTitleHeight);

    {
        // Six single columns, which is what the sketch draws: the on/off pair
        // takes two of them, and each label sits in its own beside the menu it
        // introduces. Each position is derived from the one before it, so the
        // row cannot be left half-renumbered.
        constexpr auto one          = 1;
        constexpr auto switchSpan   = 2;
        constexpr auto fromColumn   = 1 + switchSpan;
        constexpr auto masterColumn = fromColumn + one;
        constexpr auto toColumn     = masterColumn + one;
        constexpr auto lastColumn   = toColumn + one;

        const auto row = grid.addRow (metrics::pageRowHeight);

        grid.place (mpeStrip,     row, 1, switchSpan);
        grid.place (fromLabel,    row, fromColumn,   one);
        grid.place (masterButton, row, masterColumn, one);
        grid.place (toLabel,      row, toColumn,     one);
        grid.place (lastButton,   row, lastColumn,   one);
    }

    grid.frame (mpeGroup, mpeTitle, grid.addRow (metrics::pageGroupPadding));

    //  Files ------------------------------------------------------------------
    const auto filesTitle = grid.addRow (metrics::pageGroupTitleHeight);

    {
        const auto row = grid.addRow (metrics::pageRowHeight);

        grid.place (loadButton, row, 1, half);
        grid.place (saveButton, row, rightHalf, half);
    }

    grid.frame (filesGroup, filesTitle, grid.addRow (metrics::pageGroupPadding));

    //  Editing ----------------------------------------------------------------
    const auto editingTitle = grid.addRow (metrics::pageGroupTitleHeight);

    // The one flexible track on the page: the table takes whatever height is
    // left, which is what lets the page fit a short panel and fill a tall one.
    grid.place (table, grid.addFlexibleRow(), 1, full);

    {
        const auto row = grid.addRow (metrics::pageRowHeight);

        grid.place (addButton,    row, 1, half);
        grid.place (removeButton, row, rightHalf, half);
    }

    {
        // The other two ways to add a row, so they belong with `add` inside the
        // frame rather than under it.
        const auto row = grid.addRow (metrics::pageRowHeight);

        grid.place (aftertouchButton, row, 1, half);
        grid.place (polytouchButton,  row, rightHalf, half);
    }

    grid.frame (editingGroup, editingTitle, grid.addRow (metrics::pageGroupPadding));

    grid.performLayout (getLocalBounds(), getMinimumHeight());
}

} // namespace microtonos::sidebar
