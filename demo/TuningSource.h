#pragma once

#include <midi_sidebar/midi_sidebar.h>

#include "libMTSClient.h"

namespace microtonos::sidebar::demo
{

//==============================================================================
/** Where the tuning comes from, for whichever scheme is selected.

    The module's `sidebar/tuning/` headers are pure — they parse and infer and
    hold a table, and know nothing about files, timers or third-party libraries.
    This is the other half: it owns the *sources*, and pushes a `TuningTable` and
    a `tuning::Status` at whoever asks.

    **Only `mtsEsp` uses ODDSound's client.** `MTSClient::freq()` opens with
    `if (!global.isOnline()) return localTunings[note].freq`, so a connected
    master always wins — which means the same client cannot also serve the
    `midi1` scheme, where docs/tuning.md says the master must be ignored.
    `mtsSysex` therefore goes through `mtsSysex::parse` and a table of our own.
    See MtsSysex.h.

    Lives in the demo rather than the module because `libMTSClient` dlopens a
    system-installed dynamic library. That is a deployment condition a reusable
    module should not impose on whoever consumes it; a host integration belongs
    on this side of the seam.
*/
class TuningSource
{
public:
    TuningSource();
    ~TuningSource();

    //==========================================================================
    /** Which source is in force. Changing it does not discard the others: the
        sysex table and the loaded files stay put, so toggling back restores
        what was set up, which is what TuningState.h says the owner should do. */
    void setScheme (tuning::Scheme newScheme);
    tuning::Scheme getScheme() const noexcept { return scheme; }

    //==========================================================================
    /** Feeds a tuning system exclusive to the `midi1` table. Ignored under
        any other scheme — the message is still consumed by the router, since it
        was addressed to us, but a scheme that is not listening does not store
        it. Message thread. */
    void handleSysex (const juce::MidiMessage& message);

    /** Registered Parameter Numbers this reads: 0/3 and 0/4 select the tuning
        program and bank, and 0/1 and 0/2 are Channel Fine and Coarse Tuning.
        Returns true if it recognised the parameter, so the caller knows whether
        anything changed. */
    bool handleRpn (const juce::MidiRPNMessage& rpn);

    /** Master Fine and Coarse Tuning, from the two Device Control system
        exclusives. Global, where the RPNs above are per channel. */
    void setMasterFineCents (double cents);
    void setMasterCoarseCents (double cents);

    //==========================================================================
    /** Loads a selection of `.scl` and `.kbm` files as one or more programs,
        and switches to `scala`. */
    void loadFiles (const juce::Array<juce::File>& files);

    /** Loads every tuning file in a directory as a bank. Several directories is
        several banks, so this appends rather than replaces. */
    void loadBank (const juce::File& directory);

    void clearFiles();

    /** What the load button shows: how many programs and banks are loaded. */
    juce::String loadedSummary() const;

    //==========================================================================
    /** Steps to another program or bank, which the page's steppers ask for.
        Only the schemes with programs answer — MTS sysex and tuning files. */
    void setProgram (std::optional<int> program);
    void setBank (std::optional<int> bank);

    /** Every program name the current scheme can offer, for the name menu. */
    juce::StringArray availableNames() const;
    void chooseName (int index);

    //==========================================================================
    /** Recomputed from whichever source is live. Cheap enough to call from a
        timer: MTS-ESP is 128 queries and everything else is already a table. */
    void refresh();

    const TuningTable& getTable() const noexcept { return table; }
    tuning::Status     getStatus() const;

    /** The period, `specified` where the source stated one and `inferred`
        otherwise. See PeriodInference.h for why the two are different things. */
    tuning::Period getPeriod() const;

    /** Whether an MTS-ESP master is connected, which the status block shows as
        a live connection rather than as a name. */
    bool hasMaster() const;

    //==========================================================================
    /** How far this channel is displaced from where the table puts it, in
        cents: master fine, master coarse, channel fine and channel coarse, added
        up. CA-025 says exactly that — "the total displacement in cents from A440
        for each MIDI channel is summation of the displacement of this Master
        Fine Tuning and the displacement of Fine Tuning using RPN", and the same
        for coarse. */
    double tuningOffsetCents (int channel) const;

    /** The frequency actually sounding for a note: the table's, displaced.

        Kept apart from `TuningTable` on purpose. A displacement is not part of a
        tuning — the same table sounds at a different pitch under a different
        master tuning — and baking it in would also lose precision, since RPN 01
        steps by 0.0122 c where the table's own frequencies step by 0.0061 c. */
    std::optional<double> frequencyFor (int note, int channel) const;

    /** The interval between the lowest and highest sounding notes, in cents,
        looked up through the current table so it is the *tuned* interval rather
        than the twelve-tone one. Empty when nothing is held. */
    std::optional<double> intervalFor (int lowestNote, int highestNote,
                                       int lowestChannel, int highestChannel) const;

private:
    //==========================================================================
    /** Pulls all 128 notes per channel out of the MTS-ESP client. */
    void refreshFromMtsEsp();

    /** The programs the current scheme offers, or an empty list. */
    const juce::Array<scalaFiles::Program>* filePrograms() const;

    tuning::Scheme scheme = tuning::Scheme::mtsEsp;

    /** The resolved table for the current scheme — what `getTable` hands out. */
    TuningTable table;

    //==========================================================================
    //  One store per scheme, so switching does not destroy anything.

    /** Filled by `mtsSysex::apply`. Ours, and independent of the client below. */
    TuningTable sysexTable;
    juce::String sysexName;

    /** `.scl`/`.kbm` programs, in load order. A bank is a contiguous run of
        them; `bankStarts` records where each begins. */
    juce::Array<scalaFiles::Program> programs;
    juce::Array<int> bankStarts;

    /** Which program and bank each scheme is on. Kept per scheme because the
        numbers mean different things: a sysex program is one of 128 slots the
        wire names, a file program is an index into `programs`. */
    std::optional<int> sysexProgram, sysexBank;
    int fileProgram = 0;

    //==========================================================================
    //  Displacements from A440, in cents. Master is global, channel is per
    //  channel, and the four are summed — CA-025's rule.

    double masterFineCents = 0.0, masterCoarseCents = 0.0;
    std::array<double, 16> channelFineCents {}, channelCoarseCents {};

    /** ODDSound's client, registered once for the life of the plugin.
        `MTS_RegisterClient` is safe with no libMTS installed — everything then
        reports "no master" and the scheme falls back to standard tuning. */
    MTSClient* client = nullptr;

    juce::Time updated;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TuningSource)
};

} // namespace microtonos::sidebar::demo
