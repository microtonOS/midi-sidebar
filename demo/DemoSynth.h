#pragma once

#include <vector>

#include <juce_audio_processors/juce_audio_processors.h>
#include <midi_sidebar/midi_sidebar.h>

namespace microtonos::sidebar::demo
{

//==============================================================================
/** The parameters of the demo's stand-in plugin, declared once.

    A combo organ / subtractive synth, as docs/demo.md describes it: one
    oscillator with a waveform switch, one filter with cutoff and resonance, and
    one LFO that can be pointed at either the filter or the pitch — each of
    which keeps its own rate and intensity, so switching the target back finds
    what was left there.

    **It makes no sound.** `processBlock` still passes audio through untouched.
    What this is for is the right-click menu: that menu maps a *parameter* to a
    controller, and until now the demo had none — theme and sidebar edge are
    developer settings, not things a musician assigns a knob to. See
    docs/right-click.md.

    One ordered list, because three things have to agree about it and any two of
    them drifting apart is silent:

    - the parameters the processor declares,
    - the widgets the panel builds,
    - the parameter list handed to the sidebar, into which a mapping's
      `parameterIndex` is an index.

    `Index` names the positions so the panel can lay out particular controls
    without counting, and the whole thing is `controls()[Index::cutoff]`.
*/
namespace synth
{
    /** Positions in `controls()`, and therefore the indices a mapping stores. */
    enum Index
    {
        waveform = 0,
        cutoff,
        resonance,
        lfoTarget,
        filterLfoRate,
        filterLfoDepth,
        pitchLfoRate,
        pitchLfoDepth,

        count
    };

    //==========================================================================
    /** One control: what it is called, what it is measured in, what it does,
        and enough to build both the parameter and the widget.

        `choices` being non-empty is what makes it a switch rather than a knob —
        one flag fewer to keep in step with the thing it describes. */
    struct Control
    {
        juce::String id, name, unit, info;

        juce::StringArray choices;

        juce::NormalisableRange<float> range;
        float defaultValue = 0.0f;

        bool isChoice() const noexcept { return ! choices.isEmpty(); }
    };

    /** A frequency range that feels even under the finger. Hearing is
        logarithmic, so a linear cutoff knob spends most of its travel in the
        top octave; the skew puts the middle of the knob near the middle of what
        you can hear. */
    inline juce::NormalisableRange<float> frequencyRange (float low, float high, float centre)
    {
        juce::NormalisableRange<float> range { low, high };
        range.setSkewForCentre (centre);

        return range;
    }

    //==========================================================================
    /** The controls, built the first time they are asked for.

        **Not a namespace-scope object.** One of those is constructed during
        static initialisation, and every field here is a `juce::String` or a
        `juce::StringArray` — which need JUCE's own statics to be up. The order
        of the two is unspecified across translation units, and when it came out
        the wrong way round this crashed before `main` was entered: the only
        output was JUCE's version banner, which is itself printed by a static
        initialiser (`JuceVersionPrinter` in juce_SystemStats.cpp), so even the
        first line of `main` was never reached.

        A function-local static is constructed on first use, by which time
        everything it needs exists. The ids are literals rather than named
        constants for the same reason: eight more namespace-scope
        `juce::String`s would be eight more of the same hazard, and nothing
        outside this list refers to them. */
    inline const std::vector<Control>& controls()
    {
        static const std::vector<Control> list
        {
        { "waveform", "Waveform", {},
          "The shape the oscillator draws. Saw is the brightest of the three and "
          "square the hollowest; triangle sits between them with almost nothing "
          "in the upper harmonics.",
          { "saw", "triangle", "square" }, {}, 0.0f },

        { "cutoff", "Filter cutoff", "Hz",
          "Where the filter starts taking harmonics away. Everything above this "
          "frequency is progressively quieter, so lowering it darkens the tone "
          "without changing the note.",
          {}, frequencyRange (20.0f, 20000.0f, 1000.0f), 2000.0f },

        // No unit: resonance is the filter's quality factor, Q, which is a ratio
        // of frequencies and so a bare number. The letter names the quantity,
        // not what it is measured in, so it belongs in the description.
        { "resonance", "Filter resonance", {},
          "How much the filter emphasises the frequencies right at the cutoff — "
          "its quality factor, Q. High values ring; at the top the filter begins "
          "to whistle at its own cutoff frequency.",
          {}, { 0.1f, 10.0f }, 0.7f },

        { "lfoTarget", "LFO target", {},
          "Which of the two modulations you hear. Both keep their own rate and "
          "intensity, so switching back finds the settings you left rather than "
          "the ones you just made.",
          { "filter", "pitch" }, {}, 0.0f },

        { "filterLfoRate", "Filter LFO rate", "Hz",
          "How fast the filter's cutoff sweeps up and down.",
          {}, frequencyRange (0.05f, 20.0f, 2.0f), 2.0f },

        { "filterLfoDepth", "Filter LFO intensity", "%",
          "How far the filter's cutoff sweeps. At zero the LFO is still running "
          "but nothing is listening to it.",
          {}, { 0.0f, 100.0f }, 25.0f },

        { "pitchLfoRate", "Pitch LFO rate", "Hz",
          "How fast the pitch wavers. Vibrato lives between about four and seven "
          "of these.",
          {}, frequencyRange (0.05f, 20.0f, 5.0f), 5.0f },

        { "pitchLfoDepth", "Pitch LFO intensity", "%",
          "How far the pitch wavers. Small amounts read as vibrato and large "
          "ones as a siren; there is not much in between.",
          {}, { 0.0f, 100.0f }, 10.0f },
        };

        return list;
    }

    //==========================================================================
    /** The same controls as the sidebar wants them: a name, the unit its limits
        are shown in, and the description its right-click menu explains them
        with. Built from the list above, so the index the sidebar stores in a
        mapping is a position in `controls()` and cannot mean anything else. */
    inline juce::Array<controllers::Parameter> parametersForSidebar()
    {
        juce::Array<controllers::Parameter> parameters;

        for (const auto& control : controls())
            parameters.add ({ control.name, control.unit, control.info });

        return parameters;
    }

    /** Every control as an APVTS parameter, in the same order. Version 1
        throughout: nothing here has shipped, so nothing needs migrating. */
    inline void addParametersTo (juce::AudioProcessorValueTreeState::ParameterLayout& layout)
    {
        for (const auto& control : controls())
        {
            if (control.isChoice())
            {
                layout.add (std::make_unique<juce::AudioParameterChoice> (
                    juce::ParameterID { control.id, 1 }, control.name, control.choices, 0));

                continue;
            }

            layout.add (std::make_unique<juce::AudioParameterFloat> (
                juce::ParameterID { control.id, 1 }, control.name,
                control.range, control.defaultValue));
        }
    }
}

} // namespace microtonos::sidebar::demo
