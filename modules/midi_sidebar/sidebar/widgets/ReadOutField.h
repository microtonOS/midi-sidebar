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

    /** What was last set, which is not necessarily what is drawn — an empty
        value draws the placeholder. Here so that what a field says can be
        asserted without rendering it. */
    const juce::String& getValue() const noexcept { return value; }

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
                         .withMultipliedAlpha (value.isNotEmpty() ? 1.0f : shades::placeholder));

        const auto font = SidebarLookAndFeel::font (metrics::bodyFontHeight);
        g.setFont (font);

        // As many lines as the box is tall, which for the one-row fields this
        // started as is one — so nothing about them changes. The controllers
        // monitor is the field that is taller than a row, and it holds a line
        // per message. `drawFittedText` centres the block of lines vertically,
        // so a single line still sits where `drawText` put it.
        const auto lines = juce::jmax (1, (int) ((float) getHeight() / font.getHeight()));

        g.drawFittedText (text,
                          getLocalBounds().reduced (metrics::readOutPadding, 0),
                          juce::Justification::centredLeft,
                          lines);
    }

private:
    juce::String value, placeholder;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ReadOutField)
};

//==============================================================================
/** A `TextEditor` set up to hold a number and nothing else.

    Two pages need the same thing — the tuning page's modulo divisor and
    pitch-bend ranges, the presets page's split frequencies — and the rule that
    matters is the input restriction: a field that cannot be left holding
    something the plugin could not act on never needs a validation path.

    Lives beside `ReadOutField` because the two are a pair: this is the editable
    one, that is the read-only one, and a page choosing between them is choosing
    whether the value is the end-user's. */
inline void prepareNumericEditor (juce::TextEditor& editor, bool allowFraction = true)
{
    editor.setMultiLine (false);
    editor.setReturnKeyStartsNewLine (false);
    editor.setJustification (juce::Justification::centredLeft);

    editor.setInputRestrictions (0, allowFraction ? "-0123456789." : "0123456789");
}

} // namespace microtonos::sidebar
