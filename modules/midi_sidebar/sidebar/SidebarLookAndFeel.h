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
    //  Pages. One set of measurements for all three, so they cannot drift into
    //  looking like three different plugins.

    /** One row of a page: a read-out, an editable field, or a row of buttons.
        Everything on a page is one of these tall, which is what makes rows in
        different sections line up. */
    inline constexpr int pageRowHeight = 22;

    /** Between rows inside one section. */
    inline constexpr int pageRowGap = 5;

    /** The band a `GroupComponent` needs above its contents.

        `LookAndFeel_V2::drawGroupComponentOutline`, which V4 inherits, draws the
        title at a fixed 15px and runs the frame's top line through the middle
        of it — so the first row inside a group has to start below that or the
        text lands on top of it. */
    inline constexpr int pageGroupTitleHeight = 18;

    /** Inset of a group's contents from its frame: at the sides, and below the
        last row. The same figure both ways, so a group looks equally padded on
        every side that is not its title. */
    inline constexpr int pageGroupPadding = 8;

    /** Every page is laid out on this many equal columns, and each thing on it
        spans a whole number of them.

        Six because that is the finest division the sketches use: a row is
        halved, or split into thirds, or into a label and its field. One shared
        template is what makes the halves actually line up down the page —
        `program`'s field, the period, the scheme box and the scale button all
        begin at the same column because they are told to, not because their
        widths happen to add up.

        This is also why the page is one `Grid` and not one per row. A grid per
        row can only align things within that row; anything that has to line up
        across rows is then arithmetic, and arithmetic that agrees today is a
        coincidence, not a constraint. */
    inline constexpr int pageColumns = 6;

    /** The six content columns plus a gutter at each end.

        The gutters are what let a group's frame be drawn *wider* than the
        widgets inside it while everything stays in one grid: the frame spans
        every track, the widgets span only the six between the gutters, and each
        gutter is exactly `pageGroupPadding`. The alternative — laying the
        frames out separately from the rows they enclose — would mean computing
        the same row positions twice. */
    inline constexpr int pageColumnsWithGutters = pageColumns + 2;

    /** How many of those columns a field's name takes, leaving the rest of the
        row for the field itself. Every labelled row on a page uses it, which is
        what puts `program`, `updated`, `scale`, `map` and `update` in one
        column and starts all five of their values in another. */
    inline constexpr int pageLabelColumns = 2;

    /** Between two adjacent columns. */
    inline constexpr int pageColumnGap = 6;

    /** Where a page's content begins, measured from the page's left edge: past
        the gutter and the gap that follows it.

        This is what a panel title is indented by, so that it starts directly
        above the fields below it. An earlier version derived the indent from
        `LookAndFeel_V2::drawGroupComponentOutline` instead — frame inset plus
        corner radius plus text margin — to sit above a *group's* title. It
        landed 2px from this, which is no better to the eye, and it made a
        measurement of ours depend on three private numbers inside JUCE's
        drawing code that nothing documents and any release may change. */
    inline constexpr int pageContentIndent = pageGroupPadding + pageColumnGap;

    /** Inset of a read-out's text from its box, and of a call-out's contents
        from its edges. */
    inline constexpr int readOutPadding = 5;

    /** The period chooser's read-out, left of its inc/dec buttons. Wide enough
        for the longest value it can show — four digits, two decimals and the
        unit, as in "1200.00 c" — since the buttons take whatever is left. */
    inline constexpr int periodTextBoxWidth = 68;

    //==========================================================================
    //  Tables. Both of the controllers page's tables use these, so the monitor
    //  and the editing grid read as the same kind of object.

    /** One row. Taller than `pageRowHeight` because a row holds a control *and*
        has to be told apart from the row under it, which a page's rows do with
        the gap between them and a table cannot. */
    inline constexpr int tableRowHeight = 24;

    /** The editing table's column headers. The monitor has none — see
        docs/controllers.md. */
    inline constexpr int tableHeaderHeight = 18;

    /** Inset of a cell's widget from its row, so controls in adjacent rows do
        not touch. */
    inline constexpr int tableCellInset = 2;

    /** The frozen parameter column, and the gap between it and the columns that
        scroll under their own header. Wide enough for a parameter name in a
        ComboBox, since that is what the column holds. */
    inline constexpr int tableFrozenColumnWidth = 68;

    /** Editing columns, in the order docs/controllers.md draws them. They add
        up to more than the panel is wide — that is the point of the frozen
        column, and what the horizontal scrolling is for. */
    inline constexpr int tableCcWidth    = 44;
    inline constexpr int tableLimitWidth = 52;

    /** What a table column needs beyond the width of its widest text.

        Covers the indents `LookAndFeel_V2::drawButtonText` puts either side of
        a button's label — about six pixels each at this size — plus the cell's
        own inset. A named number rather than a reconstruction of JUCE's
        arithmetic, which depends on the font height and the corner radius and
        is not ours to rely on; if a label ever truncates, this is the number to
        raise. The columns holding buttons derive their widths from it, so
        `channel` and `mode` are exactly as wide as their longest entry needs
        and no wider. */
    inline constexpr int tableTextPadding = 16;

    /** Monitor columns. Unlike the editing table these are sized to fit, since
        the monitor does not scroll in either direction. */
    inline constexpr int monitorTypeWidth  = 74;
    inline constexpr int monitorChanWidth  = 48;

    /** The same width as the channel's, which is what puts the note or CC
        midway between the channel and the value.

        Every column is left-aligned, so the three numbers begin at their
        columns' left edges: equal widths here and for the channel mean equal
        gaps, and the middle one lands in the middle. Centring the text inside
        its own column does *not* — the neighbours' text sits at the far left of
        their columns while a centred one sits half a column further in, which
        pushed it visibly to the right. */
    inline constexpr int monitorNoteWidth  = monitorChanWidth;
    inline constexpr int monitorValueWidth = 78;

    /** Rows the editing table must be able to show before the page has to give
        up any more height. The table is the page's flexible track, so this is
        what sets the page's minimum. */
    inline constexpr int tableMinimumRows = 3;

    /** The channel selector's grid: a square-ish button per MIDI channel, and a
        column beside it wide enough for "deselect all". */
    inline constexpr int channelButton    = 26;
    inline constexpr int channelSideWidth = 78;

    /** Corner radius shared by read-outs and the boxes drawn around them. */
    inline constexpr float readOutCorner = 3.0f;

    //==========================================================================
    //  ChoiceStrip: a label and a row of buttons of which exactly one is on.
    //  Lives here rather than with the widget because a page laying one out has
    //  to reserve the same width the strip will use for its label.

    /** Width of the strip's label column, and the space between it and the
        first button. The default suits a page; the demo's own panel passes a
        wider one, since it has room and longer names. */
    inline constexpr int choiceLabelWidth = 54;
    inline constexpr int choiceLabelGap   = 8;

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
/** How far something is held back from full strength.

    The same kind of number as `metrics`, and gathered for the same reason: an
    alpha decides how loud a thing is, and having five of them scattered across
    five files is how two things that should match stop matching. Alphas rather
    than colours because every one of these is the *theme's* colour, quietened —
    which is what lets all four schemes work without a second palette.

    The demo keeps its own `shades` for its own scenery; that one is about the
    area standing in for a host, not about the sidebar.
*/
namespace shades
{
    /** A rail icon at rest. Full strength is for the one under the mouse. */
    inline constexpr float icon = 0.75f;

    /** Hairlines: the seam against the host's content, a section's frame, the
        rim of a read-out. All three are the same line in different places. */
    inline constexpr float hairline = 0.15f;

    /** A read-out showing what it would say rather than what it does say. */
    inline constexpr float placeholder = 0.5f;

    /** A selected table row, tinted behind the widgets sitting on it: enough to
        find, not enough to fight them. */
    inline constexpr float selectedRow = 0.25f;

    /** Anything the end-user cannot edit from the GUI — see docs/general.md.
        Dimming the whole of it says so without a word of explanation. */
    inline constexpr float readOnly = 0.6f;
}

//==============================================================================
/** Colours shared by the parts of a page that are not a widget of their own.

    A free namespace rather than `ColourIds` on a component, which is where JUCE
    normally puts them, because the things that need these — the pages — are
    included *before* the panel that would be their natural owner, and a widget
    reaching into another widget's header for an id is how the include graph
    became a cycle the last time.
*/
namespace pageColours
{
    enum ColourIds
    {
        sectionTitleColourId   = 0x1a10500,  ///< A section's name.
        sectionOutlineColourId = 0x1a10501   ///< The frame drawn around it.
    };
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

    /** Switches the whole module to another JUCE colour scheme.

        `LookAndFeel_V4::setColourScheme` on its own is not enough: it re-derives
        JUCE's own ColourIds and leaves this module's — which were derived from
        the *previous* scheme — behind, so the sidebar keeps its old colours
        while everything around it changes. This does both halves.

        The caller still has to tell the components about it. Colours resolved
        during `paint` follow immediately, but anything that caches a colour —
        the rail's icons bake theirs into a Drawable — only rebuilds on a
        look-and-feel change, so call `sendLookAndFeelChange()` on the component
        that owns this LookAndFeel afterwards. */
    void setScheme (const ColourScheme& newScheme);

    /** Teaches a LookAndFeel every ColourId this module's widgets ask for.

        The constructor calls this on itself. Call it yourself if you use your
        own LookAndFeel instead of this one: a widget whose ColourIds are
        missing gets `Colours::black` from `findColour`, plus an assertion, and
        the result is a control that renders as a black rectangle. */
    static void registerColours (juce::LookAndFeel& target, const ColourScheme& scheme);

    /** A TextButton does not shrink its label to fit, so short labels on small
        buttons are clipped to an ellipsis. Scale the font to the button. */
    juce::Font getTextButtonFont (juce::TextButton&, int buttonHeight) override;

    /** The same rule, reachable without a button to ask about. Anything sizing
        a column to its contents needs the font those contents will actually be
        drawn in — which on a short button is not `bodyFontHeight`. */
    static juce::Font buttonFont (int buttonHeight);

    /** JUCE insets a vertical slider's drawing area by `getSliderThumbRadius`
        at each end so the thumb has somewhere to overhang. That happens before
        drawLinearSlider is called, so a fader laid out beside a meter of the
        same height comes out visibly shorter — 16 px shorter at the sizes used
        here — and no amount of custom drawing can recover it. Give the fader
        its full bounds and clamp the thumb instead.

        Applies to `LinearVertical` only. Everything else keeps JUCE's layout,
        including its text box, which this would otherwise discard. */
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
