#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "../SidebarLookAndFeel.h"

namespace microtonos::sidebar
{

//==============================================================================
/** A value the plugin shows and the end-user cannot change.

    Every read-only value on every page is one of these — the interval, the
    modulo's remainder, the tuning's name, its program and bank, the time it was
    last updated, where the period came from. One design for all of them is what
    makes a page read as a single object, and it is also what makes "you cannot
    type here" legible without a word of explanation: the editable fields are
    `TextEditor`s and look like it, these are recessed boxes and do not.

    A `Label` with `setEditable(false)` would draw the text but not the box, and
    a read-only `TextEditor` would draw a box that still takes focus, shows a
    caret on click and offers a context menu. Neither is what a read-out is.
*/
class ReadOutField final : public juce::Component
{
public:
    //==========================================================================
    enum ColourIds
    {
        backgroundColourId = 0x1a10400,
        textColourId       = 0x1a10401,
        outlineColourId    = 0x1a10402
    };

    /** @param placeholder  what to show when there is no value; the spec asks
                            for an empty box in some cases and a word such as
                            "Unnamed" in others */
    explicit ReadOutField (juce::String placeholderText = {})
        : placeholder (std::move (placeholderText))
    {
        // Scenery, not a control: it must not take clicks away from whatever is
        // behind it, and it can never be focused.
        setInterceptsMouseClicks (false, false);
    }

    /** An empty string shows the placeholder. */
    void setValue (const juce::String& newValue)
    {
        if (value == newValue)
            return;

        value = newValue;
        repaint();
    }

    void paint (juce::Graphics& g) override
    {
        const auto bounds = getLocalBounds().toFloat();

        g.setColour (findColour (backgroundColourId));
        g.fillRoundedRectangle (bounds, metrics::readOutCorner);

        g.setColour (findColour (outlineColourId));
        g.drawRoundedRectangle (bounds.reduced (0.5f), metrics::readOutCorner, 1.0f);

        const auto text = value.isNotEmpty() ? value : placeholder;

        if (text.isEmpty())
            return;

        // The placeholder is dimmed so that "nothing is known yet" and "the
        // value happens to be this" cannot be confused.
        g.setColour (findColour (textColourId)
                         .withMultipliedAlpha (value.isNotEmpty() ? 1.0f : placeholderAlpha));

        g.setFont (SidebarLookAndFeel::font (metrics::bodyFontHeight));
        g.drawText (text,
                    getLocalBounds().reduced (metrics::readOutPadding, 0),
                    juce::Justification::centredLeft,
                    true);
    }

private:
    static constexpr float placeholderAlpha = 0.5f;

    juce::String value, placeholder;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ReadOutField)
};

} // namespace microtonos::sidebar
