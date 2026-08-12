#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include "../SidebarIcons.h"
#include "../SidebarLookAndFeel.h"

namespace microtonos::sidebar
{

//==============================================================================
/** A button drawn as a table header cell, with a sort arrow when it is on.

    For a control that sits in a header row without being a column of one. The
    controllers page has two: the parameter column is frozen beside the table
    rather than in it, so its two orderings — by name and by when a mapping was
    added — cannot be header columns, but they belong in the header row and
    should look like it.

    **Painted rather than styled.** `LookAndFeel::drawTableHeaderColumn` is only
    ever called by a `TableHeaderComponent`, so there is no way to ask for a
    header cell without having a header; and a `TextButton` cannot be talked
    into looking like one, because `drawButtonBackground` always rounds its
    corners. What this does instead is read the same four `TableHeaderComponent`
    ColourIds and the same `drawSortArrow`, so they match the real header beside
    them by construction rather than by two sets of numbers agreeing — the
    background, the line along the bottom, the divider between cells and the
    mark on the one in force.

    The arrow points up on both. These two are alternative *orderings* rather
    than the two directions of one, so what it says here is "this is the one
    sorting the table", which is also all it says on a column that has only ever
    been clicked once.
*/
class HeaderButton final : public juce::Button
{
public:
    /** @param name   the component name, which is also the label unless an icon
                      replaces it — and what the snapshot tool's `--click` goes
                      by either way */
    explicit HeaderButton (const juce::String& name) : juce::Button (name)
    {
        setButtonText (name);
        setClickingTogglesState (true);
    }

    /** Draws this icon instead of the label. The SVG is kept rather than the
        Drawable, because the Drawable carries its colour and has to be rebuilt
        whenever the theme changes. */
    void setIcon (const char* svg)
    {
        iconSvg = svg;
        refreshIcon();
    }

    /** A divider on the right edge, as the header draws between its columns.
        Off on the last of a run, where the gap before the table takes over. */
    void setShowsDivider (bool shouldShow)
    {
        showsDivider = shouldShow;
        repaint();
    }

    void paintButton (juce::Graphics& g, bool isMouseOver, bool isMouseDown) override
    {
        const auto text = findColour (juce::TableHeaderComponent::textColourId);

        g.fillAll (findColour (juce::TableHeaderComponent::backgroundColourId));

        const auto highlight = findColour (juce::TableHeaderComponent::highlightColourId);

        if (isMouseDown)
            g.fillAll (highlight);
        else if (isMouseOver)
            g.fillAll (highlight.withMultipliedAlpha (metrics::hoverAlpha));

        if (showsDivider)
        {
            g.setColour (findColour (juce::TableHeaderComponent::outlineColourId));
            g.fillRect (getWidth() - 1, 0, 1, getHeight());
        }

        // The line along the bottom, which is what joins this to the header
        // beside it rather than leaving it floating above the list.
        auto bounds = getLocalBounds();

        g.setColour (findColour (juce::TableHeaderComponent::outlineColourId));
        g.fillRect (bounds.removeFromBottom (1));

        if (showsDivider)
            g.fillRect (bounds.removeFromRight (1));

        auto area = bounds.reduced (metrics::tableTextPadding / 2, 0);

        if (getToggleState())
            SidebarLookAndFeel::drawSortArrow (g, area.removeFromRight (getHeight() / 2), true, text);

        if (icon != nullptr)
        {
            icon->drawWithin (g, area.toFloat().reduced ((float) metrics::tableCellInset),
                              juce::RectanglePlacement::centred, 1.0f);
            return;
        }

        g.setColour (text);
        g.setFont (SidebarLookAndFeel::font (metrics::bodyFontHeight * metrics::headerFontScale, true));
        g.drawFittedText (getButtonText(), area, juce::Justification::centredLeft, 1);
    }

    void lookAndFeelChanged() override { refreshIcon(); }

private:
    void refreshIcon()
    {
        if (iconSvg == nullptr)
            return;

        // The icon bakes its colour in, so a theme change has to rebuild it —
        // the same reason the rail rebuilds its icons rather than repainting.
        icon = icons::load (iconSvg, findColour (juce::TableHeaderComponent::textColourId));
        repaint();
    }

    const char* iconSvg = nullptr;
    std::unique_ptr<juce::Drawable> icon;
    bool showsDivider = true;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (HeaderButton)
};

} // namespace microtonos::sidebar
