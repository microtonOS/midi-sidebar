#include "DemoEditor.h"

namespace microtonos::sidebar::demo
{

namespace layout
{
    /** The content area beside the sidebar. Stands in for the host plugin's own
        UI, so its only real constraint is being wide enough to look like a
        plugin rather than a strip. */
    inline constexpr int minContentWidth     = 320;
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
}

//==============================================================================
DemoEditor::DemoEditor (DemoProcessor& p)
    : juce::AudioProcessorEditor (&p), processor (p)
{
    setLookAndFeel (&lookAndFeel);

    // Placeholder first so the sidebar is added later and therefore draws on
    // top of it, which is what an overlaying sidebar would need.
    addAndMakeVisible (placeholder);
    addAndMakeVisible (sidebar);

    sidebar.onPreferredWidthChanged = [this] { layOutSidebar (true); };
    sidebar.onPanic = [] { /* CC120 goes here once the processor sends MIDI. */ };

    // The volume slider is not yet attached to the APVTS parameter: that is a
    // later step, and doing it now would need the sidebar to hand out its
    // internals. The parameter already exists on the processor.

    // The open page is mirrored to a parameter in both directions, so the
    // choice survives the editor being destroyed and can be driven by a host.
    if (auto* pageParam = processor.apvts.getParameter ("page"))
    {
        pageAttachment = std::make_unique<juce::ParameterAttachment> (
            *pageParam,
            [this] (float value) { sidebar.setActivePage (static_cast<Sidebar::Page> (juce::roundToInt (value))); },
            nullptr);

        sidebar.onPageChanged = [this] (Sidebar::Page page)
        {
            if (pageAttachment != nullptr)
                pageAttachment->setValueAsCompleteGesture ((float) static_cast<int> (page));
        };

        pageAttachment->sendInitialUpdate();
    }

    // Minimum size is derived, not chosen: the sidebar reports the height below
    // which its rail cannot be drawn, and the content area supplies the width.
    constrainer.setSizeLimits (layout::minContentWidth + Sidebar::getRailWidth(),
                               Sidebar::getMinimumHeight(),
                               layout::maxWidth,
                               layout::maxHeight);

    setConstrainer (&constrainer);
    setResizable (true, true);

    // Restore whatever size the host last saved, falling back to the default.
    const auto savedW = processor.editorWidth .load (std::memory_order_relaxed);
    const auto savedH = processor.editorHeight.load (std::memory_order_relaxed);

    setSize (savedW > 0 ? savedW : layout::defaultContentWidth + Sidebar::getRailWidth(),
             savedH > 0 ? savedH : layout::defaultHeight);

    startTimerHz (layout::meterHz);
}

DemoEditor::~DemoEditor()
{
    setLookAndFeel (nullptr);
}

//==============================================================================
void DemoEditor::timerCallback()
{
    sidebar.setLevel (processor.outputLevelLeft .load (std::memory_order_relaxed),
                      processor.outputLevelRight.load (std::memory_order_relaxed));
}

void DemoEditor::paint (juce::Graphics& g)
{
    g.fillAll (findColour (juce::ResizableWindow::backgroundColourId));
}

void DemoEditor::resized()
{
    processor.editorWidth .store (getWidth(),  std::memory_order_relaxed);
    processor.editorHeight.store (getHeight(), std::memory_order_relaxed);

    layOutSidebar (false);
}

void DemoEditor::layOutSidebar (bool animated)
{
    const auto onLeft = sidebar.getEdge() == Sidebar::Edge::left;

    // The panel lies OVER the host's UI rather than pushing it aside, so the
    // content area never changes size. That is the whole point for a component
    // other people drop into their own plugin: their layout does not have to
    // respond to this one. Only the rail is permanently reserved, so nothing of
    // theirs is hidden while the sidebar is collapsed.
    auto content = getLocalBounds();
    content = onLeft ? content.withTrimmedLeft (Sidebar::getRailWidth())
                     : content.withTrimmedRight (Sidebar::getRailWidth());

    placeholder.setBounds (content.reduced (layout::placeholderInset));

    auto bounds = getLocalBounds();
    const auto width = sidebar.getPreferredWidth();
    const auto target = onLeft ? bounds.removeFromLeft (width)
                               : bounds.removeFromRight (width);

    // The sidebar decides how wide it wants to be; animating that change is the
    // owner's job, which is why the speed lives on the sidebar but the
    // animation happens here.
    auto& animator = juce::Desktop::getInstance().getAnimator();

    if (animated && sidebar.getAnimationMilliseconds() > 0)
    {
        animator.animateComponent (&sidebar, target, 1.0f,
                                   sidebar.getAnimationMilliseconds(), false, 1.0, 1.0);
    }
    else
    {
        // Cancel first. A ComponentAnimator keeps driving the component towards
        // the target it was given, so a plain setBounds during an animation is
        // silently undone on the next animation frame — which is what happens
        // when the window is resized while the sidebar is opening: the sidebar
        // snaps back to the size the window used to be, and whatever no longer
        // fits disappears.
        animator.cancelAnimation (&sidebar, false);
        sidebar.setBounds (target);
    }
}

} // namespace microtonos::sidebar::demo
