#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "LevelMeter.h"
#include "SidebarLookAndFeel.h"

namespace microtonos::sidebar
{

//==============================================================================
/** The volume slider and its level meter, side by side on a shared scale.

    This is a compound widget — it owns two children and lays them out against
    its own bounds. That is normally a mistake, because a compound widget cannot
    align with anything outside itself. It is right here for two reasons: the
    slider and the meter must align with *each other* and with nothing else (the
    shared scale is the whole point), and the pair appears in two places — inline
    in the rail when there is room, and inside a CallOutBox when there is not.
    Keeping them together is what makes the second case possible.
*/
class VolumeStrip final : public juce::Component
{
public:
    VolumeStrip()
    {
        slider.setSliderStyle (juce::Slider::LinearVertical);
        slider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
        // Decibels, not a 0..1 fraction and not a percentage. MIDI defines CC7
        // as a dB curve — the GM specifications give dB = 40 * log10(cc/127),
        // so CC7 64 is about -12 dB rather than half volume — and it is the
        // only way the fader and the meter beside it can be read against each
        // other, since a meter is inherently logarithmic.
        slider.setRange (metrics::floorDb, 0.0);
        slider.setValue (0.0, juce::dontSendNotification);

        slider.textFromValueFunction = [] (double value)
        {
            if (value <= metrics::floorDb)
                return juce::String (juce::CharPointer_UTF8 ("-\xe2\x88\x9e dB"));

            return juce::String (value, 1) + " dB";
        };
        slider.valueFromTextFunction = [] (const juce::String& text)
        {
            return text.getDoubleValue();
        };
        slider.updateText();

        addAndMakeVisible (slider);
        addAndMakeVisible (meter);
    }

    juce::Slider& getSlider() noexcept { return slider; }
    LevelMeter&   getMeter()  noexcept { return meter; }

    /** Where the slider's value bubble should live.

        The bubble is added as a CHILD of whatever component is given here (see
        Slider::Pimpl::showPopupDisplay), so it is clipped to that component's
        bounds. Handing it the strip clips it to about 22px and leaves only the
        bubble's tail visible, which reads on screen as a stray '<'. Handing it
        the top-level component works in the rail, but not inside a CallOutBox,
        where the top level *is* the callout and the bubble is cut off again —
        so the owner passes something big enough in both cases.
    */
    void setPopupParent (juce::Component* parent)
    {
        slider.setPopupDisplayEnabled (parent != nullptr, parent != nullptr, parent);
    }

    void parentHierarchyChanged() override { refreshSliderLayout(); }
    void lookAndFeelChanged()     override { refreshSliderLayout(); }

private:
    void refreshSliderLayout()
    {
        // Slider caches whatever LookAndFeel::getSliderLayout returned, and the
        // only place it recomputes that is resized(). Slider::lookAndFeelChanged
        // rebuilds the text box and nothing else. So a slider sized before its
        // real LookAndFeel was reachable keeps the *default* layout — including
        // the thumb indent this module overrides away, which makes the fader
        // shorter than the meter beside it.
        //
        // That is exactly what happens inside a CallOutBox: the content has to
        // be given a size before it can be attached, so it is laid out while
        // still parentless. Force the recompute once the real one arrives.
        //
        // Both hooks, because they fire in different situations: attaching to
        // an already-styled parent sends parentHierarchyChanged but NOT
        // lookAndFeelChanged, and restyling in place does the opposite.
        slider.resized();
    }

public:

    void resized() override
    {
        juce::Grid grid;
        using Track = juce::Grid::TrackInfo;
        using juce::Grid;

        // Laid out so the *drawn* marks are symmetric, not the component boxes.
        // The fader's box is wider than its track by the thumb's overhang, so
        // the leading space and the gap are each shortened by that much; what
        // ends up on screen is stripPadding, track, stripVisibleGap, meter,
        // stripPadding. The trailing track is flexible so any odd pixel from
        // the division lands there rather than skewing the marks.
        grid.templateRows    = { Track (Grid::Fr (1)) };
        grid.templateColumns = {
            Track (Grid::Px (metrics::stripPadding - metrics::faderThumbOverhang)),
            Track (Grid::Px (metrics::faderThumbDiameter)),
            Track (Grid::Px (metrics::stripVisibleGap - metrics::faderThumbOverhang)),
            Track (Grid::Px (metrics::meterWidth)),
            Track (Grid::Fr (1))
        };

        grid.items = { juce::GridItem(),
                       juce::GridItem (slider),
                       juce::GridItem(),
                       juce::GridItem (meter),
                       juce::GridItem() };

        grid.performLayout (getLocalBounds());
    }

private:

    juce::Slider slider;
    LevelMeter meter;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VolumeStrip)
};

} // namespace microtonos::sidebar
