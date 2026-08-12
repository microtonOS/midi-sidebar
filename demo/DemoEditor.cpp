#include "DemoEditor.h"
#include "DemoSettings.h"

namespace microtonos::sidebar::demo
{

//==============================================================================
DemoEditor::DemoEditor (DemoProcessor& p)
    : juce::AudioProcessorEditor (&p), processor (p), content (p.apvts)
{
    setLookAndFeel (&lookAndFeel);

    // Content first so the sidebar is added later and therefore draws on top
    // of it, which is what an overlaying sidebar needs.
    addAndMakeVisible (content);
    addAndMakeVisible (sidebar);

    // Every synth knob and switch gets the parameter menu, by the index it was
    // built from — which is the index the sidebar's parameter list uses and the
    // one a mapping stores. This is the whole point of the synth panel: the
    // developer settings in the other tab are not parameters anyone would
    // assign a controller to.
    content.getSynthPanel().attachMenu (parameterMenu);

    // Left unconnected on purpose. Assigning the next controller to arrive
    // needs a controller to arrive, and nothing here reads MIDI yet; the rule
    // for what to do when it does is in docs/right-click.md.
    parameterMenu.onMidiLearnRequested = [] (int) {};

    sidebar.onPreferredWidthChanged = [this] { layOutSidebar (true); };
    sidebar.onPanic = [] { /* CC120 goes here once the processor sends MIDI. */ };

    showSampleTuning();
    showSampleControllers();
    showSamplePresets();
    showSampleChannels();

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
    themeAttachment = attachChoice ("theme", content.getControls().getThemeStrip(),
                                    [this] (int index) { applyTheme (index); });

    edgeAttachment = attachChoice ("edge", content.getControls().getEdgeStrip(),
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

    bubbleTextAttachment = attachChoice ("bubbleText", content.getControls().getBubbleTextStrip(),
                                         [this] (int index) { applyBubbleTextColour (index); });

    // The open tab, mirrored both ways like the sidebar's page. Not through
    // `attachChoice`, which binds a ChoiceStrip; a tab bar is its own kind of
    // control.
    if (auto* viewParam = processor.apvts.getParameter ("view"))
    {
        viewAttachment = std::make_unique<juce::ParameterAttachment> (
            *viewParam,
            [this] (float value) { content.setView (juce::roundToInt (value)); },
            nullptr);

        content.onViewChanged = [this] (int index)
        {
            if (viewAttachment != nullptr)
                viewAttachment->setValueAsCompleteGesture ((float) index);
        };

        viewAttachment->sendInitialUpdate();
    }

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

    // The stand-in plugin's own parameters, from the one list that also
    // declares them and builds their widgets — so an index here, an index in
    // the panel and the index a mapping stores are the same number. They carry
    // different units, which is what lets the editing table's limits be seen
    // relabelling themselves when a row is pointed at another parameter.
    page.setParameters (synth::parametersForSidebar());

    // Figure 2 of docs/controllers.md, keeping its shape — one omni-off CC pair,
    // one channel-specific CC, one polytouch row — with the synth's parameters
    // in place of the clonewheel names the sketch still uses. The second one's
    // LSB is left empty, which its `toggle` mode ignores anyway.
    controllers::Mapping cutoff;
    cutoff.parameterIndex = synth::Index::cutoff;
    cutoff.channel = controllers::omniOffChannel;
    cutoff.msb = 11;
    cutoff.lsb = 43;
    cutoff.mode = controllers::Mode::jump;
    cutoff.min = 200.0;
    cutoff.max = 8000.0;

    controllers::Mapping resonance;
    resonance.parameterIndex = synth::Index::resonance;
    resonance.channel = 15;
    resonance.msb = 64;
    resonance.mode = controllers::Mode::toggle;
    resonance.min = 1.0;
    resonance.max = 3.0;

    // Figure 2's third row. A polytouch mapping, which is what shows the word
    // drawn across the two controller-number columns — and, being a third
    // mapping that sorts differently from the first two, it is also what keeps
    // the sort toggle from looking broken when it is merely unexercised.
    controllers::Mapping vibrato;
    vibrato.parameterIndex = synth::Index::pitchLfoDepth;
    vibrato.channel = 15;
    vibrato.source = controllers::Source::polytouch;
    vibrato.mode = controllers::Mode::toggle;
    vibrato.min = 1.0;
    vibrato.max = 3.0;

    page.setMappings ({ cutoff, resonance, vibrato });

    // Figure 1's line and one before it, newest first. The page does not
    // compose these — see the note in ControllersState.h — so the phrasing is
    // the *host's*, and this is the demo standing in for it: every number said
    // with what it is, since the columns that used to explain them are gone.
    // "cc 11" already says the message is a control change, so the word itself
    // would only repeat the column heading that went with them.
    //
    // The second line carries an LSB, which is the longest thing the monitor
    // has to show and the reason it is two lines rather than one. Nothing
    // generates these yet; see docs/demo.md.
    page.setMessages ({ "ch 16  cc 11  value 98",
                        "ch 16  cc 11  lsb 43  value 98",
                        "ch 15  note on  A4  value 102" });

}

void DemoEditor::showSampleChannels()
{
    auto& page = sidebar.getChannelsPage();

    // Both settings live at once, which is the point of the page: a lower zone
    // over channels 1 to 9, and an omni selection with a few of the rest muted.
    // Nothing reads this yet; see docs/demo.md.
    channels::Setup setup;

    setup.omniOn = true;
    setup.omniChannels = (channels::Mask) (channels::allChannels & ~0b0000'0000'1010'0100);

    setup.mpeOn = true;
    setup.zone = channels::Zone::lower;
    setup.zoneEdge = 9;

    page.setSetup (setup);
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
    auto hostArea = getLocalBounds();
    hostArea = onLeft ? hostArea.withTrimmedLeft (Sidebar::getRailWidth())
                      : hostArea.withTrimmedRight (Sidebar::getRailWidth());

    content.setBounds (hostArea.reduced (layout::placeholderInset));

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
