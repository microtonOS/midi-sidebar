# Animation

JUCE has two animation systems, and they are not a replacement pair — the newer
one animates *values*, the older one animates a component's *bounds*. Which to
reach for follows from that. See [versions](juce-versions.md) for what arrived
when.

[Example of animations](https://github.com/juce-framework/JUCE/blob/master/examples/GUI/AnimationEasingDemo.h).

## `juce::Animator` — animating values

Wrapper class for managing the lifetime of all the different animator kinds
created through the builder classes.

It uses reference counting. If you copy an Animator the resulting object will
refer to the same underlying instance, and the underlying instance is guaranteed
to remain valid for as long as you have an Animator object referencing it.

An Animator object can be registered with the AnimatorUpdater, which only stores
a weak reference to the underlying instance. If an AnimatorUpdater references the
underlying instance and it becomes deleted due to all Animator objects being
deleted, the updater will automatically remove it from its queue, so manually
removing it is not required.

See also `ValueAnimatorBuilder`, `AnimatorSetBuilder`, `AnimatorUpdater`,
`VBlankAnimatorUpdater`.

## `juce::ComponentAnimator` — animating bounds

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
