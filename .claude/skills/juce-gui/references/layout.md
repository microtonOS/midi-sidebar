# Layout

## Layout Tools

There are two kinds of layout tools.
One is just called grid, [example here](../assets/GridDemo.h).
A more complex technique is flexboxes, [example here](../assets/FlexBoxDemo.h).
They are both based on CSS.

### `juce::Grid` Class
Container that handles geometry for grid layouts (fixed columns and rows) using a set of declarative rules.

Implemented from the CSS Grid Layout specification as described at: https://css-tricks.com/snippets/css/complete-guide-grid/

See also
GridItem

### `juce::FlexBox` Class
Represents a FlexBox container, which contains and manages the layout of a set of FlexItem objects.

To use this class, set its parameters appropriately (you can search online for more help on exactly how the FlexBox protocol works!), then add your sub-items to the items array, and call performLayout() in the resized() function of your Component.

See also
FlexItem

## Grouping

Grouping is used in several examples including [flexbox](../assets/FlexBoxDemo.h) and [widgets](../assets/WidgetsDemo.h).

### `juce::GroupComponent` Class
A component that draws an outline around itself and has an optional title at the top, for drawing an outline around a group of controls.


## Bars

An example of a toolbar is available in the [widgets demo](../assets/WidgetsDemo.h).
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