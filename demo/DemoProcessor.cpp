#include "DemoProcessor.h"
#include "DemoEditor.h"
#include "DemoSettings.h"
#include "DemoSynth.h"

namespace microtonos::sidebar::demo
{

namespace ids
{
    const juce::String volume { "volume" };
    const juce::String page   { "page" };
    const juce::String theme      { "theme" };
    const juce::String edge       { "edge" };
    const juce::String bubbleText { "bubbleText" };
    const juce::String view       { "view" };
    const juce::String panelWidth { "panelWidth" };
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
        juce::StringArray { "None", "Presets", "Controllers", "Tuning", "Channels" },
        0));

    // How wide the sidebar's page area is. The end-user sets this by dragging
    // the sidebar's inner edge, which is a gesture nothing headless can perform
    // — so without a parameter the widened sidebar could not be rendered, tested
    // or automated at all. The sidebar clamps whatever it is given to its own
    // minimum and to what the window can hold, so no value here is unsafe.
    layout.add (std::make_unique<juce::AudioParameterInt> (
        juce::ParameterID { ids::panelWidth, 1 },
        "Panel width",
        metrics::panelMinWidth,
        layout::maxWidth / 2,
        metrics::panelMinWidth));

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

    // Which of the two things the host's area shows. A parameter for the same
    // reason as the page: `--param view=Synth` renders the synth panel without
    // anyone having to click a tab.
    layout.add (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { ids::view, 1 },
        "View",
        settings::viewNames,
        0));

    // The stand-in plugin's own parameters — the ones the right-click menu maps
    // to controllers. Declared from `synth::controls()`, so the parameter, the
    // widget and the index a mapping stores all come from one list. Nothing
    // reads them on the audio thread: this plugin makes no sound. See
    // docs/demo.md.
    synth::addParametersTo (layout);

    return layout;
}

//==============================================================================
void DemoProcessor::setMappings (juce::Array<controllers::Mapping> mappings)
{
    // Two copies on purpose. The router matches against its own, on the audio
    // thread; `mappingsForApply` is the message thread's, used to work out where
    // a matched parameter should go. Sharing one would need a lock on the audio
    // side for something that changes only when the table is edited.
    const juce::ScopedLock lock (matchLock);

    mappingsForApply = mappings;
    router.setMappings (std::move (mappings));
}

DemoProcessor::HeldNotes DemoProcessor::getHeldNotes() const
{
    HeldNotes held;

    auto first = true;

    for (int channel = 0; channel < 16; ++channel)
    {
        const juce::uint64 words[] { heldLow [(size_t) channel].load (std::memory_order_relaxed),
                                     heldHigh[(size_t) channel].load (std::memory_order_relaxed) };

        for (int half = 0; half < 2; ++half)
            for (int bit = 0; bit < 64; ++bit)
            {
                if ((words[half] & (juce::uint64 (1) << bit)) == 0)
                    continue;

                const auto note = half * 64 + bit;

                ++held.count;

                if (first || note < held.lowestNote)
                {
                    held.lowestNote    = note;
                    held.lowestChannel = channel + 1;
                }

                if (first || note > held.highestNote)
                {
                    held.highestNote    = note;
                    held.highestChannel = channel + 1;
                }

                first = false;
            }
    }

    return held;
}

void DemoProcessor::handleAsyncUpdate()
{
    juce::Array<MidiRouter::Result::Match> arrived;
    juce::Array<controllers::Mapping> mappings;
    juce::Array<juce::MidiMessage> candidates;
    juce::Array<juce::MidiMessage> tuning;
    juce::Array<juce::MidiRPNMessage> parameters;
    std::optional<double> volumeDb;

    {
        const juce::ScopedLock lock (matchLock);

        arrived.swapWith (pendingMatches);
        candidates.swapWith (pendingLearnCandidates);
        tuning.swapWith (pendingTuningSysex);
        parameters.swapWith (pendingParameters);
        mappings = mappingsForApply;

        std::swap (volumeDb, pendingMasterVolumeDb);
    }

    // Tuning first: parsing a sysex and stepping a tuning program both allocate,
    // which is why they waited for this thread. `handleRpn` answers whether it
    // recognised the parameter, so anything else falls through untouched.
    auto tuningMoved = false;

    for (const auto& message : tuning)
    {
        tuningSource.handleSysex (message);
        tuningMoved = true;
    }

    for (const auto& rpn : parameters)
        tuningMoved = tuningSource.handleRpn (rpn) || tuningMoved;

    if (tuningMoved && onTuningChanged != nullptr)
        onTuningChanged();

    // Reported before the matches are applied: a row learned from these is not
    // in `mappings` yet, and adding it comes back through setMappings.
    if (! candidates.isEmpty() && onLearnCandidates != nullptr)
        onLearnCandidates (candidates);

    if (volumeDb.has_value() && onMasterVolume != nullptr)
        onMasterVolume (*volumeDb);

    for (const auto& match : arrived)
    {
        if (! juce::isPositiveAndBelow (match.mappingIndex, mappings.size()))
            continue;

        const auto& mapping = mappings[match.mappingIndex];

        auto* parameter = synth::parameterAt (apvts, mapping.parameterIndex);

        if (parameter == nullptr)
            continue;

        // `valueFor` works in the parameter's own unit and needs to know where
        // it is now — four of the five modes are relative to that.
        const auto range   = parameter->getNormalisableRange();
        const auto current = (double) range.convertFrom0to1 (parameter->getValue());

        if (const auto target = midiMapper::valueFor (mapping, match.message, current))
        {
            // Snapped as well as clamped: a controller sweeping a bank number
            // should land on banks, not between them. The mapping's own limits
            // were already snapped when they were typed, so this only has to
            // catch what the mode computed in between them.
            const controllers::Range bounds { (double) range.start,
                                              (double) range.end,
                                              (double) range.interval };

            parameter->setValueNotifyingHost (range.convertTo0to1 ((float) bounds.snap (*target)));
        }
    }
}

//==============================================================================
void DemoProcessor::prepareToPlay (double, int)
{
    outputLevelLeft .store (0.0f, std::memory_order_relaxed);
    outputLevelRight.store (0.0f, std::memory_order_relaxed);
}

void DemoProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;

    // The sidebar reads the block and says what it acted on; the buffer itself
    // is only rebuilt afterwards. See the note on MidiRouter for why it does
    // not edit in place — and AudioProcessor::processBlock's own contract, which
    // makes anything left here the plugin's MIDI *output*.
    router.process (midiMessages, routed);

    if (! routed.consumed.isEmpty())
    {
        // Rebuilt rather than erased in place: MidiBuffer has no random-access
        // removal, and swapping a freshly built one is what JUCE's own MIDI
        // effects do. `kept` is a member, so this allocates only when a block
        // is bigger than any before it.
        kept.clear();

        for (const auto metadata : midiMessages)
            if (! routed.consumed.contains (metadata.samplePosition))
                kept.addEvent (metadata.getMessage(), metadata.samplePosition);

        midiMessages.swapWith (kept);
    }

    // Which notes are down, tracked here because the tuning page's interval and
    // the presets page's frequency pair are both "what is sounding". Two 64-bit
    // words per channel cover 128 notes, so a note on or off is one atomic
    // read-modify-write and the message thread reads whatever it finds.
    //
    // Note on with velocity 0 is a note off — Table II, and the commonest way a
    // keyboard sends one — so `isNoteOff (true)` has to come first.
    for (const auto metadata : midiMessages)
    {
        const auto message = metadata.getMessage();

        if (! message.isNoteOnOrOff())
            continue;

        const auto channel = message.getChannel() - 1;
        const auto note    = message.getNoteNumber();

        if (! juce::isPositiveAndBelow (channel, 16) || ! juce::isPositiveAndBelow (note, 128))
            continue;

        auto& word = note < 64 ? heldLow[(size_t) channel] : heldHigh[(size_t) channel];
        const auto bit = juce::uint64 (1) << (note % 64);

        if (message.isNoteOff (true))
            word.fetch_and (~bit, std::memory_order_relaxed);
        else
            word.fetch_or (bit, std::memory_order_relaxed);
    }

    // Handed to the editor without blocking. A block that cannot take the lock
    // drops its lines; see the note in the header.
    if (! routed.forMonitor.isEmpty())
    {
        if (const juce::ScopedTryLock lock (monitorLock); lock.isLocked())
            for (const auto& message : routed.forMonitor)
                pendingMessages.add (message);
    }

    // Parameter moves go the same way, and for a stronger reason: working out
    // where a parameter should land and telling the host about it are both
    // message-thread work.
    if (! routed.matches.isEmpty() || ! routed.learnCandidates.isEmpty()
        || ! routed.tuningSysex.isEmpty() || ! routed.parameters.isEmpty()
        || routed.masterVolumeDb.has_value())
    {
        if (const juce::ScopedTryLock lock (matchLock); lock.isLocked())
        {
            for (const auto& match : routed.matches)
                pendingMatches.add (match);

            for (const auto& candidate : routed.learnCandidates)
                pendingLearnCandidates.add (candidate);

            for (const auto& message : routed.tuningSysex)
                pendingTuningSysex.add (message);

            for (const auto& rpn : routed.parameters)
                pendingParameters.add (rpn);

            if (routed.masterVolumeDb.has_value())
                pendingMasterVolumeDb = routed.masterVolumeDb;

            triggerAsyncUpdate();
        }
    }

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
