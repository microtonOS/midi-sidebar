#pragma once

#include <cmath>
#include <optional>

#include <juce_audio_basics/juce_audio_basics.h>

#include "../SidebarLookAndFeel.h"

namespace microtonos::sidebar
{

//==============================================================================
/** Universal Real Time Device Control — the sidebar's volume, over MIDI.

    ```
    F0 7F <device id> 04 01 vv vv F7     Master Volume, 14 bits, LSB first
    F0 7F <device id> 04 02 bb bb F7     Master Balance
    ```

    Sub-ID#1 `04`, from the *Complete MIDI 1.0 Detailed Specification* 4.2.1,
    p57. The clue to the design is in the first sentence of that section: these
    address **devices**, where CC 7 and CC 8 address **channels**. They exist
    "to produce the same effect as volume and balance controls on a stereo
    amplifier … so that one Master Volume control can simultaneously fade out
    all the layers in a sound module".

    Three decisions, each of which could have gone the other way:

    **Broadcast only.** The specification has a conforming device track three
    volume scalars and multiply them — one received on its own ID, one on the
    All Call `7F`, one from channel messages. A plugin has no device ID of its
    own, so only the All Call is addressed to it, and a message aimed at a
    particular ID is left alone. Answering every ID instead would have two
    instances of the plugin in one session fight over the same message.

    **One scalar, not three.** The multiplication is for a multitimbral module
    mixing sixteen channels; the sidebar's fader *is* the device volume, and
    CC 7 is now an ordinary mappable controller rather than a second way in.
    See the note in ControllersState.h about why the built-in rows went.

    **Not consumed.** A broadcast is addressed to everything downstream, so the
    owner passes it on — unlike the tuning system exclusives, which are ours.

    Master Balance is parsed by nothing here on purpose: there is no balance
    control to move. `MidiMonitor::sysexText` still names it, which is all a
    monitor owes a message it does not act on.
*/
namespace deviceControl
{
    /** The All Call device ID: "7F" addresses every device on the line. */
    inline constexpr int broadcastId = 0x7f;

    /** Sub-ID#1 and #2 for Master Volume. */
    inline constexpr int deviceControlSubId = 0x04;
    inline constexpr int masterVolumeSubId  = 0x01;

    /** The largest 14-bit value, and so full volume. */
    inline constexpr int highestVolume = 128 * 128 - 1;

    //==========================================================================
    /** The 14-bit volume this message carries, or nothing if it is not a
        broadcast Master Volume.

        `getSysExData` omits the leading `F0` and the trailing `F7`, so a
        well-formed message is exactly six bytes: id, device, `04`, `01`, then
        the value low byte first. Anything else is not this message. */
    inline std::optional<int> masterVolumeFrom (const juce::MidiMessage& m)
    {
        if (! m.isSysEx())
            return {};

        const auto* data = m.getSysExData();

        if (data == nullptr || m.getSysExDataSize() != 6)
            return {};

        if (data[0] != 0x7f || data[1] != broadcastId)
            return {};

        if (data[2] != deviceControlSubId || data[3] != masterVolumeSubId)
            return {};

        // LSB first, unlike almost everything else in MIDI 1.0 and like pitch
        // bend. Masked because a data byte cannot carry a high bit and a
        // malformed sender should not be able to push the value out of range.
        return (data[4] & 0x7f) | ((data[5] & 0x7f) << 7);
    }

    //==========================================================================
    /** That value on the fader's own scale.

        **The square of the value is proportional to the amplitude.** General
        MIDI 2 v1.2a §3.3.4 gives the curve for CC 7 — "Regarding the curve of
        volume change messages, the square of the value is proportional to the
        volume", worked through as 127 × 127 = 16129 at 0 dB — and §4.1 says
        Master Volume follows the same one: "As with cc#7 and cc#11, the square
        of the value is proportional to the volume. See the curve definition
        given earlier for cc#7."

        Amplitude ∝ v² is 20·log₁₀(v²) = **40·log₁₀(v)** in decibels, which is
        the law `VolumeStrip`'s own comment already cites. `00 00` is defined as
        off and would be −∞, so the whole curve is floored at
        `metrics::floorDb`, where the fader reads −∞ anyway. */
    inline double decibelsFor (int value)
    {
        const auto clamped = juce::jlimit (0, highestVolume, value);

        if (clamped == 0)
            return (double) metrics::floorDb;

        const auto db = 40.0 * std::log10 ((double) clamped / (double) highestVolume);

        return juce::jmax ((double) metrics::floorDb, db);
    }
}

} // namespace microtonos::sidebar
