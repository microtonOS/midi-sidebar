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

**`TextEditor` bakes its colour into the text, so a theme change leaves it
behind.** The colour is taken from `TextEditor::textColourId` *at the moment
text is inserted* and stored with that run; `TextEditor::lookAndFeelChanged`
only rebuilds the caret. So an editor whose text was set before it reached a
styled parent keeps the **default** LookAndFeel's colour for ever — white, which
is invisible on a light scheme. The value is there, correct, and unreadable.

Re-apply it whenever the look and feel changes:

```cpp
void lookAndFeelChanged() override
{
    editor.applyColourToAllText (findColour (juce::TextEditor::textColourId), true);
}
```

The `true` makes later insertions use it too. JUCE's own documentation for
`textColourId` says as much — "calling this method won't change the colour of
existing text" — but the failure appears long after the call site.

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


**A value belongs in a bubble, not in a box.** `setPopupDisplayEnabled` shows the
number while the control is being turned, which is when it matters, and shows
nothing the rest of the time. `setTextBoxStyle` shows it always: a second thing
to read on a panel whose knob positions already say roughly where everything is,
and a row of height under every knob. Default to the bubble.

The exception is a value that has to be *compared* across controls, or read
without being touched — a set of ratios, a tuning table. Then the box earns its
row, because a bubble can only ever show one of them at a time.

The bubble appends the suffix, so `setTextValueSuffix` carries the unit either
way. The linear bars are a third case: they display text inside the rectangle.

The bubble is added as a **child** of the component passed as the third
argument, so it is clipped to that component's bounds. Pass something with room
— usually the editor. Passing the widget itself leaves a sliver of the bubble
and its tail showing, which reads on screen as a stray `<`.

### Six ways a slider comes out wrong

**A vertical fader is shorter than a meter of the same height.**
`LookAndFeel_V2::getSliderLayout` reduces a vertical slider's bounds by
`getSliderThumbRadius` — `jmin (7, h/2, w/2) + 2` — at each end so the thumb has
room to overhang. That happens *before* `drawLinearSlider` is called, so no
amount of custom drawing recovers it. Override `getSliderLayout` to return the
full bounds and clamp the thumb yourself.

**Scope that override to the style it was written for.** A LookAndFeel is asked
for the layout of *every* slider it serves, so a hand-built `SliderLayout` hands
all the others whatever you left out of it — most visibly an empty
`textBoxBounds`, which silently deletes their text boxes. Branch on
`slider.getSliderStyle()` and give everything else to the base class:

```cpp
Slider::SliderLayout getSliderLayout (Slider& slider) override
{
    if (slider.getSliderStyle() != Slider::LinearVertical)
        return LookAndFeel_V4::getSliderLayout (slider);

    ...
}
```

This one hides well. With a single slider on screen the override is correct, and
it breaks on the day someone adds a second one somewhere else in the plugin —
so the change that reveals the bug has nothing to do with the code that caused
it.

Note that the layout is cached; see the LookAndFeel caching trap in
[design](design.md#colours).

**An attached slider ignores `setNumDecimalPlacesToDisplay`.**
`SliderParameterAttachment`, and so
`AudioProcessorValueTreeState::SliderAttachment`, installs its own
`textFromValueFunction` built from the parameter's `getText`.
`Slider::getTextFromValue` consults that function first and only falls back to
the decimal-place count when there is none — so the call is silently ignored on
any attached slider. In a narrow text box the result is `1999.99…`: a value
elided by its own precision.

Set the function instead, after the attachment exists:

```cpp
slider.textFromValueFunction = [] (double value) { return juce::String (value, 0); };
slider.updateText();
```

The suffix is not part of it. `getTextFromValue` appends
`getTextValueSuffix()` to whatever the function returns, so returning the bare
number is correct and returning `"2000 Hz"` prints the unit twice.

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

**An empty range asserts.** `setRange (0, 0)` — which is what you get from
`setRange (0, jmax (0, items.size() - 1))` when the list is empty or has one
entry — trips a `jassert` inside `NormalisableRange`, whose start must be below
its end. A slider used as an index into a list needs a floor of one step that it
simply cannot leave, with the control disabled to say so.

**An inc/dec slider's internal boundary is not a grid line.** `IncDecButtons`
reads as two things — a read-out and a pair of buttons — but it is one
component, so the grid cannot place the join between them. It falls wherever the
text box's width puts it, and that width is the only number you control:

```cpp
// juce_LookAndFeel_V2.cpp, getSliderLayout — TextBoxLeft
textBoxWidth = jmax (0, jmin (slider.getTextBoxWidth(), width - minXSpace));
layout.sliderBounds.removeFromLeft (textBoxWidth);   // the buttons take the rest
```

So the width passed to `setTextBoxStyle` *is* the position of that boundary,
measured from the component's left edge. Aligning it with a column means solving
for it rather than choosing it. Three things follow:

- **The width is clamped**, to `width - 30` for a box at the left or right and
  `height - 15` for one above or below. Ask for more and you quietly get less,
  and the boundary is no longer where you said.
- **The buttons take the leftover, so they have no fixed size.** The `−|+` seam
  lands at `(width + textBoxWidth) / 2`, the middle of what remains, which means
  one `textBoxWidth` used in two cells of different widths gives two visibly
  different controls. Steppers that must match need cells that match.
- **They flip from side by side to stacked on their own.**
  `resizeIncDecButtons` decides by `buttonRect.getWidth() > buttonRect.getHeight()`,
  so widening the text box inside a narrow cell turns the pair into a `+` above
  a `−` with nothing said about it.

Flexible tracks have no compile-time width, so the column being aimed at has to
be measured on a rendered page. Name the resulting number in the look and feel
file with a comment saying it was measured and how — otherwise the next reader
takes it for a guess and rounds it.

**The value bubble is invisible in a light theme.** This one is a JUCE bug, not
a mistake at the call site, and it reproduces in JUCE's own DemoRunner — Widgets
demo, light scheme. The bubble takes its two colours from two *unrelated* pairs:

| | colour id | `LookAndFeel_V4` maps it to |
|---|---|---|
| background | `BubbleComponent::backgroundColourId` | `widgetBackground` |
| text | `TooltipWindow::textColourId` | `highlightedText` |

`BubbleComponent` has no text colour of its own, so `PopupDisplayComponent::`
`paintContent` borrows the tooltip's. For a real tooltip that pairing is safe —
its background is `highlightedFill`, and V4 keeps that pair contrasting in every
scheme. The bubble breaks the pairing, and in the Light scheme both halves land
on white:

| scheme | background | text | |
|---|---|---|---|
| Dark | `0xff323e44` | white | ok |
| Midnight | `0xff191926` | white | ok |
| Grey | `0xff606060` | black | ~3:1, legible |
| **Light** | **`0xffffffff`** | **`0xffffffff`** | **invisible** |

Fix it by overriding `TooltipWindow::textColourId` on an **ancestor**, not on
each slider — `paintContent` resolves it with
`findColour (TooltipWindow::textColourId, true)`, and that `true` walks the
parent chain, so one call on the editor covers every bubble in the plugin:

```cpp
setColour (juce::TooltipWindow::textColourId, juce::Colours::black);
removeColour (juce::TooltipWindow::textColourId);   // back to stock
```

Two things to know about that walk. It stops early at any component whose *own*
`LookAndFeel` specifies the id (`Component::findColour`: `lookAndFeel == nullptr
|| ! lookAndFeel->isColourSpecified (colourID)`), so a child with
`setLookAndFeel` on it ignores the override. And setting the id on a LookAndFeel
instead re-colours real tooltips too, where nothing was wrong.

Overriding `drawBubble` cannot fix it. That method draws only the background and
the outline; the text is drawn by `paintContent` inside `Slider::Pimpl`'s
private `PopupDisplayComponent`, which no LookAndFeel method reaches.

Changing the *background* to `highlightedFill` also works, and restores JUCE's
contrasting pair for all four schemes — but it is a design decision as much as a
fix, since the bubble then stops matching the surface it floats over.

## Tables

`TableListBox` gives you a header row that stays put while the rows scroll, and
`TableListBoxModel::refreshComponentForCell` to put a real widget in a cell. Four
things about it are not obvious.

**There is no frozen column.** The sticky *header* is `TableHeaderComponent`;
nothing anywhere pins a column, so "keep the names visible while scrolling
sideways" has to be built. It is two views of one list side by side — a
`ListBox` for the pinned column, a `TableListBox` for the rest — with their
vertical scrolling tied together.

**One class can be both models.** `ListBoxModel` and `TableListBoxModel` are
unrelated interfaces that happen to declare the same `int getNumRows()`, so a
single implementation satisfies both. Worth doing for the pair above: it makes
"the two views agree about how many rows there are" structural rather than
something to maintain.

**A `Viewport`'s scrollbar is ranged in pixels, not rows.**
`Viewport::updateVisibleArea` sets the limits from the content's height, so the
value handed to `ScrollBar::Listener::scrollBarMoved` is a pixel offset that can
be copied straight to another list of the same row height. Listening to
`ListBox::getVerticalScrollBar()` is how you tie two lists together; hide the
follower's scrollbars so only one thing can drive.

**A cell widget swallows the click that would select its row.** Put a component
in a cell and the row can only be selected by hitting the pixels between the
widgets — which makes any "act on the selected row" button feel broken. JUCE's
own WidgetsDemo answers it in the cell:

```cpp
void mouseDown (const MouseEvent& event) override
{
    // single click on the label should simply select the row
    owner.table.selectRowsBasedOnModifierKeys (row, event.mods, false);
    Label::mouseDown (event);
}
```

If the cell *wraps* its widget rather than being one, register for the child's
events too — `child.addMouseListener (this, true)` — or the click never reaches
the wrapper either.

## Buttons

| class | description |
|-------|-------------|
| `TextButton` | Simple button with text in it |
| `ToggleButton` | Toggle drawn as a tick-box with a text label beside it |
| `Slider::IncDecButtons`[^notClass] | Pair of "+" and "-" buttons to increment/decrement a value |
| `DrawableButton` | Draws images either instead of the button or inside it |

Note that although `Slider::IncDecButtons` behaves like a slider, its graphical appearance is that of a pair of buttons, which is why it is placed in this section.

A `TextButton` does not shrink its label to fit, so a narrow button clips
"FAST" to "F...". If the button is one of a row of choices, try turning the row
vertical first — see [Segmented controls](#segmented-controls) — since that
usually gives the label the width it was missing. Otherwise override
`getTextButtonFont` in the LookAndFeel to scale the font to the button height,
e.g.
`font (jmin (12.0f, (float) buttonHeight * 0.5f))`. Re-check every short-label
button after a layout change: the clipping only appears below a threshold
width, so it can be introduced by a change that had nothing to do with the
button.

For mutually exclusive buttons use `Button::setRadioGroupId` rather than
clearing the others by hand — but note three constraints. The search for
siblings covers only `getParentComponent()->getChildren()`, so every button in a
group must share one parent; and clicking the button that is already on will not
turn it off. If you need an "all off" state, route the clicks through your own
handler instead.

The third is the one that costs an afternoon. **A radio group fires `onClick` on
the button it switches OFF as well as the one it switches on.**
`Button::internalClickCallback` calls `setToggleState (shouldBeOn,
sendNotification)`, which passes that notification into
`turnOffOtherButtonsInGroup`, which calls `sendClickMessage` on each loser. So
one click produces two callbacks, and the second reports the index that was just
*deselected*.

Guard on the state rather than trusting the callback:

```cpp
button.onClick = [this, i]
{
    if (! buttons[i]->getToggleState())   // we are the loser; nothing to report
        return;

    ...
};
```

Symptoms if you do not. An owner that acts on the spurious callback
*synchronously* — pushing the choice back in by setting the toggle state — will
re-enter the `setToggleState` it is still inside, and both buttons end up drawn
as on until something rebuilds them. An owner that responds asynchronously (a
`ParameterAttachment`, for instance, which marshals through an `AsyncUpdater`)
appears to work, which is why this can hide in one part of a GUI while breaking
another that is using the identical widget.

**A right-click works a button.** `Button::mouseDown` calls
`updateState (true, true)` without ever looking at which mouse button was
pressed, and `mouseUp` fires the click if the button was down. So a right-click
selects a `TextButton`, toggles a `ToggleButton`, and picks a segment of a radio
group.

Harmless until the button also carries a context menu, at which point
right-clicking to open the menu *also* changes the setting — the one thing a
menu must not do on the way to opening. Drop the event in a subclass:

```cpp
void mouseDown (const juce::MouseEvent& event) override
{
    if (event.mods.isPopupMenu())
        return;

    juce::TextButton::mouseDown (event);
}
```

Dropping rather than consuming: the click still reaches anything registered as a
`MouseListener`, which is how the menu sees it. Guarding `mouseDown` alone is
enough — `mouseUp` only fires a click when the button was already down.

**A `DrawableButton` beside a `TextButton` looks like a different control.** Its
style decides which drawing path it takes, and only one of them is the one every
other button uses:

```cpp
// juce_DrawableButton.cpp
if (shouldDrawButtonBackground())      // the ImageOnButtonBackground styles
    lf.drawButtonBackground (g, *this, findColour (getToggleState() ? TextButton::buttonOnColourId
                                                                   : TextButton::buttonColourId), …);
else
    lf.drawDrawableButton (g, *this, …);   // a plain filled rectangle
```

`ImageFitted` and friends take the second branch: square corners, connected
edges ignored, and colours read from `DrawableButton`'s own ColourIds rather
than the `TextButton` ones everything else uses. For an icon button that has to
sit beside text buttons — in a toolbar, or as one half of a toggle — use
**`ImageOnButtonBackground`**, and it becomes the same control with a picture on
it. `setEdgeIndent` then controls how much of it the icon takes.

### Segmented controls

A row or column of buttons drawn as one control — rounded at the two outer ends,
square where they meet — needs **no custom drawing**. `setConnectedEdges` takes
any of `ConnectedOnLeft`, `ConnectedOnRight`, `ConnectedOnTop` and
`ConnectedOnBottom`, and `LookAndFeel_V4::drawButtonBackground` reads all four
to decide which corners of its rounded rectangle to keep:

```cpp
// Horizontal: joined towards the neighbours, open at the two ends.
button.setConnectedEdges ((first ? 0 : juce::Button::ConnectedOnLeft)
                          | (last ? 0 : juce::Button::ConnectedOnRight));

// Vertical is the same with Top and Bottom. It is easy to assume this only
// works horizontally — it does not.
```

**Stack word choices vertically.** Latin script runs along the horizontal axis,
so a vertical strip gives every choice a full-width line of its own, while a
horizontal one divides that width between them — three choices in an 88px
column leave 29px each, and "triangle" does not survive it. Reach for
horizontal only where the choices are short and of similar length (`on`/`off`,
`L`/`R`), or where the strip has a whole row to spread across.

This is also the first thing to check when a button label has turned into
`F...`: it is usually the orientation rather than the font.

Two things follow from the connected edges in practice:

- **The buttons must touch.** Joined edges mean nothing across a gap, and a
  rounded end on one side only looks like damage. So a segmented control cannot
  also have a gap between its members — pick one.
- **A vertical strip spanning several rows will not divide on the row gaps.**
  Given the height of *n* rows plus the gaps between them, its seams land in the
  middle of those gaps rather than on a row boundary — which is usually what you
  want, since its outer edges still align exactly with the block. Expect the odd
  pixel to go somewhere: two tracks across 49px come out 25 and 24, so a seam
  meant for the centre of a 5px gap sits half a pixel off it. Only an even gap
  can be split exactly.


[^notClass]: Not technically a class but an enum value: make a `juce::Slider` and call `setSliderStyle (juce::Slider::IncDecButtons)`.


