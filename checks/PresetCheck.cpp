// Program change and bank select, through the router.
//
// The rule under test is p13 of the Complete MIDI 1.0 Detailed Specification:
// "Bank Select alone must not change the program. The receiver remembers the
// bank and applies it when the Program Change arrives." So CC 0 and CC 32 are a
// register, not events, and only the program change is reported.
#include <midi_sidebar/midi_sidebar.h>

#include "Check.h"

using namespace microtonos::sidebar;
using namespace checks;

namespace
{
    juce::MidiMessage cc (int channel, int number, int value)
    {
        return juce::MidiMessage::controllerEvent (channel, number, value);
    }

    struct Rig
    {
        MidiRouter router;
        MidiRouter::Result result;

        Rig()
        {
            // Omni on, every channel, so nothing here is lost to the filter.
            channels::Setup setup;
            setup.omniOn = true;
            router.setChannels (setup);
        }

        /** Sends one message and returns what the block reported. */
        MidiRouter::Result& send (const juce::MidiMessage& m)
        {
            juce::MidiBuffer b;
            b.addEvent (m, 0);
            router.process (b, result);
            return result;
        }
    };
}

int main()
{
    //  Bank select alone does nothing -------------------------------------------
    {
        Rig rig;

        check (! rig.send (cc (1, 0, 2)).programChange.has_value(),
               "a bank select MSB reports nothing on its own");
        check (! rig.send (cc (1, 32, 5)).programChange.has_value(),
               "nor does the LSB");

        const auto& r = rig.send (juce::MidiMessage::programChange (1, 7));

        check (r.programChange.has_value(), "the program change is the event");
        eq (r.programChange->program, 7, "carrying its program");
        check (r.programChange->bank.has_value(), "and the bank that was pending");
        eq (*r.programChange->bank, 2 * 128 + 5, "which is MSB * 128 + LSB");
    }

    //  Both are consumed ----------------------------------------------------------
    {
        Rig rig;

        eq (rig.send (cc (1, 0, 1)).consumed.size(), 1, "bank select is taken out of the stream");
        eq (rig.send (juce::MidiMessage::programChange (1, 3)).consumed.size(), 1,
            "and so is the program change - program management is the plugin's");
    }

    //  A program change with no bank select keeps the current bank ------------------
    {
        Rig rig;

        const auto& r = rig.send (juce::MidiMessage::programChange (1, 9));

        check (r.programChange.has_value(), "the program change still arrives");
        check (! r.programChange->bank.has_value(),
               "with no bank - which means keep the one you are on, not bank zero");
    }

    //  The pending bank is spent -----------------------------------------------------
    {
        Rig rig;

        rig.send (cc (1, 0, 3));
        check (rig.send (juce::MidiMessage::programChange (1, 1)).programChange->bank.has_value(),
               "the first program change takes the bank");
        check (! rig.send (juce::MidiMessage::programChange (1, 2)).programChange->bank.has_value(),
               "a second one does not repeat it");
    }

    //  Either byte alone is a legal selection -----------------------------------------
    {
        Rig msbOnly, lsbOnly;

        msbOnly.send (cc (1, 0, 2));
        eq (*msbOnly.send (juce::MidiMessage::programChange (1, 0)).programChange->bank, 256,
            "an MSB alone selects bank 256 - the LSB defaults to zero");

        lsbOnly.send (cc (1, 32, 9));
        eq (*lsbOnly.send (juce::MidiMessage::programChange (1, 0)).programChange->bank, 9,
            "an LSB alone selects bank 9");
    }

    //  The register is per channel -------------------------------------------------------
    {
        Rig rig;

        rig.send (cc (1, 0, 1));
        rig.send (cc (2, 0, 4));

        const auto onTwo = *rig.send (juce::MidiMessage::programChange (2, 0)).programChange;
        eq (*onTwo.bank, 4 * 128, "channel 2 gets its own pending bank");
        eq (onTwo.channel, 2, "and the event says which channel");

        const auto onOne = *rig.send (juce::MidiMessage::programChange (1, 0)).programChange;
        eq (*onOne.bank, 1 * 128, "channel 1's is untouched by it");
    }

    //  The full range ----------------------------------------------------------------------
    {
        Rig rig;

        rig.send (cc (1, 0, 127));
        rig.send (cc (1, 32, 127));

        eq (*rig.send (juce::MidiMessage::programChange (1, 127)).programChange->bank, 16383,
            "127/127 is the last of 16384 banks");
    }

    //  Neither number can be mapped to anything else ------------------------------------------
    {
        check (controllers::isCcUnavailable (0), "CC 0 cannot carry a mapping");
        check (controllers::isCcUnavailable (32), "nor can CC 32");
    }

    return report ("PresetCheck");
}
