# Design

This is a deliberately restricted subset of particularly important design tools.

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

## Fonts

`juce::Font (float)` is deprecated as of JUCE 8. Use `FontOptions`:

```cpp
juce::Font (juce::FontOptions().withHeight (h).withStyle ("Bold"))
```

Leaving the typeface name unset uses the platform's default sans, which is the
right starting point until the design says otherwise.

JUCE 9 adds variable fonts, so one family can supply several weights by setting
axes rather than by shipping a file per weight. That makes the "at most three
fonts" budget easier to keep, and is the tidier way to bundle a face for a build
that cannot rely on a system font being present.

## Animations

[Example of animations](https://github.com/juce-framework/JUCE/blob/master/examples/GUI/AnimationEasingDemo.h).

### `juce::Animator` Class
Wrapper class for managing the lifetime of all the different animator kinds created through the builder classes.

It uses reference counting. If you copy an Animator the resulting object will refer to the same underlying instance, and the underlying instance is guaranteed to remain valid for as long as you have an Animator object referencing it.

An Animator object can be registered with the AnimatorUpdater, which only stores a weak reference to the underlying instance. If an AnimatorUpdater references the underlying instance and it becomes deleted due to all Animator objects being deleted, the updater will automatically remove it from its queue, so manually removing it is not required.

See also
ValueAnimatorBuilder, AnimatorSetBuilder, AnimatorUpdater, VBlankAnimatorUpdater

### `juce::ComponentAnimator` — animating bounds

The older, simpler animator, reached through
`Desktop::getInstance().getAnimator()`. It moves a component towards a target
rectangle over a duration, and is the easy way to slide a panel in and out.

While it is running it keeps driving that component every frame, so **any
`setBounds` you perform meanwhile is silently overwritten** on the next frame.
There is no error and no warning.

The usual way in is a window resize landing mid-animation: `resized()` lays
everything out correctly for the new size, the animator immediately restores the
geometry it was aiming at, and the component is left laid out for a window size
that no longer exists. Children then get positioned outside the visible area and
disappear. It presents as "my buttons vanished", not as an animation problem,
which is what makes it expensive to find.

Cancel before laying out directly:

```cpp
auto& animator = juce::Desktop::getInstance().getAnimator();

if (animated)
{
    animator.animateComponent (&child, target, 1.0f, ms, false, 1.0, 1.0);
}
else
{
    animator.cancelAnimation (&child, false);   // false = do not jump to the end
    child.setBounds (target);
}
```

`animator.isAnimating (&child)` is available if you would rather branch. Compute
the animation's target from current bounds too, or you will animate towards a
rectangle that was already stale when you asked for it.

