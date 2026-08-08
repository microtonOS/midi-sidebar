#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <midi_sidebar/midi_sidebar.h>

namespace microtonos::sidebar::demo
{

//==============================================================================
/** Marks the area a host plugin's own UI would occupy.

    A real Component rather than something the editor draws in its own `paint`.
    Drawing it in the editor meant deriving its bounds from the sidebar's
    current width, which is wrong twice over: the editor is not repainted while
    the sidebar animates, so the outline lagged behind and then stayed stale
    once the animation finished. A child gets laid out and repaints itself.
*/
class ContentPlaceholder final : public juce::Component
{
public:
    ContentPlaceholder()
    {
        setInterceptsMouseClicks (false, false);
    }

    void paint (juce::Graphics& g) override
    {
        const auto colour = findColour (juce::Label::textColourId).withMultipliedAlpha (0.25f);

        g.setColour (colour);
        g.drawRect (getLocalBounds(), 1);

        g.setFont (SidebarLookAndFeel::font (metrics::bodyFontHeight));
        g.drawText ("host plugin content", getLocalBounds(), juce::Justification::centred);
    }

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ContentPlaceholder)
};

} // namespace microtonos::sidebar::demo
