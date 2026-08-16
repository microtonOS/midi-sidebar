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
//==============================================================================
/** A menu row that draws its glyph at the **right** edge.

    `PopupMenu::Item::image` exists and is much less work, but it puts the image
    in the left gutter — the column JUCE reserves for the tick — and
    docs/controllers.md puts the mark after the name, not before it. There is no
    option for that, so the row is drawn here instead.

    Everything except the glyph is still the LookAndFeel's: `drawPopupMenuItem`
    paints the background, the highlight, the tick and the text exactly as it
    would for an ordinary item, so a marked row and an unmarked one cannot
    drift apart.
*/
class IconMenuItem final : public juce::PopupMenu::CustomComponent
{
public:
    IconMenuItem (juce::String itemText, bool ticked, const juce::Drawable* iconToDraw)
        : text (std::move (itemText)), isTicked (ticked), icon (iconToDraw)
    {
    }

    void getIdealSize (int& idealWidth, int& idealHeight) override
    {
        // Asked of the LookAndFeel so the row is the height every other row is,
        // then widened by the glyph and its margins — otherwise the menu sizes
        // itself to the text and the mark hangs off the edge.
        getLookAndFeel().getIdealPopupMenuItemSize (text, false, 0, idealWidth, idealHeight);

        idealWidth += metrics::markerSize + metrics::markerInset * 2;
    }

    void paint (juce::Graphics& g) override
    {
        getLookAndFeel().drawPopupMenuItem (g, getLocalBounds(),
                                            false,                 // isSeparator
                                            true,                  // isActive
                                            isItemHighlighted(),
                                            isTicked,
                                            false,                 // hasSubMenu
                                            text, {},
                                            nullptr,               // the left-gutter image, deliberately not used
                                            nullptr);

        if (icon == nullptr)
            return;

        const auto area = getLocalBounds().reduced (metrics::markerInset)
                                          .removeFromRight (metrics::markerSize)
                                          .withSizeKeepingCentre (metrics::markerSize, metrics::markerSize);

        icon->drawWithin (g, area.toFloat(), juce::RectanglePlacement::centred, 1.0f);
    }

private:
    juce::String text;
    bool isTicked = false;
    const juce::Drawable* icon = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (IconMenuItem)
};

//==============================================================================
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

    /** A glyph to draw beside item `i`, or nullptr for none.

        Asked per item as the menu is built rather than stored, because the
        drawables belong to whoever owns the list — a table keeps one of each
        and hands the same pointer to every cell. `PopupMenu` takes a *copy*, so
        nothing here has to outlive the menu.

        The same glyph is drawn on the button itself, so a mark seen while
        choosing is the mark seen afterwards. */
    std::function<const juce::Drawable* (int)> iconForItem;

    /** The glyph for what is currently selected, drawn at the right of the
        button. Separate from `iconForItem` because a button can show something
        that is not in its list — a built-in row's name, say. */
    void setIcon (const juce::Drawable* icon)
    {
        if (std::exchange (selectedIcon, icon) != icon)
            repaint();
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

    //==========================================================================
    /** The base class draws the background and the centred label; this adds the
        glyph at the right, **and drops it when the label has grown into that
        space**.

        A `TextButton` centres its text, so as the panel narrows the label
        spreads outwards and would slide underneath a mark drawn at a fixed
        inset. Clipping the label instead would be worse: the name is the thing
        you are reading, and the mark is a footnote to it. So the mark is what
        gives way — it is a property of the row, still visible in the menu and
        at any width where both fit. */
    void paint (juce::Graphics& g) override
    {
        juce::TextButton::paint (g);

        if (selectedIcon == nullptr)
            return;

        const auto area = iconArea();

        // Centred text, so it spreads both ways from the middle. `+ inset` is
        // the breathing space that keeps a glyph from touching a descender.
        const auto font      = getLookAndFeel().getTextButtonFont (*this, getHeight());
        const auto textWidth = juce::GlyphArrangement::getStringWidth (font, getButtonText());
        const auto textRight = (getWidth() + textWidth) * 0.5f + (float) metrics::markerInset;

        if (textRight > (float) area.getX())
            return;

        selectedIcon->drawWithin (g, area.toFloat(), juce::RectanglePlacement::centred, 1.0f);
    }

private:
    juce::Rectangle<int> iconArea() const
    {
        return getLocalBounds().reduced (metrics::markerInset)
                               .removeFromRight (metrics::markerSize)
                               .withSizeKeepingCentre (metrics::markerSize, metrics::markerSize);
    }

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

            juce::PopupMenu::Item item (items[i]);

            item.itemID   = i + 1;
            item.isTicked = i == selectedIndex;

            // Every row gets the custom component once any of them might carry a
            // glyph, not only the marked ones: a menu drawn by two different
            // paths is a menu whose rows can disagree about their height.
            if (iconForItem != nullptr)
                item.customComponent = new IconMenuItem (items[i], item.isTicked, iconForItem (i));

            menu.addItem (std::move (item));
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
    const juce::Drawable* selectedIcon = nullptr;
    int selectedIndex = -1;
    int separatorIndex = -1;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ChoiceButton)
};

} // namespace microtonos::sidebar
