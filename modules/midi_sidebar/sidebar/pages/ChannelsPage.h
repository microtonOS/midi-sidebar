#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include "../SidebarLookAndFeel.h"
#include "../widgets/ChannelGrid.h"
#include "../widgets/ChoiceStrip.h"
#include "../widgets/ReadOutField.h"
#include "ChannelsState.h"
#include "PageGrid.h"

namespace microtonos::sidebar
{

//==============================================================================
/** The channels page: what the plugin listens to, and how it treats it.

    Implements docs/channels.md. Two rows of switches at the top, then a
    `FILTER` section holding the sixteen channels and the two buttons under
    them.

    **The first switch chooses a view, not a value.** `omni` or `MPE` says which
    of the two settings the rest of the page is showing; `on` or `off` then
    belongs to whichever that is. The two are independent — a zone over channels
    1 to 9 leaves omni free to do what it likes with 10 to 16 — so switching the
    view never disturbs what the other one holds.

    **The two views are graphically independent.** Neither shows the other's
    state greyed behind it, though functionally they overlap on whatever the
    zone has claimed. That is a deliberate simplification: the cross-hatching
    needed to show one setting through the other costs more legibility than it
    buys, and flipping between the two views is one click.

    **Under MPE the grid sets the zone's extent.** A zone runs contiguously from
    one end, so clicking 8 in a lower zone gives 1 to 8. Under omni the same
    grid is a free selection, and the two buttons below change with it: `select
    all` / `mute all` for omni, `lower zone` / `upper zone` for MPE.

    Holds no MIDI and silences nothing: values in, intent out, like every other
    page.
*/
class ChannelsPage final : public juce::Component
{
public:
    ChannelsPage();

    //==========================================================================
    /** Everything at once — the two settings are read together, so they are set
        together. */
    void setSetup (channels::Setup setup);
    const channels::Setup& getSetup() const noexcept { return setup; }

    /** Whenever any of it changes. */
    std::function<void (channels::Setup)> onSetupChanged;

    //==========================================================================
    /** The height this page needs. Summed from the rows rather than measured
        once and written down, so a changed row height cannot leave it stale. */
    static constexpr int getNaturalHeight() noexcept
    {
        constexpr int tracks = 4;   // the top block, the title, the grid, the padding

        return metrics::pageTopHeight (metrics::pageTopRows)
             + metrics::pageGroupTitleHeight
             + metrics::channelGridHeight
             + metrics::pageRowHeight              // the two context buttons
             + metrics::pageGroupPadding
             + tracks * metrics::pageRowGap;
    }

    void resized() override;
    void lookAndFeelChanged() override;

private:
    //==========================================================================
    /** Shows whichever setting the view switch is on. */
    void refresh();

    /** Announces the whole setup. */
    void announce();

    /** What a click on channel `index` means, which depends on the view. */
    void channelClicked (int channelIndex);

    void showBendEditor (int channelIndex);

    //==========================================================================
    /** What sits inside the pitch-bend call-out: which channels are being set,
        and the range in cents.

        A speech bubble rather than a dialog, matching the volume fader's — the
        setting belongs to the channel that was clicked, and a bubble pointing at
        that channel says so without a sentence. The title is *not* repeated
        inside: the button that opened it is still on screen and still lit.
    */
    class BendBubble final : public juce::Component
    {
    public:
        BendBubble()
        {
            targetLabel.setJustificationType (juce::Justification::centredLeft);
            addAndMakeVisible (targetLabel);

            prepareNumericEditor (centsEditor, false);   // cents are whole numbers

            // Return commits *and* closes: the value is the whole of what the
            // bubble is for, so there is nothing left to look at.
            centsEditor.onReturnKey = [this]
            {
                commit();

                if (auto* box = findParentComponentOfClass<juce::CallOutBox>())
                    box->dismiss();
            };

            // Losing focus commits but does **not** close. Dismissing here would
            // make the bubble destroy itself the moment it appeared: a call-out
            // moves focus about as it opens, and that counts as losing it.
            centsEditor.onFocusLost = [this] { commit(); };

            addAndMakeVisible (centsEditor);

            setSize (metrics::bendBubbleWidth,
                     metrics::pageRowHeight * 2 + metrics::pageRowGap);
        }

        /** Which channels this is about, and what they are set to now. */
        void describe (const juce::String& description, int cents)
        {
            targetLabel.setText (description, juce::dontSendNotification);
            centsEditor.setText (juce::String (cents) + " c", juce::dontSendNotification);
        }

        std::function<void (int cents)> onCommit;

        void resized() override
        {
            auto area = getLocalBounds();

            targetLabel.setBounds (area.removeFromTop (metrics::pageRowHeight));
            area.removeFromTop (metrics::pageRowGap);
            centsEditor.setBounds (area.removeFromTop (metrics::pageRowHeight));
        }

        void lookAndFeelChanged() override
        {
            const auto font = SidebarLookAndFeel::font (metrics::bodyFontHeight);

            targetLabel.setFont (font);
            centsEditor.setFont (font);

            // TextEditor bakes the colour into each run as it is inserted, so
            // text set before this reached a styled parent keeps the default
            // LookAndFeel's — the same trap the presets page answers.
            centsEditor.applyColourToAllText (findColour (juce::TextEditor::textColourId), true);
        }

        void parentHierarchyChanged() override { lookAndFeelChanged(); }

    private:
        /** Idempotent: it reports what is typed, so being called twice — once
            on Return and again as focus leaves — sets the same value twice. */
        void commit()
        {
            if (onCommit != nullptr)
                onCommit (centsEditor.getText().getIntValue());
        }

        juce::Label targetLabel;
        juce::TextEditor centsEditor;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BendBubble)
    };

    bool showingMpe() const noexcept;

    channels::Setup setup;

    juce::GroupComponent filterGroup;

    /** Which setting is showing, and whether it is on. The second reads and
        writes whichever the first names. */
    ChoiceStrip settingStrip, enabledStrip;

    ChannelGrid channelGrid;

    /** The row under the grid, whose pair follows the view. Four plain buttons
        sharing the row rather than two strips: `lower zone` and `upper zone`
        *are* a state, but the grid above already shows which one is in force —
        channels 1 to 9 lit reads as a lower zone without a second thing saying
        so — and a highlight that merely repeats the matrix is noise. So all
        four behave as commands. */
    juce::TextButton selectAllButton, muteAllButton;
    juce::TextButton lowerZoneButton, upperZoneButton;

    /** Latching, and to the right of both switches because it belongs to
        neither: pitch-bend sensitivity is set per channel whether the page is
        showing omni or MPE. While it is on, clicking a channel opens its range
        rather than selecting it. Two lines of text in a narrow button, which
        `drawButtonText` already wraps to. */
    juce::TextButton bendButton;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ChannelsPage)
};

} // namespace microtonos::sidebar
