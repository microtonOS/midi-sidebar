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
    top, then `STATUS`, `FILES` and `META` as framed sections — the same shape
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
    void setIncludes (presets::Includes includes);

    void setSplitActive (bool isActive);
    void setLayer (presets::Layer layer);

    //==========================================================================
    //  Intent out.

    /** The end-user picked another preset from the name menu, or stepped the
        program or bank. */
    std::function<void (int)> onNameChosen;
    std::function<void (std::optional<int>)> onProgramChosen;
    std::function<void (std::optional<int>)> onBankChosen;

    std::function<void (bool)> onSplitToggled;
    std::function<void (presets::Layer)> onLayerChanged;
    std::function<void (presets::Includes)> onIncludesChanged;

    /** After an author or comment edit has been committed, never mid-keystroke. */
    std::function<void (presets::Meta)> onMetaEdited;

    std::function<void()> onLoadRequested;
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
        bank labels, their steppers, load/save, the include toggles, and
        author. */
    static constexpr int fixedRows = 8;
    static constexpr int groups    = 3;

    /** Every track has a gap after it except the last: the fixed rows, the
        comment, and each group's title and padding tracks. */
    static constexpr int rowGaps = fixedRows + 1 + groups * 2 - 1;

    void refreshNames();
    void commitMeta();

    //==========================================================================
    presets::Includes includes;
    presets::Status status;
    juce::StringArray availableNames;

    ReadOutField lowField, highField;

    juce::TextButton splitButton { "split" };
    ChoiceStrip layerStrip;

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

    juce::TextButton loadButton { "load" }, saveButton { "save" };
    juce::ToggleButton controllersToggle { "controllers" }, tuningToggle { "tuning" };

    juce::TextEditor authorEditor, commentEditor;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PresetsPage)
};

} // namespace microtonos::sidebar
