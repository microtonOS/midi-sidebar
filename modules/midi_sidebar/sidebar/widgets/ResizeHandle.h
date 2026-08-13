#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace microtonos::sidebar
{

/** A thin invisible strip that reports how far it has been dragged sideways.

    Deliberately *not* `juce::ResizableEdgeComponent`, which sets the bounds of
    the component it is attached to. Everything here is laid out by an owner —
    the sidebar's width is the owner's business, and the owner may animate the
    change — so a handle that moved the sidebar itself would be overwritten on
    the owner's next `resized()`, or would fight the animator. This one only
    reports; what the number means is the reader's decision.

    It draws nothing. The cursor is what says it can be dragged, which is the
    convention for a splitter between two panes, and a painted grip on an edge
    that is already a hairline would be one line too many.
*/
class ResizeHandle final : public juce::Component
{
public:
    ResizeHandle()
    {
        setMouseCursor (juce::MouseCursor::LeftRightResizeCursor);
        setName ("Sidebar width handle");
    }

    /** Called on mouse-down, before any drag: the moment to record whatever the
        deltas below will be added to. */
    std::function<void()> onDragStart;

    /** Horizontal distance from where the drag began, in screen pixels.
        Positive is rightwards regardless of which edge this handle is on. */
    std::function<void (int)> onDrag;

    //==========================================================================
    void mouseDown (const juce::MouseEvent& e) override
    {
        // The *screen* position, not the event's own x. Dragging this handle
        // moves it — that is the whole point — and `getDistanceFromDragStartX`
        // is measured in the coordinates of a component that is sliding out from
        // under the mouse as it reports, so the handle would chase itself and
        // the width would run away. A screen coordinate is the one frame of
        // reference the drag does not move.
        dragStartX = e.getScreenPosition().x;

        if (onDragStart != nullptr)
            onDragStart();
    }

    void mouseDrag (const juce::MouseEvent& e) override
    {
        if (onDrag != nullptr)
            onDrag (e.getScreenPosition().x - dragStartX);
    }

private:
    int dragStartX = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ResizeHandle)
};

} // namespace microtonos::sidebar
