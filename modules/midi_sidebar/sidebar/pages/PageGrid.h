#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "../SidebarLookAndFeel.h"

namespace microtonos::sidebar
{

//==============================================================================
/** The layout every page is built on: six equal columns between two gutters,
    rows added in the order they appear, and sections framed behind their rows.

    One `juce::Grid` for a whole page, never one per row — a grid built for a
    single row can only align things within that row, and everything that has to
    line up *between* rows is then arithmetic that agrees by coincidence. See
    `metrics::pageColumns`.

    Three pages were carrying the same twenty lines of this before it was
    extracted, which is also why the two rules that are easy to get wrong are
    built in rather than left to each page:

    - `place` counts **content** columns, 1 to `metrics::pageColumns`, and steps
      over the gutters itself. A page never has to know they are there.
    - `performLayout` takes a minimum height and never lays out below it. Given
      less room than its fixed rows need, a `Grid` makes the flexible track
      negative and then draws the rows *after* it higher up the page — so the
      symptom is sections overlapping rather than being cut off. Clamping here
      means no page can reintroduce that.
*/
class PageGrid
{
public:
    PageGrid()
    {
        grid.columnGap = juce::Grid::Px (metrics::pageColumnGap);
        grid.rowGap    = juce::Grid::Px (metrics::pageRowGap);

        // A gutter, the content columns, a gutter. The gutters are what let a
        // section's frame be drawn wider than the widgets inside it while
        // everything stays in one grid.
        grid.templateColumns.add (Track (juce::Grid::Px (metrics::pageGroupPadding)));

        for (int i = 0; i < metrics::pageColumns; ++i)
            grid.templateColumns.add (Track (juce::Grid::Fr (1)));

        grid.templateColumns.add (Track (juce::Grid::Px (metrics::pageGroupPadding)));
    }

    //==========================================================================
    /** Adds a row of a fixed height and returns its grid line, which is what
        `place` and `frame` take. Rows are numbered as they are declared, so
        inserting one does not renumber the rest. */
    int addRow (int height)
    {
        return addTrack (Track (juce::Grid::Px (height)));
    }

    /** The row that absorbs whatever height is left over. A page should have at
        most one, and having one is what lets it fit a short panel and fill a
        tall one instead of being cut off. */
    int addFlexibleRow()
    {
        return addTrack (Track (juce::Grid::Fr (1)));
    }

    //==========================================================================
    /** @param firstColumn  1-based within the six content columns
        @param columnSpan   how many of them to cover */
    void place (juce::Component& component, int row, int firstColumn, int columnSpan)
    {
        placeSpanning (component, row, row, firstColumn, columnSpan);
    }

    /** The same, over a range of rows — for a control that stands beside
        several, such as a vertical choice strip next to two buttons. */
    void placeSpanning (juce::Component& component, int firstRow, int lastRow,
                        int firstColumn, int columnSpan)
    {
        // The +1s step over the leading gutter.
        grid.items.add (juce::GridItem (component).withArea (firstRow, firstColumn + 1,
                                                             lastRow + 1, firstColumn + columnSpan + 1));
    }

    /** Frames a section, from its title band down to the padding row below its
        last row, and out to both gutters.

        The group is only a frame: its section's widgets are children of the
        *page* and are placed in this grid like everything else. Making them the
        group's children would give each section its own layout, and the columns
        would stop agreeing between sections. */
    void frame (juce::GroupComponent& group, int titleRow, int paddingRow)
    {
        grid.items.add (juce::GridItem (group).withArea (titleRow, 1,
                                                         paddingRow + 1,
                                                         metrics::pageColumnsWithGutters + 1));
    }

    //==========================================================================
    void performLayout (juce::Rectangle<int> bounds, int minimumHeight)
    {
        grid.performLayout (bounds.withHeight (juce::jmax (bounds.getHeight(), minimumHeight)));
    }

private:
    using Track = juce::Grid::TrackInfo;

    int addTrack (Track track)
    {
        grid.templateRows.add (track);
        return nextRow++;
    }

    juce::Grid grid;
    int nextRow = 1;      ///< Grid lines are 1-based.

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PageGrid)
};

} // namespace microtonos::sidebar
