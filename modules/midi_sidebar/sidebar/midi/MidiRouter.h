#pragma once

#include <juce_audio_basics/juce_audio_basics.h>

#include "../pages/ChannelsState.h"
#include "../pages/ControllersState.h"
#include "../tuning/MtsSysex.h"
#include "MidiDeviceControl.h"
#include "MidiFilter.h"
#include "MidiLearner.h"
#include "MidiMapper.h"
#include "MidiMonitor.h"

namespace microtonos::sidebar
{

//==============================================================================
/** The sidebar's MIDI layer: reads a buffer, says what it found, and says what
    should not be passed on.

    **It does not edit the buffer.** `AudioProcessor::processBlock` says "any
    messages left in the MIDI buffer when this method has finished are assumed to
    be the processor's MIDI output … your processor should be careful to clear
    any incoming messages from the array if it doesn't want them to be passed-on"
    — so pass-through is the default and consuming is the deliberate act. This
    class decides *which* messages are the deliberate act and hands the list
    back; the owner rebuilds its buffer. That keeps the decision testable and the
    ownership plain, and it is how `juce::MPEZoneLayout` and `MPEInstrument`
    treat a buffer too: both take it `const`.

    Nothing here allocates while processing. The results live in arrays the
    caller can reserve once, and no `juce::String` is built on the audio thread —
    monitor lines are composed from `MidiMonitor` on the message thread, from the
    messages this collects.
*/
class MidiRouter
{
public:
    //==========================================================================
    /** What one block produced. Cleared and refilled by `process`, so a caller
        that keeps one of these between blocks never allocates twice. */
    struct Result
    {
        /** Indices into the incoming buffer that the sidebar acted on and the
            host should not see. Ascending. */
        juce::Array<int> consumed;

        /** Messages worth showing, in arrival order, already filtered to what
            the monitor displays. Formatted later, off the audio thread. */
        juce::Array<juce::MidiMessage> forMonitor;

        /** Registered and non-registered parameters recognised this block. */
        juce::Array<juce::MidiRPNMessage> parameters;

        /** A message that drives a mapping, with the row it drives.

            The *message* is reported rather than a value, because working out
            where the parameter should go needs to know where it is now — and
            only the owner knows that. `midiMapper::valueFor` is the other half,
            called by whoever holds the parameters. */
        struct Match
        {
            int mappingIndex = 0;
            juce::MidiMessage message;
        };

        juce::Array<Match> matches;

        /** Messages that could name a control, collected while `MIDI learn` is
            armed and empty otherwise.

            **Candidates, not a decision.** Learning watches a whole gesture
            before it concludes anything — see MidiLearner.h for why taking the
            first message fails on real hardware — and a gesture spans many
            blocks. So the audio thread does no more than copy; `MidiLearner`,
            on the message thread, is what decides. */
        juce::Array<juce::MidiMessage> learnCandidates;

        /** MIDI tuning system exclusives, to be parsed off the audio thread.

            **Consumed**, unlike Master Volume: a tuning message is addressed to
            this instrument and acted on by it, where a broadcast volume is
            addressed to everything downstream as well. Carried as whole
            messages because `mtsSysex::parse` allocates a `juce::String` for the
            name, which is not something to do here. */
        juce::Array<juce::MidiMessage> tuningSysex;

        /** The volume a Master Volume system exclusive asked for, in decibels
            on the fader's own scale. Last one in the block wins: several in one
            block is a fader being dragged somewhere upstream, and only where it
            ended up matters. */
        std::optional<double> masterVolumeDb;

        void clearQuick()
        {
            consumed.clearQuick();
            forMonitor.clearQuick();
            parameters.clearQuick();
            matches.clearQuick();
            learnCandidates.clearQuick();
            tuningSysex.clearQuick();
            masterVolumeDb.reset();
        }
    };

    //==========================================================================
    void setChannels (channels::Setup newSetup) { setup = newSetup; }
    const channels::Setup& getChannels() const noexcept { return setup; }

    /** The rows of the controllers table. Copied rather than referenced: the
        table lives in the editor, which is destroyed whenever the window
        closes, and MIDI keeps arriving. */
    void setMappings (juce::Array<controllers::Mapping> newMappings)
    {
        mappings = std::move (newMappings);
    }

    //==========================================================================
    /** Starts collecting the messages a gesture is made of, for the
        right-click menu's `MIDI learn`.

        Armed rather than applied here, and collecting rather than deciding:
        this class does not own the table, the row has to be added on the
        thread that does, and the decision needs a whole gesture rather than
        one message. `Result::learnCandidates` is how the messages come back.

        The argument is not used — the parameter being learned belongs to the
        `MidiLearner` that will hold it — but it keeps the call reading like
        what it does at the call site, and `controllers::noParameter` cancels,
        which is what closing the menu without choosing should do. */
    void learnFor (int parameterIndex) { learning = parameterIndex; }

    bool isLearning() const noexcept { return learning != controllers::noParameter; }

    /** Forgets any half-received parameter number. Worth calling when transport
        stops or the plugin is reset: a selection left half-made would otherwise
        collect the next data entry that came along. */
    void reset() { rpnDetector.reset(); }

    //==========================================================================
    /** Reads the block. The buffer is `const`; see the class note.

        Order matters here. A message is filtered by channel *first*, because a
        channel the plugin is not listening to should not reach the monitor
        either — "messages and tunings on other channels are ignored"
        (docs/channels.md), and a monitor showing them would contradict the
        filter section sitting two pages away.
    */
    void process (const juce::MidiBuffer& buffer, Result& result)
    {
        result.clearQuick();

        for (const auto metadata : buffer)
        {
            const auto message = metadata.getMessage();

            // System messages have no channel, so the filter does not apply:
            // transport and system exclusive belong to the whole stream.
            if (message.getChannel() > 0 && ! midiFilter::listensTo (setup, message.getChannel()))
                continue;

            // A control change may be part of an RPN, in which case it is not a
            // control change to anybody: the specification makes data entry
            // meaningless without the selection before it, so the four bytes are
            // one event. `MidiRPNDetector` is what decides.
            if (message.isController())
            {
                if (const auto parsed = rpnDetector.tryParse (message.getChannel(),
                                                              message.getControllerNumber(),
                                                              message.getControllerValue()))
                {
                    result.parameters.add (*parsed);
                    result.consumed.add (metadata.samplePosition);
                    continue;
                }
            }

            // A broadcast Master Volume moves the sidebar's fader. Not
            // consumed: it is addressed to every device downstream as well.
            if (const auto volume = deviceControl::masterVolumeFrom (message))
                result.masterVolumeDb = deviceControl::decibelsFor (*volume);

            // A tuning system exclusive is ours, so it is taken out of the
            // stream. Recognised by its two sub-ID bytes only — the parse
            // itself allocates and happens on the message thread.
            if (message.isSysEx())
            {
                const auto* data = message.getSysExData();

                if (data != nullptr && message.getSysExDataSize() >= 4
                    && (data[0] == mtsSysex::nonRealTime || data[0] == mtsSysex::realTime)
                    && data[2] == mtsSysex::tuningSubId)
                {
                    result.tuningSysex.add (message);
                    result.consumed.add (metadata.samplePosition);
                    continue;
                }
            }

            // Learning comes before matching: a message being used to teach a
            // row is not also a message that should move something, and the row
            // it will make does not exist yet anyway.
            if (isLearning() && MidiLearner::isLearnable (message))
            {
                result.learnCandidates.add (message);
                continue;
            }

            // A message can drive more than one row — the same controller
            // aimed at two parameters is a legitimate thing to want — so this
            // does not stop at the first match.
            for (int i = 0; i < mappings.size(); ++i)
                if (midiMapper::matches (mappings[i], message))
                    result.matches.add ({ i, message });

            if (midiMonitor::lineFor (message).has_value())
                result.forMonitor.add (message);
        }
    }

private:
    int learning = controllers::noParameter;
    channels::Setup setup;
    juce::Array<controllers::Mapping> mappings;
    juce::MidiRPNDetector rpnDetector;

    JUCE_LEAK_DETECTOR (MidiRouter)
};

} // namespace microtonos::sidebar
