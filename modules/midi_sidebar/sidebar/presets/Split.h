#pragma once

#include <cmath>
#include <optional>

#include <juce_gui_basics/juce_gui_basics.h>

#include "../pages/PresetsState.h"

namespace microtonos::sidebar
{

//==============================================================================
/** The keyboard split, as a pair of gains per note.

    **Split by frequency, not by key.** Under a multichannel tuning the same note
    number is a different pitch on every channel, so a split defined by note
    number is not one split point but sixteen. Frequency is the only thing all
    the channels agree about — see docs/presets.md.

    **Why a pair of gains rather than a side.** When the two frequencies differ
    they "indicate the lower and upper bounds of a crossfade", and a note inside
    that region is played by *both* layers. There is no side to return.

    **Why the sidebar cannot apply it.** Master volume is one gain on the whole
    buffer after mixing; this is one gain per voice before mixing, and with three
    notes held either side of and inside the fade no single buffer gain expresses
    it. The sidebar is a MIDI effect with no voices to reach, so it answers the
    question and the developer applies the answer:

    ```
    const auto gains = presets::gainsFor (source.frequencyFor (note, channel), split);

    lowerVoice.setGain (gains.lower);
    upperVoice.setGain (gains.upper);
    ```

    Three wire-level alternatives were considered and rejected. Scaling note-on
    velocity is not a volume control — on most instruments it selects samples and
    opens filters — and needs note re-emission. CC 7 is per *channel*, so two
    notes on one channel at different points of the fade cannot differ. MIDI 2.0
    per-note volume would be the clean answer but needs UMP, which nothing here
    reads.
*/
namespace presets
{
    //==========================================================================
    /** How much of each layer a note gets. Both are 0…1. */
    struct LayerGains
    {
        double lower = 1.0;
        double upper = 0.0;
    };

    /** The shape of the crossfade.

        **A choice, not a deduction.** Which is right depends on how correlated
        the two layers are, and two layers of a split are neither extreme: they
        share a pitch, so their fundamentals partly correlate, and their upper
        partials differ, so those do not. Whichever is picked is wrong by up to
        3 dB in the middle for some material. Samplers usually offer the choice
        for exactly this reason.

        Both are constant, then curved, then constant — they differ only inside
        the fade. Neither is a true sigmoid: both have non-zero slope at the fade
        edges, so there is a slope discontinuity where the constant regions meet.
        A `smoothstep` third value would remove that if the corner ever shows.
    */
    enum class Curve
    {
        /** `1−t` and `t`, summing to 1. Right when the layers are **correlated**
            — near-identical signals, whose amplitudes add. Dips about 3 dB in
            the middle for anything else. */
        linear,

        /** `cos(t·π/2)` and `sin(t·π/2)`, whose **squares** sum to 1. Right when
            the layers are **uncorrelated** — different timbres, whose powers
            add. Bumps about 3 dB for near-identical layers. */
        equalPower
    };

    /** The split as the page holds it.

        `low` and `high` are the crossfade's bounds in Hz, and equal means a hard
        split point — which is what `Frequencies` already says. `active` is the
        page's `active` button: with it off the switch simply moves the whole
        keyboard between the two parameter sets, which is docs/presets.md:55-76.
    */
    struct Split
    {
        Frequencies frequencies;
        bool  active  = false;
        Layer editing = Layer::lower;
        Curve curve   = Curve::equalPower;
    };

    //==========================================================================
    /** Where a frequency sits between two others, 0…1, measured in **cents**.

        Not in hertz. Pitch is logarithmic and every other number on the tuning
        page is already in cents; a fade linear in hertz would be lopsided, with
        the upper half of a one-octave crossfade squeezed into a third of its
        span. The midpoint of a fade is therefore the *geometric* mean of its
        bounds, not the arithmetic one.
    */
    inline double positionBetween (double frequency, double low, double high)
    {
        if (frequency <= low)  return 0.0;
        if (frequency >= high) return 1.0;

        const auto span = std::log2 (high / low);

        if (span <= 0.0)
            return frequency < high ? 0.0 : 1.0;

        return std::log2 (frequency / low) / span;
    }

    /** The two gains for a note at this frequency. */
    inline LayerGains gainsFor (std::optional<double> frequency, const Split& split)
    {
        // With the split off, the switch moves the whole keyboard between the
        // two parameter sets — so the layer being edited is the one playing, at
        // full gain, and the other is silent.
        const auto whole = split.editing == Layer::lower ? LayerGains { 1.0, 0.0 }
                                                         : LayerGains { 0.0, 1.0 };

        if (! split.active || ! frequency.has_value() || *frequency <= 0.0)
            return whole;

        const auto& f = split.frequencies;

        // A split needs both bounds. One alone says where nothing is.
        if (! f.low.has_value() || ! f.high.has_value())
            return whole;

        // Reversed bounds are the end-user's, not an error — sorted here rather
        // than refused, so a fade typed the wrong way round still fades.
        const auto low  = juce::jmin (*f.low, *f.high);
        const auto high = juce::jmax (*f.low, *f.high);

        if (low <= 0.0)
            return whole;

        // A hard split: equal bounds, so there is nothing to interpolate.
        // At the point itself the upper layer takes it, matching "if the
        // indicated frequencies are equal, then that is the frequency for a hard
        // split point" read as a boundary rather than as a note of its own.
        if (! (low < high))
            return *frequency < low ? LayerGains { 1.0, 0.0 } : LayerGains { 0.0, 1.0 };

        const auto t = positionBetween (*frequency, low, high);

        if (split.curve == Curve::linear)
            return { 1.0 - t, t };

        const auto angle = t * juce::MathConstants<double>::halfPi;

        return { std::cos (angle), std::sin (angle) };
    }

    /** Which layer is *predominant* at this frequency — the one a caller wants
        when it can only pick one, such as deciding which parameter set to show.
        Inside a crossfade this is whichever has the larger gain. */
    inline Layer layerFor (std::optional<double> frequency, const Split& split)
    {
        const auto gains = gainsFor (frequency, split);

        return gains.upper > gains.lower ? Layer::upper : Layer::lower;
    }
}

} // namespace microtonos::sidebar
