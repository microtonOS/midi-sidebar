#pragma once

#include <optional>

#include <juce_gui_basics/juce_gui_basics.h>

#include "../SidebarLookAndFeel.h"
#include "../widgets/ChoiceButton.h"
#include "../widgets/ChoiceStrip.h"
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

    /** Pitch-bend range in cents, clamped to
        `controllers::highestPitchBendCents`. */
    void setPitchBendCents (int cents);

    /** Whether the plugin is reading an MPE zone, and which channels it
        covers. */
    void setMpe (controllers::Mpe mpe);
    const controllers::Mpe& getMpe() const noexcept { return mpe; }

    //==========================================================================
    //  Intent out.

    /** After any edit, insertion or removal. Read the result with
        `getMappings()`; the page keeps them, for the reason given on
        `ControllersTable`. */
    std::function<void()> onMappingsChanged;

    std::function<void()> onLoadRequested;
    std::function<void()> onSaveRequested;

    std::function<void (int)> onPitchBendCentsChosen;

    /** The MPE zone changed. The owner is expected to hand the channels it
        covers — `controllers::channelsCoveredBy` — to the tuning page, which
        cannot tune them separately; see docs/controllers.md. */
    std::function<void (controllers::Mpe)> onMpeChanged;

    //==========================================================================
    /** The height below which the page cannot show its own minimum: the
        monitor, the pitch-bend row, all three frames, every button row, and
        `tableMinimumRows` of table.

        Each `section` is given its whole content block, internal gaps included.
        The `add | remove` row used to be left out of it, which made this about
        a row short — harmless, since the flexible track only shrank, but it
        meant the number did not mean what it says. */
    static constexpr int getMinimumHeight() noexcept
    {
        return monitorHeight()
             + metrics::pageRowGap + metrics::pageRowHeight   // PB sensitivity
             + section (metrics::pageRowHeight)               // load | save
             + section (metrics::pageRowHeight)               // the MPE zone
             + section (ControllersTable::getHeightForRows (metrics::tableMinimumRows)
                            + 2 * (metrics::pageRowGap + metrics::pageRowHeight));
                                                              // table, add | remove, aftertouch | polytouch
    }

    void resized() override;
    void lookAndFeelChanged() override;

private:
    //==========================================================================
    /** One row, no header, and it never scrolls — so its height is simply that
        row. */
    static constexpr int monitorHeight() noexcept
    {
        return controllers::monitorRows * metrics::tableRowHeight;
    }

    /** Opens the call-out holding the rest of the history. */
    void showHistory();

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

    /** Commits the typed range on Return and on losing focus, rejecting
        anything the plugin could not act on — the tuning page's modulo divisor
        works the same way. */
    void applyPitchBendCents();

    /** Sends the zone out and refreshes what the two menus offer. */
    void mpeChanged();

    juce::GroupComponent filesGroup, mpeGroup, editingGroup;

    //  Pitch bend, at the top with the monitor and unframed: the sketch gives
    //  it no title, and on these pages a GroupComponent is what a *named*
    //  section looks like.
    juce::Label pitchBendLabel;
    juce::TextEditor pitchBendEditor;

    juce::TextButton loadButton { "load" }, saveButton { "save" };

    //  The MPE zone: on or off, and the channels it spans.
    ChoiceStrip mpeStrip;
    juce::Label fromLabel, toLabel;
    ChoiceButton masterButton { "mpe master" }, lastButton { "mpe last" };

    juce::TextButton addButton { "add" }, removeButton { "remove" };

    /** The two message types that cannot be described by a controller number,
        so they are added as rows of their own rather than typed into one. */
    juce::TextButton aftertouchButton { "aftertouch" }, polytouchButton { "polytouch" };

    ControllersTable table;

    int pitchBendCents = controllers::defaultPitchBendCents;
    controllers::Mpe mpe;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ControllersPage)
};

} // namespace microtonos::sidebar
