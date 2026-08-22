#include "ControllersPage.h"

namespace microtonos::sidebar
{

using namespace controllers;

//==============================================================================
ControllersPage::ControllersPage()
{
    // Frames first, so everything else draws over them — the arrangement the
    // tuning page uses and for the same reason.
    for (auto* group : { &insertGroup, &editGroup })
    {
        group->setTextLabelPosition (juce::Justification::centredLeft);
        group->setInterceptsMouseClicks (false, false);
        addAndMakeVisible (*group);
    }

    insertGroup.setText ("INSERT");
    editGroup  .setText ("EDIT");

    addAndMakeVisible (monitor);

    for (auto* b : { &ccButton, &aftertouchButton, &polytouchButton,
                     &deleteButton, &undoButton, &redoButton })
        addAndMakeVisible (*b);

    addAndMakeVisible (table);

    //  Insert -----------------------------------------------------------------
    // Each adds a blank row of its own kind. The kind is a button rather than a
    // column because it decides what the rest of the row means: the two touch
    // sources have no controller number to type into.
    ccButton        .onClick = [this] { table.addMapping (controllers::Source::control); };
    aftertouchButton.onClick = [this] { table.addMapping (controllers::Source::aftertouch); };
    polytouchButton .onClick = [this] { table.addMapping (controllers::Source::polytouch); };

    //  Edit -------------------------------------------------------------------
    deleteButton.onClick = [this] { table.removeSelectedMapping(); };
    undoButton  .onClick = [this] { table.undo(); };
    redoButton  .onClick = [this] { table.redo(); };

    table.onMappingsChanged = [this] { if (onMappingsChanged != nullptr) onMappingsChanged(); };
    table.onHistoryChanged   = [this] { refreshHistory(); };

    refreshHistory();
}

ControllersPage::~ControllersPage()
{
    // Learning is a modal act — it takes the monitor over and waits for a
    // gesture — so closing the window is a decision not to finish it. Stopping
    // the timer here is also what keeps a callback from arriving into a
    // half-destroyed page.
    cancelLearn();
}

//==============================================================================
void ControllersPage::setParameters (juce::Array<controllers::Parameter> parameters)
{
    table.setParameters (std::move (parameters));
}

void ControllersPage::setMappings (juce::Array<controllers::Mapping> mappings)
{
    table.setMappings (std::move (mappings));
}

void ControllersPage::addMapping (controllers::Mapping mapping)
{
    table.addMapping (std::move (mapping));
}

const juce::Array<controllers::Parameter>& ControllersPage::getParameters() const noexcept
{
    return table.getParameters();
}

const juce::Array<controllers::Mapping>& ControllersPage::getMappings() const noexcept
{
    return table.getMappings();
}

void ControllersPage::showMappingsFor (int parameterIndex)
{
    table.selectMappingsFor (parameterIndex);
}

void ControllersPage::removeMappingsFor (int parameterIndex)
{
    table.removeMappingsFor (parameterIndex);
}

void ControllersPage::addMessage (const juce::String& message)
{
    messages.insert (0, message);
    setMessages (messages);
}

void ControllersPage::setMessages (juce::StringArray newMessages)
{
    messages = std::move (newMessages);

    // Keeping the newest is the point of a tail; dropping from the end would
    // keep the first two messages of the session for ever.
    while (messages.size() > monitorLines)
        messages.remove (messages.size() - 1);

    refreshMonitor();
}

void ControllersPage::refreshMonitor()
{
    monitor.setValue (messages.joinIntoString ("\n"));
}

//==============================================================================
/** The window MIDI learn puts up while it waits.

    A `ThreadWithProgressWindow`, following JUCE's own DialogsDemo. The thread
    does no work — it waits, and the *message* thread is where MIDI arrives and
    where the decision is made. What the thread buys is the modal window with a
    spinning bar and a Cancel button, which is exactly the shape wanted and is
    not otherwise reachable without building one by hand.

    `setStatusMessage` is designed to be called from the worker thread, so the
    two lines are written here from flags the message thread sets. */
class ControllersPage::LearnWindow final : public juce::ThreadWithProgressWindow
{
public:
    explicit LearnWindow (ControllersPage& p, const juce::String& parameterName)
        : juce::ThreadWithProgressWindow ("MIDI learn: " + parameterName, true, true),
          page (p)
    {
        setStatusMessage (waitingMessage);
    }

    void run() override
    {
        // Beyond 0..1, so the bar spins rather than filling: there is no
        // progress to report, only a state to be in.
        setProgress (-1.0);

        while (! threadShouldExit() && ! finished.load())
        {
            setStatusMessage (moved.load() ? releaseMessage : waitingMessage);
            wait (60);
        }
    }

    /** Called on the message thread when a learnable message has arrived. */
    void noteMovement() { moved = true; }

    /** Ends the wait, so `run` returns and `threadComplete` follows. */
    void finish() { finished = true; }

    bool hasMoved() const { return moved.load(); }

    void threadComplete (bool userPressedCancel) override
    {
        page.learnWindowClosed (userPressedCancel);
        delete this;   // ThreadWithProgressWindow's own convention
    }

private:
    static inline const juce::String waitingMessage { "Move controller!" };
    static inline const juce::String releaseMessage { "Release controller!" };

    ControllersPage& page;
    std::atomic<bool> moved { false }, finished { false };
};

//==============================================================================
void ControllersPage::beginLearn (int parameterIndex)
{
    const auto& parameters = getParameters();

    cancelLearn();

    learner.begin (parameterIndex);

    const auto name = juce::isPositiveAndBelow (parameterIndex, parameters.size())
                          ? parameters[parameterIndex].name
                          : juce::String();

    // Self-deleting, so this is a borrowed pointer cleared in
    // `learnWindowClosed` rather than something owned here.
    learnWindow = new LearnWindow (*this, name);
    learnWindow->launchThread();

    // The long clock first: nothing has arrived yet, and the end-user has to
    // get from the menu to the hardware.
    waitingForFirst = true;
    startTimer (learnTimeoutMs);
}

void ControllersPage::observeLearn (const juce::MidiMessage& message)
{
    if (! learner.isActive())
        return;

    const auto before = learner.messagesSeen();

    learner.observe (message);

    // A message the learner ignored — another channel, or something that cannot
    // name a control — must not restart the clock, or a keyboard playing in the
    // background would hold learning open indefinitely.
    if (learner.messagesSeen() == before)
        return;

    if (learnWindow != nullptr)
        learnWindow->noteMovement();

    // Restarting on every message is what makes the gesture as long as the
    // end-user wants it: the short clock only runs out once they stop, which is
    // the moment the window has been asking them to reach.
    waitingForFirst = false;
    startTimer (learnSettleMs);
}

void ControllersPage::timerCallback()
{
    // The long clock expiring means nothing ever came; the short clock expiring
    // means the controller has been released. Either way the window closes, and
    // `learnWindowClosed` is where the decision is made.
    stopTimer();

    if (learnWindow != nullptr)
        learnWindow->finish();
}

void ControllersPage::learnWindowClosed (bool userPressedCancel)
{
    learnWindow = nullptr;
    stopTimer();

    const auto learned = userPressedCancel ? std::optional<controllers::Mapping>{}
                                           : learner.suggestion();

    learner.cancel();

    // Undoably, and with limits taken from the parameter's own range — which is
    // what `addMapping` already does for a mapping arriving from outside.
    if (learned.has_value())
        addMapping (*learned);

    if (onLearnFinished != nullptr)
        onLearnFinished (learned);
}

void ControllersPage::cancelLearn()
{
    if (! learner.isActive())
        return;

    stopTimer();

    // Ending the thread brings `threadComplete` round to `learnWindowClosed`,
    // which is the one place learning is torn down.
    if (learnWindow != nullptr)
        learnWindow->signalThreadShouldExit();
    else
        learner.cancel();
}

//==============================================================================
void ControllersPage::refreshHistory()
{
    // Disabled rather than hidden: a button that vanishes when there is nothing
    // to undo moves the two beside it, and a row that reflows as you work is
    // harder to aim at than one with a greyed button in it.
    undoButton.setEnabled (table.canUndo());
    redoButton.setEnabled (table.canRedo());
}

//==============================================================================
void ControllersPage::lookAndFeelChanged()
{
    if (! getLookAndFeel().isColourSpecified (pageColours::sectionTitleColourId))
        return;

    for (auto* group : { &insertGroup, &editGroup })
    {
        group->setColour (juce::GroupComponent::textColourId,    findColour (pageColours::sectionTitleColourId));
        group->setColour (juce::GroupComponent::outlineColourId, findColour (pageColours::sectionOutlineColourId));
    }
}

//==============================================================================
void ControllersPage::resized()
{
    // The same six columns as the other pages, so all three read as one plugin;
    // see PageGrid.
    PageGrid grid;

    constexpr auto full  = metrics::pageColumns;
    constexpr auto third = metrics::pageThirdColumns;

    //  The monitor, unframed at the top like the tuning page's interval block:
    //  the sketch gives it no title.
    grid.place (monitor, grid.addRow (metrics::pageTopHeight (metrics::pageTopRows)), 1, full);

    //  Insert -----------------------------------------------------------------
    const auto insertTitle = grid.addRow (metrics::pageGroupTitleHeight);

    {
        // Thirds, which six columns divide into exactly — and the same three
        // the EDIT row below uses, so the two rows line up down the page.
        const auto row = grid.addRow (metrics::pageRowHeight);

        grid.place (ccButton,         row, 1, third);
        grid.place (aftertouchButton, row, 1 + third, third);
        grid.place (polytouchButton,  row, 1 + third * 2, third);
    }

    grid.frame (insertGroup, insertTitle, grid.addRow (metrics::pageGroupPadding));

    //  Edit -------------------------------------------------------------------
    const auto editTitle = grid.addRow (metrics::pageGroupTitleHeight);

    // The one flexible track on the page: the table takes whatever height is
    // left, which is what lets the page fit a short panel and fill a tall one.
    grid.place (table, grid.addFlexibleRow(), 1, full);

    {
        const auto row = grid.addRow (metrics::pageRowHeight);

        // delete first, then the pair that puts it back — the destructive action
        // beside the table it acts on, and undo/redo in their reading order.
        grid.place (deleteButton, row, 1, third);
        grid.place (undoButton,   row, 1 + third, third);
        grid.place (redoButton,   row, 1 + third * 2, third);
    }

    grid.frame (editGroup, editTitle, grid.addRow (metrics::pageGroupPadding));

    grid.performLayout (getLocalBounds(), getMinimumHeight());
}

} // namespace microtonos::sidebar
