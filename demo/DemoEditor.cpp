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

    // Two halves. The router collects on the audio thread — it knows nothing
    // about which parameter is being learned — and the page watches the
    // gesture, because it owns both the `MidiLearner` and the monitor that has
    // to say what is happening. See docs/right-click.md for the rule and
    // MidiLearner.h for why a gesture is watched rather than a message taken.
    parameterMenu.onMidiLearnRequested = [this] (int parameterIndex)
    {
        auto& page = sidebar.getControllersPage();

        // Opened first: the monitor is where learning reports, so asking the
        // end-user to move something without showing them the page would be
        // asking them to trust that anything is listening.
        sidebar.setActivePage (Sidebar::Page::controllers);

        page.beginLearn (parameterIndex);

        processor.router.learnFor (parameterIndex);
    };

    processor.onLearnCandidates = [this] (const juce::Array<juce::MidiMessage>& candidates)
    {
        auto& page = sidebar.getControllersPage();

        for (const auto& message : candidates)
            page.observeLearn (message);
    };

    sidebar.getControllersPage().onLearnFinished =
        [this] (std::optional<controllers::Mapping> learned)
    {
        // Disarmed whichever way it ended, or the router would keep copying
        // messages nobody is reading.
        processor.router.learnFor (controllers::noParameter);

        // The page has already added the row; this only shows it, since a
        // mapping that appears silently is one the end-user cannot check.
        if (learned.has_value())
            sidebar.getControllersPage().showMappingsFor (learned->parameterIndex);
    };

    // A Master Volume system exclusive moves the fader, which moves the
    // parameter through the attachment below. See MidiDeviceControl.h.
    processor.onMasterVolume = [this] (double decibels)
    {
        sidebar.getVolumeSlider().setValue (decibels, juce::sendNotificationSync);
    };

    sidebar.onPreferredWidthChanged = [this] (bool animate) { layOutSidebar (animate); };
    sidebar.onPanic = [] { /* CC120 goes here once the processor sends MIDI. */ };

    showSampleTuning();
    showSampleControllers();
    showSamplePresets();
    showSampleChannels();

    // The fader and the parameter are one control in two places, so the
    // attachment runs both ways: dragging the fader writes the parameter, and a
    // host — or a Master Volume system exclusive, which arrives through the
    // fader — writes it back. Both are already in decibels on the same scale
    // and floor, so nothing is converted between them.
    if (auto* volumeParam = processor.apvts.getParameter ("volume"))
        volumeAttachment = std::make_unique<juce::SliderParameterAttachment> (
            *volumeParam, sidebar.getVolumeSlider());

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

    // The dragged width, mirrored the same way as the page. One direction only:
    // the sidebar clamps what it is given, so echoing the clamped result back
    // into the parameter would rewrite what the user asked for every time the
    // window was too narrow to honour it.
    if (auto* widthParam = processor.apvts.getParameter ("panelWidth"))
    {
        panelWidthAttachment = std::make_unique<juce::ParameterAttachment> (
            *widthParam,
            [this] (float value) { sidebar.setPanelWidth (juce::roundToInt (value)); },
            nullptr);

        panelWidthAttachment->sendInitialUpdate();
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
    // No longer sample data: the page is driven by `processor.tuningSource`,
    // which owns the four schemes. What is left here is the wiring — the page
    // reports intent, the source acts on it, and `refreshTuning` pushes the
    // result back. The setters are the same ones the sketch's numbers went
    // through, which is why nothing on the page had to change.
    auto& page   = sidebar.getTuningPage();
    auto& source = processor.tuningSource;

    page.setScheme (source.getScheme());

    page.onModDivisorChanged = [this] (double divisor)
    {
        modDivisor = divisor;
        refreshTuning();
    };

    page.onSchemeChanged = [this] (tuning::Scheme scheme)
    {
        processor.tuningSource.setScheme (scheme);
        refreshTuning();
    };

    page.onProgramChosen = [this] (std::optional<int> program)
    {
        processor.tuningSource.setProgram (program);
        refreshTuning();
    };

    page.onBankChosen = [this] (std::optional<int> bank)
    {
        processor.tuningSource.setBank (bank);
        refreshTuning();
    };

    page.onNameChosen = [this] (int index)
    {
        processor.tuningSource.chooseName (index);
        refreshTuning();
    };

    // One dialog for both kinds of file, as tuneBfree does: the first .scl is
    // the scale and every .kbm joins the per-channel batch. Choosing a directory
    // instead loads it as a bank.
    page.onFilesRequested = [this]
    {
        fileChooser = std::make_unique<juce::FileChooser> (
            "Load tuning files (.scl and .kbm)", juce::File(), "*.scl;*.kbm");

        const auto flags = juce::FileBrowserComponent::openMode
                         | juce::FileBrowserComponent::canSelectFiles
                         | juce::FileBrowserComponent::canSelectDirectories
                         | juce::FileBrowserComponent::canSelectMultipleItems;

        fileChooser->launchAsync (flags, [this] (const juce::FileChooser& chooser)
        {
            juce::Array<juce::File> files;

            for (const auto& result : chooser.getResults())
            {
                if (result.isDirectory())
                    processor.tuningSource.loadBank (result);
                else
                    files.add (result);
            }

            if (! files.isEmpty())
                processor.tuningSource.loadFiles (files);

            refreshTuning();
        });
    };

    // The processor calls this when MIDI has moved the tuning — a sysex or one
    // of the tuning RPNs — which happens whether or not this window is open.
    processor.onTuningChanged = [this] { refreshTuning(); };

    refreshTuning();
}

void DemoEditor::refreshTuning()
{
    auto& page   = sidebar.getTuningPage();
    auto& source = processor.tuningSource;

    page.setStatus (source.getStatus());
    page.setPeriod (source.getPeriod());
    page.setAvailableNames (source.availableNames());
    page.setLoadedSummary (source.loadedSummary());

    // The interval is between the lowest and highest notes actually held, looked
    // up through the tuning in force — so it is what is sounding rather than
    // what twelve-tone arithmetic would say. Empty when nothing is down, which
    // the page draws as "all notes off".
    const auto held = processor.getHeldNotes();

    tuning::Interval interval;
    interval.modDivisor = modDivisor;

    if (held.count > 0)
        interval.cents = source.intervalFor (held.lowestNote, held.highestNote,
                                             held.lowestChannel, held.highestChannel);

    page.setInterval (interval);
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

    // Figure 2 of docs/controllers.md, keeping its shape — one CC pair with an
    // LSB, one CC without, one polytouch row — with the synth's parameters in
    // place of the clonewheel names the sketch still uses. Three different
    // channels, so the column is visibly a channel rather than a constant. The
    // second one's LSB is left empty, which its `toggle` mode ignores anyway.
    controllers::Mapping cutoff;
    cutoff.parameterIndex = synth::Index::cutoff;
    cutoff.channel = 1;
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

    // A deliberately broken row, so the invalid wash is visible in the figures
    // and cannot regress unnoticed. CC 120 is All Sound Off — a Channel Mode
    // Message, not a control change at all — which is one of the eleven numbers
    // `controllers::isCcUnavailable` refuses under any setting.
    controllers::Mapping broken;
    broken.parameterIndex = synth::Index::filterLfoRate;
    broken.channel = 3;
    broken.msb = 120;

    page.setMappings ({ cutoff, resonance, vibrato, broken });

    // The table is now wired to the audio side. Every edit re-arms the router,
    // so a knob moved on the hardware moves the parameter the row names — which
    // is the point of the page, and the first time anything here has reached
    // past the GUI.
    page.onMappingsChanged = [this]
    {
        processor.setMappings (sidebar.getControllersPage().getMappings());
    };

    processor.setMappings (page.getMappings());

    // The examples docs/controllers.md gives — but built as real messages and
    // put through `midiMonitor::lineFor`, so the figure shows what the plugin
    // will actually print rather than a hand-typed approximation of it. If the
    // formatter and the doc drift apart, the figure says so.
    const juce::MidiMessage samples[] =
    {
        juce::MidiMessage::noteOn (2, 60, (juce::uint8) 127),
        juce::MidiMessage::controllerEvent (16, 11, 98),
        juce::MidiMessage::controllerEvent (1, 80, 101),
    };

    juce::StringArray lines;

    // Newest first, which is the order the monitor keeps.
    for (int i = juce::numElementsInArray (samples); --i >= 0;)
        if (const auto line = midiMonitor::lineFor (samples[i]))
            lines.add (*line);

    page.setMessages (lines);

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
    setup = channels::withZoneMembers (setup, channels::Zone::lower, 8);

    page.setSetup (setup);

    // The page is now wired to something. Editing it changes what the router
    // listens to on the next block — the first place a sidebar control reaches
    // the MIDI stream rather than only reporting itself.
    page.onSetupChanged = [this] (channels::Setup newSetup)
    {
        processor.router.setChannels (newSetup);
    };

    // And the other direction: an MPE Configuration Message arriving on a
    // manager channel moves the zone, so the matrix has to follow. The router
    // has already taken it — this only shows it.
    processor.onChannelsChanged = [this] (channels::Setup newSetup)
    {
        sidebar.getChannelsPage().setSetup (newSetup);
    };

    processor.router.setChannels (setup);
}

void DemoEditor::showSamplePresets()
{
    auto& page = sidebar.getPresetsPage();

    // No longer sample data for the status block: `processor.presetStore` owns
    // the presets, and a program change moves them whether or not this window
    // is open. What is still seeded here is the split, which has nowhere else to
    // come from until a preset carrying one has been loaded.
    page.setFrequencies (split.frequencies);
    page.setSplitActive (split.active);
    page.setLayer (split.editing);

    // Three presets so the page opens on something, seeded through the store's
    // own path rather than pushed at the page — the names and comment are the
    // sketch's from docs/presets.md. Only once: the editor can be reopened, and
    // a second window should not double the bank.
    if (processor.presetStore.availableNames().isEmpty())
    {
        processor.presetStore.addFromCurrentState (
            "Jimmie Smith",
            { "Hank Aaslund",
              "Drawbar registration for gospel organ. "
              "Works best with the rotary on fast. CC-BY-SA 4.0." });

        processor.presetStore.addFromCurrentState ("Gospel Chops");
        processor.presetStore.addFromCurrentState ("Blue Note");
    }

    //  The split -------------------------------------------------------------
    page.onFrequenciesEdited = [this] (presets::Frequencies edited)
    {
        split.frequencies = edited;
        refreshPresets();
    };

    page.onSplitToggled = [this] (bool isActive)
    {
        split.active = isActive;
        refreshPresets();
    };

    // The layer switch is the visible half of "two presets in one": it chooses
    // which of the two parameter sets the synth panel shows and edits. The
    // audible half — the crossfade — is the developer's, because the sidebar has
    // no voices to apply a per-note gain to. See Split.h.
    page.onLayerChanged = [this] (presets::Layer layer)
    {
        if (layer == split.editing)
            return;

        // The set being left is kept before the other is shown, or switching
        // away would discard whatever had just been edited.
        storeLayer (split.editing);
        split.editing = layer;
        recallLayer (layer);

        refreshPresets();
    };

    // With notes held the split point comes from what is sounding, which is why
    // the button reads `update?` then; with none it comes from the two fields.
    page.onSplitPointRequested = [this] (bool fromSoundingNotes)
    {
        if (fromSoundingNotes)
        {
            const auto held = processor.getHeldNotes();

            if (held.count > 0)
                split.frequencies = {
                    processor.tuningSource.frequencyFor (held.lowestNote, held.lowestChannel),
                    processor.tuningSource.frequencyFor (held.highestNote, held.highestChannel) };
        }

        refreshPresets();
    };

    //  Navigation ------------------------------------------------------------
    page.onProgramChosen = [this] (std::optional<int> program)
    {
        if (program.has_value())
            processor.presetStore.setProgram (*program - 1);   // shown 1-based

        refreshPresets();
    };

    page.onBankChosen = [this] (std::optional<int> bank)
    {
        if (bank.has_value())
            processor.presetStore.setBank (*bank - 1);

        refreshPresets();
    };

    page.onNameChosen = [this] (int index)
    {
        processor.presetStore.chooseName (index);
        refreshPresets();
    };

    page.onMetaEdited = [this] (presets::Meta meta)
    {
        processor.presetStore.setMeta (std::move (meta));
        refreshPresets();
    };

    //  Files -----------------------------------------------------------------
    page.onOpenRequested = [this]
    {
        fileChooser = std::make_unique<juce::FileChooser> (
            "Open a preset, or a directory as a bank", juce::File(), "*.xml");

        const auto flags = juce::FileBrowserComponent::openMode
                         | juce::FileBrowserComponent::canSelectFiles
                         | juce::FileBrowserComponent::canSelectDirectories
                         | juce::FileBrowserComponent::canSelectMultipleItems;

        fileChooser->launchAsync (flags, [this] (const juce::FileChooser& chooser)
        {
            for (const auto& result : chooser.getResults())
            {
                if (result.isDirectory())
                    processor.presetStore.loadBank (result);
                else
                    processor.presetStore.loadFile (result);
            }

            refreshPresets();
        });
    };

    page.onSaveRequested = [this]
    {
        fileChooser = std::make_unique<juce::FileChooser> (
            "Save this preset", juce::File(), "*.xml");

        const auto flags = juce::FileBrowserComponent::saveMode
                         | juce::FileBrowserComponent::warnAboutOverwriting;

        fileChooser->launchAsync (flags, [this] (const juce::FileChooser& chooser)
        {
            const auto file = chooser.getResult();

            if (file != juce::File())
                processor.presetStore.save (file.withFileExtension ("xml"),
                                            file.getFileNameWithoutExtension());

            refreshPresets();
        });
    };

    // A program change arrives whether or not this window is open, so the page
    // is told from the processor rather than only from its own controls.
    processor.onPresetChanged = [this] { refreshPresets(); };

    refreshPresets();
}

void DemoEditor::refreshPresets()
{
    auto& page = sidebar.getPresetsPage();
    auto& store = processor.presetStore;

    page.setStatus (store.getStatus());
    page.setAvailableNames (store.availableNames());
    page.setMeta (store.getMeta());
    page.setFrequencies (split.frequencies);
    page.setSplitActive (split.active);
    page.setLayer (split.editing);

    // The button reads `update?` only while something is sounding, since that is
    // the case where the split point could come from two places.
    page.setNotesActive (processor.getHeldNotes().count > 0);
}

void DemoEditor::storeLayer (presets::Layer layer)
{
    auto& saved = layer == presets::Layer::lower ? lowerLayer : upperLayer;

    saved.clear();

    for (const auto& control : synth::controls())
        if (auto* p = processor.apvts.getParameter (control.id))
            saved.set (control.id, p->getValue());
}

void DemoEditor::recallLayer (presets::Layer layer)
{
    const auto& saved = layer == presets::Layer::lower ? lowerLayer : upperLayer;

    // An empty set is a layer never edited, which starts as a copy of whatever
    // is live rather than as silence.
    if (saved.size() == 0)
        return;

    for (const auto& control : synth::controls())
        if (auto* p = processor.apvts.getParameter (control.id))
            if (saved.contains (control.id))
                p->setValueNotifyingHost (saved[control.id]);
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

    drainMonitor();

    // MTS-ESP has no callback: a master retunes whenever it likes and the only
    // way to see it is to ask. Under any other scheme this is nearly free —
    // everything is already a table — so it is not worth a second timer.
    refreshTuning();

    // The edited marker answers a question about the *synth's* parameters, which
    // change from the panel, from a mapped controller and from the host — none
    // of which pass through the presets page. So it is polled rather than
    // pushed, and only the transition costs anything: `refreshPresets` rebuilds
    // the name menu, which is not something to do at the meter rate.
    if (const auto edited = processor.presetStore.isEdited(); edited != shownAsEdited)
    {
        shownAsEdited = edited;
        refreshPresets();
    }
}

void DemoEditor::drainMonitor()
{
    // Taken under the lock and formatted outside it: composing a juce::String
    // is the expensive half, and the audio thread only ever tries for this lock
    // rather than waiting on it.
    juce::Array<juce::MidiMessage> arrived;

    {
        const juce::ScopedLock lock (processor.monitorLock);

        if (processor.pendingMessages.isEmpty())
            return;

        arrived.swapWith (processor.pendingMessages);
    }

    // Only the last few can be shown, so a block that arrived with hundreds of
    // messages is trimmed here rather than pushed through one at a time.
    const auto first = juce::jmax (0, arrived.size() - controllers::monitorLines);

    for (int i = first; i < arrived.size(); ++i)
        if (const auto line = midiMonitor::lineFor (arrived[i]))
            sidebar.getControllersPage().addMessage (*line);
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
