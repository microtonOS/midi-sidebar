#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include "../SidebarLookAndFeel.h"
#include "../widgets/ChoiceButton.h"
#include "../widgets/ChoiceStrip.h"
#include "../widgets/NumberStepper.h"
#include "../widgets/ReadOutField.h"
#include "PageGrid.h"
#include "PresetsState.h"

namespace microtonos::sidebar
{

//==============================================================================
/** The presets page: what is sounding, which preset it came from, and what
    travels with it.

    Implements docs/presets.md. The frequency pair and the split control at the
    top, then `STATUS`, `FILE` and `META` as framed sections — the same shape
    the other two pages use.

    Height follows the controllers page rather than the tuning page: everything
    is a fixed row except the comment box, which takes whatever is left. So the
    page has a genuine minimum and grows into anything above it.

    Values are pushed in and intent goes out, with one exception: the two `META`
    editors hold their own text while it is being typed, for the reason given on
    `ControllersTable` — an editor cannot be rebuilt from outside on every
    keystroke.
*/
class PresetsPage final : public juce::Component
{
public:
    PresetsPage();
    ~PresetsPage() override;

    //==========================================================================
    //  Values in.

    void setFrequencies (presets::Frequencies frequencies);
    void setStatus (presets::Status newStatus);

    /** The presets the name menu offers. The current one is added if it is not
        among them, so the button always shows what is loaded. */
    void setAvailableNames (juce::StringArray names);
    void setMeta (presets::Meta meta);

    void setSplitActive (bool isActive);
    void setLayer (presets::Layer layer);

    /** Whether anything is sounding, which is what the `split` button acts on.

        With nothing held it sets the split from the two frequency boxes; with
        notes held there is a second reading — take the split from what is
        sounding — and the button says `update?` to ask which one was meant. The
        page cannot know this for itself, so the owner pushes it in. */
    void setNotesActive (bool anyNotesActive);

    //==========================================================================
    //  Intent out.

    /** The end-user picked another preset from the name menu, or stepped the
        program or bank. */
    std::function<void (int)> onNameChosen;
    std::function<void (std::optional<int>)> onProgramChosen;
    std::function<void (std::optional<int>)> onBankChosen;

    /** The `active` button: whether the split is active at all. */
    std::function<void (bool)> onSplitToggled;

    /** The `split` button: set the split point. The flag is what the button was
        showing when it was pressed — false for `split`, true for `update?` —
        so the owner knows whether it was asked to take the point from the
        frequency boxes or from the notes being held. */
    std::function<void (bool fromSoundingNotes)> onSplitPointRequested;

    std::function<void (presets::Layer)> onLayerChanged;

    /** After an author or comment edit has been committed, never mid-keystroke. */
    std::function<void (presets::Meta)> onMetaEdited;

    std::function<void()> onOpenRequested;
    std::function<void()> onSaveRequested;

    //==========================================================================
    /** The height below which the page cannot show its own minimum: every fixed
        row, all three frames, and `commentMinimumRows` of comment box. */
    static constexpr int getMinimumHeight() noexcept
    {
        return fixedRows * metrics::pageRowHeight
             + rowGaps * metrics::pageRowGap
             + groups * (metrics::pageGroupTitleHeight + metrics::pageGroupPadding)
             + metrics::commentMinimumRows * metrics::pageRowHeight;
    }

    void resized() override;
    void lookAndFeelChanged() override;

private:
    //==========================================================================
    /** Rows that never change height: frequencies, split, name, the program and
        bank labels, their steppers, open/save, and author. One fewer since the
        include toggles went: a preset carries the whole state, so there was
        nothing left to choose. */
    static constexpr int fixedRows = 7;
    static constexpr int groups    = 3;

    /** Every track has a gap after it except the last: the fixed rows, the
        comment, and each group's title and padding tracks. */
    static constexpr int rowGaps = fixedRows + 1 + groups * 2 - 1;

    void refreshNames();
    void commitMeta();

    //==========================================================================
    presets::Status status;
    juce::StringArray availableNames;

    ReadOutField lowField, highField;

    /** Two buttons, one under the other, and the distinction is worth keeping
        straight because both used to be one control.

        `split` is an *action*: it sets the split point, and reads `update?`
        while notes are held. It sits on the frequency row because that row is
        the split point — the button and the two numbers are one statement.

        `active` is the *state*: whether the split applies at all. It sits on the
        layer row because that row is what the split is doing — which side is
        live, and whether it is live in the first place. */
    juce::TextButton splitButton { "split" }, activeButton { "active" };
    ChoiceStrip layerStrip;

    bool notesActive = false;

    /** Declared before the widgets that sit inside them: a group is only a
        frame, and what draws on top is decided by the order children were
        added. */
    juce::GroupComponent statusGroup, filesGroup, metaGroup;

    /** The name opens a menu of the presets on offer; program and bank are
        stepped, with their labels above them — inc/dec buttons and a label side
        by side do not fit half a page. */
    ChoiceButton nameButton { "preset name" };
    NumberStepper programStepper { "program", metrics::highestProgram };
    NumberStepper bankStepper { "bank", metrics::highestBank };
    juce::Label programLabel, bankLabel, authorLabel, commentLabel;

    juce::TextButton openButton { "open" }, saveButton { "save" };

    juce::TextEditor authorEditor, commentEditor;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PresetsPage)
};

} // namespace microtonos::sidebar
