#pragma once

#include <optional>

#include <juce_gui_basics/juce_gui_basics.h>

#include "../SidebarLookAndFeel.h"
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

    **No files section.** A preset carries the whole state, mappings included,
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
class ControllersPage final : public juce::Component
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
    const juce::Array<controllers::Mapping>& getMappings() const noexcept;

    /** Selects every row mapped to `parameterIndex`, scrolling the first into
        view. What the right-click menu's "view in sidebar" does once the
        sidebar has opened this page; see ParameterMenu. */
    void showMappingsFor (int parameterIndex);

    /** Removes the most recently added mapping for `parameterIndex` — the
        right-click menu's "unlearn". */
    void removeLatestMappingFor (int parameterIndex);

    /** Pushes one line onto the monitor, dropping the oldest. Already
        formatted — see the note in ControllersState.h about why this module
        does not compose it. This is the call an owner watching a MIDI stream
        wants. */
    void addMessage (const juce::String& message);

    /** Replaces the whole monitor at once, newest first, which is what a demo
        or a restored session does. Anything past `controllers::monitorLines` is
        ignored, and an empty list shows the placeholder. */
    void setMessages (juce::StringArray messages);

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
        at all and say so across those two columns instead. */
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
