# Demo

Used for testing the GUI without embedding the sidebar in a plugin.
A number of widgets (where the host plugin is supposed to be) makes it possible to test different developer settings.

Buttons for:

- Changing the theme between look and feel V4 Dark, Midnight, Grey, and Light.
- Forcing the text in slider value bubbles white or black, or leaving it as the
  theme gives it. See [Bubble text](#bubble-text).
- Changing whether the sidebar is on the left or the right.

The two colour settings are adjacent because they are read against each other:
the bug the second one works around only shows up in one of the themes.

## The area the widgets live in

The outlined rectangle marked *host plugin content* is where the plugin
embedding the sidebar would draw its own UI. It is there for its own sake — it
is what makes the sidebar's [overlaying](sidebar.md#placement-and-expansion)
visible, since a panel that covers nothing looks the same as one that pushes
content aside — and the developer settings go inside it because that rectangle
is exactly the space this project does not own.

Nothing in it is part of the sidebar. The first two settings are ones a real
plugin makes once, in code: `Sidebar::setEdge` and whichever colour scheme it
hands to `SidebarLookAndFeel::setScheme`. The demo turns them into buttons
because seeing all four themes should not mean four rebuilds.

Each setting is a label and a row of buttons, all three the same shape, so the
labels and the button edges line up because they are the same grid tracks. There
are deliberately no `GroupComponent` frames: each setting is a single control,
and a group of one is a label with a box drawn round it.

### Adding a fourth setting

In a second **column**, not on the end of the list. The editor has to go down to
the sidebar's own minimum height — that is the size at which the rail most needs
testing — and at that height these rows have 140px between the caption and the
bottom, which is exactly three of them. Width, by contrast, is free: the demo's
minimum *width* is derived from the controls themselves and tests nothing about
the module, so it can move whenever the controls need it to.

## Bubble text

Not a sidebar setting — a switch for a JUCE bug, and it lives here because the
demo is where a theme gets switched, which is the only place the bug can be
seen.

A slider's value bubble takes its background from
`BubbleComponent::backgroundColourId` (`widgetBackground`) and its text from
`TooltipWindow::textColourId` (`highlightedText`). Those are halves of two
different pairs, and in the Light scheme both are white, so the volume fader's
read-out disappears while you drag it. It reproduces in JUCE's own DemoRunner,
so it is not something this project caused or can properly fix.

| choice | what it does |
|---|---|
| Default | nothing — stock JUCE, so the bug is visible |
| White | `setColour (TooltipWindow::textColourId, white)` |
| Black | the same with black |

*Default* is first on purpose: a demo that could only hide the bug would be less
useful than one that can show it.

The override is applied to the **editor**, not to the fader. `paintContent`
resolves the colour with `findColour (id, true)`, and that `true` walks the
parent chain — so one call at the top covers every bubble in the plugin, which
is how a real plugin would apply the workaround. The juce-ui skill's
[Sliders reference](../.claude/skills/juce-ui/references/widgets.md#sliders) has
the full mechanism.

## Sample tuning values

The demo also fills the [controllers page](controllers.md) with the two mappings
and three monitor lines from that document's figures, and two parameters with
different units so the editing table's limits can be seen relabelling themselves.

It fills the [tuning page](tuning.md) with fixed values too, mostly the
ones from that document's own sketch, so the page can be seen populated rather
than empty. The period is the exception: it is given as *inferred* with the
12edo candidates, 100c to 1500c, so that the chooser has something to step
through — `specified` would show the duller half of that widget, one value with
the buttons disabled. They are set once in `DemoEditor::showSampleTuning` and nothing changes
them: there is no simulation behind the clock, and none of the page's callbacks
are connected to anything yet.

That is the next piece of work, and it belongs with the MIDI side rather than
here. When it arrives the page takes its values through the same setters, so
this can simply be deleted.

## Settings are parameters

Each setting is an `AudioParameterChoice` on the demo processor, mirrored to its
buttons in both directions, for the same reasons the open page is one:

- it survives the editor being closed and is saved with the session;
- a host can automate it;
- the [snapshot tool](../.claude/skills/juce-ui/scripts/README.md) can set it
  from the command line, so a state can be rendered without anyone clicking
  their way to it.

```bash
snapshot.sh --target SidebarDemo_snapshot -- --param theme=Light --param edge=Right
```

The choice lists live in one place, `demo/DemoSettings.h`, so the index the
parameter stores and the index the buttons use cannot come to mean two different
things.

<!-- The four themes are JUCE's own (LookAndFeel_V4::getDarkColourScheme and
friends) rather than palettes of ours. The module derives all of its colours
from the nine in whichever scheme it is given, so this is a real test of that
claim: if a theme comes out wrong, a colour has been hardcoded somewhere it
should not be. That is worth more than the ability to switch themes. -->

<!-- Two settings that could join these later, in rough order of usefulness:
the animation speed (Sidebar::setAnimationMilliseconds, currently only settable
in code, and 0 is a legitimate value worth being able to try), and a level
generator so the meter has something to show without routing audio in. Neither
is needed to look at the layout, which is what the demo is for now. -->


MUCH OF THE ABOVE IS AI GENERATED WITHOUT AN APPROPRIATE DOCS EDITING SKILL.

## Demo plugin

A demo plugin is an alternative page to the above.
It is a simple combo organ/subtractive synthesizer.
It has:
- One oscillator with a 3-way switch between saw, triangle, and square.
- One filter with cutoff (frequency) and resonance (Q-value).
- One LFO with target 2-way switch for filter and pitch. One rate control and one intensity control. Filter LFO rate, filter LFO intensity, pitch LFO rate, pitch LFO intensity, and LFO target are all different parameters.