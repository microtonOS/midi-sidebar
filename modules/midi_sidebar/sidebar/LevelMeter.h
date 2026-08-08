#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "SidebarLookAndFeel.h"

#include <cmath>

namespace microtonos::sidebar
{

//==============================================================================
/** A stereo level meter: two vertical bars side by side, left then right.

    Surge XT's meter is horizontal and stacks the two channels as a top and a
    bottom half; rotated ninety degrees to sit beside a vertical fader, that
    becomes two columns.

    Deliberately dumb: it paints whatever levels it was last given and knows
    nothing about where they come from. The audio thread must never call into
    this directly — the owner reads an atomic on the message thread, from a
    Timer, and pushes the values in with setLevel.
*/
class LevelMeter final : public juce::Component
{
public:
    enum ColourIds
    {
        trackColourId = 0x1a10100,
        fillColourId  = 0x1a10101
    };

    LevelMeter()
    {
        setInterceptsMouseClicks (false, false);
    }

    /** Converts a raw amplitude to a position up the column, 0 to 1.

        The scale is decibels between metrics::floorDb and 0, the same mapping
        the fader uses, so a meter reading level with the fader is the audible
        result and the two can be compared by eye. Amplitude alone would not
        work: linear amplitude crushes everything below about -20 dB into the
        bottom pixel, which is why meters that do it look dead. */
    static float positionForAmplitude (float amplitude) noexcept
    {
        if (amplitude <= 0.0f)
            return 0.0f;

        // juce::Decibels lives in juce_audio_basics, and this is a UI module —
        // not worth an audio dependency for one logarithm.
        const auto dB = juce::jmax (metrics::floorDb, std::log10 (amplitude) * 20.0f);

        return juce::jmap (dB, metrics::floorDb, 0.0f, 0.0f, 1.0f);
    }

    /** @param newLeft, newRight  raw amplitudes, 0 = silence, 1 = full scale. */
    void setLevel (float newLeft, float newRight)
    {
        newLeft  = positionForAmplitude (newLeft);
        newRight = positionForAmplitude (newRight);

        newLeft  = juce::jlimit (0.0f, 1.0f, newLeft);
        newRight = juce::jlimit (0.0f, 1.0f, newRight);

        if (! juce::approximatelyEqual (newLeft, left)
            || ! juce::approximatelyEqual (newRight, right))
        {
            left  = newLeft;
            right = newRight;
            repaint();
        }
    }

    float getLeftLevel()  const noexcept { return left; }
    float getRightLevel() const noexcept { return right; }

    void paint (juce::Graphics& g) override
    {
        auto bounds = getLocalBounds();

        // Two equal columns with the gap between them. Splitting by removal
        // rather than by dividing the width means the gap cannot be lost to
        // rounding at odd widths.
        const auto barWidth = (bounds.getWidth() - metrics::meterBarGap) / 2;

        paintBar (g, bounds.removeFromLeft (barWidth), left);
        paintBar (g, bounds.removeFromRight (barWidth), right);
    }

private:
    void paintBar (juce::Graphics& g, juce::Rectangle<int> area, float level) const
    {
        const auto bar = area.toFloat();
        const auto corner = bar.getWidth() * 0.5f;

        g.setColour (findColour (trackColourId));
        g.fillRoundedRectangle (bar, corner);

        if (level <= 0.0f)
            return;

        // Grows upwards from the bottom, matching the fader beside it.
        const auto filled = bar.withTop (bar.getBottom() - bar.getHeight() * level);

        g.setColour (findColour (fillColourId));
        g.fillRoundedRectangle (filled, corner);
    }

    float left = 0.0f;
    float right = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LevelMeter)
};

} // namespace microtonos::sidebar
