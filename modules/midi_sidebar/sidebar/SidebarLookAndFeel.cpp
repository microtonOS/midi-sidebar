#include "SidebarLookAndFeel.h"
#include "LevelMeter.h"
#include "Sidebar.h"
#include "SidebarPanel.h"
#include "widgets/ChoiceStrip.h"
#include "widgets/ReadOutField.h"

namespace microtonos::sidebar
{

SidebarLookAndFeel::SidebarLookAndFeel()
{
    // Start from JUCE's dark scheme rather than inventing a palette. Anything
    // the sidebar needs beyond those nine colours is derived from them below,
    // so re-theming means changing the scheme and nothing else.
    setScheme (getDarkColourScheme());
}

void SidebarLookAndFeel::setScheme (const ColourScheme& newScheme)
{
    setColourScheme (newScheme);
    registerColours (*this, newScheme);
}

void SidebarLookAndFeel::registerColours (juce::LookAndFeel& target, const ColourScheme& scheme)
{
    const auto window = scheme.getUIColour (ColourScheme::UIColour::windowBackground);
    const auto widget = scheme.getUIColour (ColourScheme::UIColour::widgetBackground);
    const auto text   = scheme.getUIColour (ColourScheme::UIColour::defaultText);

    // defaultFill, not highlightedFill: in the dark scheme highlightedFill is
    // the near-black surface drawn *behind* highlighted text (0xff181f22),
    // while defaultFill is the blue accent. Using the former for an accent
    // gives an invisible icon.
    const auto accent = scheme.getUIColour (ColourScheme::UIColour::defaultFill);

    // Every colour below is derived from those four, so re-theming means
    // changing the scheme above and nothing else.
    target.setColour (Sidebar::backgroundColourId,      widget);
    target.setColour (Sidebar::iconColourId,            text.withMultipliedAlpha (0.75f));
    target.setColour (Sidebar::iconOverColourId,        text);
    target.setColour (Sidebar::iconActiveColourId,      accent);
    target.setColour (Sidebar::activeIndicatorColourId, accent);
    target.setColour (Sidebar::separatorColourId,       text.withMultipliedAlpha (0.15f));

    target.setColour (LevelMeter::trackColourId, window);
    target.setColour (LevelMeter::fillColourId,  accent);

    // The fader and the meter are a matched pair on a shared scale, so they take
    // the same two colours. Without a visible background track the fader looked
    // shorter than the meter: JUCE draws its track over the full height either
    // way, but the only part with any contrast was the filled section below the
    // thumb, which cannot reach the ends because the thumb needs room.
    target.setColour (juce::Slider::backgroundColourId, window);
    target.setColour (juce::Slider::trackColourId,      accent);
    target.setColour (juce::Slider::thumbColourId,      text);

    // The panel takes the *same* surface as the rail, so the sidebar reads as
    // one object. It must not take `window`, which is what a JUCE editor paints
    // itself with — the panel would then be invisible against the host's
    // background, which is exactly the bug this replaced.
    target.setColour (SidebarPanel::backgroundColourId, widget);
    target.setColour (SidebarPanel::titleColourId,      text);

    // Pages. A section's outline is the same hairline the sidebar draws where
    // it meets the host's content, so the two agree by construction.
    target.setColour (pageColours::sectionTitleColourId,   text);
    target.setColour (pageColours::sectionOutlineColourId, text.withMultipliedAlpha (0.15f));

    // A read-out is recessed into the panel: `window` is darker than `widget`
    // in every scheme that has a dark surface, and lighter in the light one, so
    // it reads as a hollow either way. The same pairing the level meter uses
    // for its track.
    target.setColour (ReadOutField::backgroundColourId, window);
    target.setColour (ReadOutField::textColourId,       text);
    target.setColour (ReadOutField::outlineColourId,    text.withMultipliedAlpha (0.15f));

    // The chosen button of a segmented control takes the accent, the same one
    // the rail gives an active page icon, so "this one is on" means one thing
    // across the whole sidebar.
    target.setColour (ChoiceStrip::selectedColourId,     accent);
    target.setColour (ChoiceStrip::selectedTextColourId, accent.contrasting());
}

juce::Slider::SliderLayout SidebarLookAndFeel::getSliderLayout (juce::Slider& slider)
{
    // Only the vertical fader wants the treatment below. Everything else keeps
    // JUCE's layout — which matters because this used to apply to every slider,
    // and a slider that *does* want a text box, such as the period chooser's
    // inc/dec box, would have had it thrown away and drawn nothing at all.
    if (slider.getSliderStyle() != juce::Slider::LinearVertical)
        return LookAndFeel_V4::getSliderLayout (slider);

    juce::Slider::SliderLayout layout;

    // The whole component, with no thumb indent: the fader must occupy exactly
    // the rectangle the meter beside it occupies. The fader uses NoTextBox, so
    // there is nothing else competing for the space.
    layout.sliderBounds = slider.getLocalBounds();
    layout.textBoxBounds = {};

    return layout;
}

void SidebarLookAndFeel::drawLinearSlider (juce::Graphics& g, int x, int y, int width, int height,
                                           float sliderPos, float, float,
                                           juce::Slider::SliderStyle style, juce::Slider& slider)
{
    if (style != juce::Slider::LinearVertical)
    {
        LookAndFeel_V4::drawLinearSlider (g, x, y, width, height,
                                          sliderPos, 0.0f, 0.0f, style, slider);
        return;
    }

    const auto bounds = juce::Rectangle<int> (x, y, width, height).toFloat();
    const auto track = bounds.withSizeKeepingCentre ((float) metrics::faderTrackWidth,
                                                     bounds.getHeight());
    const auto corner = track.getWidth() * 0.5f;

    g.setColour (slider.findColour (juce::Slider::backgroundColourId));
    g.fillRoundedRectangle (track, corner);

    // Because the layout above hands us the full height, sliderPos reaches the
    // very top and bottom: the fill spans the same range as a meter bar.
    g.setColour (slider.findColour (juce::Slider::trackColourId));
    g.fillRoundedRectangle (track.withTop (sliderPos), corner);

    // The thumb is the one thing that must stay inside, since nothing clips it.
    const auto radius = (float) metrics::faderThumbDiameter * 0.5f;
    const auto centreY = juce::jlimit (bounds.getY() + radius, bounds.getBottom() - radius, sliderPos);

    g.setColour (slider.findColour (juce::Slider::thumbColourId));
    g.fillEllipse (juce::Rectangle<float> (metrics::faderThumbDiameter, metrics::faderThumbDiameter)
                       .withCentre ({ bounds.getCentreX(), centreY }));
}

juce::Font SidebarLookAndFeel::getTextButtonFont (juce::TextButton&, int buttonHeight)
{
    return font (juce::jmin (metrics::bodyFontHeight, (float) buttonHeight * 0.5f));
}

juce::Font SidebarLookAndFeel::font (float height, bool bold)
{
    // juce::Font (float) is deprecated; FontOptions is the supported route.
    // Leaving the typeface name unset uses the platform's default sans, which
    // is the right starting point until the design says otherwise — and it is
    // one family, so the "at most three fonts" budget is untouched.
    auto options = juce::FontOptions().withHeight (height);

    if (bold)
        options = options.withStyle ("Bold");

    return juce::Font (options);
}

} // namespace microtonos::sidebar
