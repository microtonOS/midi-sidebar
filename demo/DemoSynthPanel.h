#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <midi_sidebar/midi_sidebar.h>

#include "DemoStyle.h"
#include "DemoSynth.h"

namespace microtonos::sidebar::demo
{

//==============================================================================
/** The stand-in plugin's control panel: two groups, two switches, four knobs.

    Every widget here is something a musician would want a controller on, which
    is the whole reason it exists — the right-click menu needs parameters to sit
    on, and the developer settings in the other tab are not parameters in that
    sense. See docs/right-click.md and docs/demo.md.

    **Laid out by function.** `FILTER` and `LFO` are what the instrument is made
    of, so they are what the panel is made of, read left to right and then down.
    Nothing belonging to one of them sits outside it, and neither is split
    across a row — an earlier version put the LFO's target in one row and its
    knobs in another, which said the two were unrelated.

    The waveform switch has **no frame**, though it is read as the oscillator.
    A `GroupComponent` around one control is a label with a box drawn round it,
    and three choices that name themselves do not need the label either. An
    unframed block above framed ones is the same arrangement the sidebar's
    tuning page uses.

    **One rate knob, not two — and this is the open question.** There are four
    LFO parameters, a rate and an intensity for each target, but two controls:
    the target switch decides which pair they are pointed at.

    That is right if the two LFOs are one LFO aimed somewhere. It is wrong if
    they are genuinely different instruments — and they may be, because the
    filter is paraphonic, one filter after the voices are merged, while the
    pitch LFO is per note and can be modulated by polytouch. A shared rate knob
    then hides the fact that only one of the two can be played per note. See
    docs/demo.md; this is a decision waiting to be made, not one that has been.

    Unlike `DemoControls`, this panel owns its attachments. Those settings need
    the editor to *act* on them — apply a theme, move the sidebar — so the
    editor holds their bindings; nothing consumes these, so the wiring stays
    here where the widgets are.
*/
class DemoSynthPanel final : public juce::Component
{
public:
    explicit DemoSynthPanel (juce::AudioProcessorValueTreeState& state)
        : apvts (state),
          waveformSwitch ({}, synth::controls()[synth::Index::waveform].choices),
          targetSwitch   ({}, synth::controls()[synth::Index::lfoTarget].choices)
    {
        // Frames first, so every widget draws over them. The same arrangement
        // the sidebar's pages use: a GroupComponent is a frame, not a
        // container — its contents stay children of the panel and stay in the
        // panel's one grid, which is what keeps the columns aligned between
        // groups.
        for (auto* group : { &filterGroup, &lfoGroup })
        {
            group->setTextLabelPosition (juce::Justification::centredLeft);
            group->setInterceptsMouseClicks (false, false);
            addAndMakeVisible (*group);
        }

        filterGroup.setText ("FILTER");
        lfoGroup   .setText ("LFO");

        // Vertical, both of them. Latin script runs along the horizontal axis,
        // so stacking the choices gives each word a full-width line of its own;
        // a horizontal strip divides that width between them and starts
        // eliding — "triangle" in a third of 88px does not survive.
        for (auto* choice : { &waveformSwitch, &targetSwitch })
        {
            choice->setOrientation (ChoiceStrip::Orientation::vertical);
            addAndMakeVisible (*choice);
        }

        bind (waveformSwitch, waveformAttachment, synth::Index::waveform);
        bind (targetSwitch,   targetAttachment,   synth::Index::lfoTarget);

        // Re-point the two shared knobs whenever the target changes, and once
        // now so they start on whatever the parameter already says.
        targetSwitch.onChoice = [this] (int index)
        {
            if (targetAttachment != nullptr)
                targetAttachment->setValueAsCompleteGesture ((float) index);

            retargetLfoKnobs();
        };

        cutoffKnob   .attach (apvts, synth::Index::cutoff,    "cutoff");
        resonanceKnob.attach (apvts, synth::Index::resonance, "resonance");

        for (auto* knob : { &cutoffKnob, &resonanceKnob, &rateKnob, &intensityKnob })
            addAndMakeVisible (*knob);

        retargetLfoKnobs();
    }

    /** Gives every widget the parameter menu, by the index it stands for. The
        panel does this rather than the editor because the panel is what knows
        which widget is which parameter — and, for the two shared knobs, what
        knows that the answer changes. */
    void attachMenu (ParameterMenu& menu)
    {
        menu.attachTo (waveformSwitch, synth::Index::waveform);
        menu.attachTo (targetSwitch,   synth::Index::lfoTarget);

        menu.attachTo (cutoffKnob.slider,    synth::Index::cutoff);
        menu.attachTo (resonanceKnob.slider, synth::Index::resonance);

        // Asked at the moment of the click, not now: these two edit whichever
        // pair the target switch is pointing at.
        menu.attachTo (rateKnob.slider,      [this] { return rateIndex(); });
        menu.attachTo (intensityKnob.slider, [this] { return intensityIndex(); });
    }

    void resized() override
    {
        // One grid for the whole panel, with the frames placed into it as
        // spanning items and a padding track inside each — the arrangement the
        // sidebar's pages use, and the reason the LFO's switch lines up with
        // the oscillator's and its knobs with the filter's.
        juce::Grid grid;
        using Track = juce::Grid::TrackInfo;
        using Px    = juce::Grid::Px;

        const Track pad   { Px (metrics::pageGroupPadding) };
        const Track knob  { Px (layout::knobSize) };
        const Track gap   { Px (layout::controlsGap) };
        const Track title { Px (metrics::pageGroupTitleHeight) };
        const Track cell  { Px (layout::knobCellHeight) };

        // Every line number below is the value of a counter that was
        // incremented by declaring a track, never a number typed in — so
        // inserting a track cannot leave half the panel pointing at the old
        // lines. The same reason `PageGrid` numbers the sidebar's rows this way.
        auto nextColumn = 1, nextRow = 1;

        const auto addColumn = [&] (const Track& track) { grid.templateColumns.add (track); return nextColumn++; };
        const auto addRow    = [&] (const Track& track) { grid.templateRows   .add (track); return nextRow++; };

        const auto oscFirstColumn = addColumn (pad);
        const auto switchColumn   = addColumn (Track (Px (layout::switchColumnWidth)));
        /*                       */ addColumn (pad);
        /*                       */ addColumn (gap);

        const auto filterFirstColumn = addColumn (pad);
        const auto leftKnob          = addColumn (knob);
        /*                          */ addColumn (gap);
        const auto rightKnob         = addColumn (knob);
        /*                          */ addColumn (pad);
        const auto filterLastColumn  = nextColumn;

        const auto topRow     = addRow (title);
        const auto topContent = addRow (cell);
        /*                   */ addRow (pad);
        const auto topLastRow = addRow (gap);

        const auto lfoRow     = addRow (title);
        const auto lfoContent = addRow (cell);
        /*                   */ addRow (pad);
        const auto lfoLastRow = nextRow;

        const auto place = [&grid] (juce::Component& c, int row, int column)
        {
            grid.items.add (juce::GridItem (c).withArea (row, column, row + 1, column + 1));
        };

        const auto frame = [&grid] (juce::Component& c, int firstRow, int lastRow,
                                    int firstColumn, int lastColumn)
        {
            grid.items.add (juce::GridItem (c).withArea (firstRow, firstColumn, lastRow, lastColumn));
        };

        frame (filterGroup, topRow, topLastRow, filterFirstColumn, filterLastColumn);
        frame (lfoGroup,    lfoRow, lfoLastRow, oscFirstColumn,    filterLastColumn);

        // The switches keep their natural height and sit in the middle of the
        // row, so a two-choice switch and a three-choice one have buttons of
        // the same size rather than each dividing the row it is in.
        const auto placeSwitch = [&grid] (ChoiceStrip& c, int row, int column, int choices)
        {
            grid.items.add (juce::GridItem (c)
                                .withArea (row, column, row + 1, column + 1)
                                .withHeight ((float) (choices * layout::switchButtonHeight))
                                .withAlignSelf (juce::GridItem::AlignSelf::center));
        };

        placeSwitch (waveformSwitch, topContent, switchColumn, waveformSwitch.getChoiceCount());
        placeSwitch (targetSwitch,   lfoContent, switchColumn, targetSwitch.getChoiceCount());

        place (cutoffKnob,    topContent, leftKnob);
        place (resonanceKnob, topContent, rightKnob);
        place (rateKnob,      lfoContent, leftKnob);
        place (intensityKnob, lfoContent, rightKnob);

        // The block keeps its own size and sits at the top left of whatever it
        // is given. Stretching it would stretch the knob cells, and a knob's
        // radius comes from the smaller side of its cell.
        const auto oscWidth    = metrics::pageGroupPadding * 2 + layout::switchColumnWidth;
        const auto filterWidth = metrics::pageGroupPadding * 2 + layout::knobSize * 2
                                   + layout::controlsGap;

        const auto groupHeight = metrics::pageGroupTitleHeight + layout::knobCellHeight
                                   + metrics::pageGroupPadding;

        const auto width  = oscWidth + layout::controlsGap + filterWidth;
        const auto height = groupHeight * 2 + layout::controlsGap;

        auto area = getLocalBounds().reduced (layout::controlsPadding);

        // Centred horizontally and anchored to the top, which is what the
        // settings tab does with its own block — the two are the same plugin
        // and should not each drift to a different corner.
        grid.performLayout (area.removeFromTop (juce::jmin (area.getHeight(), height))
                                .withSizeKeepingCentre (juce::jmin (area.getWidth(), width), height));
    }

    void lookAndFeelChanged() override
    {
        if (! getLookAndFeel().isColourSpecified (pageColours::sectionTitleColourId))
            return;

        for (auto* group : { &filterGroup, &lfoGroup })
        {
            group->setColour (juce::GroupComponent::textColourId,
                              findColour (pageColours::sectionTitleColourId));
            group->setColour (juce::GroupComponent::outlineColourId,
                              findColour (pageColours::sectionOutlineColourId));
        }
    }

    /** The value bubbles need somewhere with room to be drawn in, and the only
        moment that is reachable is once this panel has a parent. */
    void parentHierarchyChanged() override
    {
        if (auto* host = findPopupHost (*this))
            for (auto* knob : { &cutoffKnob, &resonanceKnob, &rateKnob, &intensityKnob })
                knob->showValueOver (*host);
    }

private:
    //==========================================================================
    /** A knob with its name over it, and its value in a bubble while you turn
        it.

        **No text box.** A read-out that is always there is a second thing to
        read on a panel whose knob positions already say roughly where
        everything is, and it costs a row under every knob. The bubble appears
        while the value is being changed, which is when the number matters.
    */
    struct Knob final : public juce::Component
    {
        Knob()
        {
            slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
            slider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);

            label.setJustificationType (juce::Justification::centred);

            // Scenery: a click on the name should reach the knob's menu rather
            // than stopping at the word above it.
            label.setInterceptsMouseClicks (false, false);

            addAndMakeVisible (label);
            addAndMakeVisible (slider);
        }

        /** Points the knob at a parameter, replacing whatever it was on. The
            formatting has to be re-applied every time, because a new attachment
            brings its own `textFromValueFunction` — see the note below. */
        void attach (juce::AudioProcessorValueTreeState& state, int controlIndex,
                     const juce::String& labelText)
        {
            const auto& control = synth::controls()[(size_t) controlIndex];

            label.setText (labelText, juce::dontSendNotification);

            // The component name is what the snapshot tool goes by, and it must
            // say which knob this is rather than which parameter it currently
            // holds — the shared ones change parameter under the same name.
            slider.setName (labelText);

            attachment.reset();
            attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
                state, control.id, slider);

            slider.setTextValueSuffix (control.unit.isEmpty() ? juce::String()
                                                              : " " + control.unit);

            // **After the attachment, and as a function.**
            // `SliderParameterAttachment` installs its own
            // `textFromValueFunction`, and `Slider::getTextFromValue` reads that
            // before it ever looks at `setNumDecimalPlacesToDisplay` — so the
            // decimal-place call is silently ignored on an attached slider, and
            // the bubble reads 1999.9998779. The suffix is not part of this:
            // JUCE appends it to whatever this returns.
            const auto decimals = control.range.end >= 100.0f ? 0 : 2;

            slider.textFromValueFunction = [decimals] (double value)
            {
                return juce::String (value, decimals);
            };

            slider.updateText();
        }

        /** The bubble is added as a *child* of whatever is passed here and is
            clipped to it, so it has to be something with room — this panel
            would leave a sliver of it showing at the edges. */
        void showValueOver (juce::Component& host)
        {
            slider.setPopupDisplayEnabled (true, true, &host);
        }

        void resized() override
        {
            auto area = getLocalBounds();

            label .setBounds (area.removeFromTop (layout::knobLabelHeight));
            slider.setBounds (area);
        }

        void lookAndFeelChanged() override
        {
            label.setFont (SidebarLookAndFeel::font (metrics::bodyFontHeight));
        }

        juce::Label label;
        juce::Slider slider;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
    };

    //==========================================================================
    int rateIndex() const
    {
        return targetSwitch.getSelectedIndex() == pitchTarget ? synth::Index::pitchLfoRate
                                                              : synth::Index::filterLfoRate;
    }

    int intensityIndex() const
    {
        return targetSwitch.getSelectedIndex() == pitchTarget ? synth::Index::pitchLfoDepth
                                                              : synth::Index::filterLfoDepth;
    }

    void retargetLfoKnobs()
    {
        rateKnob     .attach (apvts, rateIndex(),      "rate");
        intensityKnob.attach (apvts, intensityIndex(), "intensity");
    }

    /** Binds a switch to its choice parameter. `ChoiceStrip` is not a JUCE
        widget, so there is no ready-made attachment for it. */
    void bind (ChoiceStrip& strip, std::unique_ptr<juce::ParameterAttachment>& attachment,
               int controlIndex)
    {
        auto* parameter = apvts.getParameter (synth::controls()[(size_t) controlIndex].id);

        if (parameter == nullptr)
        {
            jassertfalse;   // A control was renamed in one place and not the other.
            return;
        }

        attachment = std::make_unique<juce::ParameterAttachment> (
            *parameter,
            [&strip] (float value) { strip.setSelectedIndex (juce::roundToInt (value)); },
            nullptr);

        // The target switch needs more than this, so it sets its own onChoice
        // after the fact; this covers the waveform, which needs only the echo
        // back to the parameter.
        strip.onChoice = [raw = attachment.get()] (int index)
        {
            raw->setValueAsCompleteGesture ((float) index);
        };

        attachment->sendInitialUpdate();
    }

    /** Which entry of the target switch is the pitch LFO — the second, as
        `synth::controls()` lists them. */
    static constexpr int pitchTarget = 1;

    juce::AudioProcessorValueTreeState& apvts;

    juce::GroupComponent filterGroup, lfoGroup;

    ChoiceStrip waveformSwitch, targetSwitch;
    Knob cutoffKnob, resonanceKnob, rateKnob, intensityKnob;

    std::unique_ptr<juce::ParameterAttachment> waveformAttachment, targetAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DemoSynthPanel)
};

} // namespace microtonos::sidebar::demo
