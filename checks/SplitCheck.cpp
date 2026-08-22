// The keyboard split's crossfade.
//
// Split by frequency rather than by key, because under a multichannel tuning the
// same note number is a different pitch on every channel — see Split.h. The
// assertion that earns its place here is the geometric-mean one: it is the only
// one that fails if the fade is written in hertz instead of cents.
#include <midi_sidebar/midi_sidebar.h>

#include "Check.h"

using namespace microtonos::sidebar;
using namespace checks;

namespace
{
    presets::Split split (double low, double high, bool active = true,
                          presets::Curve curve = presets::Curve::equalPower)
    {
        presets::Split s;

        s.frequencies = { low, high };
        s.active = active;
        s.curve = curve;

        return s;
    }
}

int main()
{
    //  A hard split ------------------------------------------------------------
    {
        const auto s = split (440.0, 440.0);

        const auto below = presets::gainsFor (439.0, s);
        near (below.lower, 1.0, 1e-12, "below a hard split is all lower");
        near (below.upper, 0.0, 1e-12, "and none of the upper");

        const auto above = presets::gainsFor (441.0, s);
        near (above.lower, 0.0, 1e-12, "above it is all upper");
        near (above.upper, 1.0, 1e-12, "and none of the lower");

        const auto at = presets::gainsFor (440.0, s);
        near (at.upper, 1.0, 1e-12, "the point itself belongs to the upper layer");
    }

    //  The crossfade's ends ------------------------------------------------------
    {
        const auto s = split (440.0, 880.0);

        near (presets::gainsFor (400.0, s).lower, 1.0, 1e-12, "below the fade is all lower");
        near (presets::gainsFor (440.0, s).lower, 1.0, 1e-12, "and at its low bound");
        near (presets::gainsFor (880.0, s).upper, 1.0, 1e-12, "at the high bound it is all upper");
        near (presets::gainsFor (900.0, s).upper, 1.0, 1e-12, "and above it");
    }

    //  **Cents, not hertz** --------------------------------------------------------
    {
        // A one-octave fade from 440 to 880. Halfway in *pitch* is 440·√2 =
        // 622.25 Hz, the geometric mean — not 660, the arithmetic one. A fade
        // written in hertz would put the midpoint at 660 and squeeze the upper
        // half of the octave into a third of its span.
        const auto s = split (440.0, 880.0);

        const auto geometric = 440.0 * std::sqrt (2.0);

        near (presets::positionBetween (geometric, 440.0, 880.0), 0.5, 1e-12,
              "the geometric mean is halfway through the fade");
        near (presets::positionBetween (660.0, 440.0, 880.0), 0.585, 0.001,
              "the arithmetic mean is NOT - it sits well past halfway");

        const auto mid = presets::gainsFor (geometric, s);
        near (mid.lower, mid.upper, 1e-12, "so the two gains are equal there");

        // One octave up from the low bound is halfway; two octaves would be
        // past the end. Checked at a quarter as well, to catch a fade that is
        // linear in the log but scaled wrongly.
        near (presets::positionBetween (440.0 * std::pow (2.0, 0.25), 440.0, 880.0), 0.25, 1e-12,
              "a quarter of an octave is a quarter of the fade");
    }

    //  Equal power: the squares sum to one ------------------------------------------
    {
        const auto s = split (440.0, 880.0, true, presets::Curve::equalPower);

        auto worst = 0.0;

        for (int i = 0; i <= 20; ++i)
        {
            const auto f = 440.0 * std::pow (2.0, (double) i / 20.0);
            const auto g = presets::gainsFor (f, s);

            worst = juce::jmax (worst, std::abs (g.lower * g.lower + g.upper * g.upper - 1.0));

            if (g.lower < 0.0 || g.lower > 1.0 || g.upper < 0.0 || g.upper > 1.0)
                check (false, "a gain left 0..1 inside the fade");
        }

        near (worst, 0.0, 1e-12, "equal power keeps lower^2 + upper^2 at 1 across the fade");
    }

    //  Linear: the gains sum to one -------------------------------------------------
    {
        const auto s = split (440.0, 880.0, true, presets::Curve::linear);

        auto worst = 0.0;

        for (int i = 0; i <= 20; ++i)
        {
            const auto f = 440.0 * std::pow (2.0, (double) i / 20.0);
            const auto g = presets::gainsFor (f, s);

            worst = juce::jmax (worst, std::abs (g.lower + g.upper - 1.0));
        }

        near (worst, 0.0, 1e-12, "linear keeps lower + upper at 1 across the fade");

        // The two curves differ in the middle and agree at the ends, which is
        // what makes the choice audible only inside the fade.
        const auto geometric = 440.0 * std::sqrt (2.0);
        const auto linear = presets::gainsFor (geometric, s);
        const auto power  = presets::gainsFor (geometric, split (440.0, 880.0));

        near (linear.lower, 0.5, 1e-12, "linear is at 0.5 halfway");
        near (power.lower, std::sqrt (0.5), 1e-12, "equal power is at 1/root 2 - about 3 dB higher");
    }

    //  The split switched off ---------------------------------------------------------
    {
        auto s = split (440.0, 880.0, false);

        s.editing = presets::Layer::lower;
        const auto lower = presets::gainsFor (10000.0, s);
        near (lower.lower, 1.0, 1e-12, "with the split off, the whole keyboard is the edited layer");
        near (lower.upper, 0.0, 1e-12, "and the other is silent");

        s.editing = presets::Layer::upper;
        near (presets::gainsFor (20.0, s).upper, 1.0, 1e-12, "whichever layer that is");
    }

    //  Degenerate input ------------------------------------------------------------------
    {
        presets::Split none;
        none.active = true;

        near (presets::gainsFor (440.0, none).lower, 1.0, 1e-12,
              "a split with no frequencies behaves as switched off");

        auto oneOnly = none;
        oneOnly.frequencies.low = 440.0;
        near (presets::gainsFor (440.0, oneOnly).lower, 1.0, 1e-12,
              "and so does one bound alone - one frequency says where nothing is");

        check (! presets::gainsFor (std::nullopt, split (440.0, 880.0)).upper,
               "an unmapped note gets the edited layer, not a crossfade");
    }

    //  Reversed bounds are the end-user's, not an error -------------------------------------
    {
        const auto s = split (880.0, 440.0);

        for (int i = 0; i <= 10; ++i)
        {
            const auto f = 400.0 * std::pow (2.0, (double) i / 5.0);
            const auto g = presets::gainsFor (f, s);

            if (g.lower < 0.0 || g.lower > 1.0 || g.upper < 0.0 || g.upper > 1.0)
                check (false, "a reversed pair produced a gain outside 0..1");
        }

        check (true, "a reversed pair is sorted rather than refused, and stays in 0..1");
        near (presets::gainsFor (440.0, s).lower, 1.0, 1e-12, "and fades the same way round");
    }

    //  layerFor, for callers that can only pick one ------------------------------------------
    {
        const auto s = split (440.0, 880.0);

        check (presets::layerFor (450.0, s) == presets::Layer::lower, "just inside the fade is lower");
        check (presets::layerFor (870.0, s) == presets::Layer::upper, "near the top is upper");
        check (presets::layerFor (100.0, s) == presets::Layer::lower, "well below is lower");
    }

    return report ("SplitCheck");
}
