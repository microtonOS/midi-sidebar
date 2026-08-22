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

        /** A message that drives a mapping, with the row it drives and where
            the controller now stands.

            Where the *parameter* should go is still the owner's to work out —
            four of the five modes are relative to where it already is, and only
            the owner knows that. `midiMapper::valueFor` is the other half. */
        struct Match
        {
            int mappingIndex = 0;
            juce::MidiMessage message;

            /** The controller's position after this message, and the largest it
                can be — 127 for a lone MSB or a touch message, 16383 once the
                row has an LSB. Resolved here rather than by the owner because
                only the router holds the registers; see `midiMapper::Register`
                for why a 14-bit controller is a register and not a pair. */
            int value = 0;
            int highest = midiMapper::highestValue;
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

        /** An MPE Configuration Message, if one arrived.

            The zone comes from the *channel* it was sent on — 1 is the lower
            zone, 16 the upper — and `memberChannels` is its `mm` byte, which is
            a count and not a channel number. Zero deactivates that zone. */
        struct MpeConfiguration
        {
            channels::Zone zone = channels::Zone::lower;
            int memberChannels = 0;
        };

        std::optional<MpeConfiguration> mpeConfiguration;

        /** Master Fine and Coarse Tuning, in cents, from the two Device Control
            messages CA-025 added. Displacements from A440 applied to the whole
            instrument, so they compose with whatever tuning is in force rather
            than replacing it. Last in the block wins, as with the volume. */
        std::optional<double> masterFineCents, masterCoarseCents;

        void clearQuick()
        {
            consumed.clearQuick();
            forMonitor.clearQuick();
            parameters.clearQuick();
            matches.clearQuick();
            learnCandidates.clearQuick();
            tuningSysex.clearQuick();
            masterVolumeDb.reset();
            masterFineCents.reset();
            masterCoarseCents.reset();
            mpeConfiguration.reset();
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

        // One register per row, kept in step by index. Resized rather than
        // rebuilt so that editing an unrelated row does not lose where a
        // controller had got to.
        registers.resize (mappings.size());
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

            // **The MPE Configuration Message is read before the filter.**
            // It arrives on a manager channel — 1 for the lower zone, 16 for
            // the upper — and a plugin configured for one zone is generally not
            // listening to the other's manager channel, so filtering first would
            // make it impossible to ever be reconfigured onto that zone. It is
            // configuration rather than performance, like a system message.
            //
            // Its own detector, because feeding the main one an unfiltered
            // stream would have it consume control changes on channels the
            // plugin is deliberately ignoring.
            if (message.isController())
                if (const auto mcm = mpeConfigurationFrom (message))
                    result.mpeConfiguration = *mcm;

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

            // The two tuning members of the same family. Passed on for the same
            // reason the volume is: a broadcast is addressed to everything
            // downstream as well.
            if (const auto fine = deviceControl::masterFineTuningFrom (message))
                result.masterFineCents = *fine;

            if (const auto coarse = deviceControl::masterCoarseTuningFrom (message))
                result.masterCoarseCents = *coarse;

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

                // **Still shown.** The `continue` below skips *matching* — a
                // message teaching a row should not also drive something — but
                // it must not skip the monitor, or the stream goes quiet at
                // exactly the moment the end-user is moving a controller and
                // wants to see it arrive.
                if (midiMonitor::lineFor (message).has_value())
                    result.forMonitor.add (message);

                continue;
            }

            // A message can drive more than one row — the same controller
            // aimed at two parameters is a legitimate thing to want — so this
            // does not stop at the first match.
            for (int i = 0; i < mappings.size(); ++i)
            {
                const auto& mapping = mappings[i];

                if (! midiMapper::matches (mapping, message))
                    continue;

                if (i >= registers.size())
                    continue;

                auto& reg = registers.getReference (i);
                const auto hasLsb = mapping.lsb.has_value();

                if (mapping.source == controllers::Source::control)
                {
                    const auto number = message.getControllerNumber();

                    // Which byte arrived decides what happens: a high byte sets
                    // the coarse value *and clears the fine one*, a low byte
                    // refines what is there. An LSB before any MSB has nothing
                    // to refine, so it is stored and not acted on.
                    if (mapping.msb.has_value() && number == *mapping.msb)
                        reg.applyMsb (midiMapper::valueOf (message));
                    else
                        reg.applyLsb (midiMapper::valueOf (message));

                    if (! reg.started)
                        continue;
                }
                else
                {
                    // Aftertouch has no pair, so the register is a plain store.
                    reg.applyMsb (midiMapper::valueOf (message));
                }

                result.matches.add ({ i, message,
                                      reg.valueOf (hasLsb),
                                      midiMapper::Register::highestOf (hasLsb) });
            }

            if (midiMonitor::lineFor (message).has_value())
                result.forMonitor.add (message);
        }
    }

private:
    /** RPN 6 on channel 1 or 16, as a zone and a member-channel count.

        Not consumed: an MCM is addressed to every MPE receiver downstream as
        well, and swallowing it would reconfigure this plugin while leaving the
        rest of the chain on the old layout. */
    std::optional<Result::MpeConfiguration> mpeConfigurationFrom (const juce::MidiMessage& m)
    {
        const auto channel = m.getChannel();

        if (channel != channels::lowerManagerChannel && channel != channels::upperManagerChannel)
            return {};

        const auto parsed = mcmDetector.tryParse (channel,
                                                  m.getControllerNumber(),
                                                  m.getControllerValue());

        if (! parsed.has_value() || parsed->isNRPN
            || parsed->parameterNumber != channels::mpeConfigurationRpn)
            return {};

        // `mm` is the MSB of data entry. `MidiRPNDetector` reports 14 bits once
        // an LSB has followed, so the top seven are taken — MPE says nothing
        // about an LSB here and a sender that adds one means the same count.
        const auto members = parsed->is14BitValue ? parsed->value >> 7 : parsed->value;

        return Result::MpeConfiguration {
            channel == channels::lowerManagerChannel ? channels::Zone::lower
                                                     : channels::Zone::upper,
            juce::jlimit (0, channels::numChannels - 1, members) };
    }

    /** Sees channels 1 and 16 whatever the filter says; see `process`. */
    juce::MidiRPNDetector mcmDetector;

    int learning = controllers::noParameter;
    channels::Setup setup;
    juce::Array<controllers::Mapping> mappings;

    /** Where each row's controller currently is. Parallel to `mappings`. */
    juce::Array<midiMapper::Register> registers;

    juce::MidiRPNDetector rpnDetector;

    JUCE_LEAK_DETECTOR (MidiRouter)
};

} // namespace microtonos::sidebar
