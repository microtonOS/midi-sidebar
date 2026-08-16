#include "ControllersPage.h"

namespace microtonos::sidebar
{

using namespace controllers;

//==============================================================================
ControllersPage::ControllersPage()
{
    // Frames first, so everything else draws over them — the arrangement the
    // tuning page uses and for the same reason.
    for (auto* group : { &insertGroup, &editGroup })
    {
        group->setTextLabelPosition (juce::Justification::centredLeft);
        group->setInterceptsMouseClicks (false, false);
        addAndMakeVisible (*group);
    }

    insertGroup.setText ("INSERT");
    editGroup  .setText ("EDIT");

    addAndMakeVisible (monitor);

    for (auto* b : { &ccButton, &aftertouchButton, &polytouchButton,
                     &deleteButton, &undoButton, &redoButton })
        addAndMakeVisible (*b);

    addAndMakeVisible (table);

    //  Insert -----------------------------------------------------------------
    // Each adds a blank row of its own kind. The kind is a button rather than a
    // column because it decides what the rest of the row means: the two touch
    // sources have no controller number to type into.
    ccButton        .onClick = [this] { table.addMapping (controllers::Source::control); };
    aftertouchButton.onClick = [this] { table.addMapping (controllers::Source::aftertouch); };
    polytouchButton .onClick = [this] { table.addMapping (controllers::Source::polytouch); };

    //  Edit -------------------------------------------------------------------
    deleteButton.onClick = [this] { table.removeSelectedMapping(); };
    undoButton  .onClick = [this] { table.undo(); };
    redoButton  .onClick = [this] { table.redo(); };

    table.onMappingsChanged = [this] { if (onMappingsChanged != nullptr) onMappingsChanged(); };
    table.onHistoryChanged   = [this] { refreshHistory(); };
    table.onSelectionChanged = [this] { refreshDeleteButton(); };

    refreshHistory();
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

void ControllersPage::refreshHistory()
{
    // Disabled rather than hidden: a button that vanishes when there is nothing
    // to undo moves the two beside it, and a row that reflows as you work is
    // harder to aim at than one with a greyed button in it.
    undoButton.setEnabled (table.canUndo());
    redoButton.setEnabled (table.canRedo());

    refreshDeleteButton();
}

void ControllersPage::refreshDeleteButton()
{
    // The three built-in rows cannot be removed — the sidebar would then have no
    // way to answer bank select — so on those the button restores the row's
    // defaults instead, and says which of the two it is about to do. A mixed
    // selection reads `reset`, the less destructive of the two words.
    deleteButton.setButtonText (table.selectionIsBuiltin() ? "reset" : "delete");
}

//==============================================================================
void ControllersPage::lookAndFeelChanged()
{
    if (! getLookAndFeel().isColourSpecified (pageColours::sectionTitleColourId))
        return;

    for (auto* group : { &insertGroup, &editGroup })
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

    constexpr auto full  = metrics::pageColumns;
    constexpr auto third = metrics::pageThirdColumns;

    //  The monitor, unframed at the top like the tuning page's interval block:
    //  the sketch gives it no title.
    grid.place (monitor, grid.addRow (metrics::pageTopHeight (metrics::pageTopRows)), 1, full);

    //  Insert -----------------------------------------------------------------
    const auto insertTitle = grid.addRow (metrics::pageGroupTitleHeight);

    {
        // Thirds, which six columns divide into exactly — and the same three
        // the EDIT row below uses, so the two rows line up down the page.
        const auto row = grid.addRow (metrics::pageRowHeight);

        grid.place (ccButton,         row, 1, third);
        grid.place (aftertouchButton, row, 1 + third, third);
        grid.place (polytouchButton,  row, 1 + third * 2, third);
    }

    grid.frame (insertGroup, insertTitle, grid.addRow (metrics::pageGroupPadding));

    //  Edit -------------------------------------------------------------------
    const auto editTitle = grid.addRow (metrics::pageGroupTitleHeight);

    // The one flexible track on the page: the table takes whatever height is
    // left, which is what lets the page fit a short panel and fill a tall one.
    grid.place (table, grid.addFlexibleRow(), 1, full);

    {
        const auto row = grid.addRow (metrics::pageRowHeight);

        // delete first, then the pair that puts it back — the destructive action
        // beside the table it acts on, and undo/redo in their reading order.
        grid.place (deleteButton, row, 1, third);
        grid.place (undoButton,   row, 1 + third, third);
        grid.place (redoButton,   row, 1 + third * 2, third);
    }

    grid.frame (editGroup, editTitle, grid.addRow (metrics::pageGroupPadding));

    grid.performLayout (getLocalBounds(), getMinimumHeight());
}

} // namespace microtonos::sidebar
