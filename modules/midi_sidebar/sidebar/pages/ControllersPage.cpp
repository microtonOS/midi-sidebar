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
    if (! learner.isActive())
    {
        monitor.setValue (messages.joinIntoString ("\n"));
        return;
    }

    // Three lines, because that is what the monitor is. The third tracks the
    // current best guess rather than only the final one, so a sweep is visibly
    // converging on something while the end-user is still moving it.
    juce::StringArray lines;

    lines.add ("learning  " + learningName);

    const auto seen = learner.messagesSeen();

    lines.add (seen == 0 ? juce::String ("move a control...")
                         : juce::String ("keep moving the control..."));

    if (const auto guess = learner.suggestion())
    {
        auto line = channelName (guess->channel) + "  ";

        line << (guess->cc.has_value() ? "CC " + juce::String (*guess->cc)
                                       : sourceNames[static_cast<int> (guess->source)]);

        lines.add (line + "  " + juce::String (seen) + " messages");
    }

    monitor.setValue (lines.joinIntoString ("\n"));
}

//==============================================================================
void ControllersPage::beginLearn (int parameterIndex)
{
    const auto& parameters = getParameters();

    learner.begin (parameterIndex);
    learningName = juce::isPositiveAndBelow (parameterIndex, parameters.size())
                       ? parameters[parameterIndex].name
                       : juce::String();

    // The long clock first: nothing has arrived yet, and the end-user has to
    // get from the menu to the hardware.
    waitingForFirst = true;
    startTimer (learnTimeoutMs);

    refreshMonitor();
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

    // Restarting on every message is what makes the gesture as long as the
    // end-user wants it: the short clock only runs out once they stop.
    waitingForFirst = false;
    startTimer (learnSettleMs);

    refreshMonitor();
}

void ControllersPage::timerCallback()
{
    // The long clock expiring means nothing ever came, so there is nothing to
    // decide; the short clock expiring means the gesture is over.
    if (waitingForFirst)
        cancelLearn();
    else
        finishLearn();
}

void ControllersPage::finishLearn()
{
    const auto learned = learner.suggestion();

    stopTimer();
    learner.cancel();

    // Undoably, and with limits taken from the parameter's own range — which is
    // what `addMapping` already does for a mapping arriving from outside.
    if (learned.has_value())
        addMapping (*learned);

    refreshMonitor();

    if (onLearnFinished != nullptr)
        onLearnFinished (learned);
}

void ControllersPage::cancelLearn()
{
    if (! learner.isActive())
        return;

    stopTimer();
    learner.cancel();
    refreshMonitor();

    if (onLearnFinished != nullptr)
        onLearnFinished ({});
}

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
