#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "../SidebarLookAndFeel.h"

namespace microtonos::sidebar
{

//==============================================================================
/** One setting: a label, and a row of buttons of which exactly one is on.

    The module's only multi-choice control, used both on the pages and by the
    demo's own panel. The buttons are drawn as a single segmented control —
    connected edges, no gaps — so the row reads as one choice rather than as
    several independent switches.

    The label is inside the strip rather than in the owner's grid, which is
    normally the wrong way round for something that has to align across rows.
    It is safe as long as strips are stacked in one column at the same width,
    which is what both users of it do; the label column is a named measurement
    (`metrics::choiceLabelWidth`) so an owner can reserve the same width for
    labels of its own and have them line up.
*/
class ChoiceStrip final : public juce::Component
{
public:
    //==========================================================================
    enum ColourIds
    {
        selectedColourId     = 0x1a10300,   ///< Fill behind the chosen button.
        selectedTextColourId = 0x1a10301    ///< Its text, which must read on that fill.
    };

    /** Which way the choices run.

        Segmented either way: the buttons touch and only the outer corners are
        rounded, so the strip reads as one control rather than as several
        independent switches. `LookAndFeel_V4` does this itself, given the
        connected edges — there is no custom drawing here. */
    enum class Orientation { horizontal, vertical };

    /** @param title       what the setting is called, or empty for no label at
                           all — which is what an owner passes when it wants the
                           label in its own grid instead, so that the label
                           lines up with the rest of the page rather than only
                           with this strip
        @param choices     one button per entry
        @param labelWidth  width of the label column; the default suits a page
    */
    ChoiceStrip (const juce::String& title,
                 const juce::StringArray& choices,
                 int labelWidth = metrics::choiceLabelWidth)
        : label ("Label", title),
          labelColumnWidth (title.isNotEmpty() ? labelWidth : 0)
    {
        label.setJustificationType (juce::Justification::centredLeft);

        if (title.isNotEmpty())
            addAndMakeVisible (label);

        for (int i = 0; i < choices.size(); ++i)
        {
            auto* button = buttons.add (new SegmentButton (choices[i]));

            button->setClickingTogglesState (true);

            // A radio group is scoped to the shared parent, so every strip can
            // use the same id without the strips interfering with each other.
            // Unlike the sidebar's page buttons, refusing to switch the active
            // one off is exactly what is wanted: there is no "no choice".
            button->setRadioGroupId (radioGroupId);

            button->onClick = [this, i]
            {
                // A radio group notifies the button it switches *off* as well
                // as the one it switches on: `Button::internalClickCallback`
                // passes `sendNotification` into `turnOffOtherButtonsInGroup`,
                // which calls `sendClickMessage` on the loser. So this fires
                // twice per click, once with the wrong index, and an owner that
                // acts on it synchronously — pushing the choice back in through
                // setSelectedIndex — re-enters the toggle it is still inside
                // and leaves *both* buttons on.
                //
                // The button that is on is the choice. The other one is JUCE
                // telling us it is no longer selected, which we already know.
                if (! buttons[i]->getToggleState())
                    return;

                if (onChoice != nullptr)
                    onChoice (i);
            };

            addAndMakeVisible (button);
        }

        applyConnectedEdges();
        setSelectedIndex (0);
    }

    void setOrientation (Orientation newOrientation)
    {
        if (orientation == newOrientation)
            return;

        orientation = newOrientation;
        applyConnectedEdges();
        resized();
    }

    /** Called with the index of the choice the user picked. Not called when the
        selection is set from code — see `setSelectedIndex`. */
    std::function<void (int)> onChoice;

    /** How many choices this strip offers, so an owner can size a column from
        its widest row rather than from a number written down twice. */
    int getChoiceCount() const noexcept { return buttons.size(); }

    /** Which choice is showing, or -1 before there is one. Read from the
        buttons rather than kept alongside them: a radio group already holds
        exactly one selection, and a copy of it is a second answer that can
        disagree. */
    int getSelectedIndex() const noexcept
    {
        for (int i = 0; i < buttons.size(); ++i)
            if (buttons[i]->getToggleState())
                return i;

        return -1;
    }

    /** Shows a selection without announcing it, so that a value arriving from
        wherever this strip mirrors cannot bounce straight back out through
        `onChoice` and start a loop. */
    void setSelectedIndex (int index)
    {
        if (auto* button = buttons[index])
            button->setToggleState (true, juce::dontSendNotification);
    }

    /** Both hooks, because they fire in different situations and this widget
        needs either. Being added to an already-styled parent sends
        `parentHierarchyChanged` but *not* `lookAndFeelChanged`, which is
        exactly what happens to a strip built inside a `CallOutBox`: it is
        constructed parentless, attached, and never told. Without this its
        buttons keep LookAndFeel_V4's toggled colour and the selection is
        unreadable — while the identical strip on the page looks right, because
        something else happened to send a look-and-feel change after it. */
    void parentHierarchyChanged() override { lookAndFeelChanged(); }

    void lookAndFeelChanged() override
    {
        auto& lf = getLookAndFeel();

        // Bail before the LookAndFeel that knows this module's ColourIds is
        // reachable — at construction, and again during teardown, when the
        // owner's setLookAndFeel(nullptr) sends a change to its children.
        // findColour would otherwise return black and assert.
        if (! lf.isColourSpecified (selectedColourId))
            return;

        // Without this the pair reads the wrong way round: LookAndFeel_V4 draws
        // a toggled TextButton *darker* than its neighbours, which on a dark
        // theme looks like the disabled one rather than the chosen one.
        for (auto* button : buttons)
        {
            button->setColour (juce::TextButton::buttonOnColourId, lf.findColour (selectedColourId));
            button->setColour (juce::TextButton::textColourOnId,   lf.findColour (selectedTextColourId));
        }

        label.setFont (SidebarLookAndFeel::font (metrics::bodyFontHeight));
    }

    void resized() override
    {
        auto bounds = getLocalBounds();

        if (labelColumnWidth > 0)
        {
            label.setBounds (bounds.removeFromLeft (labelColumnWidth));
            bounds.removeFromLeft (metrics::choiceLabelGap);
        }

        juce::Grid grid;
        using Track = juce::Grid::TrackInfo;

        const auto vertical = orientation == Orientation::vertical;

        // Equal flexible tracks rather than an extent divided by the button
        // count: the remainder is then distributed by the layout instead of
        // being abandoned at one end, which on a segmented control would show
        // up as a gap in the middle of it.
        auto& along = vertical ? grid.templateRows : grid.templateColumns;
        auto& across = vertical ? grid.templateColumns : grid.templateRows;

        across.add (Track (juce::Grid::Fr (1)));

        for (int i = 0; i < buttons.size(); ++i)
        {
            along.add (Track (juce::Grid::Fr (1)));
            grid.items.add (juce::GridItem (buttons[i]));
        }

        // No gap in either direction: the buttons have to touch for the joined
        // edges to mean anything. Where a vertical strip spans two rows of a
        // page, this puts its seam in the middle of the gap between those rows
        // and its outer edges exactly on their outer edges.
        grid.performLayout (bounds);
    }

private:
    /** Squares off every edge where one button meets the next, leaving the two
        outer ends rounded. `LookAndFeel_V4::drawButtonBackground` reads these
        flags and picks which corners to round, so the segmented look costs
        nothing but saying which edges are joined — in either direction. */
    void applyConnectedEdges()
    {
        const auto vertical = orientation == Orientation::vertical;

        for (int i = 0; i < buttons.size(); ++i)
        {
            const auto atStart = i == 0;
            const auto atEnd   = i == buttons.size() - 1;

            const auto towardsStart = vertical ? juce::Button::ConnectedOnTop
                                               : juce::Button::ConnectedOnLeft;
            const auto towardsEnd   = vertical ? juce::Button::ConnectedOnBottom
                                               : juce::Button::ConnectedOnRight;

            buttons[i]->setConnectedEdges ((atStart ? 0 : towardsStart)
                                           | (atEnd ? 0 : towardsEnd));
        }
    }

    /** One segment, which ignores a right-click.

        `Button::mouseDown` does not look at *which* mouse button was pressed —
        it calls `updateState (true, true)` for any of them — so a right-click
        on a plain `TextButton` selects it. On a strip carrying a context menu
        that means the setting changes on the way to opening the menu, which is
        the one thing a menu must not do.

        Dropping the event rather than consuming it: the click still travels to
        anything listening, which is how `ParameterMenu` sees it. Guarding
        `mouseDown` alone is enough — `Button::mouseUp` only fires the click
        when the button was already down. */
    struct SegmentButton final : public juce::TextButton
    {
        using juce::TextButton::TextButton;

        void mouseDown (const juce::MouseEvent& event) override
        {
            if (event.mods.isPopupMenu())
                return;

            juce::TextButton::mouseDown (event);
        }
    };

    /** Any non-zero value; see the comment where it is used. */
    static constexpr int radioGroupId = 1;

    juce::Label label;
    const int labelColumnWidth;
    Orientation orientation = Orientation::horizontal;
    juce::OwnedArray<juce::TextButton> buttons;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ChoiceStrip)
};

} // namespace microtonos::sidebar
