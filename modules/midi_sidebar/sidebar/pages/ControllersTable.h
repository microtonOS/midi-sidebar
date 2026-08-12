#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include "../SidebarIcons.h"
#include "../SidebarLookAndFeel.h"
#include "../widgets/ChoiceButton.h"
#include "../widgets/ChoiceStrip.h"
#include "../widgets/HeaderButton.h"
#include "../widgets/ReadOutField.h"
#include "ControllersState.h"

namespace microtonos::sidebar
{

//==============================================================================
/** The editing table: one row per controller mapping, with the parameter names
    frozen at the left while the rest scrolls sideways.

    **JUCE has no frozen column.** `TableListBox` pins the *header row* — that is
    what `TableHeaderComponent` does, and it satisfies half of what
    docs/controllers.md asks for — but nothing anywhere pins a column. So this is
    two views of one list side by side: a `ListBox` holding the parameter column,
    and a `TableListBox` holding the six that scroll, with their vertical
    scrolling tied together.

    Being both models at once is deliberate rather than clever. `ListBoxModel`
    and `TableListBoxModel` are unrelated interfaces that happen to declare the
    same `getNumRows()`, so one implementation serves both — which is exactly
    the invariant the design needs: the two views cannot disagree about how many
    rows there are.

    Unlike the tuning page, this one **owns its data**. A table cannot be
    rebuilt from the outside on every keystroke without fighting the editor the
    user is typing into, so the mappings live here and `onMappingsChanged`
    reports them afterwards.
*/
class ControllersTable final : public juce::Component,
                              public juce::ListBoxModel,
                              public juce::TableListBoxModel,
                              private juce::ScrollBar::Listener
{
public:
    ControllersTable();
    ~ControllersTable() override;

    //==========================================================================
    /** The parameters a mapping may target, named and given a unit by the
        developer. Rebuilding this re-labels every row. */
    void setParameters (juce::Array<controllers::Parameter> newParameters);
    const juce::Array<controllers::Parameter>& getParameters() const noexcept { return parameters; }

    void setMappings (juce::Array<controllers::Mapping> newMappings);
    const juce::Array<controllers::Mapping>& getMappings() const noexcept { return mappings; }

    /** Selects every row mapped to `parameterIndex` and scrolls the first into
        view, which is what the right-click menu's "view in sidebar" does. */
    void selectMappingsFor (int parameterIndex);

    /** Removes the most recently added mapping for `parameterIndex`, or does
        nothing if it has none. "Latest" is by insertion, not by what is on top:
        the display order is a view, and unlearning should undo the last thing
        that was learned however the table happens to be sorted. */
    void removeLatestMappingFor (int parameterIndex);

    /** How the frozen column orders the rows, when no table column is doing it.
        The mappings themselves are always held in the order they were added;
        this only decides how they are shown. */
    enum class Order
    {
        recent,        ///< Most recently added first — the clock button.
        alphabetical   ///< By parameter name, a at the top — the "abc" button.
    };

    /** Appends a blank mapping and selects it, which is what `add` does. Under
        `Order::recent` it appears at the top, since that is where the newest
        row belongs.

        The source is an argument rather than a column because it decides what
        the rest of the row means: an aftertouch or polytouch row has no
        controller number, so its MSB and LSB are replaced by the word itself.
        That is why docs/controllers.md gives those two their own buttons beside
        `add` instead of a menu inside the table. */
    void addMapping (controllers::Source source = controllers::Source::control);

    /** Removes the selected row, or the last one when nothing is selected, so
        the button is never dead while there is something to remove. */
    void removeSelectedMapping();

    /** Called after any edit, insertion or removal, never while one is in
        progress. */
    std::function<void()> onMappingsChanged;

    /** Height that shows `rows` rows under the header — what the page reserves
        when it has the room, and its minimum when it does not.

        The horizontal scrollbar is counted because it is always present: the
        columns are wider than the panel by design, which is the whole reason
        the parameter names are frozen beside them. Leaving it out cost the last
        row its bottom few pixels, and only at the minimum height, where there
        was nothing spare to hide it. */
    static constexpr int getHeightForRows (int rows) noexcept
    {
        return metrics::tableHeaderHeight
             + rows * metrics::tableRowHeight
             + metrics::scrollbarThickness;
    }

    //==========================================================================
    void resized() override;
    void lookAndFeelChanged() override;
    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails&) override;

    //==========================================================================
    /** Shared by both models on purpose; see the class note. */
    int getNumRows() override;

    //  ListBoxModel — the frozen parameter column.
    void paintListBoxItem (int row, juce::Graphics&, int width, int height, bool selected) override;
    juce::Component* refreshComponentForRow (int row, bool selected, juce::Component* existing) override;

    //  TableListBoxModel — the columns that scroll.
    void paintRowBackground (juce::Graphics&, int row, int width, int height, bool selected) override;
    void paintCell (juce::Graphics&, int row, int columnId, int width, int height, bool selected) override;
    juce::Component* refreshComponentForCell (int row, int columnId, bool selected,
                                              juce::Component* existing) override;
    void selectedRowsChanged (int lastRowSelected) override;

    /** A sortable header was clicked, or `setSortColumnId` was called. Column 0
        means "no column", which is when the frozen column's own two orderings
        apply. */
    void sortOrderChanged (int newSortColumnId, bool isForwards) override;

private:
    //==========================================================================
    /** Column ids, which `TableListBox` needs to be 1-based. `param` is not one
        of the table's columns — it is the frozen list beside it — but it shares
        the numbering so that one set of accessors can answer for every cell. */
    enum ColumnId { param = 1, channel, msb, lsb, mode, minimum, maximum };

    struct ChoiceCell;
    struct NumberCell;

    //  What a cell shows, what it shows while being edited, and what typing
    //  into it means. Kept here rather than in the cell widgets so that every
    //  rule about a column lives in one place.
    juce::String textFor (int row, int columnId) const;
    juce::String editableTextFor (int row, int columnId) const;
    void commitText (int row, int columnId, const juce::String& text);

    /** How wide a column has to be for its title and every one of its entries.
        Static because the column widths are set before there is anything in the
        table to ask. */
    static int widthForContents (const juce::String& title, const juce::StringArray& items);

    /** What a choice column offers, where its dividing line goes, and what its
        cells are called in the component tree. */
    juce::String nameFor (int columnId) const;
    juce::StringArray itemsFor (int columnId) const;
    int separatorFor (int columnId) const;

    /** 0-based, unlike a ComboBox's item ids — the button deals in indices, and
        so do the enums and the parameter list behind these columns. */
    int choiceFor (int row, int columnId) const;
    void commitChoice (int row, int columnId, int index);

    /** Orders `displayOrder` by whichever mechanism is in force. */
    void applyOrdering();

    /** True when the row's mode ignores the LSB, which the two threshold modes
        do. The cell is disabled rather than hidden: the number is still part of
        the mapping, it just has no effect. */
    bool ignoresLsb (int row) const;

    /** What drives the row. `control` for anything out of range, so a caller
        never has to check the row first. */
    controllers::Source sourceFor (int row) const;

    const controllers::Parameter* parameterFor (int row) const;

    void changed();
    void refreshRows();

    /** Selects a row on behalf of a cell.

        A cell's widget swallows the click that would otherwise reach the table,
        so a row could only be selected by hitting the few pixels between the
        widgets. JUCE's own WidgetsDemo solves it exactly this way: the custom
        cell's `mouseDown` selects the row and then carries on with whatever the
        click was for. */
    void selectRowFromCell (int row, const juce::ModifierKeys& mods);

    /** Display row to index in `mappings`. Every model callback goes through
        it, so the two orders cannot be confused: `mappings` is what the owner
        gets, and it never changes just because the view was re-sorted. */
    int mappingIndexFor (int row) const;
    void rebuildOrder();
    void setOrder (Order newOrder);
    void scrollBarMoved (juce::ScrollBar*, double newRangeStart) override;

    //==========================================================================
    juce::Array<controllers::Parameter> parameters;
    juce::Array<controllers::Mapping> mappings;

    /** Display order over `mappings`, rebuilt whenever either changes. */
    juce::Array<int> displayOrder;

    /** Which of the two mechanisms is ordering the table.

        `sortColumn` is a column id, or 0 for "none" — and then `order` decides,
        which is what the two buttons above the frozen column set. Keeping both
        rather than folding the buttons into the header means the sort survives
        a trip through a column and back: pressing `abc` clears the header's
        column, pressing a header leaves `order` alone underneath. */
    int sortColumn = 0;
    bool sortForwards = true;
    Order order = Order::recent;

    /** True while `setSortColumnId` is being called from our own code, so the
        `sortOrderChanged` it provokes does not undo what provoked it. */
    bool settingSortColumn = false;

    /** The two orderings of the frozen column, in the strip above it — the one
        piece of the header the parameter names do not need, since they have no
        title of their own.

        Drawn as header cells rather than as a segmented pair of buttons: they
        sit in the header row and do what a sortable column does, so looking
        like the columns beside them is the honest thing. See HeaderButton. */
    HeaderButton recentButton { "recent" };
    HeaderButton alphabeticalButton { "abc" };

    juce::ListBox frozenColumn { "parameters", this };
    juce::TableListBox table { "mappings", this };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ControllersTable)
};

} // namespace microtonos::sidebar
