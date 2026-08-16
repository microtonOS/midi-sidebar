---
name: juce-ui
description: Create a GUI in JUCE by adding widgets in a grid/flexbox layout (following the signal flow) as well as adding menus and popup windows. Some examples include knobs, sliders, buttons, toggles; context menus, sidebars, and sidepanels; popup windows for loading and saving files as well as custom windows. Edit colour palettes and fonts and other designs and customizations of various elements.
allowed-tools: WebFetch(domain:docs.juce.com) WebFetch(domain:forum.juce.com)
---

# JUCE UI

Iterate with continuous feedback from the user.
This is meant as a reusable skill for various JUCE projects, so the details of the user feedback may vary.
It can make sense to update the skills file depending on the feedback from the user—ask to do this if something is missing or inconsistent and it is general enough to extend to other projects.

## References

This file is the whole of what you need to *start*. The references below are for
the questions that come up once you are working; open one when its line
describes what you are doing, not before.

| open | when you are |
|---|---|
| [layout](references/layout.md) | deciding where things go — grids, columns, spans, sizing tracks, and the two ways a `Grid` fails without saying so |
| [widgets](references/widgets.md) | choosing or wiring a control — sliders, buttons, tables, text and menus, and what each gets wrong by default |
| [look and feel](references/look-and-feel.md) | deciding how it looks — the colour scheme, custom `ColourId`s, and when an override belongs on the LookAndFeel |
| [fonts](references/fonts.md) | picking or sizing type, or a label will not fit |
| [animation](references/animation.md) | moving something over time, or something you laid out will not stay where you put it |
| [popups](references/popups.md) | putting something in front — menus, call-out boxes, file choosers, tooltips |
| [versions](references/juce-versions.md) | a GUI that compiled before a JUCE bump does not compile after it |

One more lives in the **juce-review** skill and is worth knowing about from here,
because several of its faults present as GUI faults:
[juce-violations](../juce-review/references/juce-violations.md) — the crash
before `main`, and the traps a JUCE module's unity build sets.

The [Gotchas](#gotchas) list near the end of this file is the other way in: it is
indexed by *symptom*, for when you can see what is wrong but not what caused it.

## Files

Decide the file structure when you create the first file, not once it has grown.
A GUI that is not split up from the start becomes one enormous file, and then
nobody — including you — can see what is in it.

One file for each of:
- the look and feel: every colour, font and size constant (see Layout mechanics
  rule 1), and the `LookAndFeel` subclass that uses them;
- each custom widget;
- each page, panel or window;
- the editor that owns the pages.

Small widgets can be header-only; that is normal in JUCE and both of the
reference plugins below do it. Put them under a `ui/` or `gui/` directory rather
than beside the processor.

If a GUI file passes roughly 500 lines it contains a page or a widget that wants
extracting. Treat that as the trigger, not a suggestion.

For scale: RippleRX's entire UI is about 30 kB across 12 files in `src/ui/`;
tuneBfree's is 146 kB in a single `PluginEditor.cpp`. Same framework, comparable
plugin. The split version is also what makes `layout_lint.py` output usable,
since each report then names a file you can hold in your head.

## Workflow

Start out with a skeletal GUI and then add more details step by step:

1. Create a "look and feel" file for global design choices such as colour palettes, widget ratios and sizes, fonts etc. As we start with the JUCE default choices this file will be mostly empty and filled out little by little. Make sure the `LookAndFeel (Dark)` colour theme is the initialized colour palette. This is the one file every layout constant belongs in — see [Files](#files). Also decide now whether the editor will be resizable, because that changes what those constants mean; see [Resizing](#resizing).
2. In another file — one file per page, per [Files](#files) — create an empty page. <!-- I think that is MainComponent.cpp and .h in the GUI example. Not sure about the Audio example. Maybe check whats customary and update this. If the JUCE project already exists it may be something else --> Prepare the page for adding widgets later on by first setting up a layout tool. Use the `Grid` class. (Only use the `FlexBox` class if prompted by the user, and be prepared that `Grid` may change to `FlexBox` in future iterations.)
3. **Count the parameters against the controls before placing anything.** A spec
   that names four parameters and describes two controls has not said whether
   one control serves two parameters or two were forgotten, and *both readings
   produce a working panel* — which is why this has to be asked rather than
   decided. The same question arises wherever a mode switch appears: whether the
   controls it selects between are one set re-pointed or two sets shown at once
   is a fact about the instrument, not a layout choice. Ask; do not pick the
   tidier one. Then group them — see [Group before you place](references/layout.md#group-before-you-place).
4. Add the widgets for the variables the user wants exposed. Use the JUCE default designs for now. Make a best effort attempt to lay them out in a reasonable layout. Follow the aesthetic considerations in the [Layout](#layout) section below. Run the screenshot tool and check the output. Iterate if necessary. Ask for feedback on whether it is an acceptable first pass.
5. If the user is not satisfied, ask the user for a mockup. The designs in the mockup don't matter as we are still in the layout stage. By default, suggest that the user detail the mockup in either the docs (e.g. as markdown files containing html mockups or image mockups or natural language mockups) or a TODO file. The reason for doing it in the docs already is that the manual is half-done already. However, depending on the user and the agent, some other mockup method may be preferable, so take that into account as well.
6. Connect the widgets to the variables via the plugin state, i.e. the APVTS. Ask the user to try it out and iterate on the feedback.
7. When 6 is working. Ask if there is anything to finetune regarding the layout from step 5. If so, go back to step 5.
8. Ask the user whether they would like to add another page or window or panel and repeat steps 1 to 6 for that new addition, giving it its own file rather than extending an existing one. Ask whether they would instead want to develop the look and feel further.
9. Generate look and feel for all pages windows and panels. Make it beautiful according to the users preferences. If unstated, assume that the user want an elegant but simple design. Run the screenshot tool and check the output. Iterate if necessary. Ask the user for feedback and iterate. Make up a plan for what design features to add in which order so you get an iterative process going. Only do it all at once if the user asks you to.
10. As a final step, go over the code and see if it can be cleaned up, e.g.: Are there design variables that have been hardcoded into a specific widget rather than placed in the "look and feel" file(s)? Are there legacy names of variables and files that do no longer make sense? Are important motivations for decisions you have iterated on explained as comments in the code? Are there gotchas or other things that should be added to the skill file? Are there any problems with licensing that the user should be aware of? Any other relevant question you can think of?

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

   **One grid per page, not one per row.** This is the same rule and it is where
   it is usually broken. A grid built for a single row can only align things
   *within* that row; every alignment between rows is then arithmetic again, and
   the rule above has been obeyed to the letter and lost. Give the page one
   `Grid` whose columns are the finest division any row needs, and let each
   widget span a whole number of them — see [layout](references/layout.md#spans).

   ```cpp
   // Ragged. Each row divides its own width, so nothing lines up down the page,
   // and a row "split in half" is only halved by coincidence.
   layOutRow (rowA, { Px (labelW), Fr (1) },            { &label,  &field  });
   layOutRow (rowB, { Px (shortW), Fr (1), Px (w), Fr (1) }, { ... });

   // Declared. Six shared columns; a span of 3 is half the page in every row.
   place (programLabel, row, 1,         labelColumns);
   place (programField, row, 1 + labelColumns, half - labelColumns);
   place (bankLabel,    row, rightHalf, labelColumns);
   ```

   The giveaway that you are about to make this mistake: a `resized()` that
   constructs more than one `Grid`, or a per-row helper taking a list of track
   widths. Both mean the columns exist only inside a row.

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

7. **Snapshot before asking, and vary two things at once.** After any layout
   change, render with the tool in [scripts](scripts/README.md) and *look* at
   the results. Do not spend the user's attention on something you can see
   yourself.

   Vary size **and** state together rather than one at a time. Checking three
   sizes with the UI in a single state, then several states at a single size,
   proves less than it appears to: it misses everything that only breaks at an
   intersection, and an intersection is exactly where layout faults live,
   because that is where the content finally runs out of room.

   Faults concentrate at the minimum, so be thorough there and sparing
   elsewhere: render **every distinct state at the minimum size**, then the
   default and one large size in whichever state is busiest. States that differ
   only in their content and not in their geometry count as one. For most GUIs
   that is four or five images rather than a full matrix — cheap enough that
   there is no excuse for skipping it, and it is where the bugs are.

   **Then check alignment, which is a different test from fit.** "Nothing is
   clipped and everything fits" is what you will naturally look for, and a
   misalignment of ten or twenty pixels passes it easily — it reads as a design
   choice rather than as a fault. So look for it deliberately: trace a vertical
   line down the image at each column edge and confirm the same things start on
   it in every row. A row that should be halved must split at the page's
   midpoint, not near it. Where there is a mockup, check against the divisions
   *it* is drawn on — see [layout](references/layout.md#choosing-the-columns).

### Resizing

Hosts resize plugin editors and users expect to be able to. Decide whether the
editor is resizable in step 1, not later: the answer determines whether the
constants in the look and feel file are absolute pixels or values to be scaled,
and retrofitting means revisiting every one of them.

- **Set it up on the editor, with the constrainer as a member.**
  `setResizable (true, true)`, then `setResizeLimits (minW, minH, maxW, maxH)`,
  and `setFixedAspectRatio` if the design has one. JUCE keeps a pointer to the
  `ComponentBoundsConstrainer` and does not own it, so a local variable leaves a
  dangling pointer.

- **The minimum size is derived, not chosen.** It is the size at which the
  top-anchored and bottom-anchored content collides — mechanics rule 6. Add up
  the fixed track extents and use that. A round number picked by eye will be
  wrong in one direction or the other.

- **Persist the size in the processor's state, not the editor's.** The host
  saves and restores editor size, and the editor is destroyed and recreated
  every time the window is closed and reopened.

- **Reflow and scale are different designs; pick one and say which.** Reflowing
  re-solves the tracks and leaves text at its original size, so a bigger window
  shows more breathing room. Scaling multiplies everything including fonts, so a
  bigger window shows the same layout larger. Reflow is the better default;
  scaling suits skeuomorphic designs built on bitmap artwork.

- **If you scale, every constant becomes `constant * scale`.** This is the
  second reason for mechanics rule 6: a literal cannot be scaled. Pass the
  factor explicitly down the component tree rather than querying the display,
  which is unreliable inside a host.

- **`setBounds` bypasses the constrainer.** Only user drags and host-driven
  resizes go through it, so testing by calling `setBounds` proves nothing about
  what the user will experience.

- **Snapshot at the minimum, the default and the maximum.** This is mechanics
  rule 7 with concrete sizes to test:
  `snapshot.sh --target X -- --size 640x360 --name min`.

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

Every one of these is a fault whose symptom does not point at its cause, which
is why they are listed here rather than left to be found in the references: you
would not know which reference to open. Each entry is what you will actually be
looking at; follow the link once you recognise it.

- **Nothing runs at all, and the only output is JUCE's version banner.** A crash
  in *static initialisation*, before `main`. A `juce::String`, `StringArray` or
  `Identifier` at namespace scope — a table of widget names or parameter
  descriptors, say — is constructed before JUCE's own statics under some link
  orders, and dies on a string pool that does not exist yet. The banner is
  printed by a static initialiser too (`JuceVersionPrinter` in
  `juce_SystemStats.cpp`), so it is **not** evidence that `main` began; a
  `std::cerr` on its first line never printing confirms it. Return the table
  from a function holding a `static` local, so it is built on first use. Being
  order-dependent, this can start crashing when an unrelated file joins the
  build.

- **A knob has a number under it that nobody asked for.** A text box shows the
  value always and costs a row under every control; the bubble shows it while
  the control is being turned, which is when it is wanted.
  See [widgets](references/widgets.md#sliders).

- **A tab's label is invisible on a light scheme.** Its colour falls back to the
  tab's background `contrasting()`, and a transparent background contrasts as
  black. See [look and feel](references/look-and-feel.md#an-override-the-lookandfeel-has-to-own).

- **A slider shows `1999.99…` however few decimals you ask for.** A parameter
  attachment installs a `textFromValueFunction`, which outranks
  `setNumDecimalPlacesToDisplay`.
  See [widgets](references/widgets.md#sliders).

- **Non-ASCII text renders as `â€¦`.** A multi-byte UTF-8 literal reached
  `juce::String` as a bare `const char*`. Wrap every one:
  `juce::String (juce::CharPointer_UTF8 (s))`. Affects accented letters,
  fractions, dashes, ellipses, `∞` — anything you did not type on a US keyboard.

- **Everything renders black, or one widget does.** A custom `ColourId` was
  never registered, or was read before a LookAndFeel was reachable. Watch the
  log for the assertion even when the picture looks plausible; black on a dark
  background is easy to miss. See [look and feel](references/look-and-feel.md#colours).

- **A pop-up looks like a different application.** A `CallOutBox` launched onto
  the desktop does not inherit your LookAndFeel.
  See [popups](references/popups.md#calloutbox).

- **A fader is shorter than the meter beside it.** `getSliderLayout` insets a
  vertical slider by the thumb radius before any drawing happens.
  See [widgets](references/widgets.md#sliders).

- **A fader's fill looks right but drags the wrong way.** An inverted fader
  needs a reversed range, not a mirrored paint.
  See [widgets](references/widgets.md#sliders).

- **A slider's value bubble is invisible in one theme.** A JUCE bug, not your
  call site: the bubble's background and its text come from two unrelated colour
  pairs, which collide in `LookAndFeel_V4`'s Light scheme.
  See [widgets](references/widgets.md#sliders).

- **A `TextEditor`'s text is invisible after a theme change.** Its colour is
  baked in when the text is inserted, and `lookAndFeelChanged` does not revisit
  it. See [widgets](references/widgets.md#text-and-menus).

- **A widget that was fine draws nothing once a second one appears.** A
  `LookAndFeel` override written for one widget applies to every widget of that
  type. See [look and feel](references/look-and-feel.md#overriding-a-lookandfeel).

- **A whole component is missing, and `setVisible` is not the reason.** A `Grid`
  auto-placed it into an implicit row past the ones you declared — two things
  sharing a cell need explicit areas.
  See [layout](references/layout.md#two-ways-a-grid-fails-quietly).

- **Identical knobs come out different sizes.** A rotary slider's radius is
  `jmin (width, height)`, so the cell's aspect ratio decides it, not its area.
  See [widgets](references/widgets.md#sliders).

- **A short button label turns into "F...".** `TextButton` does not shrink its
  font to fit. See [widgets](references/widgets.md#buttons).

- **One widget keeps JUCE's grey while everything around it follows the theme.**
  Its colour ids are set by `LookAndFeel_V2` and never revisited by
  `LookAndFeel_V4`, so no scheme will move them — `TableHeaderComponent` is the
  usual one. Two of its `drawX` methods go further and ignore their own ids: a
  white band across the top of the header, and a sort arrow at a hardcoded
  translucent black. See
  [look and feel](references/look-and-feel.md#widgets-the-scheme-does-not-reach).

- **A new element is the right colour and still invisible.** Its shade was
  derived with `contrasting()` from the wrong sibling and landed on the
  neighbour's own colour. `widgetBackground` is darker than `windowBackground` in
  three of JUCE's four schemes and lighter in the fourth, so "away from this one"
  is not "away from the other one". See [look and feel](references/look-and-feel.md#colours).

- **A menu item's icon insists on being to the left of the text.**
  `PopupMenu::Item::image` draws in the tick's gutter and takes no justification.
  A mark on the other side needs a `PopupMenu::CustomComponent` that calls
  `drawPopupMenuItem` itself. See
  [popups](references/popups.md#an-items-image-goes-in-the-left-gutter-and-only-there).

- **An icon drawn on a button is fine until the button narrows, then the label
  slides over it.** `TextButton` centres its text, so it spreads outwards into
  whatever you drew at a fixed inset. Measure it.
  See [widgets](references/widgets.md#buttons).

- **A label sits where the layout put it while its control moves away.** A label
  attached with `attachToComponent` follows its owner and must not also be placed
  in the grid. See [widgets](references/widgets.md#buttons).

- **A button is there, is labelled, and nothing can find it by name.** It was
  default-constructed and labelled with `setButtonText`, which sets the text but
  not the *component* name. Accessibility, name lookups and the snapshot tool's
  `--click` all go by the name. See [widgets](references/widgets.md#buttons).

- **A column header will not go back to unsorted, or its title elides the moment
  it becomes sortable.** `columnClicked` only ever toggles ascending and
  descending, and the arrow takes `height / 2` off the title's width.
  See [widgets](references/widgets.md#sortable-columns-and-the-header-as-a-component).

- **Two files that do not include each other collide.** A JUCE module is a unity
  build, so a file-scope `using namespace` in one `.cpp` is still in force in the
  next. See
  [juce-review](../juce-review/references/juce-violations.md#a-juce-module-is-a-unity-build).

- **`make_unique<T>()` fails with the deleted copy constructor as the candidate.**
  `JUCE_DECLARE_NON_COPYABLE` user-declares constructors, which suppresses the
  implicit default one. Add `T() = default;`. See
  [juce-review](../juce-review/references/juce-violations.md#juce_declare_non_copyable-removes-the-default-constructor).

- **Controls vanish after resizing the window.** `setBounds` loses to a running
  `ComponentAnimator`. See [animation](references/animation.md#jucecomponentanimator--animating-bounds).

- **A label is the right size and the wrong shape.** `drawFittedText` squashes
  text horizontally before it breaks or truncates it, so it reads as a condensed
  typeface rather than as text that did not fit.
  See [fonts](references/fonts.md#fitting-text-to-a-widget).

- **A wall of unrelated errors around icons after a JUCE bump.** Something the
  GUI relies on moved between releases — `Drawable` ceasing to be a `Component`
  in JUCE 9 is the expensive one, and `juce::Font (float)`'s deprecation in
  JUCE 8 the common one. See [versions](references/juce-versions.md).


## Resources

### Documentation
There are numerous examples in `JUCE/examples/GUI`.
Documentation is available at [docs](https://docs.juce.com/master/).
There is also the [forum](https://forum.juce.com/) for further discussions.


### Tools
To look at the GUI you have built, use the snapshot tool in
[scripts](scripts/README.md). It renders the editor to a PNG in software — no
window, no screen capture — so it works headless and inside a sandbox. Never use
the operating system's screenshot utility.


### Role Models
tiagolr's RipplerX ([GitHub](https://github.com/tiagolr/ripplerx)) is a beautiful opensource plugin in the simple but elegant style.
The Surge Synth Team's [Ob-Xf](https://surge-synth-team.org/ob-xf/) ([GitHub](https://github.com/surge-synthesizer/OB-Xf)) is a good example in the more skeuomorphic style.
(Surge XT itself is not a good role model for a GUI even though some UI elements are good.)
Read for ideas if you get stuck in a loop where you and the user struggle to communicate UI ideas, but note that both are under GPL-3.0 licenses (or later for OB-Xf).
