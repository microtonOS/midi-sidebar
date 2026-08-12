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
    target.setColour (Sidebar::iconColourId,            text.withMultipliedAlpha (shades::icon));
    target.setColour (Sidebar::iconOverColourId,        text);
    target.setColour (Sidebar::iconActiveColourId,      accent);
    target.setColour (Sidebar::activeIndicatorColourId, accent);
    target.setColour (Sidebar::separatorColourId,       text.withMultipliedAlpha (shades::hairline));

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
    target.setColour (pageColours::sectionOutlineColourId, text.withMultipliedAlpha (shades::hairline));

    // A read-out is recessed into the panel: `window` is darker than `widget`
    // in every scheme that has a dark surface, and lighter in the light one, so
    // it reads as a hollow either way. The same pairing the level meter uses
    // for its track.
    target.setColour (ReadOutField::backgroundColourId, window);
    target.setColour (ReadOutField::textColourId,       text);
    target.setColour (ReadOutField::outlineColourId,    text.withMultipliedAlpha (shades::hairline));

    // The chosen button of a segmented control takes the accent, the same one
    // the rail gives an active page icon, so "this one is on" means one thing
    // across the whole sidebar.
    target.setColour (ChoiceStrip::selectedColourId,     accent);
    target.setColour (ChoiceStrip::selectedTextColourId, accent.contrasting());

    // Table headers. `LookAndFeel_V2` hardcodes these four in its own colour
    // table — a pale blue-white with black text — and `LookAndFeel_V4` never
    // touches them, so a header is the one part of a V4 plugin that ignores the
    // scheme entirely. On a dark theme it arrives as a bright band across the
    // table. Registered here so it follows the sidebar like everything else.
    target.setColour (juce::TableHeaderComponent::backgroundColourId, widget);
    target.setColour (juce::TableHeaderComponent::textColourId,       text);
    target.setColour (juce::TableHeaderComponent::outlineColourId,    text.withMultipliedAlpha (shades::hairline));
    target.setColour (juce::TableHeaderComponent::highlightColourId,  accent.withMultipliedAlpha (shades::selectedRow));

    // Tabs. Not a sidebar widget — nothing in the module uses one — but this
    // LookAndFeel dresses the whole plugin, and JUCE's default here is a trap:
    // with none of these ids specified, `LookAndFeel_V2::drawTabButtonText`
    // falls back to `button.getTabBackgroundColour().contrasting()`, and a tab
    // given a transparent background is treated as black, so the label comes
    // out white — invisible on the Light scheme. The demo's tabs did exactly
    // that.
    //
    // Note also *where* the override has to go. That method asks whether the
    // button OR the LookAndFeel specifies the id, then reads the value from the
    // **LookAndFeel** either way — so setting these on the tab bar makes the
    // condition true and then fetches a colour that was never set. They belong
    // here and nowhere else.
    target.setColour (juce::TabbedButtonBar::tabTextColourId,      text.withMultipliedAlpha (shades::readOnly));
    target.setColour (juce::TabbedButtonBar::frontTextColourId,    text);
    target.setColour (juce::TabbedButtonBar::tabOutlineColourId,   text.withMultipliedAlpha (shades::hairline));
    target.setColour (juce::TabbedButtonBar::frontOutlineColourId, text.withMultipliedAlpha (shades::icon));
}

void SidebarLookAndFeel::drawSortArrow (juce::Graphics& g, juce::Rectangle<int> area,
                                        bool forwards, juce::Colour colour)
{
    juce::Path arrow;

    // JUCE's own shape, from LookAndFeel_V2::drawTableHeaderColumn: a triangle
    // in a unit box, scaled to fit. Kept identical so the mark on the ordering
    // buttons cannot drift from the one on the columns beside them.
    arrow.addTriangle (0.0f, 0.0f,
                       0.5f, forwards ? -0.8f : 0.8f,
                       1.0f, 0.0f);

    g.setColour (colour);
    g.fillPath (arrow, arrow.getTransformToScaleToFit (area.reduced (2).toFloat(), true));
}

void SidebarLookAndFeel::drawTableHeaderBackground (juce::Graphics& g, juce::TableHeaderComponent& header)
{
    g.fillAll (header.findColour (juce::TableHeaderComponent::backgroundColourId));

    g.setColour (header.findColour (juce::TableHeaderComponent::outlineColourId));

    auto area = header.getLocalBounds();
    g.fillRect (area.removeFromBottom (1));

    // A divider on the right of every column, as JUCE draws them — which is
    // what the two ordering buttons beside this header have to reproduce.
    for (int i = header.getNumColumns (true); --i >= 0;)
        g.fillRect (header.getColumnPosition (i).removeFromRight (1));
}

void SidebarLookAndFeel::drawTableHeaderColumn (juce::Graphics& g, juce::TableHeaderComponent& header,
                                                const juce::String& columnName, int /*columnId*/,
                                                int width, int height,
                                                bool isMouseOver, bool isMouseDown, int columnFlags)
{
    const auto highlight = header.findColour (juce::TableHeaderComponent::highlightColourId);

    if (isMouseDown)
        g.fillAll (highlight);
    else if (isMouseOver)
        g.fillAll (highlight.withMultipliedAlpha (metrics::hoverAlpha));

    auto area = juce::Rectangle<int> (width, height).reduced (metrics::tableTextPadding / 2, 0);

    const auto text = header.findColour (juce::TableHeaderComponent::textColourId);

    constexpr auto sorted = juce::TableHeaderComponent::sortedForwards
                              | juce::TableHeaderComponent::sortedBackwards;

    if ((columnFlags & sorted) != 0)
        drawSortArrow (g, area.removeFromRight (height / 2),
                       (columnFlags & juce::TableHeaderComponent::sortedForwards) != 0,
                       text);

    g.setColour (text);
    g.setFont (font (metrics::bodyFontHeight * metrics::headerFontScale, true));
    g.drawFittedText (columnName, area, juce::Justification::centredLeft, 1);
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
    return buttonFont (buttonHeight);
}

juce::Font SidebarLookAndFeel::buttonFont (int buttonHeight)
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
