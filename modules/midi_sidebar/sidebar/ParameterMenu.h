#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include "Sidebar.h"
#include "pages/ControllersState.h"

namespace microtonos::sidebar
{

//==============================================================================
/** What the right-click menu offers, as data rather than as a `PopupMenu`.

    Implements docs/right-click.md. Separated from the menu that shows it for a
    practical reason: **a `PopupMenu` cannot be rendered headlessly**.
    `PopupMenu::getParentArea` dereferences `getDisplayForPoint(...)` with no
    null check (juce_PopupMenu.cpp:920) and a process with no displays crashes
    there, so the snapshot tool cannot capture any menu in this project — see
    ChoiceButton, which carries the same note.

    Everything decided *about* the menu therefore lives in `itemsFor`, which is
    an ordinary function over ordinary values and can be called, printed and
    checked anywhere. What is left in the menu itself is only the translation
    into JUCE's API.
*/
namespace parameterMenu
{
    enum class Action
    {
        none,           ///< A header or a read-out; nothing happens.
        info,           ///< Opens the submenu holding the parameter's description.
        viewInSidebar,
        midiLearn,
        unlearn
    };

    struct Item
    {
        juce::String text;
        Action action  = Action::none;
        bool enabled   = true;
        bool isHeader  = false;

        /** Draw a dividing line above this item. The sketch has one rule, under
            `info`, separating what the parameter *is* from what it is
            assigned to. */
        bool separatorBefore = false;
    };

    /** The menu for one parameter: its name, its description, what it is
        assigned to, and what can be done about that. */
    juce::Array<Item> itemsFor (const controllers::Parameter& parameter,
                                const juce::Array<controllers::Mapping>& mappings,
                                int parameterIndex);
}

//==============================================================================
/** The right-click menu a host puts on its own parameter widgets.

    A plugin embedding the sidebar owns one of these beside it and calls
    `attachTo` for each widget that stands for a parameter. Right-clicking that
    widget then shows what the parameter is, what controller is assigned to it,
    and offers to change that — without the sidebar having to be opened.

    **Widgets are identified by index**, into the same parameter list the
    controllers page was given: a mapping's `parameterIndex` already means that,
    so the menu and the table cannot come to disagree about which parameter is
    which. The module never sees the host's `AudioProcessorParameter` — it has
    no dependency on juce_audio_processors and this does not add one.

    Three of the four actions are carried out here, because nothing about them
    needs MIDI: the description, opening the table at the right rows, and
    removing the last mapping. `MIDI learn` is the exception and reports out
    instead — there is no MIDI to learn from yet.
*/
class ParameterMenu final
{
public:
    /** @param sidebarToDriveAndRead  where the mappings are read from, and what
                                       "view in sidebar" opens */
    explicit ParameterMenu (Sidebar& sidebarToDriveAndRead);
    ~ParameterMenu();

    /** Gives `widget` the menu on right-click. The widget must outlive nothing
        in particular: an attachment holds a `SafePointer` and quietly does
        nothing once its widget has gone. */
    void attachTo (juce::Component& widget, int parameterIndex);

    /** The same, for a widget that does not always stand for the same
        parameter. A knob shared between two targets — one rate control for a
        filter LFO and a pitch LFO, say — edits whichever the mode switch
        selects, and its menu has to follow it. The index is asked for at the
        moment of the click rather than at attach time. */
    void attachTo (juce::Component& widget, std::function<int()> parameterIndex);

    /** Shows the menu over `over`, for hosts that already handle their own
        clicks and would rather call this than be listened to. */
    void showFor (int parameterIndex, juce::Component& over);

    /** The end-user asked for the next controller they touch to be assigned to
        this parameter. Nothing here acts on it: what to do needs a MIDI message
        to have arrived, and the rule for it — see docs/right-click.md — depends
        on which channel that message came in on. */
    std::function<void (int parameterIndex)> onMidiLearnRequested;

    /** Called with the menu just before it opens, for a developer who wants
        items of their own on the end — modulation routing, say, which
        docs/right-click.md sketches as one possibility. Use ids from 1000 up;
        this class uses small ones. */
    std::function<void (juce::PopupMenu&, int parameterIndex)> onExtendMenu;

private:
    //==========================================================================
    /** One widget's worth of listening.

        A per-widget object rather than one `Component*`-keyed map on this
        class, because a map entry outlives the widget it names — JUCE does not
        tell a `MouseListener` when the component it listens to is deleted, and
        the next right-click anywhere would then dereference it. */
    struct Attachment;

    void perform (parameterMenu::Action action, int parameterIndex);

    Sidebar& sidebar;
    juce::OwnedArray<Attachment> attachments;

    // A menu is shown asynchronously and its callback outlives the click, so it
    // has to be able to find out that this object went away in between. The
    // Component::SafePointer the widgets use is not available here: this is not
    // a Component.
    JUCE_DECLARE_WEAK_REFERENCEABLE (ParameterMenu)
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ParameterMenu)
};

} // namespace microtonos::sidebar
