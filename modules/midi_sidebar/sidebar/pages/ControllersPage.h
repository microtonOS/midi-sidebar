#pragma once

#include <optional>

#include <juce_gui_basics/juce_gui_basics.h>

#include "../SidebarLookAndFeel.h"
#include "../midi/MidiLearner.h"
#include "../widgets/ChoiceButton.h"
#include "../widgets/ReadOutField.h"
#include "ControllersState.h"
#include "ControllersTable.h"
#include "PageGrid.h"

namespace microtonos::sidebar
{

//==============================================================================
/** The controllers page: what is arriving, and what it is mapped to.

    Implements docs/controllers.md. The newest messages at the top, then
    `INSERT` and `EDIT` as framed sections, the same shape the tuning page uses.

    **No files section.** A preset is meant to carry the mappings too — though it
    does not yet; see TODO.md. The intent is that they travel with the sound,
    so a second way to save just the mappings was a second thing to keep in
    step. See the presets page, whose `FILE` section is now the only one.

    Channels are not here. Omni and the MPE zones were a section of this page
    and are now a page of their own, because they are the whole plugin's
    business rather than the controller table's — see `ChannelsPage`. The
    per-mapping `channel` column stays: that is one mapping's scope.

    **Its height works the other way round from the tuning page.** There every
    row was fixed and a short panel cut the bottom off. Here everything but the
    editing table is fixed — the monitor, two rows of buttons, two frames —
    and the table is the single flexible track, so the page has a genuine
    minimum and simply grows into anything above it. That is the pattern the
    presets page should follow.
*/
class ControllersPage final : public juce::Component,
                              private juce::Timer
{
public:
    ControllersPage();
    ~ControllersPage() override;

    //==========================================================================
    //  Values in.

    /** The parameters a mapping may target. The unit on each is what its limits
        are displayed in. */
    void setParameters (juce::Array<controllers::Parameter> parameters);

    const juce::Array<controllers::Parameter>& getParameters() const noexcept;

    void setMappings (juce::Array<controllers::Mapping> mappings);

    /** Appends one already-formed mapping, undoably — what MIDI learn does. */
    void addMapping (controllers::Mapping mapping);
    const juce::Array<controllers::Mapping>& getMappings() const noexcept;

    /** Selects every row mapped to `parameterIndex`, scrolling the first into
        view. What the right-click menu's "view in sidebar" does once the
        sidebar has opened this page; see ParameterMenu. */
    void showMappingsFor (int parameterIndex);

    /** Removes every mapping for `parameterIndex` — the right-click menu's
        "unlearn". */
    void removeMappingsFor (int parameterIndex);

    /** Pushes one line onto the monitor, dropping the oldest. Already
        formatted — see the note in ControllersState.h about why this module
        does not compose it. This is the call an owner watching a MIDI stream
        wants. */
    void addMessage (const juce::String& message);

    /** Replaces the whole monitor at once, newest first, which is what a demo
        or a restored session does. Anything past `controllers::monitorLines` is
        ignored, and an empty list shows the placeholder. */
    void setMessages (juce::StringArray messages);

    /** What the monitor currently reads. The counterpart to `setMessages`. */
    const juce::String& getMonitorText() const noexcept { return monitor.getValue(); }

    //==========================================================================
    //  MIDI learn.

    /** Starts watching for the control the end-user is about to move, on behalf
        of `parameterIndex`, and takes the monitor over to say so.

        The owner should also arm its `MidiRouter`; this page has no MIDI of its
        own, and the two halves meet in `observeLearn`. Calling this again for
        another parameter abandons the first, which is what choosing `MIDI
        learn` twice should do.

        Puts up a modal window — "Move controller!", then "Release controller!"
        once something has moved — rather than taking the monitor over, so the
        incoming stream stays readable while the gesture is made.

        The name shown is looked up here rather than passed in: the page already
        holds the parameter list, and a caller supplying its own name would be a
        second place for it to go stale. */
    void beginLearn (int parameterIndex);

    /** One message the router collected while learning. Restarts the settling
        clock, so a gesture is however long the end-user keeps moving. */
    void observeLearn (const juce::MidiMessage& message);

    bool isLearning() const noexcept { return learner.isActive(); }

    /** Stops without adding anything. Also called by the destructor: learning
        is a modal act and closing the window should end it. */
    void cancelLearn();

    /** Told when learning ends, so the owner can disarm its router — with the
        mapping if one was learned, or nothing if it timed out or was cancelled.
        The row itself has already been added by then. */
    std::function<void (std::optional<controllers::Mapping>)> onLearnFinished;

    //==========================================================================
    //  Intent out.

    /** After any edit, insertion or removal. Read the result with
        `getMappings()`; the page keeps them, for the reason given on
        `ControllersTable`. */
    std::function<void()> onMappingsChanged;


    //==========================================================================
    /** The height below which the page cannot show its own minimum: the
        monitor, both frames, every button row, and `tableMinimumRows` of
        table.

        Each `section` is given its whole content block, internal gaps
        included — the button row under the table as well as the table, which
        an earlier version left out and so came up a row short. */
    static constexpr int getMinimumHeight() noexcept
    {
        return metrics::pageTopHeight (metrics::pageTopRows)  // the monitor
             + section (metrics::pageRowHeight)               // the three insert buttons
             + section (ControllersTable::getHeightForRows (metrics::tableMinimumRows)
                            + metrics::pageRowGap + metrics::pageRowHeight);
                                                              // table, then undo | redo | delete
    }

    void resized() override;
    void lookAndFeelChanged() override;

private:
    //==========================================================================
    /** Enables the two history buttons from what the table can actually do. */
    void refreshHistory();

    //==========================================================================
    //  Learning. Two clocks, following Mixxx's wizard: one waiting for anything
    //  at all, one waiting for the gesture to stop. Only ever one is running,
    //  so a single Timer serves both and `waitingForFirst` says which.

    void timerCallback() override;

    /** Writes the message stream to the field. Learning no longer takes the
        monitor over — it puts a window up instead, so the stream stays visible
        while a controller is being moved, which is the point. */
    void refreshMonitor();

    /** Called by `LearnWindow` on the message thread once its wait has ended,
        however it ended. The one place learning is torn down. */
    void learnWindowClosed (bool userPressedCancel);

    class LearnWindow;

    MidiLearner learner;

    /** Self-deleting, per `ThreadWithProgressWindow`'s convention, so this is a
        borrowed pointer cleared in `learnWindowClosed`. */
    LearnWindow* learnWindow = nullptr;

    bool waitingForFirst = false;

    /** A framed section: its title band, its content, the padding under it, and
        the row gaps that separate all three. */
    static constexpr int section (int contentHeight) noexcept
    {
        return metrics::pageGroupTitleHeight
             + contentHeight
             + metrics::pageGroupPadding
             + metrics::pageRowGap * 3;
    }

    juce::GroupComponent insertGroup, editGroup;

    /** The monitor: the newest message, as one line of text. A `ReadOutField`
        like every other read-only value in the module, rather than a table —
        one that never scrolls and never sorts is four columns of furniture
        around a single sentence. */
    ReadOutField monitor { "nothing yet" };

    /** Newest first. Kept here rather than in the field, which draws a string
        and knows nothing about messages. */
    juce::StringArray messages;

    /** One button per kind of row. `CC` makes the ordinary sort, with an MSB
        and maybe an LSB; the other two make rows that have no controller number
        at all and say so across those two columns instead.

        Literals rather than `controllers::sourceNames`, which the *cells* use:
        the cell for an ordinary row holds a number, so its entry there is
        "control" and would be the wrong word on a button that adds one. */
    juce::TextButton ccButton { "CC" },
                     aftertouchButton { "aftertouch" },
                     polytouchButton { "polytouch" };

    /** Undo and redo read as words at a third of the panel, so the arrows
        docs/controllers.md offers as a fallback are not needed. */
    juce::TextButton deleteButton { "delete" }, undoButton { "undo" },
                     redoButton { "redo" };

    ControllersTable table;


    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ControllersPage)
};

} // namespace microtonos::sidebar
