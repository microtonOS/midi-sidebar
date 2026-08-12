#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace microtonos::sidebar
{

//==============================================================================
/** A table header whose columns cycle through three states rather than two.

    JUCE's own `columnClicked` toggles between ascending and descending and
    never lets go:

    ```cpp
    // juce_TableHeaderComponent.cpp
    setSortColumnId (columnId, (ci->propertyFlags & sortedForwards) == 0);
    ```

    Once a column has been clicked, some column is sorting the table for ever.
    That leaves no way back to the order the rows arrived in, which for a list
    of things the end-user is adding one at a time is the most useful order
    there is — the row you just made is the one you are looking for.

    So a click here goes ascending, descending, *neither*, and "neither" is a
    sort column of 0, which the owner reads as "newest first".

    **It decides nothing itself.** The next state depends on what else is
    sorted, and on this page there are two of these — one over the frozen
    parameter column and one over the columns that scroll — which must agree
    about which of them is in force. So a click is reported and the owner
    answers by pushing a state into both.
*/
class SortingHeader final : public juce::TableHeaderComponent
{
public:
    // Explicit because JUCE_DECLARE_NON_COPYABLE declares a deleted copy
    // constructor, and any user-declared constructor suppresses the implicit
    // default one.
    SortingHeader() = default;

    /** A sortable column was clicked. Not a request to sort by it: the owner
        works out which of the three states comes next. */
    std::function<void (int columnId)> onColumnClicked;

    void columnClicked (int columnId, const juce::ModifierKeys& mods) override
    {
        // The popup menu is JUCE's own column-visibility menu, which this table
        // does not offer — its columns are the specification, not a preference.
        if (mods.isPopupMenu())
            return;

        if (onColumnClicked != nullptr)
            onColumnClicked (columnId);
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SortingHeader)
};

} // namespace microtonos::sidebar
