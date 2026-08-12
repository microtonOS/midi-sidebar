#pragma once

#include <optional>

#include <juce_gui_basics/juce_gui_basics.h>

namespace microtonos::sidebar
{

//==============================================================================
/** Everything the presets page displays and everything it can be asked for.

    Free of files and of MIDI, like `tuning` and `controllers`: a preset arrives
    here as a name, two numbers and some text that something else has already
    worked out.

    See docs/presets.md for the specification.
*/
namespace presets
{
    //==========================================================================
    /** The pair of frequencies at the top of the page, in Hz.

        Always low and high, whatever they are describing — which is what lets
        them be read as one thing rather than two unrelated read-outs:

        - one note sounding: both show that note;
        - several: the lowest and the highest, mirroring the tuning page's
          interval;
        - none: where the crossfade between the two halves of a split begins and
          ends, which are equal when the split is sharp.

        Empty when there is nothing to say at all.
    */
    struct Frequencies
    {
        std::optional<double> low, high;
    };

    /** Which half of a split is being edited — and, when the split is off,
        played as well. Which of those it means is the owner's decision; the
        page only reports the choice. */
    enum class Layer { lower, upper };

    inline const juce::StringArray layerNames { "lower", "upper" };

    //==========================================================================
    /** Which preset is loaded. Read-only on the page for now: it says where you
        are, and getting somewhere else is a job for the controls that will
        replace this block later. */
    struct Status
    {
        juce::String name;
        std::optional<int> program, bank;
    };

    /** Whatever the preset's author wanted to say. `comment` is deliberately
        long-form — usage suggestions, a licence — which is why the page gives
        it a multi-line box and its only flexible row. */
    struct Meta
    {
        juce::String author, comment;
    };

}

} // namespace microtonos::sidebar
