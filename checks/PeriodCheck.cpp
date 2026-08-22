// Period inference: what interval a scale repeats at, from frequencies alone.
//
// Adapted from tuneBfree's `inferScaleSize`. The default is whichever candidate
// sits nearest an octave, which is exactly right for any equal division *of* the
// octave and an admitted guess for anything else — see PeriodInference.h and
// docs/tuning.md.
#include <midi_sidebar/midi_sidebar.h>

#include "Check.h"

using namespace microtonos::sidebar;
using namespace checks;

namespace
{
    /** `steps` equal divisions of `periodRatio`, as a 128-note sequence. */
    juce::Array<double> equalDivision (int steps, double periodRatio)
    {
        juce::Array<double> f;

        for (int i = 0; i < 128; ++i)
            f.add (8.1758 * std::pow (periodRatio, (double) i / (double) steps));

        return f;
    }
}

int main()
{
    //  12edo: every step size repeats, so the rule has to pick the octave ------
    {
        const auto f = equalDivision (12, 2.0);
        const auto best = periodInference::best (f);

        check (best.has_value(), "12edo has a period");
        near (best->cents, 1200.0, 1e-6, "12edo reports the octave, not one step");
        eq (best->size, 12, "and a scale size of 12");

        const auto period = periodInference::periodFor (f);

        check (period.candidates.size() > 1, "several candidates are offered");
        check (period.candidates.contains (period.cents.value_or (-1.0)),
               "the chosen period is among them");

        auto ascending = true;
        for (int i = 1; i < period.candidates.size(); ++i)
            ascending = ascending && period.candidates[i] > period.candidates[i - 1];
        check (ascending, "and they ascend, so a stepper walking them is sane");

        near (period.candidates.getFirst(), 100.0, 1e-6,
              "one step is still offered — it is a period, just not the useful one");
    }

    //  Other equal divisions of the octave -------------------------------------
    {
        near (periodInference::best (equalDivision (5, 2.0))->cents,  1200.0, 1e-6, "5edo");
        near (periodInference::best (equalDivision (7, 2.0))->cents,  1200.0, 1e-6, "7edo");
        near (periodInference::best (equalDivision (31, 2.0))->cents, 1200.0, 1e-6, "31edo");
    }

    //  Equal divisions of something else are a guess, and say so ---------------
    {
        // Bohlen-Pierce: 13 equal divisions of 3/1, so the candidates are
        // multiples of 146.3 c and none of them is an octave.
        const auto bp = periodInference::best (equalDivision (13, 3.0));

        eq (bp->size, 8, "13ed3 picks the 8-step interval, being nearest an octave");
        near (bp->cents, 8.0 * 1200.0 * std::log2 (3.0) / 13.0, 1e-6,
              "which is not the tritave — unstated periods are not recoverable");

        // The tritave is still in the list, so the end-user can step to it.
        auto hasTritave = false;
        for (const auto c : periodInference::periodFor (equalDivision (13, 3.0)).candidates)
            hasTritave = hasTritave || std::abs (c - 1200.0 * std::log2 (3.0)) < 1e-6;
        check (hasTritave, "but the tritave is among the candidates to step to");

        near (periodInference::best (equalDivision (9, 1.5))->cents, 1169.925, 0.01,
              "9ed(3/2) likewise reports the candidate nearest an octave");
    }

    //  A table that does not repeat --------------------------------------------
    {
        // A period spanning all but one of the sequence used to be "confirmed"
        // by a single comparison, so every table repeated. Periods are now
        // capped at half the sequence, which is what this catches.
        juce::Array<double> f;
        for (int i = 0; i < 128; ++i)
            f.add (100.0 + (double) i * (double) i);

        check (! periodInference::best (f).has_value(), "a non-repeating table has no period");
        check (! periodInference::periodFor (f).cents.has_value(), "and periodFor says so");
    }

    //  Degenerate input ---------------------------------------------------------
    {
        check (! periodInference::best ({}).has_value(), "an empty sequence has no period");
        check (! periodInference::best ({ 440.0 }).has_value(), "nor does one frequency");
    }

    //  End to end, through the table's own merge --------------------------------
    {
        const auto period = periodInference::periodFor (standardTuning::table().sortedFrequencies());

        near (period.cents.value_or (0.0), 1200.0, 1e-6, "standard tuning gives the octave");
        check (period.source == tuning::PeriodSource::inferred, "and is marked inferred");
    }

    return report ("PeriodCheck");
}
