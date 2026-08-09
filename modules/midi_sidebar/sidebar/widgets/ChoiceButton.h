#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "../PopupHost.h"
#include "../SidebarLookAndFeel.h"

namespace microtonos::sidebar
{

//==============================================================================
/** A button showing the current choice, which opens a menu of the others.

    What a `ComboBox` does, without the arrow. JUCE draws that arrow inside the
    box's own bounds and it costs about twenty pixels — affordable on a page,
    ruinous in a table cell forty-odd pixels wide, where it leaves "omni"
    rendered as "o...". This is the pattern JUCE's own Widgets demo uses on its
    Menus page: a plain button whose `onClick` shows a `PopupMenu` targeted at
    itself.

    The current choice is ticked in the menu, so what the arrow was signalling —
    that there is more here than one value — is carried by the menu itself.

    `onClick` belongs to this class; use `onChoice` instead.
*/
class ChoiceButton final : public juce::TextButton
{
public:
    /** @param name  the *component* name, which stays put while the button's
                      text changes with the selection. Worth setting: it is what
                      identifies the control in a component tree and to the
                      snapshot tool's `--click`. */
    explicit ChoiceButton (const juce::String& name = {}) : juce::TextButton (name)
    {
        onClick = [this] { showMenu(); };
    }

    /** @param newItems           what the menu offers, in order
        @param separatorBefore    index to draw a dividing line above, or -1.
                                  The controllers page uses it for the rule
                                  between the modes that read a value and the
                                  ones that only test a threshold. */
    void setItems (juce::StringArray newItems, int separatorBefore = -1)
    {
        items = std::move (newItems);
        separatorIndex = separatorBefore;

        refreshText();
    }

    /** Silent by default: this is normally the owner saying what the value is,
        and reporting that back would be an echo. */
    void setSelectedIndex (int index, juce::NotificationType notification = juce::dontSendNotification)
    {
        if (selectedIndex == index)
            return;

        selectedIndex = index;
        refreshText();

        if (notification != juce::dontSendNotification && onChoice != nullptr)
            onChoice (selectedIndex);
    }

    int getSelectedIndex() const noexcept { return selectedIndex; }

    /** Called only when the end-user picks from the menu. */
    std::function<void (int)> onChoice;

private:
    void refreshText()
    {
        setButtonText (juce::isPositiveAndBelow (selectedIndex, items.size()) ? items[selectedIndex]
                                                                              : juce::String());
    }

    void showMenu()
    {
        juce::PopupMenu menu;
        menu.setLookAndFeel (&getLookAndFeel());

        for (int i = 0; i < items.size(); ++i)
        {
            if (i == separatorIndex)
                menu.addSeparator();

            menu.addItem (i + 1, items[i], true, i == selectedIndex);
        }

        // A SafePointer, not `this`: these are used as table cells, and a table
        // deletes and rebuilds its cells whenever its content is refreshed —
        // which can happen while the menu is open.
        juce::Component::SafePointer<ChoiceButton> safe (this);

        // On the desktop, as `ComboBox` and JUCE's own demo do. A call-out gets
        // `withParentComponent` in this module because its *contents* resolve
        // ColourIds through the hierarchy; a menu does not, since it is drawn
        // entirely by the LookAndFeel set above — and staying off the parent
        // lets a long menu, such as seventeen channels, extend past a short
        // editor instead of being scrolled inside it.
        //
        // Note that this cannot be rendered by the snapshot tool at all:
        // `PopupMenu::getParentArea` dereferences `getDisplayForPoint(...)`
        // without a null check (juce_PopupMenu.cpp:920), and a headless process
        // has no displays, so *any* menu segfaults there. That is JUCE's bug
        // rather than this widget's, and it does not arise anywhere a display
        // exists. `withParentComponent` does not avoid it: the dereference
        // happens before the parent is consulted.
        menu.showMenuAsync (juce::PopupMenu::Options{}
                                .withTargetComponent (this)
                                .withMinimumWidth (getWidth()),
                            [safe] (int result)
                            {
                                if (safe != nullptr && result > 0)
                                    safe->setSelectedIndex (result - 1, juce::sendNotification);
                            });
    }

    juce::StringArray items;
    int selectedIndex = -1;
    int separatorIndex = -1;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ChoiceButton)
};

} // namespace microtonos::sidebar
