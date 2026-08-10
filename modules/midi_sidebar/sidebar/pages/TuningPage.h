#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include "../PopupHost.h"
#include "../SidebarLookAndFeel.h"
#include "../widgets/ChoiceButton.h"
#include "../widgets/ChoiceStrip.h"
#include "../widgets/NumberStepper.h"
#include "../widgets/ReadOutField.h"
#include "PageGrid.h"
#include "TuningState.h"

namespace microtonos::sidebar
{

//==============================================================================
/** The tuning page: what the plugin is currently tuned to, and how it is being
    told.

    Implements docs/tuning.md. Four sections, top to bottom: the interval
    between the sounding notes, the status of the tuning, its period, and the
    settings that decide where a tuning comes from.

    **It holds no tuning of its own.** Values are pushed in with `setInterval`,
    `setStatus` and `setPeriod`; everything the end-user does leaves through a
    callback. So the page can be built and looked at before any MIDI exists, and
    does not have to change when it does — the only state it keeps is what it
    would otherwise have to ask for back, such as which scheme is selected.
*/
class TuningPage final : public juce::Component
{
public:
    TuningPage();
    ~TuningPage() override;

    //==========================================================================
    //  Values in.

    void setInterval (const tuning::Interval& newInterval);
    void setStatus   (const tuning::Status& newStatus);
    void setPeriod   (const tuning::Period& newPeriod);

    /** The tunings the name menu offers. The current one is added if it is not
        among them, so the button always shows what is loaded even before
        anything has supplied a list. */
    void setAvailableNames (juce::StringArray names);

    void setScheme      (tuning::Scheme scheme);
    void setUpdateMode  (tuning::UpdateMode mode);
    void setChannels    (bool omniOn, tuning::ChannelMask mask);

    /** Shown on the two file buttons. Empty means nothing is loaded, which the
        buttons draw as a prompt rather than as a filename. */
    void setScaleFileName   (const juce::String& name);
    void setMappingSummary  (const juce::String& summary);

    //==========================================================================
    //  Intent out. None of these change the page; the owner is expected to act
    //  and push the result back in, so that what is on screen is always what
    //  the plugin actually has.

    /** The end-user picked another tuning from the name menu, or stepped the
        program or bank. Nothing here changes the page: the owner acts and
        pushes the result back through `setStatus`. */
    std::function<void (int)> onNameChosen;
    std::function<void (std::optional<int>)> onProgramChosen;
    std::function<void (std::optional<int>)> onBankChosen;

    std::function<void (tuning::Scheme)>     onSchemeChanged;
    std::function<void (tuning::UpdateMode)> onUpdateModeChanged;
    std::function<void (bool, tuning::ChannelMask)> onChannelsChanged;
    std::function<void (double)> onModDivisorChanged;

    /** The period the end-user stepped to, in cents — always one of the
        candidates handed in by `setPeriod`, never a value they invented. */
    std::function<void (double)> onPeriodChosen;

    std::function<void()> onScaleFileRequested;
    std::function<void()> onMappingFilesRequested;

    //==========================================================================
    /** The height this page needs to show everything.

        Summed from the sections rather than measured once and written down, so
        a changed row height or gap cannot leave the number stale. The panel
        does not always have this much — see the note in docs/tuning.md about
        small heights — and this is the number that decision will need.
    */
    static constexpr int getNaturalHeight() noexcept
    {
        // Counted from the grid `resized` builds: nine rows of content, and for
        // each of the three groups a title band above its rows and a padding
        // track below them.
        constexpr int contentRows = 10;     // interval, mod, name, the program
                                            // and bank labels, their steppers,
                                            // updated, period, scheme, and the
                                            // two rows the load buttons and the
                                            // update choices share
        constexpr int groups = 3;           // status, period, settings
        constexpr int tracks = contentRows + groups * 2;

        return contentRows * metrics::pageRowHeight
             + groups * (metrics::pageGroupTitleHeight + metrics::pageGroupPadding)
             + (tracks - 1) * metrics::pageRowGap;
    }

    void resized() override;
    void lookAndFeelChanged() override;

private:
    //==========================================================================
    void showChannelSelector();
    void refreshNames();
    void refreshInterval();
    void refreshPeriod();

    /** The modulo divisor commits on Return and on losing focus, and rejects
        anything that is not a number, so it cannot leave the page showing
        something the plugin does not have. */
    void applyModDivisor();

    //==========================================================================
    tuning::Interval interval;
    tuning::Status   status;
    tuning::Period   period;

    bool omni = false;
    tuning::ChannelMask channelMask = tuning::allChannels;

    /** The labels naming a field beside or above it. Collected so that the one
        thing they share — font, colour, alignment — is applied in a loop rather
        than six times. */
    juce::Label modLabel, equalsLabel, programLabel, bankLabel, updatedLabel;

    juce::StringArray availableNames;

    //  Interval section.
    ReadOutField intervalField;
    juce::TextEditor modEditor;
    ReadOutField modResultField;

    /** The three framed sections. Declared before every widget that sits inside
        one, because a `GroupComponent` is only a frame — its contents are not
        its children but the page's, laid out in the page's own grid — and the
        drawing order is the order the children were added. Anything added
        after these draws on top of them, which is the whole arrangement.

        The interval and modulo at the top have no frame, as specified. */
    juce::GroupComponent statusGroup, periodGroup, settingsGroup;

    //  Status section. The name opens a menu of the tunings on offer; program
    //  and bank are stepped rather than read, with their labels above them —
    //  inc/dec buttons and a label side by side do not fit half a page.
    ChoiceButton nameButton { "tuning name" };
    NumberStepper programStepper { "program", metrics::highestProgram };
    NumberStepper bankStepper { "bank", metrics::highestBank };
    ReadOutField updatedField;

    //  Period section. The chooser steps through `choices` rather than holding
    //  a number of its own, so the end-user can only ever land on a period the
    //  plugin offered; its value is an index into that list.
    juce::Slider periodChooser;
    juce::Array<double> choices;
    ReadOutField periodSourceField;

    //  Settings section. A ChoiceButton rather than a ComboBox, so the scheme
    //  menu matches the ones in the controllers table — see ChoiceButton.
    ChoiceButton schemeButton { "scheme" };
    juce::TextButton channelsButton { "channels" };
    juce::TextButton scaleButton, mapButton;
    ChoiceStrip updateStrip;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TuningPage)
};

} // namespace microtonos::sidebar
