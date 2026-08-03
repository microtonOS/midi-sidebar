---
name: juce-gui
description: Create a GUI in JUCE by adding widgets in a grid/flexbox layout (following the signal flow) as well as adding menus and popup windows. Some examples include knobs, sliders, buttons, toggles; context menus, sidebars, and sidepanels; popup windows for loading and saving files as well as custom windows. Edit colour palettes and fonts and other designs and customizations of various elements.
allowed-tools: WebFetch(domain:docs.juce.com) WebFetch(domain:forum.juce.com)
---

# JUCE GUI

Iterate with continuous feedback from the user.
This is meant as a reusable skill for various JUCE projects, so the details of the user feedback may vary.
It can make sense to update the skills file depending on the feedback from the user—ask to do this if something is missing or inconsistent and it is general enough to extend to other projects.
Start out with a skeletal GUI and then add more details step by step:

1. Create a "look and feel" file for global design choices such as colour palettes, widget ratios and sizes, fonts etc. As we start with the JUCE default choices this file will be mostly empty and filled out little by little. Make sure the `LookAndFeel (Dark)` colour theme is the initialized colour palette.
2. In another file, create an empty page. <!-- I think that is MainComponent.cpp and .h in the GUI example. Not sure about the Audio example. Maybe check whats customary and update this. If the JUCE project already exists it may be something else --> Prepare the page for adding widgets later on by first setting up a layout tool. Use the `Grid` class. (Only use the `FlexBox` class if prompted by the user, and be prepared that `Grid` may change to `FlexBox` in future iterations.)
3. Add the widgets for the variables the user wants exposed. Use the JUCE default designs for now. Make a best effort attempt to lay them out in a reasonable layout. Follow the aesthetic considerations in the [Layout](#layout) section below. Run the screenshot tool and check the output. Iterate if necessary. Ask for feedback on whether it is an acceptable first pass.
4. If the user is not satisfied, ask the user for a mockup. The designs in the mockup don't matter as we are still in the layout stage. By default, suggest that the user detail the mockup in either the docs (e.g. as markdown files containing html mockups or image mockups or natural language mockups) or a TODO file. The reason for doing it in the docs already is that the manual is half-done already. However, depending on the user and the agent, some other mockup method may be preferable, so take that into account as well.
5. Connect the widgets to the variables via the plugin state, i.e. the APVTS. Ask the user to try it out and iterate on the feedback.
6. When 5 is working. Ask if there is anything to finetune regarding the layout from step 4. If so, go back to step 4.
7. Ask the user whether they would like to add another page or window or panel and repeat steps 1 to 5 for that new addition. Ask whether they would instead want to develop the look and feel further.
8. Generate look and feel for all pages windows and panels. Make it beautiful according to the users preferences. If unstated, assume that the user want an elegant but simple design. Run the screenshot tool and check the output. Iterate if necessary. Ask the user for feedback and iterate. Make up a plan for what design features to add in which order so you get an iterative process going. Only do it all at once if the user asks you to.
9. As a final step, go over the code and see if it can be cleaned up, e.g.: Are there design variables that have been hardcoded into a specific widget rather than placed in the "look and feel" file(s)? Are there legacy names of variables and files that do no longer make sense? Are important motivations for decisions you have iterated on explained as comments in the code? Are there gotchas or other things that should be added to the skill file? Are there any problems with licensing that the user should be aware of? Any other relevant question you can think of?

The exact ordering of these points may vary a bit from one project to another.
In addition, the user may accept a suboptimal result and then at a later time point go back and iterate more on earlier steps.

## Layout

Try to place the widgets so that their associated function follow the signal flow from left to right.

Avoid unnecessary empty grid cells/flexboxes. If unavoidable, try again with the layout design and see if it really is unavoidable. If it really is unavoidable follow these priorities:
1. Empty spaces at the edges of the page or window is worse than empty grid cells in the middle. 
2. Top and left empty space is worse than bottom and right.

Note that having some grid cells/flexboxes that are considerably airier than others is also bad even though that is a lesser evil.

Widgets should have some kind of descriptive text or symbol.
For a row of widgets the text labels above (or possibly below) should be aligned.
Likewise for a column of widgets.

Widgets with related functions should be placed in groups.
There is a specific `GroupComponent` class for this which you should use by default.
Later, during the GUI finetuning phase, this may be replaced by a custom grouping component.

### Mechanics

Everything above describes what a good layout looks like. These rules are how
you produce one, and — more importantly — how it survives being edited later.
Layout is the part of a GUI you cannot check by reading the code: the result
emerges from arithmetic spread across many statements, interacting with a window
size that is unknown when the code is written. Assume you cannot verify a layout
by inspection, and follow rules that make the invariants structural instead.

1. **Layout constants live in the look and feel file, never as locals in
   `resized()`.** If two pages need the same measurement, there must be exactly
   one definition of it. A constant defined inside one `resized()` and copied
   into another, however carefully commented, will drift apart.

2. **Alignment is declared, not computed.** If two widgets must line up, put
   them in the same `Grid` row or column and let the layout hold them there.
   Never derive a shared coordinate arithmetically — an expression that happens
   to evaluate to the same number as another expression is a coincidence, not a
   constraint, and it breaks silently when either side changes.

3. **Never divide a region by an item count.** `int cellW = area.getWidth() / n`
   followed by `n` fixed-width cells truncates: the leftover 0..n-1 pixels are
   abandoned at one edge, and the size of that gap changes as the window
   resizes. Use `n` equal flexible tracks (`Grid::Fr`) and let the layout
   distribute the remainder.

4. **A compound widget cannot align with anything outside itself.** A component
   that owns a caption, a control and a value read-out lays them out relative to
   its *own* bounds, so two instances with different bounds put their captions at
   different heights and the parent cannot correct it. Use one only where it will
   never need to align across columns. Where captions must align along a row,
   make caption, control and value three rows of the parent grid instead.

5. **A rotary slider draws at `jmin (width, height)`** of what is left after its
   caption and text box are removed. A non-square cell therefore yields a smaller
   knob than its width suggests, and two pages with different cell shapes get
   visibly different knobs. If knobs must match, the remaining area must be
   square and its size must come from the shared constant.

6. **No unnamed numeric literal in `resized()` beyond 0, 1 and 2.** Every other
   number is a design decision and belongs in the look and feel file with a name.
   This is the rule that decides whether the next edit is safe: an unnamed number
   cannot be found, cannot be reused, and gives no clue whether it was chosen or
   left over. A `resized()` with dozens of bare numbers has already lost, however
   good it looks right now.

   It is also the only rule here that can be checked mechanically, so check it
   rather than trusting your own care:

   ```
   python3 scripts/layout_lint.py <source-dir>
   ```

   That also reports rules 1 and 3. Run it after every layout change and before
   handing back to the user. Suppress a deliberate exception on its own line
   with a trailing `// layout-lint: allow`, and use `--max <n>` to hold an
   existing codebase to its current count while you bring it down.

7. **Snapshot before asking.** After any layout change, render at minimum,
   default and large window sizes with the tool in [scripts](scripts/README.md)
   and look at the results. Most layout faults appear at one end of the size
   range only. Do not spend the user's attention on something you can see
   yourself.

## Design

The design should be both beautiful and functional/self-explanatory.
By default, assume that the user wants a simple and elegant design.
If they want lots of skeuomorphics and flourishes, they have to ask for it.
This should also keep the code tidy as fewer variables have to be set.

In the simple and elegant vein:
- The colour palette should be limited to a few colours. The default colour scheme in look and feel has 9 colours so start with that. It should never have to be more than double of that.
- There should only be a small number of widget designs. It is acceptable to go very minimal with just one kind of slider design and one kind of button design. You should aim for no more than 3 kinds of knobs, 3 kinds of sliders, 3 kinds of toggle buttons, and 3 kinds of radio buttons, 3 fonts, 2 kinds of combo boxes, 2 kinds of text boxes. It may exceed this but then discuss it with the user. Note that a design here refers to both how the widgets are coloured/drawn and how big they are.
- Do not use unnecessary text. For instance value+unit of different variables can appear in a popup display whenever changed rather than constantly being shown.


In the functional/self-explanatory vein:
- Name the widgets with easy to understand but short names. By default put names above widgets. Putting it below could also be a respectable choice. Putting it besides is harder if you use widgets such as knobs or vertical sliders as you get a lot of unnecessary empty space.
- If two widgets differ in design, it should be because they are functionally distinct somehow or that they are somehow labelled in different ways.
   - E.g. a continuous variable and a discrete variable are obviously very different and can have different designs.
   - E.g. a variable controlled by a sustain or expression pedal may have a different appearance to a generic controller.
   - E.g. sometimes you may give a variable a name. At other times, it may be difficult to name the variable, but you can name the minimum value and the maximum value because you are interpolating between the two.
- Ideally, design choices should be consistent across pages. If impossible, then because of layout issues you may have different size parameters for different pages.


## Gotchas
- **UTF-8 string literals get mangled.** Passing a `const char*` with multi-byte
   UTF-8 (the ' fractions ⅓⅔⅗, em-dash —, ellipsis …, middle-dot ·) straight to
   `juce::String`/`setText`/`setButtonText` renders garbage like `â€¦`. Wrap every
   non-ASCII literal: `juce::String (juce::CharPointer_UTF8 (s))`. We keep a
   `utf8()` helper and store the bytes as hex escapes (e.g. `"\xe2\x85\x93"`).

- **`juce::Font(float)` is deprecated in JUCE 8.** Use
   `juce::Font (juce::FontOptions().withName("Futura").withHeight(h).withStyle("Bold"))`.
   We wrap this in `uiFont(size, bold)`. One font family, two weights → satisfies
   "≤3 fonts". Century-Gothic-like geometric sans (Futura on macOS; bundle a .ttf
   for the RPi build later).

- **Small buttons clip their text.** A `TextButton` does not shrink its font to
   fit, so "FAST" renders as "F...". Override `getTextButtonFont` in the
   LookAndFeel to scale the font to the button height, e.g.
   `uiFont (jmin (12.0f, (float) buttonHeight * 0.5f))`. Check every short-label
   button after a layout change, because the clipping only appears below a
   threshold width.

- **An inverted fader needs a reversed range, not a mirrored paint.** For a
   fader whose maximum sits at the *bottom* (a Hammond drawbar pulled out, a
   fader that reads top-down), painting the fill upside-down in
   `drawLinearSlider` looks right but leaves the mouse dragging the wrong way.
   Give the slider a reversed `NormalisableRange` instead — proportion 0 maps to
   the maximum value — so direction and fill agree by construction.

- **A rotary slider's drawn radius comes from jmin (width, height) of its bounds**, so cells of differing aspect ratio give differently-sized knobs even at equal area. Fix this in the layout, never at the widget: give the knob's grid track a fixed size, or GridItem (knob).withSize (kKnob, kKnob). Do not pass a different hand-written rectangle to each knob's setBounds.

- **JUCE 9: `Drawable` no longer derives from `Component`.** It moved into
   `juce_graphics`, so code that added a `Drawable` as a child component, or
   called `Component` methods on one, no longer compiles. Wrap it in the new
   `DrawableComponent`. Check this first when porting GUI code from JUCE 8.
   JUCE 9 also adds variable fonts, so a single variable family can supply
   several weights — that makes the "≤3 fonts" rule easier to keep, and is the
   tidier way to bundle a face for the RPi build.


## Resources
There are numerous examples in `JUCE/examples/GUI`.
Documentation is available at [docs](https://docs.juce.com/master/).
There is also the [forum](https://forum.juce.com/) for further discussions.

To look at the GUI you have built, use the snapshot tool in
[scripts](scripts/README.md). It renders the editor to a PNG in software — no
window, no screen capture — so it works headless and inside a sandbox. Never use
the operating system's screenshot utility.