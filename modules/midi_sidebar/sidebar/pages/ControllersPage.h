#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include "../SidebarLookAndFeel.h"
#include "ControllersState.h"
#include "ControllersTable.h"
#include "PageGrid.h"

namespace microtonos::sidebar
{

//==============================================================================
/** The controllers page: what is arriving, and what it is mapped to.

    Implements docs/controllers.md. A monitor of the last few MIDI messages at
    the top, then `FILES` and `EDITING` as framed sections, the same shape the
    tuning page uses.

    **Its height works the other way round from the tuning page.** There every
    row was fixed and a short panel cut the bottom off. Here everything but the
    editing table is fixed — three monitor rows, two rows of buttons, two frames
    — and the table is the single flexible track, so the page has a genuine
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

    void setMappings (juce::Array<controllers::Mapping> mappings);
    const juce::Array<controllers::Mapping>& getMappings() const noexcept;

    /** Pushes one message onto the monitor, dropping the oldest. This is the
        call an owner watching a MIDI stream actually wants. */
    void addMessage (controllers::Message message);

    /** Replaces the whole monitor at once, which is what a demo or a restored
        session does. Anything past `controllers::monitorRows` is ignored. */
    void setMessages (juce::Array<controllers::Message> messages);

    //==========================================================================
    //  Intent out.

    /** After any edit, insertion or removal. Read the result with
        `getMappings()`; the page keeps them, for the reason given on
        `ControllersTable`. */
    std::function<void()> onMappingsChanged;

    std::function<void()> onLoadRequested;
    std::function<void()> onSaveRequested;

    //==========================================================================
    /** The height below which the page cannot show its own minimum: the monitor,
        both frames, both button rows, and `tableMinimumRows` of table. */
    static constexpr int getMinimumHeight() noexcept
    {
        return monitorHeight()
             + section (ControllersTable::getHeightForRows (metrics::tableMinimumRows))
             + section (metrics::pageRowHeight);
    }

    void resized() override;
    void lookAndFeelChanged() override;

private:
    //==========================================================================
    /** Three rows, no header, and it never scrolls — so its height is simply
        its rows. */
    static constexpr int monitorHeight() noexcept
    {
        return controllers::monitorRows * metrics::tableRowHeight;
    }

    /** A framed section: its title band, its content, the padding under it, and
        the row gaps that separate all three. */
    static constexpr int section (int contentHeight) noexcept
    {
        return metrics::pageGroupTitleHeight
             + contentHeight
             + metrics::pageGroupPadding
             + metrics::pageRowGap * 3;
    }

    /** The monitor. A `TableListBox` because docs/controllers.md says the inner
        tables are real tables, even though this one neither scrolls nor sorts. */
    class Monitor;

    std::unique_ptr<Monitor> monitor;

    juce::GroupComponent filesGroup, editingGroup;

    juce::TextButton loadButton { "load" }, saveButton { "save" };
    juce::TextButton addButton  { "add" },  removeButton { "remove" };

    ControllersTable table;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ControllersPage)
};

} // namespace microtonos::sidebar
