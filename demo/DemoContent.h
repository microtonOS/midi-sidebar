#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <midi_sidebar/midi_sidebar.h>

#include "DemoControls.h"
#include "DemoSettings.h"
#include "DemoStyle.h"
#include "DemoSynthPanel.h"

namespace microtonos::sidebar::demo
{

//==============================================================================
/** The area a host plugin's own UI would occupy, and the two things the demo
    puts in it.

    The outline and the caption mark where the host's UI would be — which is how
    the sidebar's overlaying can be seen at all, since a panel that covers
    nothing looks the same as one that pushes content aside. Inside it are two
    tabs:

    - **Settings**, the developer choices a real plugin makes once in code, and
    - **Synth**, a stand-in plugin whose knobs are things a musician would
      actually assign a controller to. That is what the right-click menu needs;
      see docs/right-click.md.

    `juce::TabbedComponent` rather than anything of ours: it is what JUCE's own
    DemoRunner uses for its Demo/Code tabs (`DemoContentComponent`) and what the
    Widgets demo uses for its pages (`DemoTabbedComponent`) — the same class in
    both places, so this is the conventional answer rather than a found one.
*/
class DemoContent final : public juce::Component
{
public:
    explicit DemoContent (juce::AudioProcessorValueTreeState& state)
        : synth (state)
    {
        // Scenery; its children are the point, so they still take clicks.
        setInterceptsMouseClicks (false, true);

        caption.setJustificationType (juce::Justification::centred);
        addAndMakeVisible (caption);

        tabs.setTabBarDepth (layout::tabBarDepth);

        // No outline of its own: this component already draws the one that
        // marks the host's area, and a second rectangle just inside the first
        // reads as a mistake.
        tabs.setOutline (0);

        // Transparent, so the panel behind shows through and the tabs do not
        // introduce a surface of their own. `false`: these are members, and the
        // tab bar must not delete them. In `viewNames`' order, which is the
        // order the parameter's indices mean.
        tabs.addTab (settings::viewNames[settings::settingsView], juce::Colours::transparentBlack, &controls, false);
        tabs.addTab (settings::viewNames[settings::synthView],    juce::Colours::transparentBlack, &synth,    false);

        tabs.onTabChanged = [this] (int index)
        {
            if (onViewChanged != nullptr)
                onViewChanged (index);
        };

        addAndMakeVisible (tabs);
    }

    DemoControls&   getControls()   noexcept { return controls; }
    DemoSynthPanel& getSynthPanel() noexcept { return synth; }

    /** Silent: this is the parameter saying which view is open, and reporting
        it back would be an echo. */
    void setView (int index)
    {
        if (juce::isPositiveAndBelow (index, tabs.getNumTabs()))
            tabs.setCurrentTabIndex (index, false);
    }

    /** The end-user picked a tab. */
    std::function<void (int)> onViewChanged;

    void paint (juce::Graphics& g) override
    {
        g.setColour (dimmedText());
        g.drawRect (getLocalBounds(), 1);
    }

    void resized() override
    {
        auto area = getLocalBounds();

        // The caption is padded because it names the whole rectangle; the tabs
        // are not, because a tab bar that stops short of the edges looks like a
        // control that has been dropped in rather than like the top of the
        // area. Each tab's own content does its own padding.
        caption.setBounds (area.removeFromTop (layout::controlsPadding + layout::captionHeight)
                               .withTrimmedTop (layout::controlsPadding));

        tabs.setBounds (area.reduced (layout::controlsPadding, 0)
                            .withTrimmedBottom (layout::controlsPadding));
    }

    void lookAndFeelChanged() override
    {
        caption.setColour (juce::Label::textColourId, dimmedText());
        caption.setFont (SidebarLookAndFeel::font (metrics::bodyFontHeight));
    }

private:
    //==========================================================================
    /** `currentTabChanged` is a virtual rather than a callback, so mirroring the
        open tab to a parameter needs this much of a subclass and no more. */
    struct Tabs final : public juce::TabbedComponent
    {
        Tabs() : juce::TabbedComponent (juce::TabbedButtonBar::TabsAtTop) {}

        void currentTabChanged (int newIndex, const juce::String&) override
        {
            if (onTabChanged != nullptr)
                onTabChanged (newIndex);
        }

        std::function<void (int)> onTabChanged;
    };

    /** The outline and the caption are both scenery: present enough to read,
        quiet enough not to compete with the sidebar. */
    juce::Colour dimmedText() const
    {
        return getLookAndFeel().findColour (juce::Label::textColourId)
                               .withMultipliedAlpha (shades::scenery);
    }

    juce::Label caption { "Caption", "host plugin content" };

    DemoControls controls;
    DemoSynthPanel synth;
    Tabs tabs;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DemoContent)
};

} // namespace microtonos::sidebar::demo
