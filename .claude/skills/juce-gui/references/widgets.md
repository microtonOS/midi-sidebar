# Widgets

A subset of available widgets that are particularly useful.
[Examples here](../assets/WidgetsDemo.h).

## Text and Menus

| class | description |
|-------|-------------|
| `TextEditor` | Single-line textbox |
| `ComboBox` | A dropdown menu showing the current selection and a downwards arrow. |
| `ListBox` | Scroll instead of dropdown. |
| `TableListBox` | A scrollable table |

## Sliders

A slider control for changing a value.

The slider can be horizontal, vertical, or rotary, and can optionally have a text-box inside it to show an editable display of the current value.

To use it, create a Slider object and use the `setSliderStyle()` method to set up the type you want. To set up the text-entry box, use `setTextBoxStyle()`.

To define the values that it can be set to, see the setRange() and setValue() methods.

There are also lots of custom tweaks you can do by subclassing and overriding some of the virtual methods, such as changing the scaling, changing the format of the text display, custom ways of limiting the values, etc.

You can register `Slider::Listener` objects with a slider, and they'll be called when the value changes.


| class | description |
|-------|-------------|
| `Slider::Rotary` | Knob. |
| `Slider::LinearBar` and `Slider::LinearBarVertical` | Rectangle in a box that can be lengthened or shortened. |
| `Slider::LinearHorizontal` and `Slider:LinearVertical` | Slider with a nubbin. Standard design which is extended below. |
| `Slider::ThreeValueHorizontal` and `Slider:ThreeValueVertical` | The nubbin is contained within min and max limiters. |
| `Slider::TwoValueHorizontal` and `Slider:TwoValueVertical` | The same as above but with the nubbin removed so there is only a range. |


`SetPopupDisplayEnabled` adds a speech bubble. A common usage is to display the value and append the relevant unit if any.
A less elegant way is to display the value in an associated textbox by using `setTextBoxSlide`.
The linear bars interestingly display text inside the rectangle.

## Buttons

| class | description |
|-------|-------------|
| `TextButton` | Simple button with text in it |
| `ToggleButton` | Button with toggle functionality |
| `RadioButton` | Toggle functionality but different design: text with tick-box next to it |
| `Slider::IncDecButtons` | Pair of "+" and "-" buttons to increment/decrement a value |
| `DrawableButton` | Draws images either instead of the button or inside it |





