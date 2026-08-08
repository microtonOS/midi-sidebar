#pragma once

#include <midi_sidebar/midi_sidebar.h>

namespace microtonos::sidebar::demo
{

//==============================================================================
/** The demo's own look and feel: every measurement and every shade it uses.

    The same rule the module follows in `metrics`: if a number matters it is
    named here, and nothing in a `resized()` contains a literal other than 0, 1
    or 2. It is a header rather than a block inside the editor because the
    editor and the controls panel share several of these, and a constant copied
    into two `resized()` bodies drifts apart.

    Kept separate from the module's `metrics` — these are the demo's own
    numbers, and a plugin embedding the sidebar has no use for them.
*/
namespace layout
{
    /** The content area beside the sidebar. Stands in for the host plugin's own
        UI, so its only constraint is holding the developer controls; see
        `contentWidthFor` below, which derives the minimum from them. */
    inline constexpr int defaultContentWidth = 460;

    /** Default height sits well above the breakpoint so the sidebar opens with
        the volume strip showing and a comfortable gap above it. */
    inline constexpr int defaultHeight = 420;
    inline constexpr int maxWidth      = 1600;
    inline constexpr int maxHeight     = 1200;

    /** Meter refresh. Fast enough to look continuous, slow enough to cost
        nothing. */
    inline constexpr int meterHz = 24;

    /** Inset of the "host plugin content" placeholder from the area left over
        beside the sidebar. */
    inline constexpr int placeholderInset = 8;

    //==========================================================================
    //  The developer controls inside that placeholder.

    /** Padding between the placeholder's outline and what it holds. */
    inline constexpr int controlsPadding = 12;

    /** The caption naming the area, above the controls. */
    inline constexpr int captionHeight = 22;

    /** Vertical space between the caption and the first group, and between the
        groups themselves. */
    inline constexpr int controlsGap = 10;

    /** One row of choice buttons. Deliberately taller than the module's rail
        buttons: these are labelled with words rather than icons. */
    inline constexpr int choiceButtonHeight = 26;

    /** Each setting is a label on the left and its choices on the right, so the
        labels share one column and line up because they are the same track —
        not because two expressions happen to agree.

        No `GroupComponent`: every one of these settings is a single control, and
        a group of one is a label with a box drawn round it. Three rows of the
        same shape also survive the smallest window, which three framed groups
        do not — the sidebar's minimum height leaves 172px here, and the framed
        version needed 214. */
    /** Wider than the module's default, which is sized for a 248px page: this
        panel has the room and its settings have longer names. The gap between
        the label and the buttons is the widget's own, so it is taken from
        `metrics` rather than defined a second time here. */
    inline constexpr int choiceLabelWidth = 90;

    /** What one choice button is allowed to shrink to and grow to. The minimum
        is enough for the longest choice, "Midnight", at the button font. The
        maximum exists because these rows stop looking like controls once they
        are half a metre wide: past it the block stays put and the placeholder
        around it gets emptier, which is the right way round for an area that is
        standing in for someone else's UI. */
    inline constexpr int choiceButtonMinWidth = 64;
    inline constexpr int choiceButtonMaxWidth = 96;

    /** Width of a row of `choiceCount` choices with buttons of `buttonWidth`. */
    constexpr int controlsWidthFor (int choiceCount, int buttonWidth) noexcept
    {
        return choiceLabelWidth + metrics::choiceLabelGap + buttonWidth * choiceCount;
    }

    /** Narrowest the content area can be and still show a row of `choiceCount`
        choices without eliding their labels. Passed the widest row the controls
        actually hold, so adding a fifth theme moves the minimum by itself.

        Unlike the minimum *height*, which is the sidebar's own and tests
        something, this number tests nothing about the module — it is only the
        demo's furniture needing room. */
    constexpr int contentWidthFor (int choiceCount) noexcept
    {
        return placeholderInset * 2
             + controlsPadding * 2
             + controlsWidthFor (choiceCount, choiceButtonMinWidth);
    }
}

//==============================================================================
/** How far the demo's own furniture is held back from the text colour.

    No colours of its own, deliberately: everything here is the current theme's
    text colour at some alpha, so all four themes work without a second palette
    to keep in step — and so nothing in this area can out-shout the sidebar,
    which is the thing being demonstrated.
*/
namespace shades
{
    /** The caption and the outline marking the host's area. Legible, but read
        as scenery rather than as content — unlike the settings' own labels,
        which are part of a control and keep the theme's full text colour. */
    inline constexpr float scenery = 0.5f;
}

} // namespace microtonos::sidebar::demo
