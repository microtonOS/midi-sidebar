#pragma once

#include <cmath>
#include <iostream>

#include <juce_core/juce_core.h>

/** The whole test harness.

    These checks are console apps rather than a framework: each is a `main` that
    exercises one header and returns the number of failures, which is what
    `ctest` reads. That is enough, it adds no dependency, and it keeps a check
    readable as a list of statements about behaviour rather than as a fixture
    hierarchy.

    Every assertion prints, pass or fail. A green run that says nothing tells you
    only that nothing crashed; a green run that lists what it checked is a
    description of the contract, and reading it is how you notice a rule that
    stopped being tested.
*/
namespace checks
{
    inline int failures = 0;

    inline void check (bool ok, const juce::String& what)
    {
        std::cout << (ok ? "  ok    " : "  FAIL  ") << what << std::endl;

        if (! ok)
            ++failures;
    }

    /** Two doubles, within a tolerance, printing both when they differ. The
        tolerance is explicit at every call: how close is close enough is part of
        what is being asserted, and a shared epsilon would hide that. */
    inline void near (double got, double want, double tolerance, const juce::String& what)
    {
        const auto ok = std::abs (got - want) <= tolerance;

        std::cout << (ok ? "  ok    " : "  FAIL  ") << what
                  << "  (got " << got << ", want " << want << ")" << std::endl;

        if (! ok)
            ++failures;
    }

    inline void eq (int got, int want, const juce::String& what)
    {
        const auto ok = got == want;

        std::cout << (ok ? "  ok    " : "  FAIL  ") << what
                  << "  (got " << got << ", want " << want << ")" << std::endl;

        if (! ok)
            ++failures;
    }

    /** Prints the heading, and returns the exit code `ctest` wants: zero for a
        clean run, otherwise the number of failures. */
    inline int report (const juce::String& name)
    {
        std::cout << (failures == 0 ? name + ": ALL PASSED"
                                    : name + ": " + juce::String (failures) + " FAILED")
                  << std::endl;

        return failures;
    }
}
