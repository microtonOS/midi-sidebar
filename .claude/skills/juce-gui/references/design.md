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

## Animations

[Example of animations](https://github.com/juce-framework/JUCE/blob/master/examples/GUI/AnimationEasingDemo.h).

### `juce::Animator` Class
Wrapper class for managing the lifetime of all the different animator kinds created through the builder classes.

It uses reference counting. If you copy an Animator the resulting object will refer to the same underlying instance, and the underlying instance is guaranteed to remain valid for as long as you have an Animator object referencing it.

An Animator object can be registered with the AnimatorUpdater, which only stores a weak reference to the underlying instance. If an AnimatorUpdater references the underlying instance and it becomes deleted due to all Animator objects being deleted, the updater will automatically remove it from its queue, so manually removing it is not required.

See also
ValueAnimatorBuilder, AnimatorSetBuilder, AnimatorUpdater, VBlankAnimatorUpdater

