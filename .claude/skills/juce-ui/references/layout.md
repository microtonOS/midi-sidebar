# Layout

This is a deliberately restricted set of things related to layouts.
Prefer these, and if something doesn't fit, please ask.

## Layout Tools

There are two kinds of layout tools.
One is just called grid, [example here](https://github.com/juce-framework/JUCE/blob/master/examples/GUI/GridDemo.h).
A more complex technique is flexboxes, [example here](https://github.com/juce-framework/JUCE/blob/master/examples/GUI/FlexBoxDemo.h).

### How much CSS knowledge transfers

Both are ports of the CSS specifications, close to one-to-one, so existing CSS
layout knowledge is worth using here rather than relearning.

**Transfers directly.** Track sizing, fractional units (`Grid::Fr` = CSS `fr`),
gaps, `justifyContent`, `alignItems`, `flexDirection`, `flexGrow`. The mental
model is the same: declare the container's structure, then place items into it.
The css-tricks Complete Guide linked below is the reference JUCE's own
documentation points at, so it is an accurate guide to `juce::Grid` and not just
an analogy.

**Does not transfer.**
- There is no cascade and no stylesheet. Appearance lives in `LookAndFeel`
  methods that receive the widget's state as parameters, so there is no `:hover`
  or `:focus` — you branch on `isMouseOver` inside the drawing method.
- There are no `em`, `rem`, percentages or media queries. Anything responsive is
  written by hand in `resized()`.
- There are no audio widgets. Knobs, meters, drawbars and multi-state switches
  have no CSS equivalent, and neither does the convention around them. That is
  the part this skill exists to cover.

### `juce::Grid` Class
Container that handles geometry for grid layouts (fixed columns and rows) using a set of declarative rules.

Implemented from the CSS Grid Layout specification as described at: https://css-tricks.com/snippets/css/complete-guide-grid/

See also
GridItem

### Spans

A `GridItem` can cover several tracks, which is what lets one grid serve a whole
page instead of one grid per row. Without it, the only way to give two widgets
different widths is a separate grid with its own track list — and then nothing
lines up between rows.

```cpp
// Grid lines are 1-based, and the end line is exclusive: this covers
// columns 2 and 3 of row 4.
grid.items.add (GridItem (component).withArea (4, 2, 5, 4));
//                                             ^  ^  ^  ^
//                                    rowStart ┘  │  │  └ columnEnd
//                                    columnStart ┘  └ rowEnd
```

Two habits keep this readable. Number the rows with a running counter as you
declare their tracks, so inserting a row does not renumber everything below it;
and name the lines that recur:

```cpp
int nextRow = 1;
const auto addRow = [&] (int height)
{
    grid.templateRows.add (Track (Px (height)));
    return nextRow++;
};

constexpr auto half      = numColumns / 2;
constexpr auto rightHalf = 1 + half;      // the line the right-hand half starts on
```

`GridItem::Span` exists for auto-placed items, but explicit line numbers are
worth the few extra characters: they say where a thing is rather than where it
ended up, and they survive a widget being added in the middle.

### Two ways a Grid fails quietly

**An item you did not place goes into a new row you did not declare.** Grid
auto-places anything without an explicit area, and when it runs out of declared
rows it invents implicit ones *after* them. So two components meant to share a
cell — overlapping pages, a widget behind another — put the second one below the
grid, outside the visible bounds. It does not warn, and it looks exactly like a
component that was never made visible, which sends you hunting through
`setVisible` instead. Anything that must share a cell needs an explicit
`withArea`.

**A flexible track that cannot fit pulls the rows after it upwards.** Give a
`Grid` less height than its fixed tracks need and the `Fr` track takes what is
left, which is negative — so the rows below it are laid out *higher* than the
ones above, and the content overlaps instead of being clipped. Overlap is the
signature; if two sections are drawn on top of each other, look for a flexible
track that has run out of room rather than for a bad rectangle. Lay out at
`jmax (getHeight(), minimumHeight)` so that overflow is clipped honestly.

### Choosing the columns

The column count is a decision, and the wrong one forces per-row grids later.
Take the **finest division the design uses anywhere**, and make that the
template:

1. **Find the smallest unit.** Look for the narrowest thing that has to line up
   with something in another row — a field beside its label, one half of a split
   row. The column count is how many of those fit across.
2. **Make every track equal and flexible** (`Fr (1)`). Fixed-width columns are
   what pull a grid out of alignment: each row's leftover then differs, so its
   later columns start in a different place.
3. **Give each widget a whole-number span.** A row split in half is a span of
   `n/2`, and it is then halved exactly, at every window size.
4. **Name the spans in the look and feel file** — `pageColumns`,
   `pageLabelColumns` — so a row reads as "a label, then the rest" rather than
   as arithmetic. This is also how rule 6 gets satisfied honestly, instead of by
   naming pixel widths that were guesses.

Six is a good default for a settings page: it covers halves, thirds, and a
label plus its field.

When you are working from a mockup, read its **structure**, not its measurements.
Any mockup notation has some way of saying "this covers more than one unit" —
merged cells, a box drawn across two columns, an element that visibly begins at
the midpoint — and that is the notation telling you what the unit is. The
rendered widths are a property of the tool the mockup was drawn in, and copying
them is how a design that specified halves ends up with none.

### `juce::FlexBox` Class
Represents a FlexBox container, which contains and manages the layout of a set of FlexItem objects.

To use this class, set its parameters appropriately (you can search online for more help on exactly how the FlexBox protocol works!), then add your sub-items to the items array, and call performLayout() in the resized() function of your Component.

See also
FlexItem

## Grouping

Grouping is used in several examples including [flexbox](https://github.com/juce-framework/JUCE/blob/master/examples/GUI/FlexBoxDemo.h) and [widgets](https://github.com/juce-framework/JUCE/blob/master/examples/GUI/WidgetsDemo.h).

### `juce::GroupComponent` Class
A component that draws an outline around itself and has an optional title at the top, for drawing an outline around a group of controls.

**Use it as the default for a named section of a page**, in preference to a
label with a rule or a bare gap. The frame carries the grouping at small sizes
where a heading alone does not, and the title lands inside the border so it
costs no extra row. Two conditions, both from experience rather than taste:

- **Only where it groups more than one control.** A frame around a single widget
  is a label with a box drawn round it — noise that reads as structure.
- **A group is a frame, not a container.** Keep the section's widgets as
  children of the *page* so they stay in the page's one grid; put the
  `GroupComponent` behind them as an item spanning the section's rows, and give
  it `setInterceptsMouseClicks (false, false)`. Making them its children hands
  each group its own layout, and columns stop aligning between sections —
  mechanics rule 2 all over again.

To inset the contents from the frame while everything stays in one grid, add a
gutter track at each end: the frames span every track, the widgets span only the
ones between the gutters.

**Aligning a title above a group's title.** A panel or page heading sitting
above framed sections has two reasonable references: the content inside the
frames, or the group titles themselves. Default to the **content edge** — it is
a number you already own, being your own gutter plus column gap, and the two
references land within a couple of pixels of each other anyway.

Matching the group titles exactly is the better answer where the headings are
what the eye follows — a stack of sections with little content, say, or a design
where the titles carry more weight than the fields. It costs something, though:
the offset is three private constants inside
`LookAndFeel_V2::drawGroupComponentOutline` (frame inset, corner radius, text
margin) that nothing exposes and any release may change, so reconstruct it only
deliberately, and leave a comment saying where the number came from.


## Bars

An example of a toolbar is available in the [widgets demo](https://github.com/juce-framework/JUCE/blob/master/examples/GUI/WidgetsDemo.h).
A sidebar can either be [custom made](https://juce.com/tutorials/tutorial_rectangle_advanced/) or use the `SidePanel` class

### `juce::SidePanel` Class
A component that is positioned on either the left- or right-hand side of its parent, containing a header and some content.

This sort of component is typically used for navigation and forms in mobile applications.

When triggered with the showOrHide() method, the SidePanel will animate itself to its new position. This component also contains some logic to reactively resize and dismiss itself when the user drags it.


### `juce::Toolbar` Class
A toolbar component.

A toolbar contains a horizontal or vertical strip of ToolbarItemComponents, and looks after their order and layout.

Items (icon buttons or other custom components) are added to a toolbar using a ToolbarItemFactory - each type of item is given a unique ID number, and a toolbar might contain more than one instance of a particular item type.

Toolbars can be interactively customised, allowing the user to drag the items around, and to drag items onto or off the toolbar, using the ToolbarItemPalette component as a source of new items.

See also
ToolbarItemFactory, ToolbarItemComponent, ToolbarItemPalette


### juce::TabbedComponent Class Reference
A component with a TabbedButtonBar along one of its sides.

This makes it easy to create a set of tabbed pages, just add a bunch of tabs with addTab(), and this will take care of showing the pages for you when the user clicks on a different tab.

See also
TabbedButtonBar