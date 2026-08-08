#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace microtonos::sidebar
{

//==============================================================================
/** Every measurement the sidebar uses.

    Nothing in a `resized()` anywhere in this module may contain a numeric
    literal other than 0, 1 or 2 — if a number matters, it is named here. That
    is what lets the layout be changed later without hunting through the code,
    and it is checked by `scripts/layout_lint.py`.

    Values that depend on other values are derived rather than written out, so
    changing one measurement cannot leave a second one silently stale.
*/
namespace metrics
{
    /** Icon buttons in the rail: presets, controllers, tuning, volume, panic. */
    inline constexpr int railButtonCount = 5;

    /** Side of the square hit area of one rail button. */
    inline constexpr int railButton = 36;

    /** Side of the icon drawn inside it. The ratio comes from the mockup in
        docs/sidebar.md, which uses a 2em icon in a 3.5em button. */
    inline constexpr int railIcon = 20;

    /** Vertical space between two rail buttons. */
    inline constexpr int railGap = 4;

    /** Space between the rail's contents and its edges. */
    inline constexpr int railPadding = 6;

    /** Width of the collapsed sidebar. */
    inline constexpr int railWidth = railButton + railPadding * 2;

    /** Height of the volume slider + meter strip that replaces the volume
        button once there is room for it. */
    inline constexpr int volumeStripHeight = 96;

    /** The level meter running alongside the volume slider. Two channels, so
        two bars plus the gap between them; the total is what the strip's layout
        reserves. */
    inline constexpr int meterBarWidth = 4;
    inline constexpr int meterBarGap   = 2;
    inline constexpr int meterWidth    = meterBarWidth * 2 + meterBarGap;

    /** Space between the volume slider and the meter. */
    inline constexpr int meterGap = 4;

    /** The fader's track is drawn to the same rectangle as a meter bar, so the
        two read as a matched pair on one scale. Its track is wider because it
        is a control rather than a read-out. */
    inline constexpr int faderTrackWidth    = 6;
    inline constexpr int faderThumbDiameter = 10;

    /** Bottom of the scale, in decibels. The fader and the meter both use it,
        which is what lets them be read against each other: the same distance up
        the column means the same thing for both. Below this the fader is off
        and the meter is empty. */
    inline constexpr float floorDb = -60.0f;

    /** Space between the fader's track and the nearest meter bar. */
    inline constexpr int stripVisibleGap = 4;

    /** Padding either side of the fader-and-meter group.

        Derived, not chosen, from the rule that makes the strip look right: the
        distance from the column's left edge to the fader's *track* must equal
        the distance from its right edge to the outer meter bar. Measuring from
        the drawn lines rather than from the component boxes is the whole point
        — the fader's box is wider than its track, because the thumb overhangs,
        so laying the boxes out symmetrically leaves the visible marks
        lopsided.

        With the current sizes this lands on 8px, the same as the icons'
        optical padding, (railButton - railIcon) / 2. That is a pleasing
        coincidence rather than a constraint: the icons are not all the same
        visible width, so they cannot serve as the reference. */
    inline constexpr int stripPadding = (railButton
                                         - faderTrackWidth
                                         - stripVisibleGap
                                         - meterWidth) / 2;

    /** How far the thumb sticks out past its track on each side. The fader's
        component has to be at least this much wider than the track, or the
        thumb is clipped — which is why the fader's box and its track need
        separate positions. */
    inline constexpr int faderThumbOverhang = (faderThumbDiameter - faderTrackWidth) / 2;

    /** Width of the panel revealed when a page button is active. */
    inline constexpr int panelWidth = 260;

    //==========================================================================
    //  Derived sizes. These are the numbers the editor should use for its
    //  resize limits — see the Resizing rules in the skill: a minimum size is
    //  derived from the content, never picked by eye.

    /** The rail is always the same shape: three page buttons at the top, the
        volume control and the panic button anchored to the bottom, and one
        flexible track between them that absorbs whatever is left over. Only the
        volume control changes with height, from a button to the slider strip,
        so the rail's fixed height is a function of that one extent.

        Six tracks means five gaps — including the one either side of the
        flexible track, which is easy to forget and leaves the rail a few pixels
        too tall for its stated minimum. */
    inline constexpr int railPageButtons = 3;
    inline constexpr int railTrackCount  = railPageButtons + 2 + 1;   // + panic + slack

    constexpr int railFixedHeight (int volumeExtent) noexcept
    {
        return railPadding * 2
             + railButton * railPageButtons
             + volumeExtent
             + railButton                                   // panic
             + railGap * (railTrackCount - 1);
    }

    /** Below this the rail cannot be drawn at all, so it is the sidebar's
        minimum height. */
    inline constexpr int railMinHeight = railFixedHeight (railButton);

    /** At and above this there is room to replace the volume button with the
        slider-and-meter strip. */
    inline constexpr int regularBreakpoint = railFixedHeight (volumeStripHeight);

    //==========================================================================
    /** Default animation time for expanding and collapsing, in milliseconds.
        The developer can override this; see Sidebar::setAnimationMilliseconds. */
    inline constexpr int defaultAnimationMs = 180;

    /** Text sizes. One family, so the "few designs" rule is satisfied by
        construction. */
    inline constexpr float titleFontHeight = 15.0f;
    inline constexpr float bodyFontHeight  = 13.0f;
}

//==============================================================================
/** Look and feel for the sidebar.

    Starts from JUCE's dark scheme and changes as little as possible; colours
    are added here as the design develops rather than at the widgets that use
    them.
*/
class SidebarLookAndFeel : public juce::LookAndFeel_V4
{
public:
    SidebarLookAndFeel();

    /** Teaches a LookAndFeel every ColourId this module's widgets ask for.

        The constructor calls this on itself. Call it yourself if you use your
        own LookAndFeel instead of this one: a widget whose ColourIds are
        missing gets `Colours::black` from `findColour`, plus an assertion, and
        the result is a control that renders as a black rectangle. */
    static void registerColours (juce::LookAndFeel& target, const ColourScheme& scheme);

    /** A TextButton does not shrink its label to fit, so short labels on small
        buttons are clipped to an ellipsis. Scale the font to the button. */
    juce::Font getTextButtonFont (juce::TextButton&, int buttonHeight) override;

    /** JUCE insets a vertical slider's drawing area by `getSliderThumbRadius`
        at each end so the thumb has somewhere to overhang. That happens before
        drawLinearSlider is called, so a fader laid out beside a meter of the
        same height comes out visibly shorter — 16 px shorter at the sizes used
        here — and no amount of custom drawing can recover it. Give the fader
        its full bounds and clamp the thumb instead. */
    juce::Slider::SliderLayout getSliderLayout (juce::Slider&) override;

    /** Draws the fader to the same rectangle a meter bar occupies, so the pair
        line up exactly. */
    void drawLinearSlider (juce::Graphics&, int x, int y, int width, int height,
                           float sliderPos, float minSliderPos, float maxSliderPos,
                           juce::Slider::SliderStyle, juce::Slider&) override;

    // Call-outs are left to LookAndFeel_V4, which fills them with
    // `widgetBackground` at 0.8 alpha over a drop shadow and rims them with a
    // 2px `outline` stroke. That is deliberately unlike the opaque rail behind
    // it: the translucency and the brighter rim are what make a pop-up read as
    // floating above the sidebar rather than as part of it.

    /** The single font family for the whole module. */
    static juce::Font font (float height, bool bold = false);

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SidebarLookAndFeel)
};

} // namespace microtonos::sidebar
