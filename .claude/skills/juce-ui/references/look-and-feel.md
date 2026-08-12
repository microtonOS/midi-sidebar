# Look and feel

Every colour a JUCE widget draws with is resolved through a `LookAndFeel`, and
most of the ways a widget comes out the wrong colour are really ways that
resolution went somewhere you did not expect. This file is that system: the
colour scheme, custom `ColourId`s, and when a `drawX` has to be overridden rather
than configured.

## Colours

The standard set of colours to use.
- windowBackground 	
- widgetBackground 	
- menuBackground 	
- outline 	
- defaultText 	
- defaultFill 	
- highlightedText 	
- highlightedFill 	
- menuText 	

`numColours` is 9.

see 
[`juce::LookAndFeel_V4::ColourScheme`](https://docs.juce.com/master/classjuce_1_1LookAndFeel__V4_1_1ColourScheme.html)

**Do not assume which of the two backgrounds is darker.** The relation flips
between JUCE's own schemes: widget is *darker* than window in the dark, midnight
and grey schemes (`0xff263238` against `0xff323e44` in the dark one) and *lighter*
in the light scheme (`0xffffffff` against `0xffefefef`). So `contrasting (amount)`
does not mean "away from the other one" — it means away from *this* one, in
whichever direction this one implies, and a shade derived from the wrong sibling
lands on the neighbour's own colour. `widgetBackground.contrasting (0.06f)` in the
dark scheme gives `#333e44` against a row painted `windowBackground` `#323e44`:
one step apart on paper, indistinguishable on screen, and it reads as "the new
element did not draw". Derive a shade from the surface the element sits
*against*, and check the result in a light scheme as well as a dark one.

### Custom ColourIds, and why a widget renders black

`findColour` looks an ID up in the component's LookAndFeel. If it is not there,
JUCE asserts — in `findColour`, in `juce_LookAndFeel.cpp` — and returns
`Colours::black`. There are two ways in, and they look identical on screen.

**The ID was never registered.** Every custom `ColourId` a widget declares must
be given a value with `setColour` in the LookAndFeel's constructor, including
the ones belonging to child widgets, which are the easy ones to forget. Provide
a `static registerColours (LookAndFeel&, ...)` so a consumer using their own
LookAndFeel can install them too.

**The colour was read before a LookAndFeel was attached.** A component's
constructor has no LookAndFeel yet, so `findColour` falls back to the default
one, which does not know your IDs. Reading colours inside `paint` is always
safe, because painting happens long after attachment. Reading them in a
constructor is not.

That second case only bites when the result is *cached*, and the caches are
easy to miss:

| cached thing | how it gets stale |
|---|---|
| a `Drawable` recoloured with `replaceColour` | holds the colour it was given |
| a pre-rendered `Image`, a `Path` with a stored fill | same |
| a `Slider`'s **layout** | `getSliderLayout` is asked once, in `resized()`; `Slider::lookAndFeelChanged` rebuilds only the text box, so call `slider.resized()` |

Refresh any of these when the LookAndFeel becomes available, and override
**both** `lookAndFeelChanged()` and `parentHierarchyChanged()` to do it. Neither
alone is enough: attaching to an already-styled parent sends
`parentHierarchyChanged` but no look-and-feel change, and restyling in place
does the opposite.

The same trap fires in reverse during teardown, when an owner's
`setLookAndFeel (nullptr)` sends a look-and-feel change to children that can no
longer resolve anything. Guard with `LookAndFeel::isColourSpecified`, which
answers without asserting.

### An override the LookAndFeel has to own

Some `LookAndFeel` methods ask whether a colour is specified and then read it
from somewhere else. `LookAndFeel_V2::drawTabButtonText` is the clearest case:

```cpp
if (button.isFrontTab() && (button.isColourSpecified (frontTextColourId)
                                || isColourSpecified (frontTextColourId)))
    col = findColour (frontTextColourId);          // ← the LookAndFeel's, either way
else if (…)
else
    col = button.getTabBackgroundColour().contrasting();
```

`isColourSpecified` and `findColour` there are unqualified, so both are the
**LookAndFeel's**. Setting the id on the button satisfies the condition and then
fetches a colour the LookAndFeel never had. The override belongs on the
LookAndFeel and nowhere else.

The fallback is the other half of it. With nothing specified anywhere the text
is `getTabBackgroundColour().contrasting()` — and a tab given
`Colours::transparentBlack`, the natural choice when the panel behind should
show through, contrasts as though it were black. The label comes out white and
vanishes on a light scheme.

Before relying on a `drawX` default, read its fallback. `contrasting()` applied
to a colour that was never meant to be seen is a common way to get one.

### Widgets the scheme does not reach

`LookAndFeel_V4`'s constructor walks a list of colour ids and assigns them from
the four scheme colours. **Ids not on that list keep whatever `LookAndFeel_V2`
gave them**, which is a literal from 2012 and has no relation to the scheme. The
symptom is one widget in the wrong palette while everything around it follows the
theme, and it will not be fixed by choosing a different scheme.

`TableHeaderComponent` is the clearest case: `textColourId`, `backgroundColourId`,
`outlineColourId` and `highlightColourId` are all set in `V2` and none are
revisited in `V4`, so a themed app has to `setColour` them itself. Grep
`LookAndFeel_V4::initialiseColours` for the id in question before assuming the
scheme covers it.

Two of that class's `drawX` methods go further and ignore their own ids:

- `drawTableHeaderBackground` fills the lower half from `backgroundColourId` but
  paints the **top half `Colours::white` literally**, so a dark header comes out
  with a white band across it.
- `drawTableHeaderColumn` draws the sort arrow at a hardcoded `0x99000000` —
  translucent black, invisible on any dark header, and unaffected by
  `textColourId`.

Neither can be reached with `setColour`; both need the method overridden. This is
the general shape of the problem, not a fact about tables: when a colour will not
move, read the `V2` implementation before believing the id is wrong.

## Overriding a LookAndFeel

**An override applies to every widget of that type in the whole project, not to
the one you wrote it for.** This is obvious stated plainly and very easy to miss
in practice, because an override written while there is only one slider on
screen is indistinguishable from a correct one until the second slider arrives —
possibly months later, in a different page, and the symptom is that the new
widget draws *nothing*.

Scope the override by asking the widget what it is:

```cpp
juce::Slider::SliderLayout MyLookAndFeel::getSliderLayout (juce::Slider& slider)
{
    // Only the vertical fader wants the treatment below.
    if (slider.getSliderStyle() != juce::Slider::LinearVertical)
        return LookAndFeel_V4::getSliderLayout (slider);
    ...
}
```

The same applies to `drawLinearSlider`, `getTextButtonFont`, `drawButtonBackground`
and the rest: branch on the style, or on something the widget carries, and hand
everything else back to the base class. A real case: a `getSliderLayout` written
to give a fader its full height also discarded the *text box* of every slider,
which was invisible until a slider that needed one was added, and then looked
like a broken widget rather than a look-and-feel bug.

The commonest reason to reach for one of these is text that will not fit its
widget, which is `getTextButtonFont` — see [fonts](fonts.md#fitting-text-to-a-widget),
and note that everything above applies to it.
