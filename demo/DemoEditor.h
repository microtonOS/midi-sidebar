#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <midi_sidebar/midi_sidebar.h>

#include "ContentPlaceholder.h"
#include "DemoProcessor.h"

namespace microtonos::sidebar::demo
{

//==============================================================================
/** Hosts the sidebar next to an empty content area.

    The content area stands in for whatever plugin the sidebar is added to. All
    this editor does is own the sidebar, give it the full height, and react when
    it wants to be a different width.
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

    DemoProcessor& processor;

    SidebarLookAndFeel lookAndFeel;
    ContentPlaceholder placeholder;
    Sidebar sidebar;

    // Must outlive the editor: setConstrainer keeps a pointer and does not take
    // ownership, so a local would dangle.
    juce::ComponentBoundsConstrainer constrainer;

    std::unique_ptr<juce::ParameterAttachment> pageAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DemoEditor)
};

} // namespace microtonos::sidebar::demo
