# Fonts

## `FontOptions`, not `Font (float)`

`juce::Font (float)` is deprecated as of JUCE 8 — see
[versions](juce-versions.md). Use `FontOptions`:

```cpp
juce::Font (juce::FontOptions().withHeight (h).withStyle ("Bold"))
```

Leaving the typeface name unset uses the platform's default sans, which is the
right starting point until the design says otherwise.

## Variable fonts and the budget

The skill's widget-design budget allows **three fonts**, alongside the limits on
knob, slider and button designs — see the budget in `SKILL.md`. Fewer is usually
better: a face for headings, a face for body text, and at most one more for
anything that has to look mechanical, such as a value read-out.

JUCE 9 adds variable fonts, so one family can supply several weights by setting
axes rather than by shipping a file per weight. That makes the budget easier to
keep, and is the tidier way to bundle a face for a build that cannot rely on a
system font being present.

## Fitting text to a widget

JUCE widgets do not shrink their font to fit. A `TextButton` clips `FAST` to
`F...` rather than setting it smaller. The fix is a `getTextButtonFont` override
in the LookAndFeel scaling the height to the button —
`font (jmin (12.0f, (float) buttonHeight * 0.5f))`. It is a font answer to a
layout symptom, so it is described where the symptom appears:
[widgets](widgets.md#buttons).

That override, like every other, applies to **every** button of that type in the
project. Read [look and feel](look-and-feel.md#overriding-a-lookandfeel) before writing one.

Where only one piece of text needs it, `Graphics::drawFittedText` is the
per-call alternative. Note what it actually does when the text is too big, in
this order: squash it horizontally down to `minimumHorizontalScale`, break it
over up to `maximumNumberOfLines` lines, and only then truncate with an ellipsis.
Horizontally squashed text at a small size is hard to spot in a screenshot and
looks like a different typeface; pass `1.0f` to forbid the squashing and get an
honest ellipsis instead.
