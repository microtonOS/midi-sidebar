#include "DemoEditor.h"
#include "DemoSettings.h"

namespace microtonos::sidebar::demo
{

//==============================================================================
DemoEditor::DemoEditor (DemoProcessor& p)
    : juce::AudioProcessorEditor (&p), processor (p)
{
    setLookAndFeel (&lookAndFeel);

    // Controls first so the sidebar is added later and therefore draws on top
    // of them, which is what an overlaying sidebar needs.
    addAndMakeVisible (controls);
    addAndMakeVisible (sidebar);

    sidebar.onPreferredWidthChanged = [this] { layOutSidebar (true); };
    sidebar.onPanic = [] { /* CC120 goes here once the processor sends MIDI. */ };

    showSampleTuning();
    showSampleControllers();
    showSamplePresets();

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

    // The two developer settings, bound the same way. Their initial update is
    // what puts the sidebar on the saved edge and the saved theme on screen,
    // so neither is applied twice — once here and once from a default.
    themeAttachment = attachChoice ("theme", controls.getThemeStrip(),
                                    [this] (int index) { applyTheme (index); });

    edgeAttachment = attachChoice ("edge", controls.getEdgeStrip(),
                                   [this] (int index)
                                   {
                                       sidebar.setEdge (settings::edgeFor (index));

                                       // Not animated: an edge change teleports
                                       // the sidebar across the window, and
                                       // sliding it through the middle of the
                                       // host's UI reads as a bug rather than
                                       // as a transition.
                                       layOutSidebar (false);
                                   });

    bubbleTextAttachment = attachChoice ("bubbleText", controls.getBubbleTextStrip(),
                                         [this] (int index) { applyBubbleTextColour (index); });

    // Minimum size is derived, not chosen: the sidebar reports the height below
    // which its rail cannot be drawn, and the content area is as narrow as the
    // widest row of developer controls — the four themes — allows.
    constrainer.setSizeLimits (layout::contentWidthFor (settings::themeNames.size())
                                   + Sidebar::getRailWidth(),
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
std::unique_ptr<juce::ParameterAttachment> DemoEditor::attachChoice (const juce::String& parameterID,
                                                                    ChoiceStrip& strip,
                                                                    std::function<void (int)> apply)
{
    auto* parameter = processor.apvts.getParameter (parameterID);

    if (parameter == nullptr)
    {
        jassertfalse;   // A parameter was renamed in the processor but not here.
        return {};
    }

    // ParameterAttachment hands a choice parameter's value over already
    // denormalised, so this is the index rather than a 0..1 fraction.
    auto attachment = std::make_unique<juce::ParameterAttachment> (
        *parameter,
        [&strip, applyChoice = std::move (apply)] (float value)
        {
            const auto index = juce::roundToInt (value);

            strip.setSelectedIndex (index);
            applyChoice (index);
        },
        nullptr);

    strip.onChoice = [raw = attachment.get()] (int index)
    {
        raw->setValueAsCompleteGesture ((float) index);
    };

    attachment->sendInitialUpdate();

    return attachment;
}

void DemoEditor::applyTheme (int themeIndex)
{
    lookAndFeel.setScheme (settings::schemeFor (themeIndex));

    // Changing the colours in a LookAndFeel does not tell anything that it
    // happened. Most widgets look them up while painting and so only need a
    // repaint, but the rail's icons bake their colour into a Drawable and
    // rebuild it on a look-and-feel change — which is what this sends, to this
    // component and every descendant.
    sendLookAndFeelChange();
    repaint();
}

void DemoEditor::showSampleTuning()
{
    // Static values, not a simulation: enough to see the page populated and to
    // check that nothing is clipped once the boxes have text in them. Something
    // has to drive these for real, and that is the MIDI side's job — the page
    // takes them through the same setters either way, so nothing here has to
    // change when it arrives.
    //
    // The numbers are the ones from the sketch in docs/tuning.md, so the page
    // can be compared against the thing it implements.
    auto& page = sidebar.getTuningPage();

    page.setInterval ({ 1902.98, tuning::defaultModDivisor });

    // A few tunings for the name menu, so it has something to open.
    page.setAvailableNames ({ "Pythagorean 12", "Meantone 1/4", "Werckmeister III", "12edo" });
    page.setStatus ({ "Pythagorean 12", 3, 1, juce::Time::getCurrentTime() });

    // An inferred period with alternatives, so the chooser has something to
    // step through — the 12edo case from docs/tuning.md, where every multiple
    // of the repeating interval is a period. `specified` would be the duller
    // half of the widget: one value and the buttons disabled.
    juce::Array<double> candidates;

    for (auto cents = 100.0; cents <= 1500.0; cents += 100.0)
        candidates.add (cents);

    page.setPeriod ({ 1200.0, tuning::PeriodSource::inferred, candidates });
}

void DemoEditor::showSampleControllers()
{
    auto& page = sidebar.getControllersPage();

    // Two parameters with different units, so the editing table's limits can be
    // seen relabelling themselves when a row is pointed at the other one.
    // Built up rather than braced: juce::Array cannot deduce an element type
    // from a braced initialiser, so each one is added explicitly.
    juce::Array<controllers::Parameter> parameters;

    parameters.add ({ "swell",  "%" });
    parameters.add ({ "rotary", {} });

    page.setParameters (parameters);

    // The two mappings from Figure 2 of docs/controllers.md, including the
    // second one's empty LSB — which its `toggle` mode ignores anyway.
    controllers::Mapping swell;
    swell.parameterIndex = 0;
    swell.channel = controllers::omniChannel;
    swell.msb = 11;
    swell.lsb = 43;
    swell.mode = controllers::Mode::jump;
    swell.min = 10.0;
    swell.max = 100.0;

    controllers::Mapping rotary;
    rotary.parameterIndex = 1;
    rotary.channel = 15;
    rotary.msb = 64;
    rotary.mode = controllers::Mode::toggle;
    rotary.min = 1.0;
    rotary.max = 3.0;

    // A second mapping onto swell, added last. Without it the sample data sorts
    // the same either way — swell then rotary — and the sort toggle would look
    // broken when it was merely unexercised.
    controllers::Mapping swellFine;
    swellFine.parameterIndex = 0;
    swellFine.channel = 1;
    swellFine.msb = 12;
    swellFine.mode = controllers::Mode::scale;
    swellFine.min = 0.0;
    swellFine.max = 50.0;

    page.setMappings ({ swell, rotary, swellFine });

    // Figure 1's three monitor lines, newest first. Nothing generates these
    // yet; see docs/demo.md.
    juce::Array<controllers::Message> messages;

    messages.add ({ "note on", "15", "A4", "102" });
    messages.add ({ "sysex",   {},   {},   {} });
    messages.add ({ "control", "16", "11", "98" });

    page.setMessages (messages);
}

void DemoEditor::showSamplePresets()
{
    auto& page = sidebar.getPresetsPage();

    // The sketch's own numbers, and a comment long enough to need more than one
    // line — which is the point of giving it the page's flexible row.
    page.setFrequencies ({ 220.0, 440.0 });
    page.setAvailableNames ({ "Jimmie Smith", "Gospel Chops", "Blue Note" });
    page.setStatus ({ "Jimmie Smith", 1, {} });

    page.setMeta ({ "Hank Aaslund",
                    "Drawbar registration for gospel organ. "
                    "Works best with the rotary on fast. CC-BY-SA 4.0." });

    page.setIncludes ({ true, false });
    page.setSplitActive (true);
    page.setLayer (presets::Layer::lower);
}

void DemoEditor::applyBubbleTextColour (int bubbleTextIndex)
{
    // On the editor, not on the fader. A slider's value bubble resolves its
    // text with `findColour (TooltipWindow::textColourId, true)`, and that
    // `true` walks the parent chain — so one override up here reaches every
    // bubble in the plugin, which is how a real plugin would apply the
    // workaround rather than repeating it at each slider. (The walk does stop
    // early at any component that has its own LookAndFeel specifying the id.)
    if (const auto colour = settings::bubbleTextColourFor (bubbleTextIndex))
        setColour (juce::TooltipWindow::textColourId, *colour);
    else
        removeColour (juce::TooltipWindow::textColourId);
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

    controls.setBounds (content.reduced (layout::placeholderInset));

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
