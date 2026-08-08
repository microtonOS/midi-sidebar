#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <midi_sidebar/midi_sidebar.h>

#include "DemoStyle.h"

namespace microtonos::sidebar::demo
{

//==============================================================================
/** One setting: a label, and a row of buttons of which exactly one is on.

    One widget design covers every setting the demo has, which is what keeps its
    own UI from competing with the sidebar it exists to show. The buttons are
    drawn as a single segmented control — connected edges, no gaps — so the row
    reads as one choice rather than as several independent switches.

    The label is inside the strip rather than in the parent's grid, which is
    normally the wrong way round for something that has to align across rows.
    It is safe here because the strips are stacked in one full-width column, so
    every instance is the same width and the label column lands in the same
    place by construction. Were they ever laid out at differing widths, the
    label would have to become a track of the parent's grid instead.
*/
class ChoiceStrip final : public juce::Component
{
public:
    ChoiceStrip (const juce::String& title, const juce::StringArray& choices)
        : label ("Label", title)
    {
        label.setJustificationType (juce::Justification::centredLeft);
        addAndMakeVisible (label);

        for (int i = 0; i < choices.size(); ++i)
        {
            auto* button = buttons.add (new juce::TextButton (choices[i]));

            button->setClickingTogglesState (true);

            // A radio group is scoped to the shared parent, so every strip can
            // use the same id without the strips interfering with each other.
            // Unlike the sidebar's page buttons, refusing to switch the active
            // one off is exactly what is wanted: there is no "no theme".
            button->setRadioGroupId (radioGroupId);

            // Segmented: only the outer ends of the row keep a rounded corner.
            button->setConnectedEdges ((i > 0 ? juce::Button::ConnectedOnLeft : 0)
                                       | (i < choices.size() - 1 ? juce::Button::ConnectedOnRight : 0));

            button->onClick = [this, i]
            {
                if (onChoice != nullptr)
                    onChoice (i);
            };

            addAndMakeVisible (button);
        }

        setSelectedIndex (0);
    }

    /** Called with the index of the choice the user picked. Not called when the
        selection is set from code — see `setSelectedIndex`. */
    std::function<void (int)> onChoice;

    /** How many choices this strip offers, so the owner can size the column
        from its widest row rather than from a number written down twice. */
    int getChoiceCount() const noexcept { return buttons.size(); }

    /** Shows a selection without announcing it, so that a value arriving from
        the parameter this strip mirrors cannot bounce straight back out through
        `onChoice` and start a loop. */
    void setSelectedIndex (int index)
    {
        if (auto* button = buttons[index])
            button->setToggleState (true, juce::dontSendNotification);
    }

    void lookAndFeelChanged() override
    {
        auto& lf = getLookAndFeel();

        // Bail before the LookAndFeel that knows the module's ColourIds is
        // reachable — at construction, and again during teardown, when the
        // editor's setLookAndFeel(nullptr) sends a change to its children.
        // findColour would otherwise return black and assert.
        if (! lf.isColourSpecified (Sidebar::iconActiveColourId))
            return;

        // The chosen button takes the same accent the sidebar gives its active
        // page icon, so "this one is on" means one thing across the window.
        // Without it the pair reads the wrong way round: V4 draws a toggled
        // TextButton *darker* than its neighbours, which on a dark theme looks
        // like the disabled one rather than the chosen one.
        const auto accent = lf.findColour (Sidebar::iconActiveColourId);

        for (auto* button : buttons)
        {
            button->setColour (juce::TextButton::buttonOnColourId, accent);
            button->setColour (juce::TextButton::textColourOnId, accent.contrasting());
        }

        label.setFont (SidebarLookAndFeel::font (metrics::bodyFontHeight));
    }

    void resized() override
    {
        auto bounds = getLocalBounds();

        label.setBounds (bounds.removeFromLeft (layout::choiceLabelWidth));
        bounds.removeFromLeft (layout::choiceLabelGap);

        juce::Grid grid;
        using Track = juce::Grid::TrackInfo;

        // Equal flexible tracks rather than a width divided by the button
        // count: the remainder is then distributed by the layout instead of
        // being abandoned at one edge, which on a segmented control would show
        // up as a gap in the middle of it.
        grid.templateRows = { Track (juce::Grid::Fr (1)) };

        for (int i = 0; i < buttons.size(); ++i)
        {
            grid.templateColumns.add (Track (juce::Grid::Fr (1)));
            grid.items.add (juce::GridItem (buttons[i]));
        }

        grid.performLayout (bounds);
    }

private:
    /** Any non-zero value; see the comment where it is used. */
    static constexpr int radioGroupId = 1;

    juce::Label label;
    juce::OwnedArray<juce::TextButton> buttons;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ChoiceStrip)
};

} // namespace microtonos::sidebar::demo
