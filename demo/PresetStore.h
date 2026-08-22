#pragma once

#include <midi_sidebar/midi_sidebar.h>

#include <juce_audio_processors/juce_audio_processors.h>

namespace microtonos::sidebar::demo
{

//==============================================================================
/** The presets the plugin can be on, and the file format they travel in.

    The presets counterpart of `TuningSource`, and the same division of labour:
    the module's `presets::` state says what the page *shows*, and this owns
    where it comes from — a store in memory, a `.xml` on disk, a directory read
    as a bank.

    **A preset is the plugin's parameters, and only those.** It was briefly the
    whole APVTS state, which was wrong in a way that showed immediately: the
    sidebar's own settings live in the same tree — which page is open, the panel
    width, the theme — so loading a preset yanked the GUI to whatever page had
    been open when the preset was made. A preset is a sound, not a window.

    So the caller names the parameters that constitute one, and nothing else is
    touched. Loading sets each of them rather than calling `replaceState`, which
    would have to put *something* in the fields it does not own.

    Message thread only — it reads files and builds strings.
*/
class PresetStore
{
public:
    //==========================================================================
    /** One preset: what it is called, what it sets, and what its author said. */
    struct Preset
    {
        juce::String name;
        presets::Meta meta;

        /** One property per named parameter, holding its normalised value.

            Flat and self-describing, so the `.xml` on disk is readable and a
            parameter that no longer exists is simply skipped on load rather
            than being an error. */
        juce::ValueTree state;
    };

    /** @param stateToUse     where the parameters live
        @param presetParameters  the ids a preset consists of — the plugin's own,
                                 not the sidebar's settings */
    PresetStore (juce::AudioProcessorValueTreeState& stateToUse,
                 juce::StringArray presetParameters);

    //==========================================================================
    //  Navigation.

    /** Program and bank as MIDI names them, 0-based on the wire.

        A bank is a *run* of programs here, the same shape `scalaFiles` uses for
        tuning banks, so a program number indexes within its bank rather than
        across the whole store. */
    void setProgram (int program);
    void setBank (int bank);

    /** A program change with the bank that came with it — the router reports
        the two as one event, because "bank select alone must not change the
        program". An empty bank means stay on the current one. */
    void handleProgramChange (int program, std::optional<int> bank);

    /** The name menu's entries, and choosing one. Indices are into the *current
        bank*, matching what the menu shows. */
    juce::StringArray availableNames() const;
    void chooseName (int index);

    //==========================================================================
    //  Files.

    /** Reads one `.xml`, appending it to the current bank and loading it. */
    bool loadFile (const juce::File& file);

    /** Reads every `.xml` in a directory as a new bank, which is what
        "opening a directory of preset files interprets the directory as a bank"
        asks for. Several directories is several banks, so this appends. */
    bool loadBank (const juce::File& directory);

    /** Writes the live state under `name`, metadata included, and adopts it as
        the loaded preset so the edited marker clears. */
    bool save (const juce::File& file, const juce::String& name);

    /** Appends the live state as a preset without writing a file.

        What `save` does minus the file, and what a plugin shipping factory
        presets wants. The demo uses it to start with something in the store, so
        the page shows real presets moving through the real path rather than
        values pushed straight at it. */
    void addFromCurrentState (const juce::String& name, presets::Meta presetMeta = {});

    //==========================================================================
    //  What the page shows.

    presets::Status getStatus() const;
    presets::Meta   getMeta() const noexcept { return meta; }

    void setMeta (presets::Meta newMeta);

    /** True when the live state differs from the preset that was loaded, which
        is the `*` docs/presets.md asks for. Compared against the loaded copy
        rather than tracked by a dirty flag, so an edit that is undone stops
        counting as one. */
    bool isEdited() const;

private:
    //==========================================================================
    /** Where the current bank starts and how long it is. */
    int bankStart() const;
    int bankSize() const;

    /** Applies a preset to the processor, and remembers it as loaded. */
    void load (int index);

    /** The named parameters as they stand, and the reverse. */
    juce::ValueTree captureState() const;
    void applyState (const juce::ValueTree& tree);

    juce::AudioProcessorValueTreeState& apvts;

    /** What a preset is made of. Everything outside this list is the host's or
        the sidebar's, and a preset never writes to it. */
    juce::StringArray parameterIds;

    juce::Array<Preset> presets;

    /** Index into `presets` where each bank begins. Always has at least one
        entry, so "the current bank" is never a special case. */
    juce::Array<int> bankStarts { 0 };

    int currentBank = 0;
    int currentProgram = 0;

    /** The state as loaded, for the edited comparison. */
    juce::ValueTree loadedState;

    presets::Meta meta;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PresetStore)
};

} // namespace microtonos::sidebar::demo
