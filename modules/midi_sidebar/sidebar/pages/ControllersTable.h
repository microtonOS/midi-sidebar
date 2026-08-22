#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include "../SidebarLookAndFeel.h"
#include "../widgets/ChoiceButton.h"
#include "../widgets/ChoiceStrip.h"
#include "../widgets/SortingHeader.h"
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

    /** Removes **every** mapping for `parameterIndex` — what `unlearn` does.

        All of them rather than the most recent: unlearn is the undo of MIDI
        learn, and a parameter learned three times is one somebody wants to stop
        responding. Clearing one of three leaves it still responding, which
        looks like the command failed. */
    void removeMappingsFor (int parameterIndex);

    /** Appends a blank mapping and selects it, which is what `add` does. With
        no column sorting, it appears at the top, since that is where the newest
        row belongs.

        The source is an argument rather than a column because it decides what
        the rest of the row means: an aftertouch or polytouch row has no
        controller number, so its MSB and LSB cells are replaced by the word
        itself. That is why docs/controllers.md gives those two their own buttons
        beside `add` instead of a menu inside the table. */
    void addMapping (controllers::Source source = controllers::Source::control);

    /** Appends a mapping that already knows what it is — what `MIDI learn`
        produces. Undoable, unlike `setMappings`: this is an edit, and an edit
        that wiped the history would be a strange thing to do to somebody who
        had just been editing. Its limits are set from the parameter's range. */
    void addMapping (controllers::Mapping mapping);

    /** Removes every selected row, or the last one when nothing is selected, so
        the button is never dead while there is something to remove. */
    void removeSelectedMapping();

    /** Called after any edit, insertion or removal, never while one is in
        progress. */
    std::function<void()> onMappingsChanged;

    //==========================================================================
    //  Undo, over the mappings and nothing else.

    void undo();
    void redo();

    bool canUndo() const noexcept { return ! undoStack.isEmpty(); }
    bool canRedo() const noexcept { return ! redoStack.isEmpty(); }

    /** Called whenever `canUndo` or `canRedo` may have changed, so the buttons
        driving them are never dead-looking while there is something to do. */
    std::function<void()> onHistoryChanged;

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
    void paint (juce::Graphics&) override;
    void resized() override;
    void lookAndFeelChanged() override;
    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails&) override;

    //==========================================================================
    /** Shared by both models on purpose; see the class note. */
    int getNumRows() override;

    //  ListBoxModel — the frozen parameter column.
    void paintListBoxItem (int row, juce::Graphics&, int width, int height, bool selected) override;
    juce::Component* refreshComponentForRow (int row, bool selected, juce::Component* existing) override;
    void listBoxItemClicked (int row, const juce::MouseEvent&) override;

    //  TableListBoxModel — the columns that scroll.
    void paintRowBackground (juce::Graphics&, int row, int width, int height, bool selected) override;
    void paintCell (juce::Graphics&, int row, int columnId, int width, int height, bool selected) override;
    juce::Component* refreshComponentForCell (int row, int columnId, bool selected,
                                              juce::Component* existing) override;
    void selectedRowsChanged (int lastRowSelected) override;

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

    /** How wide a column has to be for its title, every one of its entries, and
        its sort arrow if it has one. Static because the column widths are set
        before there is anything in the table to ask. */
    static int widthForContents (const juce::String& title, const juce::StringArray& items,
                                 bool sortable = false);

    /** What a choice column offers, where its dividing line goes, and what its
        cells are called in the component tree. */
    juce::String nameFor (int columnId) const;
    juce::StringArray itemsFor (int columnId) const;
    int separatorFor (int columnId) const;

    /** 0-based, unlike a ComboBox's item ids — the button deals in indices, and
        so do the enums and the parameter list behind these columns. */
    int choiceFor (int row, int columnId) const;
    void commitChoice (int row, int columnId, int index);

    /** Orders `displayOrder` by whatever `sortColumn` currently says. */
    void applyOrdering();

    /** Ascending, descending, neither — and neither means newest first. Both
        headers route their clicks here, so only one of them can be sorting at a
        time and the other is told to show nothing. */
    void cycleSort (int columnId);

    /** True for the columns worth ordering; see the note where they are
        declared. */
    static bool isSortable (int columnId) noexcept;

    /** True when the row's mode ignores the LSB, which the two threshold modes
        do. The cell is disabled rather than hidden: the number is still part of
        the mapping, it just has no effect. */
    bool ignoresLsb (int row) const;

    /** The glyph marking how far this row's parameter reaches, or nullptr for
        the unmarked case. Owned by the table and rebuilt on a theme change, so
        a cell borrows it rather than parsing an SVG of its own every repaint. */
    const juce::Drawable* markerFor (int row) const;

    /** The same glyph by *parameter* rather than by row, for the menu items —
        which are parameters, and exist before any row points at one. */
    const juce::Drawable* markerForParameter (int parameterIndex) const;

    /** True when this cell holds a controller number the plugin cannot use, so
        the cell is washed red and the row does nothing. Two ways in, both from
        docs/controllers.md: the number is one of the eleven that are not
        control changes at all, or an LSB duplicates an MSB already mapped.

        Public because the cell components ask it; see `NumberCell::paint`. */
    bool isCellInvalid (int row, int columnId) const;

    /** What drives the row. `control` for anything out of range, so a caller
        never has to check the row first. */
    controllers::Source sourceFor (int row) const;

    const controllers::Parameter* parameterFor (int row) const;

    /** What this row's target can actually take — the parameter's range, or a
        built-in's own while it is still doing its own job. The `min` and `max`
        columns are clamped and snapped to it, because those columns restrict
        the travel rather than extending it. */
    controllers::Range rangeFor (int row) const;

    void changed();
    void refreshRows();

    /** Snapshots the mappings before something changes them. Every mutation
        calls this first; `setMappings` does not, because an owner replacing the
        list wholesale is not an edit to undo — it clears the history instead. */
    void pushUndo();

    /** Both stacks hold whole copies of the list. See `metrics::undoDepth`. */
    juce::Array<juce::Array<controllers::Mapping>> undoStack, redoStack;

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
    void scrollBarMoved (juce::ScrollBar*, double newRangeStart) override;

    //==========================================================================
    juce::Array<controllers::Parameter> parameters;
    juce::Array<controllers::Mapping> mappings;

    /** Display order over `mappings`, rebuilt whenever either changes. */
    /** True while the frozen column's selection is being brought into line with
        the table's. Both lists share this one model, so setting the follower
        re-enters `selectedRowsChanged` through its own model. */
    bool syncingSelection = false;

    juce::Array<int> displayOrder;

    /** Which column is sorting the table, or 0 for none — and none means the
        order the mappings were added in, newest first. One number for both
        headers, which is what stops them disagreeing about who is in force. */
    int sortColumn = 0;
    bool sortForwards = true;

    juce::ListBox frozenColumn { "parameters", this };
    juce::TableListBox table { "mappings", this };

    /** Both headers, owned by the lists they sit on — `ListBox::
        setHeaderComponent` and `TableListBox::setHeader` each take ownership —
        so these are the borrowed pointers used to drive them. The frozen
        column gets a real header rather than a pair of buttons dressed as one:
        it is then drawn by the LookAndFeel, exactly like the one beside it. */
    SortingHeader* frozenHeader = nullptr;
    SortingHeader* tableHeader  = nullptr;

    /** One of each, not one per cell: the glyphs are the same in every row, and
        recolouring one means reparsing its SVG — a theme-change job rather than
        a paint-time one. */
    std::unique_ptr<juce::Drawable> perNoteMarker, lowerMarker, upperMarker;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ControllersTable)
};

} // namespace microtonos::sidebar
