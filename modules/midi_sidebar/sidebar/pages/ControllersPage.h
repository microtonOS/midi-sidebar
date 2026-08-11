#pragma once

#include <optional>

#include <juce_gui_basics/juce_gui_basics.h>

#include "../SidebarLookAndFeel.h"
#include "../widgets/ChoiceButton.h"
#include "../widgets/ChoiceStrip.h"
#include "../widgets/ReadOutField.h"
#include "ControllersState.h"
#include "ControllersTable.h"
#include "PageGrid.h"

namespace microtonos::sidebar
{

//==============================================================================
/** The controllers page: what is arriving, and what it is mapped to.

    Implements docs/controllers.md. The newest message and the pitch-bend range
    at the top, then `FILES`, `MPE` and `EDITING` as framed sections, the same
    shape the tuning page uses.

    **Its height works the other way round from the tuning page.** There every
    row was fixed and a short panel cut the bottom off. Here everything but the
    editing table is fixed — two rows at the top, three rows of buttons, three
    frames — and the table is the single flexible track, so the page has a
    genuine minimum and simply grows into anything above it. That is the pattern
    the presets page should follow.
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

    /** The line the monitor shows, replacing whatever was there. Already
        formatted — see the note in ControllersState.h about why this module
        does not compose it. An empty string shows the placeholder. */
    void setMessage (const juce::String& message);

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
        return metrics::pageRowHeight                         // the monitor
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
    /** A framed section: its title band, its content, the padding under it, and
        the row gaps that separate all three. */
    static constexpr int section (int contentHeight) noexcept
    {
        return metrics::pageGroupTitleHeight
             + contentHeight
             + metrics::pageGroupPadding
             + metrics::pageRowGap * 3;
    }

    /** Commits the typed range on Return and on losing focus, rejecting
        anything the plugin could not act on — the tuning page's modulo divisor
        works the same way. */
    void applyPitchBendCents();

    /** Sends the zone out and refreshes what the two menus offer. */
    void mpeChanged();

    juce::GroupComponent filesGroup, mpeGroup, editingGroup;

    /** The monitor: the newest message, as one line of text. A `ReadOutField`
        like every other read-only value in the module, rather than a table —
        one that never scrolls and never sorts is four columns of furniture
        around a single sentence. */
    ReadOutField monitor { "nothing yet" };

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
