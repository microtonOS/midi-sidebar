#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

namespace microtonos::sidebar::demo
{

//==============================================================================
/** The thinnest processor that will host an editor.

    This exists so the sidebar has somewhere to live: a JUCE module cannot be
    loaded in a host or rendered by the snapshot tool on its own. It passes
    audio through untouched and owns nothing but the parameters the sidebar
    needs.
*/
class DemoProcessor final : public juce::AudioProcessor
{
public:
    DemoProcessor();
    ~DemoProcessor() override = default;

    //==========================================================================
    void prepareToPlay (double sampleRate, int maximumExpectedSamplesPerBlock) override;
    void releaseResources() override {}
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    //==========================================================================
    const juce::String getName() const override { return "Sidebar Demo"; }

    bool acceptsMidi() const override  { return true; }
    bool producesMidi() const override { return true; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return "Default"; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    //==========================================================================
    juce::AudioProcessorValueTreeState apvts;

    /** Peak level of the last block per channel, for the sidebar's stereo
        meter. Written on the audio thread, read on the message thread — hence
        atomic. */
    std::atomic<float> outputLevelLeft  { 0.0f };
    std::atomic<float> outputLevelRight { 0.0f };

    /** Editor size, kept here because the host saves and restores it and the
        editor itself is destroyed every time the window closes. */
    std::atomic<int> editorWidth  { 0 };
    std::atomic<int> editorHeight { 0 };

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    /** Cached in the constructor: looking a parameter up by name on the audio
        thread allocates and is not realtime safe. */
    std::atomic<float>* volumeGain = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DemoProcessor)
};

} // namespace microtonos::sidebar::demo
