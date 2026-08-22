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
        page only reports the choice.

        **Named by frequency, not by position on the keyboard.** Under a
        multichannel tuning the same note number is a different pitch on every
        channel, so "left hand" and "right hand" mean nothing and only the
        frequency does. See docs/presets.md and `presets::gainsFor`. */
    enum class Layer { lower, upper };

    /** The strip shows the two slash glyphs rather than these, which are kept
        for prose — a menu line or a tooltip. The icons are `icons::splitLower`
        and `icons::splitUpper`, the same pair the controllers table marks a
        parameter's scope with, so one symbol means one thing across two pages. */
    inline const juce::StringArray layerNames { "lower frequencies", "higher frequencies" };

    //==========================================================================
    /** Which preset is loaded, and whether it still is what its file says. */
    struct Status
    {
        juce::String name;
        std::optional<int> program, bank;

        /** The live parameters differ from the preset as stored, which the page
            marks with `icons::edited` at the right of the name.

            A flag rather than a `*` glued onto `name`, so the name a menu
            matches against stays the name. */
        bool edited = false;
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
