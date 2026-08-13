#include "Sidebar.h"
#include "SidebarIcons.h"

namespace microtonos::sidebar
{

using Track = juce::Grid::TrackInfo;
using juce::Grid;
using juce::GridItem;

//==============================================================================
Sidebar::Sidebar()
{
    // A component name is what accessibility reads out and what anything
    // walking the tree addresses — a screenshot tool cropping to this component,
    // a UI test finding it. Unnamed is the default and costs nothing visible,
    // which is exactly why it is easy to leave that way.
    setName ("Sidebar");

    for (auto* b : { &presetsButton, &controllersButton, &tuningButton, &channelsButton })
    {
        b->setClickingTogglesState (true);
        addAndMakeVisible (*b);
    }

    // The three page buttons behave like a radio group with one extra state:
    // clicking the active one turns it off and collapses the sidebar. That is
    // why Button::setRadioGroupId is NOT used here — JUCE's radio groups
    // deliberately refuse to let you switch the active button off, which is
    // exactly the behaviour docs/sidebar.md asks for ("the menu is collapsed if
    // and only if none"). Routing every click through setActivePage keeps the
    // page state in one place.
    presetsButton    .onClick = [this] { pageButtonClicked (Page::presets); };
    controllersButton.onClick = [this] { pageButtonClicked (Page::controllers); };
    tuningButton     .onClick = [this] { pageButtonClicked (Page::tuning); };
    channelsButton   .onClick = [this] { pageButtonClicked (Page::channels); };

    volumeButton.onClick = [this] { showVolumeCallOut(); };
    panicButton .onClick = [this] { if (onPanic != nullptr) onPanic(); };

    addAndMakeVisible (volumeButton);
    addAndMakeVisible (panicButton);
    addAndMakeVisible (volumeStrip);
    addChildComponent (panel);

    // Every delta is measured from the width the drag started at, not from the
    // last one reported, so a pointer that runs past an end and comes back finds
    // the panel where it left it rather than however far the clamp had eaten.
    widthHandle.onDragStart = [this] { panelWidthAtDragStart = panelWidth; };

    widthHandle.onDrag = [this] (int deltaX)
    {
        // The handle is on the inner edge, so which way widens depends on which
        // side of the window the sidebar is on: dragging right widens a
        // left-hand sidebar and narrows a right-hand one.
        const auto widening = edge == Edge::left ? deltaX : -deltaX;
        setPanelWidth (panelWidthAtDragStart + widening);
    };

    // Added last so it is on top of the rail buttons it overlaps, and hidden
    // until there is a panel to resize.
    addChildComponent (widthHandle);

    // DrawableButton fits the image to the whole button, so the icon size is an
    // inset rather than a size. Without this, railIcon would have no effect and
    // every icon would be as large as its hit area.
    for (auto* b : { &presetsButton, &controllersButton, &tuningButton, &channelsButton,
                     &volumeButton, &panicButton })
        b->setEdgeIndent ((metrics::railButton - metrics::railIcon) / 2);

    refreshToggleStates();
}

Sidebar::~Sidebar() = default;

//==============================================================================
void Sidebar::setEdge (Edge newEdge)
{
    if (edge == newEdge)
        return;

    edge = newEdge;
    resized();
}

void Sidebar::setActivePage (Page newPage)
{
    if (activePage == newPage)
        return;

    activePage = newPage;

    panel.setVisible (activePage != Page::none);
    panel.showTuningPage (activePage == Page::tuning);
    panel.showControllersPage (activePage == Page::controllers);
    panel.showPresetsPage (activePage == Page::presets);
    panel.showChannelsPage (activePage == Page::channels);
    panel.setTitle ([this]() -> juce::String
    {
        switch (activePage)
        {
            case Page::presets:     return "Presets";
            case Page::controllers: return "Controllers";
            case Page::tuning:      return "Tuning";
            case Page::channels:    return "Channels";
            case Page::none:        break;
        }

        return {};
    }());

    refreshToggleStates();
    resized();

    if (onPageChanged != nullptr)
        onPageChanged (activePage);

    // Animated: the panel sliding out is what makes it read as one object
    // rather than as something that appeared.
    if (onPreferredWidthChanged != nullptr)
        onPreferredWidthChanged (true);
}

void Sidebar::pageButtonClicked (Page page)
{
    setActivePage (activePage == page ? Page::none : page);
}

int Sidebar::largestPanelWidth() const noexcept
{
    // Not a constant, which is why it is not in metrics:: beside the minimum: it
    // depends on the window the sidebar is lying on. Leaving the rail's own
    // width of that window uncovered means there is always something visible
    // past the sidebar, and so always a way back for someone who drags too far.
    //
    // With no parent yet — during construction, or in a test — there is nothing
    // to measure, so nothing is imposed.
    if (auto* parent = getParentComponent())
        return juce::jmax (metrics::panelMinWidth, parent->getWidth() - metrics::railWidth * 2);

    return std::numeric_limits<int>::max();
}

void Sidebar::setPanelWidth (int newWidth)
{
    const auto clamped = juce::jlimit (metrics::panelMinWidth, largestPanelWidth(), newWidth);

    if (clamped == panelWidth)
        return;

    panelWidth = clamped;

    // Not animated: the width is already following the pointer, and a target
    // that moves every few milliseconds is one the animator never reaches.
    if (activePage != Page::none && onPreferredWidthChanged != nullptr)
        onPreferredWidthChanged (false);
}

void Sidebar::setLevel (float left, float right)
{
    volumeStrip.getMeter().setLevel (left, right);
}

int Sidebar::getPreferredWidth() const noexcept
{
    if (activePage == Page::none)
        return metrics::railWidth;

    // Clamped here as well as on the way in, so a window that shrinks under a
    // sidebar already dragged wide narrows it to fit rather than letting it
    // cover the whole editor. The stored width is left alone, so growing the
    // window back gives back the width that was chosen.
    return metrics::railWidth + juce::jmin (panelWidth, largestPanelWidth());
}

//==============================================================================
void Sidebar::refreshToggleStates()
{
    presetsButton    .setToggleState (activePage == Page::presets,     juce::dontSendNotification);
    controllersButton.setToggleState (activePage == Page::controllers, juce::dontSendNotification);
    tuningButton     .setToggleState (activePage == Page::tuning,      juce::dontSendNotification);
    channelsButton   .setToggleState (activePage == Page::channels,    juce::dontSendNotification);
}

void Sidebar::refreshIcons()
{
    // Bail unless the LookAndFeel we would resolve against actually knows our
    // ColourIds. Two moments where it does not: before this component has been
    // added to a styled parent, and again during teardown, when the owner's
    // setLookAndFeel(nullptr) sends a look-and-feel change to its children.
    // Without this guard the icons are rebuilt in Colours::black on the way
    // out, and every colour lookup asserts.
    if (! getLookAndFeel().isColourSpecified (iconColourId))
        return;

    const auto normal = findColour (iconColourId);
    const auto over   = findColour (iconOverColourId);
    const auto active = findColour (iconActiveColourId);

    const auto setUp = [&] (juce::DrawableButton& button, const char* svg)
    {
        button.setImages (icons::load (svg, normal).get(),
                          icons::load (svg, over).get(),
                          icons::load (svg, active).get(),
                          nullptr,
                          icons::load (svg, active).get());
    };

    setUp (presetsButton,     icons::presets);
    setUp (controllersButton, icons::controllers);
    setUp (tuningButton,      icons::tuning);
    setUp (channelsButton,    icons::channels);
    setUp (volumeButton,      icons::volume);
    setUp (panicButton,       icons::panic);
}

void Sidebar::lookAndFeelChanged()
{
    refreshIcons();
}

void Sidebar::parentHierarchyChanged()
{
    // Both volume strips put their value bubble in the editor rather than in
    // themselves or in the callout, either of which is too small to hold it.
    volumeStrip.setPopupParent (findPopupHost (*this));

    // The icons bake the current colours into their Drawables, so they have to
    // be rebuilt whenever those colours could have changed. Doing it in the
    // constructor is too early: there is no LookAndFeel attached yet, so
    // findColour falls back to the default one, fails to find these IDs and
    // returns black. Being added to a parent is the point at which the real
    // LookAndFeel becomes reachable.
    refreshIcons();
}

void Sidebar::showVolumeCallOut()
{
    // Only reachable in compact density, where the strip has no room in the
    // rail. Elsewhere the button is hidden and the strip is laid out inline.
    auto* parent = findPopupHost (*this);

    if (parent == nullptr || parent == this)
        return;

    auto content = std::make_unique<VolumeStrip>();
    content->setSize (metrics::railButton, metrics::volumeStripHeight);

    // The callout is barely bigger than the strip inside it, so its own bubble
    // would be clipped exactly as the strip's was. Point it at the editor.
    content->setPopupParent (parent);

    // Launch INSIDE the editor rather than on the desktop. A desktop callout
    // has no parent component, so getLookAndFeel() finds nothing and falls back
    // to the default one: the fader reverts to JUCE's stock drawing and comes
    // out a different height, and none of this module's ColourIds resolve. It
    // also puts the callout in its own window, which leaves the fader's value
    // bubble — which lives in the editor — stranded behind it, and hides the
    // whole thing from createComponentSnapshot.
    //
    // With a parent, areaToPointTo is relative to that parent rather than the
    // screen.
    juce::CallOutBox::launchAsynchronously (
        std::move (content),
        parent->getLocalArea (&volumeButton, volumeButton.getLocalBounds()),
        parent);
}

//==============================================================================
Sidebar::Density Sidebar::densityFor (int height) noexcept
{
    return height >= metrics::regularBreakpoint ? Density::regular : Density::compact;
}

bool Sidebar::isRailOnLeft() const noexcept
{
    // The rail rides the panel's inner boundary, so it always sits on the side
    // facing the content: the panel takes the window edge and the rail is
    // pushed to the opposite side of the sidebar's own bounds. Collapsed, the
    // sidebar is only as wide as the rail and this makes no difference.
    return edge == Edge::right;
}

void Sidebar::paint (juce::Graphics& g)
{
    // Rail and panel share one surface with no divider between them: the
    // sidebar should read as a single object lying on the content, and a line
    // down the middle of it splits it into two strips instead.
    g.fillAll (findColour (backgroundColourId));

    // A hairline where the sidebar meets the content, because the two
    // backgrounds are only a tone apart.
    const auto bounds = getLocalBounds();

    g.setColour (findColour (separatorColourId));
    g.fillRect (edge == Edge::left ? bounds.withLeft (bounds.getRight() - 1)
                                   : bounds.withWidth (1));
}

void Sidebar::resized()
{
    auto bounds = getLocalBounds();

    const auto railArea = isRailOnLeft() ? bounds.removeFromLeft (metrics::railWidth)
                                         : bounds.removeFromRight (metrics::railWidth);

    panel.setBounds (bounds);
    layOutRail (railArea, densityFor (getHeight()));

    // The grab strip straddles the inner edge — the side facing the content,
    // which is the boundary being moved and where the hairline is drawn. It
    // overlaps the rail's outermost pixels; those are padding, so nothing
    // clickable is covered.
    const auto edgeStrip = edge == Edge::left
                         ? getLocalBounds().removeFromRight (metrics::resizeHandleWidth)
                         : getLocalBounds().removeFromLeft (metrics::resizeHandleWidth);

    widthHandle.setBounds (edgeStrip);

    // Nothing to resize with no panel open, and a resize cursor over an edge
    // that will not move is a promise the sidebar does not keep.
    widthHandle.setVisible (activePage != Page::none);
}

void Sidebar::layOutRail (juce::Rectangle<int> area, Density density)
{
    const auto compact = density == Density::compact;

    // The volume control is a button when there is no room for the strip, and
    // the strip itself when there is. Exactly one of them is ever visible.
    volumeButton.setVisible (compact);
    volumeStrip .setVisible (! compact);

    Grid grid;
    grid.templateColumns = { Track (Grid::Fr (1)) };
    grid.rowGap = Grid::Px (metrics::railGap);

    const Track button { Grid::Px (metrics::railButton) };
    const Track strip  { Grid::Px (metrics::volumeStripHeight) };
    const Track slack  { Grid::Fr (1) };

    // One shape at every height: pages at the top, volume and panic anchored to
    // the bottom, and a single flexible track between them taking up whatever
    // is left. Growing the window then only grows the gap, continuously, rather
    // than moving controls from one end to the other at a threshold. Density
    // changes one track — the volume control — and nothing else.
    grid.templateRows = { button, button, button, button, slack,
                          compact ? button : strip, button };

    grid.items = { GridItem (presetsButton),
                   GridItem (controllersButton),
                   GridItem (tuningButton),
                   GridItem (channelsButton),
                   GridItem(),
                   compact ? GridItem (volumeButton) : GridItem (volumeStrip),
                   GridItem (panicButton) };

    grid.performLayout (area.reduced (metrics::railPadding));
}

} // namespace microtonos::sidebar
