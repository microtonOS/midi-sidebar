# What changed between JUCE versions

An index, not an explanation. It exists for the moment a GUI that compiled last
month stops compiling, or draws differently, after a JUCE bump — when you know
the symptom but not which reference to open. Each row says what moved and points
at the file that covers it.

Check the version you are actually on with `JUCE_MAJOR_VERSION` in
`juce_core/system/juce_StandardHeader.h`; every module's `version:` field reports
the framework's version, not its own age, so it cannot tell you when a module was
introduced.

| change | in | where it is covered |
|---|---|---|
| `juce::Font (float)` deprecated in favour of `FontOptions` | 8 | [fonts](fonts.md#fontoptions-not-font-float) |
| variable fonts — one family, several weights by axis | 9 | [fonts](fonts.md#variable-fonts-and-the-budget) |
| the `juce_animation` module: `Animator`, the builder classes, `AnimatorUpdater`. `ComponentAnimator` is unchanged and still the one for bounds | 8.x | [animation](animation.md) |
| `Drawable` moved to `juce_graphics` and **stopped being a `Component`** | 9 | [widgets](widgets.md#drawables-and-icon-buttons) |

## The one that costs a morning

`Drawable` is the expensive one, because the error is in the wrong place. Porting
a GUI from JUCE 8 gives errors at `addAndMakeVisible (drawable)` and at every
`setBounds` on one — a list of unrelated-looking failures across every file that
draws an icon, none of which mentions the base class that went away.

`DrawableButton` still takes `Drawable`s directly, so a rail of icon buttons
needs no change at all. It is only code that treated a `Drawable` as a child
component that has to wrap it in a `DrawableComponent`. Check this first when a
port produces a wall of errors around icons.
