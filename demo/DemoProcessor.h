#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <midi_sidebar/midi_sidebar.h>

#include "TuningSource.h"
#include "DemoSynth.h"
#include "PresetStore.h"

namespace microtonos::sidebar::demo
{

//==============================================================================
/** The thinnest processor that will host an editor.

    This exists so the sidebar has somewhere to live: a JUCE module cannot be
    loaded in a host or rendered by the snapshot tool on its own. It passes
    audio through untouched and owns nothing but the parameters the sidebar
    needs.
*/
class DemoProcessor final : public juce::AudioProcessor,
                           private juce::AsyncUpdater
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

    //==========================================================================
    /** The sidebar's MIDI layer, driven from `processBlock`. Owned by the
        processor rather than the editor: MIDI arrives whether or not a window
        is open, and a mapping that stopped working when you closed the GUI
        would be a strange plugin. */
    MidiRouter router;

    /** What the last block found, for the editor's timer to drain. Written on
        the audio thread and read on the message thread, so it is guarded — a
        lock-free queue would be better and is the next step; see TODO.md.

        `tryEnter` on the audio side, never `enter`: a block that cannot have the
        lock drops its monitor lines rather than waiting for the GUI. Losing a
        line from a display of three is nothing; blocking the audio thread is
        everything. */
    juce::CriticalSection monitorLock;
    juce::Array<juce::MidiMessage> pendingMessages;

    /** The mappings the router matches against, pushed in when the table
        changes. Kept here rather than read from the editor: MIDI arrives whether
        or not a window is open. */
    void setMappings (juce::Array<controllers::Mapping> mappings);

    /** Called on the message thread with the messages `MIDI learn` collected,
        so the owner can feed them to its `MidiLearner`.

        Messages rather than a finished mapping: learning watches a whole
        gesture before it concludes anything — see MidiLearner.h — and the thing
        that watches lives on the page, beside the monitor it reports into. */
    std::function<void (const juce::Array<juce::MidiMessage>&)> onLearnCandidates;

    /** Called on the message thread when a Master Volume system exclusive has
        asked for a level, in decibels on the fader's own scale. */
    std::function<void (double)> onMasterVolume;

    /** Where the tuning comes from. On the processor rather than the editor so
        that a tuning survives the window closing, exactly like the router's
        mappings — MIDI arrives whether or not anything is open.

        Touched only from the message thread: it reads files, queries MTS-ESP
        and builds strings, none of which belongs in an audio callback. */
    TuningSource tuningSource;

    /** Called on the message thread after anything has changed the tuning, so
        the editor can push the new state at the page. */
    std::function<void()> onTuningChanged;

    /** Called on the message thread when an MPE Configuration Message has
        reconfigured the zone, so the channels page can show it. */
    std::function<void (channels::Setup)> onChannelsChanged;

    /** The presets, and where the plugin is among them. On the processor beside
        the tuning source, for the same reason: a program change arrives whether
        or not a window is open. */
    PresetStore presetStore { apvts, synth::parameterIds() };

    /** Called on the message thread after a program change has moved the
        preset, so the page can show it. */
    std::function<void()> onPresetChanged;

    /** What the router is filtering by. The editor reads it to apply an MCM and
        writes it back; the channels page owns it while a window is open. */
    channels::Setup getChannels() const { return router.getChannels(); }

    /** The lowest and highest notes currently held, with the channels they
        arrived on, or nothing when the keyboard is empty. Drives the tuning
        page's interval and the presets page's frequency pair. */
    struct HeldNotes
    {
        int lowestNote = 0, highestNote = 0;
        int lowestChannel = 1, highestChannel = 1;
        int count = 0;
    };

    HeldNotes getHeldNotes() const;

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    /** Scratch reused every block, so a steady stream of MIDI allocates nothing
        after the first few blocks have grown them. */
    MidiRouter::Result routed;
    juce::MidiBuffer kept;

    /** Matches waiting to move a parameter.

        Applied on the **message thread**, through `AsyncUpdater`, because
        `setValueNotifyingHost` is not something to call from an audio callback.
        On the processor rather than the editor, so a controller still works with
        the window closed — which is most of the time in a real session. */
    void handleAsyncUpdate() override;

    juce::CriticalSection matchLock;
    juce::Array<MidiRouter::Result::Match> pendingMatches;
    juce::Array<controllers::Mapping> mappingsForApply;
    juce::Array<juce::MidiMessage> pendingLearnCandidates;
    juce::Array<juce::MidiMessage> pendingTuningSysex;
    juce::Array<juce::MidiRPNMessage> pendingParameters;
    std::optional<double> pendingMasterVolumeDb;
    std::optional<double> pendingMasterFineCents, pendingMasterCoarseCents;
    std::optional<MidiRouter::Result::MpeConfiguration> pendingMpeConfiguration;
    std::optional<MidiRouter::Result::BendSensitivity>   pendingBendSensitivity;
    std::optional<MidiRouter::Result::ProgramChange> pendingProgramChange;

    /** Which notes are down, one bit per note per channel. Written on the audio
        thread and read on the message thread; `std::atomic` per channel rather
        than a lock, because a torn read costs one frame of a read-out that is
        redrawn many times a second. */
    std::array<std::atomic<juce::uint64>, 16> heldLow {}, heldHigh {};

    /** Cached in the constructor: looking a parameter up by name on the audio
        thread allocates and is not realtime safe. */
    std::atomic<float>* volumeGain = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DemoProcessor)
};

} // namespace microtonos::sidebar::demo
