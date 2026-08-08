#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace microtonos::sidebar
{

//==============================================================================
/** The largest ancestor of `component` that still uses its LookAndFeel — where
    a pop-up belongs.

    **Not `getTopLevelComponent()`.** In a standalone build, and in some hosts,
    the editor sits inside a window this module knows nothing about.
    `setLookAndFeel` styles a component and its *descendants*, so that window is
    an ancestor and keeps the default LookAndFeel. Parenting a pop-up to it
    hands the pop-up the default styling: unregistered ColourIds, drawing
    overrides bypassed, a widget that looks right inside the editor and wrong
    in the pop-up lifted out of it.

    This is easy to get away with under the snapshot tool, where the editor *is*
    the top-level component and the two agree — and then to discover only in a
    real host. Which is why it is a named function rather than a line of code
    each caller writes for itself.
*/
inline juce::Component* findPopupHost (juce::Component& component)
{
    auto& ours = component.getLookAndFeel();
    juce::Component* host = &component;

    // Climb while the styling still matches, and stop at the first ancestor
    // that does not share it — that is the boundary of what will render like
    // us. If nobody has set a LookAndFeel at all, everything compares equal and
    // this walks to the top, which is also right.
    for (auto* c = component.getParentComponent(); c != nullptr; c = c->getParentComponent())
    {
        if (&c->getLookAndFeel() != &ours)
            break;

        host = c;
    }

    return host;
}

} // namespace microtonos::sidebar
