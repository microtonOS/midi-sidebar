#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include "../SidebarIcons.h"
#include "../SidebarLookAndFeel.h"
#include "../widgets/ChoiceButton.h"
#include "../widgets/ChoiceStrip.h"
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

    void setMappings (juce::Array<controllers::Mapping> newMappings);
    const juce::Array<controllers::Mapping>& getMappings() const noexcept { return mappings; }

    /** How the rows are ordered. The mappings themselves are always held in the
        order they were added; this only decides how they are shown. */
    enum class Order
    {
        recent,        ///< Most recently added first — the clock button.
        alphabetical   ///< By parameter name, a at the top — the "abc" button.
    };

    /** Appends a blank mapping and selects it, which is what `add` does. Under
        `Order::recent` it appears at the top, since that is where the newest
        row belongs. */
    void addMapping();

    /** Removes the selected row, or the last one when nothing is selected, so
        the button is never dead while there is something to remove. */
    void removeSelectedMapping();

    /** Called after any edit, insertion or removal, never while one is in
        progress. */
    std::function<void()> onMappingsChanged;

    /** Height that shows `rows` rows under the header — what the page reserves
        when it has the room, and its minimum when it does not. */
    static constexpr int getHeightForRows (int rows) noexcept
    {
        return metrics::tableHeaderHeight + rows * metrics::tableRowHeight;
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

    /** True when the row's mode ignores the LSB, which the two threshold modes
        do. The cell is disabled rather than hidden: the number is still part of
        the mapping, it just has no effect. */
    bool ignoresLsb (int row) const;

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
    Order order = Order::recent;

    /** The sort toggle, in the strip above the frozen column — which is the one
        piece of the header the parameter names do not need, since they have no
        title of their own.

        Not a `ChoiceStrip`: one of the two is an icon, and a strip builds
        TextButtons. The pair still takes the same accent for "this one is on",
        so it reads as the same kind of control. */
    /** ImageOnButtonBackground, not ImageFitted: only that style routes through
        `LookAndFeel::drawButtonBackground`, which is what rounds a button's
        corners and honours its connected edges. `ImageFitted` goes to
        `drawDrawableButton` instead and fills a plain rectangle — which is why
        this sat square-cornered beside a rounded "abc". */
    juce::DrawableButton recentButton { "recent", juce::DrawableButton::ImageOnButtonBackground };
    juce::TextButton alphabeticalButton { "abc" };

    juce::ListBox frozenColumn { "parameters", this };
    juce::TableListBox table { "mappings", this };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ControllersTable)
};

} // namespace microtonos::sidebar
