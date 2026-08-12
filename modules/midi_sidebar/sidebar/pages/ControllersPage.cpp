#include "ControllersPage.h"

namespace microtonos::sidebar
{

using namespace controllers;

//==============================================================================
ControllersPage::ControllersPage()
{
    // Frames first, so everything else draws over them — the arrangement the
    // tuning page uses and for the same reason.
    for (auto* group : { &filesGroup, &editingGroup })
    {
        group->setTextLabelPosition (juce::Justification::centredLeft);
        group->setInterceptsMouseClicks (false, false);
        addAndMakeVisible (*group);
    }

    filesGroup  .setText ("FILES");
    editingGroup.setText ("EDITING");

    addAndMakeVisible (monitor);

    for (auto* b : { &loadButton, &saveButton, &addButton, &removeButton,
                     &aftertouchButton, &polytouchButton })
        addAndMakeVisible (*b);

    addAndMakeVisible (table);

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

void ControllersPage::addMessage (const juce::String& message)
{
    messages.insert (0, message);
    setMessages (messages);
}

void ControllersPage::setMessages (juce::StringArray newMessages)
{
    messages = std::move (newMessages);

    // Keeping the newest is the point of a tail; dropping from the end would
    // keep the first two messages of the session for ever.
    while (messages.size() > monitorLines)
        messages.remove (messages.size() - 1);

    monitor.setValue (messages.joinIntoString ("\n"));
}

//==============================================================================
void ControllersPage::lookAndFeelChanged()
{
    if (! getLookAndFeel().isColourSpecified (pageColours::sectionTitleColourId))
        return;

    for (auto* group : { &filesGroup, &editingGroup })
    {
        group->setColour (juce::GroupComponent::textColourId,    findColour (pageColours::sectionTitleColourId));
        group->setColour (juce::GroupComponent::outlineColourId, findColour (pageColours::sectionOutlineColourId));
    }
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

    //  The monitor, unframed at the top like the tuning page's interval block:
    //  the sketch gives it no title.
    grid.place (monitor, grid.addRow (metrics::pageTopHeight (metrics::pageTopRows)), 1, full);

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
