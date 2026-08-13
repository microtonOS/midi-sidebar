#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "LevelMeter.h"
#include "SidebarLookAndFeel.h"
#include "SidebarPanel.h"
#include "VolumeStrip.h"
#include "widgets/ResizeHandle.h"

namespace microtonos::sidebar
{

//==============================================================================
/** A sidebar for managing presets, continuous controllers and microtunings.

    The rail is always visible. Activating one of the first three buttons
    expands a panel beside it; activating none collapses it again. See
    docs/sidebar.md for the specification this implements.

    The sidebar decides its own width — `getPreferredWidth()` — and tells its
    owner when that changes, so the owner controls how (and whether) the change
    is animated. That keeps the module independent of the host's layout.
*/
class Sidebar final : public juce::Component
{
public:
    //==========================================================================
    enum ColourIds
    {
        backgroundColourId      = 0x1a10000,
        iconColourId            = 0x1a10001,
        iconOverColourId        = 0x1a10002,
        iconActiveColourId      = 0x1a10003,
        activeIndicatorColourId = 0x1a10004,
        separatorColourId       = 0x1a10005
    };

    /** Which side of its parent the sidebar lives on. The developer's choice;
        it flips which edge the panel opens towards. */
    enum class Edge { left, right };

    /** The three expandable pages, plus the collapsed state. Exactly one of
        these is true at a time. */

    enum class Page { none, presets, controllers, tuning, channels };

    //==========================================================================
    Sidebar();
    ~Sidebar() override;

    void setEdge (Edge newEdge);
    Edge getEdge() const noexcept { return edge; }

    /** Time the owner should take to animate a width change. The sidebar does
        not animate itself; this is here so the developer sets the speed in one
        place and the owner reads it. */
    void setAnimationMilliseconds (int ms) noexcept { animationMs = juce::jmax (0, ms); }
    int getAnimationMilliseconds() const noexcept   { return animationMs; }

    void setActivePage (Page newPage);
    Page getActivePage() const noexcept { return activePage; }

    /** Rail width when collapsed, rail + panel when a page is open. */
    int getPreferredWidth() const noexcept;

    /** How wide the page area is, not counting the rail.

        The end-user drags the sidebar's inner edge to change this, and the owner
        may also set it — to restore a saved one, say. It is clamped to
        `metrics::panelMinWidth` at the bottom and, at the top, to what leaves the
        rail's own width of the window still showing: a sidebar that could cover
        the thing it is sitting on has no way back for someone who drags too far.

        Setting it fires `onPreferredWidthChanged` unless the clamped value is
        the one already in force. */
    void setPanelWidth (int newWidth);
    int getPanelWidth() const noexcept { return panelWidth; }

    /** Smallest height at which the rail can be drawn. Feed this into the
        editor's resize limits rather than choosing a number by eye. */
    static constexpr int getMinimumHeight() noexcept { return metrics::railMinHeight; }

    /** Width of the rail alone, i.e. the sidebar's width when collapsed. */
    static constexpr int getRailWidth() noexcept { return metrics::railWidth; }

    /** Called when getPreferredWidth() has changed and the owner should
        re-lay out.

        The argument says whether the change is worth animating. Opening a page
        is: the panel sliding out is what makes it read as one object rather than
        as something appearing. A drag is not — the width is already following
        the mouse, and animating towards a target that moves every few
        milliseconds lags behind the pointer and never arrives. */
    std::function<void (bool animate)> onPreferredWidthChanged;

    /** Called when the user opens or closes a page, so the owner can mirror the
        state somewhere it survives the editor being destroyed. */
    std::function<void (Page)> onPageChanged;

    /** Called when the all-sound-off button is pressed (CC120). */
    std::function<void()> onPanic;

    /** Levels for the stereo meter, 0 to 1 per channel. The owner pushes these
        in from a Timer on the message thread; never call it from the audio
        thread. */
    void setLevel (float left, float right);

    /** The tuning page, so the owner can push values into it and take its
        callbacks. Handed out rather than mirrored through the sidebar: the page
        has a wide interface, and forwarding all of it would be a second copy of
        the same API to keep in step. */
    TuningPage&      getTuningPage()      noexcept { return panel.getTuningPage(); }
    ChannelsPage&    getChannelsPage()    noexcept { return panel.getChannelsPage(); }
    ControllersPage& getControllersPage() noexcept { return panel.getControllersPage(); }
    PresetsPage&     getPresetsPage()     noexcept { return panel.getPresetsPage(); }

    //==========================================================================
    void paint (juce::Graphics& g) override;
    void resized() override;
    void lookAndFeelChanged() override;
    void parentHierarchyChanged() override;

private:
    //==========================================================================
    /** How much vertical room there is, which decides the rail's layout.
        The thresholds are in metrics:: and are derived from the content. */
    enum class Density
    {
        compact,   ///< Volume is an icon button; the strip does not fit.
        regular    ///< Volume is the slider-and-meter strip.
    };

    static Density densityFor (int height) noexcept;

    /** True when the rail occupies the left-hand strip of the sidebar's own
        bounds. Computed rather than stored, so paint() and resized() cannot
        disagree about where the rail is. */
    bool isRailOnLeft() const noexcept;

    /** The widest the panel may be given the window it is lying on. */
    int largestPanelWidth() const noexcept;

    void layOutRail (juce::Rectangle<int> area, Density density);
    void pageButtonClicked (Page page);
    void showVolumeCallOut();
    void refreshIcons();
    void refreshToggleStates();

    juce::DrawableButton presetsButton  { "Presets",     juce::DrawableButton::ImageFitted };
    juce::DrawableButton controllersButton { "Controllers", juce::DrawableButton::ImageFitted };
    juce::DrawableButton tuningButton   { "Tuning",      juce::DrawableButton::ImageFitted };
    juce::DrawableButton channelsButton { "Channels",    juce::DrawableButton::ImageFitted };
    juce::DrawableButton volumeButton   { "Volume",      juce::DrawableButton::ImageFitted };
    juce::DrawableButton panicButton    { "All sound off", juce::DrawableButton::ImageFitted };

    VolumeStrip volumeStrip;
    SidebarPanel panel;
    ResizeHandle widthHandle;

    Edge edge = Edge::left;
    Page activePage = Page::none;
    int animationMs = metrics::defaultAnimationMs;

    int panelWidth = metrics::panelMinWidth;

    /** The panel width when the current drag began, so every delta is measured
        from one fixed number. Accumulating deltas instead would drift by
        whatever the clamp swallowed each time the pointer went past an end. */
    int panelWidthAtDragStart = metrics::panelMinWidth;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Sidebar)
};

} // namespace microtonos::sidebar
