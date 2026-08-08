# Widgets

This is a deliberately restricted set of widgets.
Prefer these, and if something doesn't fit, please ask.
[Examples here](https://github.com/juce-framework/JUCE/blob/master/examples/GUI/WidgetsDemo.h).

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
| `Slider::LinearHorizontal` and `Slider::LinearVertical` | Slider with a nubbin. Standard design which is extended below. |
| `Slider::ThreeValueHorizontal` and `Slider::ThreeValueVertical` | The nubbin is contained within min and max limiters. |
| `Slider::TwoValueHorizontal` and `Slider::TwoValueVertical` | The same as above but with the nubbin removed so there is only a range. |


`setPopupDisplayEnabled` adds a speech bubble. A common usage is to display the value and append the relevant unit if any.
A less elegant way is to display the value in an associated textbox by using `setTextBoxStyle`.
The linear bars interestingly display text inside the rectangle.

The bubble is added as a **child** of the component passed as the third
argument, so it is clipped to that component's bounds. Pass something with room
— usually the editor. Passing the widget itself leaves a sliver of the bubble
and its tail showing, which reads on screen as a stray `<`.

### Three ways a slider comes out wrong

**A vertical fader is shorter than a meter of the same height.**
`LookAndFeel_V2::getSliderLayout` reduces a vertical slider's bounds by
`getSliderThumbRadius` — `jmin (7, h/2, w/2) + 2` — at each end so the thumb has
room to overhang. That happens *before* `drawLinearSlider` is called, so no
amount of custom drawing recovers it. Override `getSliderLayout` to return the
full bounds and clamp the thumb yourself.

Note that the layout is cached; see the LookAndFeel caching trap in
[design](design.md#colours).

**An inverted fader drags the wrong way.** For a fader whose maximum sits at the
*bottom* — a Hammond drawbar pulled out, a control that reads top-down —
painting the fill upside-down in `drawLinearSlider` looks correct but leaves the
mouse dragging backwards. Give the slider a reversed `NormalisableRange`
instead, so proportion 0 maps to the maximum value and direction and fill agree
by construction.

**Rotary sliders come out different sizes.** The drawn radius is
`jmin (width, height)` of what is left after the caption and text box, so two
cells of equal *area* but different aspect ratio give visibly different knobs.
Fix it in the layout, never at the widget: give the knob's grid track a fixed
size, or `GridItem (knob).withSize (kKnob, kKnob)`. Do not hand each knob its
own literal rectangle.

## Buttons

| class | description |
|-------|-------------|
| `TextButton` | Simple button with text in it |
| `ToggleButton` | Toggle drawn as a tick-box with a text label beside it |
| `Slider::IncDecButtons`[^notClass] | Pair of "+" and "-" buttons to increment/decrement a value |
| `DrawableButton` | Draws images either instead of the button or inside it |

Note that although `Slider::IncDecButtons` behaves like a slider, its graphical appearance is that of a pair of buttons, which is why it is placed in this section.

A `TextButton` does not shrink its label to fit, so a narrow button clips
"FAST" to "F...". Override `getTextButtonFont` in the LookAndFeel to scale the
font to the button height, e.g.
`font (jmin (12.0f, (float) buttonHeight * 0.5f))`. Re-check every short-label
button after a layout change: the clipping only appears below a threshold
width, so it can be introduced by a change that had nothing to do with the
button.

For mutually exclusive buttons use `Button::setRadioGroupId` rather than
clearing the others by hand — but note two constraints. The search for siblings
covers only `getParentComponent()->getChildren()`, so every button in a group
must share one parent; and clicking the button that is already on will not turn
it off. If you need an "all off" state, route the clicks through your own
handler instead.


[^notClass]: Not technically a class but an enum value: make a `juce::Slider` and call `setSliderStyle (juce::Slider::IncDecButtons)`.


