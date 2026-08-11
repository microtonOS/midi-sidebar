#pragma once

#include <optional>

#include <juce_gui_basics/juce_gui_basics.h>
#include <midi_sidebar/midi_sidebar.h>

namespace microtonos::sidebar::demo
{

//==============================================================================
/** The developer settings the demo exposes, and the names they go by.

    Each one is a list of choices shared by three places that must agree: the
    parameter the processor declares, the buttons the controls panel draws, and
    the code in the editor that acts on the choice. Declaring the list once here
    is what keeps a renamed or reordered choice from meaning two different
    things at two ends of the plugin — the parameter's index *is* the index into
    these arrays.

    These are settings the *developer* embedding the sidebar would make once, in
    code. The demo turns them into runtime controls because that is the point of
    a demo: seeing all four themes means switching between them, not four
    rebuilds.
*/
namespace settings
{
    //==========================================================================
    /** The four schemes `LookAndFeel_V4` ships with, in its own order, dark
        first — the sidebar's starting theme, and the one its derived colours
        were designed against. */
    inline const juce::StringArray themeNames { "Dark", "Midnight", "Grey", "Light" };

    inline juce::LookAndFeel_V4::ColourScheme schemeFor (int themeIndex)
    {
        using V4 = juce::LookAndFeel_V4;

        switch (themeIndex)
        {
            case 1:  return V4::getMidnightColourScheme();
            case 2:  return V4::getGreyColourScheme();
            case 3:  return V4::getLightColourScheme();
            default: break;
        }

        return V4::getDarkColourScheme();
    }

    //==========================================================================
    /** Which side of the host's UI the sidebar lives on. Ordered so the index
        matches `Sidebar::Edge`, which is what lets the parameter be cast to it
        rather than switched over. */
    inline const juce::StringArray edgeNames { "Left", "Right" };

    inline Sidebar::Edge edgeFor (int edgeIndex)
    {
        return edgeIndex == 1 ? Sidebar::Edge::right : Sidebar::Edge::left;
    }

    //==========================================================================
    /** Which of the two things the host's area shows: the developer settings, or
        the stand-in plugin whose knobs the right-click menu is demonstrated on.

        A parameter like the rest, for the same three reasons — it survives the
        editor closing, a host can automate it, and the snapshot tool can render
        either view with `--param view=Synth` rather than clicking a tab. */
    inline const juce::StringArray viewNames { "Settings", "Synth" };

    inline constexpr int settingsView = 0;
    inline constexpr int synthView    = 1;

    //==========================================================================
    /** Text colour for slider value bubbles, working around a JUCE bug.

        A bubble takes its background from `BubbleComponent::backgroundColourId`
        — `widgetBackground` — and its text from `TooltipWindow::textColourId` —
        `highlightedText`. Those two are from unrelated pairs, and in the Light
        scheme both are white, so the value read-out is invisible. `Slider`'s
        popup has no text colour of its own to set instead; see the juce-ui
        skill's Sliders reference.

        "Default" is deliberately first, and means *leave JUCE alone*: the demo
        should be able to show the bug, not only hide it. The override belongs
        on an ancestor rather than on each slider, which is how a real plugin
        would apply it — one call covering every bubble it ever shows.
    */
    inline const juce::StringArray bubbleTextNames { "Default", "White", "Black" };

    /** Nothing when the choice is "Default", so the caller removes its override
        rather than setting one. */
    inline std::optional<juce::Colour> bubbleTextColourFor (int bubbleTextIndex)
    {
        switch (bubbleTextIndex)
        {
            case 1:  return juce::Colours::white;
            case 2:  return juce::Colours::black;
            default: break;
        }

        return {};
    }
}

} // namespace microtonos::sidebar::demo
