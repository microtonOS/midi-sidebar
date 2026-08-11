#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <midi_sidebar/midi_sidebar.h>

#include "DemoSettings.h"
#include "DemoStyle.h"

namespace microtonos::sidebar::demo
{

//==============================================================================
/** The developer settings, as one tab of the host's area.

    Nothing here is part of the sidebar; a plugin embedding it makes these
    choices once, in code. The rectangle they sit in — and the caption naming it
    — belong to `DemoContent`, which holds this and the synth panel as its two
    tabs: the outline marks the whole of the host's area, not this half of it.

    Height is the scarce direction here, not width: the editor has to go down to
    the sidebar's own minimum height so that the rail can be tested there, and
    that leaves 140px for these rows. At `choiceButtonHeight` plus
    `controlsGap` each, three is what fits — which is what there is. A fourth
    setting therefore does not go on the end; it goes in a second **column**,
    which costs nothing, since the window may be as wide as it likes and
    `controlsWidthFor` already gives a column's width. Adding rows instead would
    push the last one out of sight at exactly the size the sidebar most needs
    testing at.

    A real Component rather than something the editor draws in its own `paint`.
    Drawing it in the editor meant deriving its bounds from the sidebar's
    current width, which is wrong twice over: the editor is not repainted while
    the sidebar animates, so the outline lagged behind and then stayed stale
    once the animation finished. A child gets laid out and repaints itself.
*/
class DemoControls final : public juce::Component
{
public:
    DemoControls()
    {
        // The panel itself is scenery and should not swallow clicks; its
        // children are the point, so they still get them.
        setInterceptsMouseClicks (false, true);

        addAndMakeVisible (themeStrip);
        addAndMakeVisible (bubbleTextStrip);
        addAndMakeVisible (edgeStrip);
    }

    ChoiceStrip& getThemeStrip()      noexcept { return themeStrip; }
    ChoiceStrip& getBubbleTextStrip() noexcept { return bubbleTextStrip; }
    ChoiceStrip& getEdgeStrip()       noexcept { return edgeStrip; }

    void resized() override
    {
        auto area = getLocalBounds().reduced (layout::controlsPadding);

        // Capped and centred. Without the cap a wide window stretches a row of
        // four buttons across the entire editor, which stops reading as a
        // control.
        const auto widest = juce::jmax (themeStrip.getChoiceCount(),
                                        edgeStrip.getChoiceCount(),
                                        bubbleTextStrip.getChoiceCount());

        const auto capped = juce::jmin (area.getWidth(),
                                        layout::controlsWidthFor (widest, layout::choiceButtonMaxWidth));

        juce::Grid grid;
        using Track = juce::Grid::TrackInfo;

        grid.templateColumns = { Track (juce::Grid::Fr (1)) };

        // Settings anchored to the top with one flexible track below them
        // taking up the rest. Growing the window then only grows the empty part
        // of the placeholder, which is the part that is standing in for
        // something else anyway.
        //
        // One row per setting, all the same height, so the label column and
        // every button edge line up because they are the same tracks.
        //
        // Theme and bubble text are adjacent because both are about colour and
        // are read against each other — the bug the second one works around
        // only appears in one of the themes. This is the display order only;
        // the parameters keep their own order in the processor, since their
        // indices are what a host automates.
        const Track row { juce::Grid::Px (layout::choiceButtonHeight) };

        grid.templateRows = { row, row, row, Track (juce::Grid::Fr (1)) };
        grid.rowGap = juce::Grid::Px (layout::controlsGap);

        grid.items = { juce::GridItem (themeStrip),
                       juce::GridItem (bubbleTextStrip),
                       juce::GridItem (edgeStrip),
                       juce::GridItem() };

        grid.performLayout (area.withSizeKeepingCentre (capped, area.getHeight()));
    }

private:
    // Declared in the order they appear; `resized` is what actually decides it.
    // The widget is the module's; only the label column is the demo's own, and
    // it is wider than a page's because there is room here and the names are
    // longer.
    ChoiceStrip themeStrip      { "Theme",        settings::themeNames,      layout::choiceLabelWidth };
    ChoiceStrip bubbleTextStrip { "Bubble text",  settings::bubbleTextNames, layout::choiceLabelWidth };
    ChoiceStrip edgeStrip       { "Sidebar edge", settings::edgeNames,       layout::choiceLabelWidth };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DemoControls)
};

} // namespace microtonos::sidebar::demo
