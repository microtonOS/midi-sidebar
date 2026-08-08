#include "DemoProcessor.h"
#include "DemoEditor.h"
#include "DemoSettings.h"

namespace microtonos::sidebar::demo
{

namespace ids
{
    const juce::String volume { "volume" };
    const juce::String page   { "page" };
    const juce::String theme      { "theme" };
    const juce::String edge       { "edge" };
    const juce::String bubbleText { "bubbleText" };
}

//==============================================================================
DemoProcessor::DemoProcessor()
    : juce::AudioProcessor (BusesProperties()
                                .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                                .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "state", createParameterLayout())
{
    // Cached once, here: looking a parameter up by name on the audio thread
    // allocates and is not realtime safe.
    volumeGain = apvts.getRawParameterValue (ids::volume);
}

juce::AudioProcessorValueTreeState::ParameterLayout DemoProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    // CC7 is hardcoded to volume by the specification; the parameter is here so
    // the sidebar's fader has something real to attach to. In decibels, on the
    // same scale and floor the fader and meter use, so all three agree.
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ids::volume, 1 },
        "Volume",
        juce::NormalisableRange<float> { metrics::floorDb, 0.0f },
        0.0f));

    // Which page the sidebar has open. A parameter rather than a dev-only hook
    // so a host can automate it — which matters for this project's headless
    // motivation, and incidentally lets the snapshot tool render the expanded
    // states with --param page=Tuning.
    layout.add (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { ids::page, 1 },
        "Page",
        juce::StringArray { "None", "Presets", "Controllers", "Tuning" },
        0));

    // The developer settings the demo exposes as buttons. Parameters for the
    // same three reasons the page is one: they survive the editor being
    // closed, they are saved with the session, and they can be set from the
    // command line — `--param theme=Light` renders a theme the agent never has
    // to click its way to. The choice lists live in DemoSettings.h so the
    // parameter's index and the button's index cannot drift apart.
    layout.add (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { ids::theme, 1 },
        "Theme",
        settings::themeNames,
        0));

    layout.add (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { ids::edge, 1 },
        "Sidebar edge",
        settings::edgeNames,
        0));

    // Not a sidebar setting at all: a switch for a JUCE bug that makes slider
    // value bubbles unreadable in some themes. It is here because the demo is
    // where a theme is switched, which is the only place the bug can be seen.
    layout.add (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { ids::bubbleText, 1 },
        "Bubble text",
        settings::bubbleTextNames,
        0));

    return layout;
}

//==============================================================================
void DemoProcessor::prepareToPlay (double, int)
{
    outputLevelLeft .store (0.0f, std::memory_order_relaxed);
    outputLevelRight.store (0.0f, std::memory_order_relaxed);
}

void DemoProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    for (auto i = getTotalNumInputChannels(); i < getTotalNumOutputChannels(); ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    // Apply the volume, then measure: the meter is post-fader, so what it shows
    // is what actually leaves the plugin, and the fader reads as a ceiling with
    // the level beneath it. Metering before the fader would put the two on
    // different signals and the shared scale would mean nothing.
    if (volumeGain != nullptr)
        buffer.applyGain (juce::Decibels::decibelsToGain (volumeGain->load(), metrics::floorDb));

    // No allocation, no locking, no parameter lookups beyond the cached
    // pointer. A mono input feeds both meter columns.
    const auto numChannels = buffer.getNumChannels();
    const auto numSamples  = buffer.getNumSamples();

    const auto peakOf = [&] (int channel)
    {
        return numChannels > 0 ? buffer.getMagnitude (juce::jmin (channel, numChannels - 1), 0, numSamples)
                               : 0.0f;
    };

    outputLevelLeft .store (peakOf (0), std::memory_order_relaxed);
    outputLevelRight.store (peakOf (1), std::memory_order_relaxed);
}

//==============================================================================
juce::AudioProcessorEditor* DemoProcessor::createEditor()
{
    return new DemoEditor (*this);
}

void DemoProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();

    // Editor size travels with the state so the host can restore it.
    state.setProperty ("editorWidth",  editorWidth .load (std::memory_order_relaxed), nullptr);
    state.setProperty ("editorHeight", editorHeight.load (std::memory_order_relaxed), nullptr);

    if (auto xml = state.createXml())
        copyXmlToBinary (*xml, destData);
}

void DemoProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
    {
        auto state = juce::ValueTree::fromXml (*xml);

        if (! state.isValid())
            return;

        editorWidth .store ((int) state.getProperty ("editorWidth",  0), std::memory_order_relaxed);
        editorHeight.store ((int) state.getProperty ("editorHeight", 0), std::memory_order_relaxed);

        apvts.replaceState (state);
    }
}

} // namespace microtonos::sidebar::demo

//==============================================================================
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new microtonos::sidebar::demo::DemoProcessor();
}
