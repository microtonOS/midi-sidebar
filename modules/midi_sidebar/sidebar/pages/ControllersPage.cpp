#include "ControllersPage.h"

namespace microtonos::sidebar
{

using namespace controllers;

//==============================================================================
/** The last few messages to arrive, oldest at the bottom.

    Read-only, headerless and fixed at `monitorRows` rows, so none of
    `TableListBox`'s scrolling, sorting or selection is wanted — only its
    columns. It stays a table anyway because docs/controllers.md says the inner
    tables are real tables, and because the alternative is hand-drawing four
    columns that would then drift from the ones below.
*/
class ControllersPage::Monitor final : public juce::TableListBox,
                                       private juce::TableListBoxModel
{
public:
    Monitor()
    {
        setModel (this);
        setRowHeight (metrics::tableRowHeight);

        // No header: the sketch draws none, and with four columns of obvious
        // content it earns its row back.
        setHeaderHeight (0);

        const auto fixed = juce::TableHeaderComponent::visible;

        getHeader().addColumn ("type",    Column::type,  metrics::monitorTypeWidth,  0, -1, fixed);
        getHeader().addColumn ("chan",    Column::chan,  metrics::monitorChanWidth,  0, -1, fixed);
        getHeader().addColumn ("note/cc", Column::note,  metrics::monitorNoteWidth,  0, -1, fixed);
        getHeader().addColumn ("value",   Column::value, metrics::monitorValueWidth, 0, -1, fixed);

        // Nothing here is selectable or scrollable; it is a read-out that
        // happens to have columns.
        setClickingTogglesRowSelection (false);
        getViewport()->setScrollBarsShown (false, false);
    }

    ~Monitor() override { setModel (nullptr); }

    void setMessages (juce::Array<Message> newMessages)
    {
        messages = std::move (newMessages);

        // Keeping the newest is the point of a tail; dropping from the end
        // would keep the first three messages of the session for ever.
        while (messages.size() > monitorRows)
            messages.remove (messages.size() - 1);

        updateContent();
        repaint();
    }

    void addMessage (Message message)
    {
        messages.insert (0, std::move (message));
        setMessages (messages);
    }

private:
    enum Column { type = 1, chan, note, value };

    int getNumRows() override { return monitorRows; }

    void paintRowBackground (juce::Graphics&, int, int, int, bool) override {}

    void paintCell (juce::Graphics& g, int row, int columnId, int width, int height, bool) override
    {
        if (! juce::isPositiveAndBelow (row, messages.size()))
            return;

        const auto& message = messages[row];

        const auto text = columnId == Column::type ? message.type
                        : columnId == Column::chan ? message.channel
                        : columnId == Column::note ? message.noteOrCc
                                                   : message.value;

        // All four columns are left-aligned. The note or CC still reads as
        // sitting midway between the channel and the value, because its column
        // is the same width as the channel's — see `metrics::monitorNoteWidth`,
        // which is where that is arranged.
        g.setColour (findColour (ReadOutField::textColourId).withMultipliedAlpha (shades::readOnly));
        g.setFont (SidebarLookAndFeel::font (metrics::bodyFontHeight));
        g.drawText (text, juce::Rectangle<int> (width, height).reduced (metrics::readOutPadding, 0),
                    juce::Justification::centredLeft, true);
    }

    juce::Array<Message> messages;
};

//==============================================================================
ControllersPage::ControllersPage()
    : monitor (std::make_unique<Monitor>())
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

    addAndMakeVisible (*monitor);

    for (auto* b : { &loadButton, &saveButton, &addButton, &removeButton })
        addAndMakeVisible (*b);

    addAndMakeVisible (table);

    loadButton.onClick = [this] { if (onLoadRequested != nullptr) onLoadRequested(); };
    saveButton.onClick = [this] { if (onSaveRequested != nullptr) onSaveRequested(); };

    addButton   .onClick = [this] { table.addMapping(); };
    removeButton.onClick = [this] { table.removeSelectedMapping(); };

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

const juce::Array<controllers::Mapping>& ControllersPage::getMappings() const noexcept
{
    return table.getMappings();
}

void ControllersPage::addMessage (controllers::Message message)
{
    monitor->addMessage (std::move (message));
}

void ControllersPage::setMessages (juce::Array<controllers::Message> messages)
{
    monitor->setMessages (std::move (messages));
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

    monitor->setColour (juce::ListBox::backgroundColourId, findColour (ReadOutField::backgroundColourId));
    monitor->setColour (juce::ListBox::outlineColourId,    findColour (ReadOutField::outlineColourId));
    monitor->setOutlineThickness (1);
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

    //  Monitor, unframed at the top like the tuning page's interval block.
    grid.place (*monitor, grid.addRow (monitorHeight()), 1, full);

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

    grid.frame (editingGroup, editingTitle, grid.addRow (metrics::pageGroupPadding));

    grid.performLayout (getLocalBounds(), getMinimumHeight());
}

} // namespace microtonos::sidebar
