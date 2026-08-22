// The Master Volume system exclusive, and its decibel law.
//
//     F0 7F <device id> 04 01 vv vv F7      LSB first, 00 00 = off
//
// Sub-ID#1 04, Complete MIDI 1.0 Detailed Specification 4.2.1 p57. The curve is
// General MIDI 2 v1.2a §3.3.4, referenced by §4.1: "the square of the value is
// proportional to the volume", which is 40·log10(v/vmax) in decibels.
#include <midi_sidebar/midi_sidebar.h>

#include "Check.h"

using namespace microtonos::sidebar;
using namespace checks;

namespace
{
    juce::MidiMessage sysex (std::initializer_list<juce::uint8> bytes)
    {
        const std::vector<juce::uint8> v (bytes);
        return juce::MidiMessage::createSysExMessage (v.data(), (int) v.size());
    }

    juce::MidiMessage masterVolume (int deviceId, int value)
    {
        return sysex ({ 0x7f, (juce::uint8) deviceId, 0x04, 0x01,
                        (juce::uint8) (value & 0x7f),
                        (juce::uint8) ((value >> 7) & 0x7f) });
    }
}

int main()
{
    //  Parsing -----------------------------------------------------------------
    {
        const auto full = deviceControl::masterVolumeFrom (masterVolume (0x7f, 16383));
        check (full.has_value() && *full == 16383, "7F 7F parses as 16383");

        const auto off = deviceControl::masterVolumeFrom (masterVolume (0x7f, 0));
        check (off.has_value() && *off == 0, "00 00 parses as 0");

        const auto mid = deviceControl::masterVolumeFrom (masterVolume (0x7f, 8192));
        check (mid.has_value() && *mid == 8192, "the value is LSB first");
    }

    //  Addressing ---------------------------------------------------------------
    {
        check (! deviceControl::masterVolumeFrom (masterVolume (0x03, 8192)).has_value(),
               "a message addressed to one device id is ignored - a plugin has none");
        check (! deviceControl::masterVolumeFrom (
                     sysex ({ 0x7f, 0x7f, 0x04, 0x02, 0x00, 0x40 })).has_value(),
               "master balance is not master volume");
        check (! deviceControl::masterVolumeFrom (
                     sysex ({ 0x7e, 0x7f, 0x04, 0x01, 0x00, 0x40 })).has_value(),
               "the non-real-time header (7E) is not this message");
        check (! deviceControl::masterVolumeFrom (
                     sysex ({ 0x7f, 0x7f, 0x04, 0x01, 0x00 })).has_value(),
               "a truncated message is refused");
        check (! deviceControl::masterVolumeFrom (
                     juce::MidiMessage::controllerEvent (1, 7, 100)).has_value(),
               "a control change is not a system exclusive");
    }

    //  The curve ----------------------------------------------------------------
    {
        near (deviceControl::decibelsFor (16383), 0.0, 1e-9, "full scale is 0 dB");
        near (deviceControl::decibelsFor (0), (double) metrics::floorDb, 1e-9,
              "zero is the floor, since the law would give minus infinity");

        // Halving the *value* is -12.04 dB under a square law, not -6.02. This
        // is the assertion that would catch someone "fixing" it to 20·log10.
        near (deviceControl::decibelsFor (16383 / 2), -12.04, 0.05,
              "half scale is about -12 dB - the square law, not a linear one");

        // GM2's own worked example is for CC 7 at 96 of 127.
        near (deviceControl::decibelsFor ((int) (16383.0 * 96.0 / 127.0)),
              40.0 * std::log10 (96.0 / 127.0), 0.01,
              "matches 40*log10 at GM2's own example point");

        check (deviceControl::decibelsFor (1) >= (double) metrics::floorDb,
               "nothing falls below the floor");
        check (deviceControl::decibelsFor (999999) <= 0.0, "nothing rises above 0 dB");
    }

    //  Master Fine Tuning, CA-025 -------------------------------------------------
    {
        const auto fine = [] (int lsb, int msb)
        {
            return deviceControl::masterFineTuningFrom (
                sysex ({ 0x7f, 0x7f, 0x04, 0x03, (juce::uint8) lsb, (juce::uint8) msb }));
        };

        // CA-025's own three rows: 00 00, 00 40, 7F 7F.
        near (*fine (0x00, 0x40), 0.0, 1e-9, "00 40 is no displacement");
        near (*fine (0x00, 0x00), -100.0, 1e-9, "00 00 is -100 cents");
        near (*fine (0x7f, 0x7f), 100.0 / 8192.0 * 8191.0, 1e-9, "7F 7F is +99.988 cents");

        // One step is 100/8192 — coarser than the 0.0061 c of an MTS frequency
        // field, which is why a displacement is applied at query time rather
        // than baked into the table.
        near (*fine (0x01, 0x40) - *fine (0x00, 0x40), 100.0 / 8192.0, 1e-12,
              "one step is 100/8192 cents, about 0.0122");

        check (! deviceControl::masterFineTuningFrom (
                     sysex ({ 0x7f, 0x03, 0x04, 0x03, 0x00, 0x40 })).has_value(),
               "and a message aimed at one device id is ignored");
    }

    //  Master Coarse Tuning, whose LSB is always 0 ---------------------------------
    {
        const auto coarse = [] (int lsb, int msb)
        {
            return deviceControl::masterCoarseTuningFrom (
                sysex ({ 0x7f, 0x7f, 0x04, 0x04, (juce::uint8) lsb, (juce::uint8) msb }));
        };

        near (*coarse (0x00, 0x40), 0.0, 1e-9, "00 40 is no displacement");
        near (*coarse (0x00, 0x00), -6400.0, 1e-9, "00 00 is -64 semitones");
        near (*coarse (0x00, 0x7f), 6300.0, 1e-9, "00 7F is +63 semitones");

        // "Note that the LSB is always 0" — so a sender putting something there
        // is malformed, and accepting it would make the message mean something
        // the specification does not define.
        check (! coarse (0x01, 0x40).has_value(), "a non-zero LSB is refused");
    }

    //  The three do not collide -----------------------------------------------------
    {
        const auto volumeMessage = masterVolume (0x7f, 8192);

        check (! deviceControl::masterFineTuningFrom (volumeMessage).has_value(),
               "master volume is not read as fine tuning");
        check (! deviceControl::masterCoarseTuningFrom (volumeMessage).has_value(),
               "nor as coarse tuning");

        const auto fineMessage = sysex ({ 0x7f, 0x7f, 0x04, 0x03, 0x00, 0x40 });
        check (! deviceControl::masterVolumeFrom (fineMessage).has_value(),
               "and fine tuning is not read as a volume");
    }

    //  The monitor names all three ---------------------------------------------------
    {
        const auto fine = midiMonitor::lineFor (sysex ({ 0x7f, 0x7f, 0x04, 0x03, 0x00, 0x60 }));
        check (fine.has_value() && fine->contains ("master fine tuning"),
               "the monitor names master fine tuning");
        check (fine.has_value() && fine->contains ("c"), "and shows its displacement");

        const auto coarse = midiMonitor::lineFor (sysex ({ 0x7f, 0x7f, 0x04, 0x04, 0x00, 0x42 }));
        check (coarse.has_value() && coarse->contains ("master coarse tuning"),
               "and master coarse tuning");
        check (coarse.has_value() && coarse->contains ("2 st"), "as +2 semitones");
    }

    //  The monitor still names it ------------------------------------------------
    {
        const auto line = midiMonitor::lineFor (masterVolume (0x7f, 8192));

        check (line.has_value() && line->contains ("master volume"),
               "the monitor names a master volume message");
        check (line.has_value() && line->contains ("dB"),
               "and shows the level it asked for");
    }

    return report ("DeviceControlCheck");
}
