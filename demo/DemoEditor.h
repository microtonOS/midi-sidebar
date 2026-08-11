#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <midi_sidebar/midi_sidebar.h>

#include "DemoControls.h"
#include "DemoProcessor.h"
#include "DemoStyle.h"

namespace microtonos::sidebar::demo
{

//==============================================================================
/** Hosts the sidebar next to the area a host plugin's UI would occupy.

    That area holds the demo's developer controls — theme and which edge the
    sidebar lives on — so the settings a real plugin would make in code can be
    changed while it runs. Everything the editor does beyond owning the sidebar
    is bookkeeping between those controls, the parameters that hold their
    values, and the sidebar itself.
*/
class DemoEditor final : public juce::AudioProcessorEditor,
                         private juce::Timer
{
public:
    explicit DemoEditor (DemoProcessor&);
    ~DemoEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    void layOutSidebar (bool animated);
    void applyTheme (int themeIndex);
    void applyBubbleTextColour (int bubbleTextIndex);

    /** Hands the controllers page's MPE zone to the tuning page, which cannot
        tune those channels separately. The two pages know nothing about each
        other; joining them is the owner's job. */
    void applyMpeToTuning (controllers::Mpe mpe);

    /** Fill the pages with fixed sample values, so they can be looked at
        populated. Nothing drives them yet; see docs/demo.md. */
    void showSampleTuning();
    void showSampleControllers();
    void showSamplePresets();

    /** Two-way binding between a choice parameter and a strip of buttons: the
        parameter drives `apply`, and a click on a button drives the parameter.
        Both settings need exactly this, and doing it twice by hand is how the
        two halves end up subtly different. */
    std::unique_ptr<juce::ParameterAttachment> attachChoice (const juce::String& parameterID,
                                                             ChoiceStrip& strip,
                                                             std::function<void (int)> apply);

    DemoProcessor& processor;

    SidebarLookAndFeel lookAndFeel;
    DemoControls controls;
    Sidebar sidebar;

    // Must outlive the editor: setConstrainer keeps a pointer and does not take
    // ownership, so a local would dangle.
    juce::ComponentBoundsConstrainer constrainer;

    std::unique_ptr<juce::ParameterAttachment> pageAttachment;
    std::unique_ptr<juce::ParameterAttachment> themeAttachment;
    std::unique_ptr<juce::ParameterAttachment> edgeAttachment;
    std::unique_ptr<juce::ParameterAttachment> bubbleTextAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DemoEditor)
};

} // namespace microtonos::sidebar::demo
