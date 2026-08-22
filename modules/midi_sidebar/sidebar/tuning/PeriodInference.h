#pragma once

#include <cmath>

#include <juce_audio_basics/juce_audio_basics.h>

#include "../pages/TuningState.h"

namespace microtonos::sidebar
{

//==============================================================================
/** Working out what interval a scale repeats at, from its frequencies alone.

    Adapted from `inferScaleSize` in tuneBfree (`src/tuning.cpp`), which is the
    good part of it: a pure function of a frequency sequence, with a two-pass
    search that gets equal divisions right. What is *not* carried over is
    `extendFrequencies` and its `scaleLen`/`gamutSize` argument — those exist to
    drive more than 128 tonewheels and are that project's own problem.

    Three changes beyond dropping those:

    - it returns **cents**, since that is the unit the tuning page works in;
    - it returns **every** period it finds rather than the first, because
      `tuning::Period` offers the end-user a list to step through;
    - it takes a sequence rather than a fixed 128-element array, so a merged
      multi-channel table can be handed to it.

    The relation being tested is `f[i + n] / f[i] == r` for every `i` — note
    *n* scale degrees up is always `r` times the pitch. `n` is the scale size and
    `r` the period. Note that this is an index relation, so the sequence must be
    in scale-degree order; it does **not** require the frequencies to ascend
    monotonically, which matters because a keyboard mapping can fold them.
*/
namespace periodInference
{
    /** How close two ratios must be to count as equal. tuneBfree's value, on
        the *ratio* rather than on the frequencies, so it means the same thing
        at every octave. About 0.0000017 cents — far below anything audible, and
        below MTS's own 0.0061 c resolution, so it is testing arithmetic
        rather than perception. */
    inline constexpr double ratioTolerance = 1.0e-6;

    /** The largest whole-number ratio looked for in the first pass. tuneBfree
        stops below 10; a scale repeating at more than three octaves and a half
        is not something to guess at from a table. */
    inline constexpr double highestWholeRatio = 9.0;

    //==========================================================================
    /** One way the sequence repeats: `size` scale degrees spanning `cents`. */
    struct Candidate
    {
        int    size  = 0;
        double cents = 0.0;
    };

    /** Whether stepping `size` places always multiplies the frequency by the
        same ratio.

        **A period may span at most half the sequence.** Without that, a size of
        *n*-1 is confirmed by exactly one comparison and every table "repeats" —
        which is not a hypothetical: a table of `100 + i²` reports a period,
        because nothing contradicts a claim made about a single pair. Half means
        the relation is checked at least as many times as it spans, which is the
        weakest bound that still amounts to evidence. */
    inline bool repeatsAt (const juce::Array<double>& sequence, int size, double ratio)
    {
        if (size <= 0 || size > sequence.size() / 2 || ratio <= 0.0)
            return false;

        for (int i = 0; i + size < sequence.size(); ++i)
        {
            if (sequence[i] <= 0.0)
                return false;

            if (std::abs (sequence[i + size] / sequence[i] - ratio) > ratioTolerance)
                return false;
        }

        return true;
    }

    inline double centsForRatio (double ratio)
    {
        return 1200.0 * std::log2 (ratio);
    }

    //==========================================================================
    /** Every period the sequence admits, ascending by scale size.

        Two passes, which is tuneBfree's shape. The first tests against *exact*
        whole-number ratios, so a table that repeats at a true octave reports
        1200 c rather than 1199.999999 c; the second takes the ratio from the
        data itself and so finds everything the first cannot. The second alone
        would be complete — the first is there for the arithmetic.
    */
    inline juce::Array<Candidate> candidates (const juce::Array<double>& sequence)
    {
        juce::Array<Candidate> found;

        if (sequence.size() < 2)
            return found;

        // Pass one: whole-number ratios, smallest scale size first within each.
        for (double ratio = 2.0; ratio <= highestWholeRatio; ratio += 1.0)
            for (int size = 1; size < sequence.size(); ++size)
                if (repeatsAt (sequence, size, ratio))
                {
                    found.add ({ size, centsForRatio (ratio) });
                    break;   // the smallest size for this ratio is the one worth having
                }

        // Pass two: whatever else repeats, taking the ratio from the sequence
        // itself. Skips sizes already reported.
        for (int size = 1; size < sequence.size(); ++size)
        {
            if (sequence[0] <= 0.0)
                break;

            const auto ratio = sequence[size] / sequence[0];

            if (! repeatsAt (sequence, size, ratio))
                continue;

            const auto already = std::any_of (found.begin(), found.end(),
                                              [size] (const Candidate& c) { return c.size == size; });

            if (! already)
                found.add ({ size, centsForRatio (ratio) });
        }

        return found;
    }

    /** An octave, and the target the choice below aims at. */
    inline constexpr double octaveCents = 1200.0;

    /** The candidate to show, or nothing when the sequence does not repeat.

        **Whichever period is closest to an octave.** That one rule covers the
        equal divisions too: 12edo repeats at *every* scale size — one step is as
        much a period as twelve are — so its candidates are 100 c, 200 c, … and
        the nearest to an octave is 1200 c, which is the answer wanted.

        Distance is in cents, which is already logarithmic, so 600 c is nearer to
        1200 c than 2400 c is. A period is more usefully small than large. On an
        exact tie the smaller wins, so the answer never depends on iteration
        order.

        **Where a scale divides something other than an octave, this is a
        guess and is meant to be.** Bohlen-Pierce is 13 equal divisions of 3/1,
        so its candidates are multiples of 146.3 c and this reports 1170.4 c
        rather than the 1902 c tritave. A rule preferring whole-number ratios
        would recover that particular case and still fail on 9 equal divisions of
        3/2, because once a period is unstated the frequencies genuinely do not
        say which multiple was meant — a Farey-style argument might rank them,
        but the octave is the simpler rule and this is a **default**, editable
        from the page and affecting nothing that sounds.

        A tuning that states its period does not come through here at all: a
        Scala file reports `PeriodSource::specified` from its own last tone, and
        MTS-ESP from `MTS_GetPeriodRatio`. Inference is for the tunings that say
        nothing. */
    inline std::optional<Candidate> best (const juce::Array<double>& sequence)
    {
        std::optional<Candidate> found;

        for (const auto& c : candidates (sequence))
        {
            if (! found.has_value())
            {
                found = c;
                continue;
            }

            const auto distance  = std::abs (c.cents - octaveCents);
            const auto incumbent = std::abs (found->cents - octaveCents);

            if (distance < incumbent
                || (juce::exactlyEqual (distance, incumbent) && c.cents < found->cents))
                found = c;
        }

        return found;
    }

    //==========================================================================
    /** The page's own type, filled in from a sequence.

        `candidates` there is a list of cents for the end-user to step through,
        so it is sorted ascending — a stepper that jumped about would be unusable
        — while `cents` is `best()`, which is usually somewhere in the middle of
        them rather than at either end. */
    inline tuning::Period periodFor (const juce::Array<double>& sequence)
    {
        tuning::Period period;

        period.source = tuning::PeriodSource::inferred;

        const auto chosen = best (sequence);

        if (! chosen.has_value())
            return period;

        period.cents = chosen->cents;

        for (const auto& c : candidates (sequence))
            period.candidates.add (c.cents);

        period.candidates.sort();

        return period;
    }
}

} // namespace microtonos::sidebar
