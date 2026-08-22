#pragma once

#include <optional>

#include <juce_audio_basics/juce_audio_basics.h>

#include "MidiDeviceControl.h"

namespace microtonos::sidebar
{

//==============================================================================
/** One MIDI message as one line of the controllers page's monitor.

    The wording is docs/controllers.md's, down to `CC` in capitals and the note
    given as a *number*: a retuned note number no longer means what its letter
    name says, which is why this plugin does not print `A4`.

    **Musical messages and transport only.** Timing clock arrives 24 times a
    quarter note and active sensing every 300 ms, so showing either would flush
    the three lines faster than they can be read — the monitor would stop being a
    record of what you did. Those return `nullopt` and are simply not shown.

    Composed here rather than by the owner, which the module used to leave to
    whoever was reading the MIDI. That was the right call while a line might have
    had to say `A4`; now that a note is a number, nothing in a line needs
    knowledge this module lacks.
*/
namespace midiMonitor
{
    /** Pitch bend's no-effect position, as 14 bits. The specification gives it
        as data bytes `00 40H`, which is 64 * 128 = 8192 (p19). Pitch bend is
        also the one 14-bit message "always transmitted with both data bytes",
        unlike the controllers whose LSB is optional. */
    inline constexpr int pitchBendCentre = 8192;

    /** `Bn` with a controller number of 120–127, which are not control changes.
        Table IV of the MIDI 1.0 Detailed Specification 4.2.1. */
    inline juce::String channelModeText (const juce::MidiMessage& m)
    {
        const auto number = m.getControllerNumber();
        const auto value  = m.getControllerValue();

        switch (number)
        {
            case 120: return "all sound off";
            case 121: return "reset all controllers";
            case 122: return value == 0 ? "local control off" : "local control on";
            case 123: return "all notes off";
            case 124: return "omni off";
            case 125: return "omni on";

            // The third byte is the number of channels, and zero has its own
            // meaning: "the number of channels equals the number of voices in
            // the receiver".
            case 126: return value == 0 ? "mono" : "mono " + juce::String (value);

            case 127: return "poly";
            default:  break;
        }

        return {};
    }

    /** A universal system exclusive named by what it is, or a manufacturer's
        named by whose it is and how long.

        Never dumped: the monitor is three lines and a bulk tuning dump is 408
        bytes. Table VIIa gives the sub-IDs. */
    inline juce::String sysexText (const juce::MidiMessage& m)
    {
        const auto* data = m.getSysExData();
        const auto  size = m.getSysExDataSize();

        if (data == nullptr || size < 1)
            return "Sysex";

        // F0 is not included in getSysExData, so data[0] is the ID byte.
        const auto id = data[0];

        if (id == 0x7e || id == 0x7f)
        {
            const auto sub1 = size > 2 ? data[2] : -1;
            const auto sub2 = size > 3 ? data[3] : -1;

            if (sub1 == 0x08)
            {
                switch (sub2)
                {
                    case 0x00: return "Sysex  bulk tuning dump request";
                    case 0x01: return "Sysex  bulk tuning dump";
                    case 0x02: return "Sysex  single note tuning change";
                    case 0x04: return "Sysex  key-based tuning dump";
                    case 0x05:
                    case 0x06: return "Sysex  scale/octave tuning dump";
                    case 0x08:
                    case 0x09: return "Sysex  scale/octave tuning";
                    default:   return "Sysex  MIDI tuning";
                }
            }

            // The one universal message the sidebar acts on, so it shows what
            // it asked for rather than only that it arrived. `deviceControl`
            // refuses anything not addressed to the broadcast id, which is the
            // same test the fader applies — so a line without a level is a
            // message aimed at somebody else, and says so by omission.
            if (sub1 == 0x04 && sub2 == 0x01)
            {
                if (const auto volume = deviceControl::masterVolumeFrom (m))
                    return "Sysex  master volume  "
                         + juce::String (deviceControl::decibelsFor (*volume), 1) + " dB";

                return "Sysex  master volume";
            }

            // The two CA-025 added. Shown with their displacement for the same
            // reason the volume is: the sidebar acts on them, so what they asked
            // for is worth reading.
            if (sub1 == 0x04 && sub2 == 0x03)
            {
                if (const auto cents = deviceControl::masterFineTuningFrom (m))
                    return "Sysex  master fine tuning  "
                         + juce::String (*cents, 2) + " c";

                return "Sysex  master fine tuning";
            }

            if (sub1 == 0x04 && sub2 == 0x04)
            {
                if (const auto cents = deviceControl::masterCoarseTuningFrom (m))
                    return "Sysex  master coarse tuning  "
                         + juce::String (*cents / 100.0, 0) + " st";

                return "Sysex  master coarse tuning";
            }

            if (sub1 == 0x04 && sub2 == 0x02) return "Sysex  master balance";
            if (sub1 == 0x09)                 return "Sysex  general MIDI";

            return "Sysex  universal";
        }

        // 00 introduces a three-byte manufacturer id; anything else is one byte.
        const auto idBytes = id == 0x00 ? 3 : 1;

        return "Sysex  id " + juce::String::toHexString (data, idBytes, 0).toUpperCase()
             + "  " + juce::String (size) + " bytes";
    }

    //==========================================================================
    /** The line for this message, or nothing if the monitor does not show it. */
    inline std::optional<juce::String> lineFor (const juce::MidiMessage& m)
    {
        const auto ch = "ch " + juce::String (m.getChannel()) + "  ";

        if (m.isNoteOn())
            return ch + "note on  " + juce::String (m.getNoteNumber())
                      + "  velocity " + juce::String (m.getVelocity());

        if (m.isNoteOff (false))
            return ch + "note off  " + juce::String (m.getNoteNumber());

        if (m.isAftertouch())
            return ch + "polytouch  " + juce::String (m.getNoteNumber())
                      + "  value " + juce::String (m.getAfterTouchValue());

        if (m.isChannelPressure())
            return ch + "aftertouch " + juce::String (m.getChannelPressureValue());

        if (m.isPitchWheel())
        {
            // Centred, and signed so the centre is visibly zero. JUCE reports
            // the raw 14 bits, 0 to 16383, where "the center (no effect)
            // position is achieved with data byte values of 00, 64 (00H, 40H)"
            // — that is 8192 (MIDI 1.0 Detailed Specification 4.2.1, p19).
            //
            // The range is therefore **-8192 to +8191**, not symmetric: 16384
            // values cannot sit evenly either side of a centre, and the spare
            // one is at the bottom. An explicit `+` keeps the reading
            // unambiguous, since a bare number here would look like a CC value.
            const auto bend = m.getPitchWheelValue() - pitchBendCentre;

            return ch + "pitchbend " + (bend > 0 ? "+" : "") + juce::String (bend);
        }

        if (m.isProgramChange())
            return ch + "PC " + juce::String (m.getProgramChangeNumber());

        if (m.isController())
        {
            // 120-127 are channel mode messages rather than control changes, and
            // say what they do rather than showing a number.
            if (const auto mode = channelModeText (m); mode.isNotEmpty())
                return ch + mode;

            // An MSB and its LSB are two independent messages on the wire;
            // pairing them is what a *mapping* does, not what the stream says.
            // So each is its own line.
            return ch + "CC " + juce::String (m.getControllerNumber())
                      + "  value " + juce::String (m.getControllerValue());
        }

        if (m.isSysEx())
            return sysexText (m);

        // Transport, which is rare and meaningful. No channel: these are system
        // messages and belong to the whole stream.
        if (m.isMidiStart())    return juce::String ("start");
        if (m.isMidiStop())     return juce::String ("stop");
        if (m.isMidiContinue()) return juce::String ("continue");

        // Everything else is deliberately silent: timing clock, active sensing,
        // MTC quarter frame, song position, song select, system reset.
        return {};
    }

    /** An RPN or NRPN as one line, since the control changes carrying it are
        one event rather than four independent ones — the specification makes
        CC 6 meaningless without the selection before it.

        Named where the sidebar knows the name, because `RPN 0/3` says less than
        `tuning program` to anyone who has not memorised Table IIIa. */
    inline juce::String lineFor (const juce::MidiRPNMessage& rpn)
    {
        const auto ch   = "ch " + juce::String (rpn.channel) + "  ";
        const auto kind = rpn.isNRPN ? "NRPN " : "RPN ";
        const auto number = juce::String (rpn.parameterNumber / 128) + "/"
                          + juce::String (rpn.parameterNumber % 128);

        const auto named = [&]() -> juce::String
        {
            if (rpn.isNRPN)
                return {};

            switch (rpn.parameterNumber)
            {
                case 0: return "pitchbend sensitivity";
                case 1: return "fine tuning";
                case 2: return "coarse tuning";
                case 3: return "tuning program";
                case 4: return "tuning bank";
                case 6: return "MPE zone";
                default: return {};
            }
        }();

        return ch + kind + number + "  "
             + (named.isNotEmpty() ? named + " " : juce::String())
             + juce::String (rpn.value);
    }
}

} // namespace microtonos::sidebar
