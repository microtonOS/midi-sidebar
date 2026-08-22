#include "PresetStore.h"

namespace microtonos::sidebar::demo
{

namespace
{
    /** The root tag of a preset file, and the two attributes metadata rides in.

        Metadata is written onto the root rather than into a sibling element, so
        a preset is one tree and "the metadata travels with the preset" is a
        property of the format rather than a convention the reader has to know.
        Both attribute names are prefixed, so they cannot collide with a
        parameter called `author`. */
    const juce::String presetTag    { "SidebarPreset" };
    const juce::String nameAttr     { "presetName" };
    const juce::String authorAttr   { "presetAuthor" };
    const juce::String commentAttr  { "presetComment" };
}

//==============================================================================
PresetStore::PresetStore (juce::AudioProcessorValueTreeState& stateToUse,
                          juce::StringArray presetParameters)
    : apvts (stateToUse), parameterIds (std::move (presetParameters))
{
}

//==============================================================================
juce::ValueTree PresetStore::captureState() const
{
    juce::ValueTree tree { presetTag };

    for (const auto& id : parameterIds)
        if (const auto* parameter = apvts.getParameter (id))
            tree.setProperty (id, parameter->getValue(), nullptr);

    return tree;
}

void PresetStore::applyState (const juce::ValueTree& tree)
{
    for (const auto& id : parameterIds)
    {
        // Skipped rather than defaulted when absent: a preset written before a
        // parameter existed says nothing about it, and nothing is not zero.
        if (! tree.hasProperty (id))
            continue;

        if (auto* parameter = apvts.getParameter (id))
            parameter->setValueNotifyingHost ((float) tree[id]);
    }
}

//==============================================================================
int PresetStore::bankStart() const
{
    const auto bank = juce::jlimit (0, juce::jmax (0, bankStarts.size() - 1), currentBank);

    return bankStarts.isEmpty() ? 0 : bankStarts[bank];
}

int PresetStore::bankSize() const
{
    const auto start = bankStart();
    const auto next  = currentBank + 1 < bankStarts.size() ? bankStarts[currentBank + 1]
                                                           : presets.size();

    return juce::jmax (0, next - start);
}

//==============================================================================
void PresetStore::load (int index)
{
    if (! juce::isPositiveAndBelow (index, presets.size()))
        return;

    const auto& preset = presets.getReference (index);

    if (preset.state.isValid())
    {
        applyState (preset.state);

        // Re-read rather than copied from the preset: a parameter the preset did
        // not mention keeps its live value, so *that* is the state a later edit
        // is measured against.
        loadedState = captureState();
    }

    meta = preset.meta;
}

//==============================================================================
void PresetStore::setProgram (int program)
{
    currentProgram = juce::jlimit (0, juce::jmax (0, bankSize() - 1), program);

    load (bankStart() + currentProgram);
}

void PresetStore::setBank (int bank)
{
    currentBank = juce::jlimit (0, juce::jmax (0, bankStarts.size() - 1), bank);

    // Clamped rather than kept: the new bank may be shorter, and a program
    // number past its end is not a place to be.
    setProgram (currentProgram);
}

void PresetStore::handleProgramChange (int program, std::optional<int> bank)
{
    // The bank first, so the program lands inside it. An empty bank means the
    // sender selected none, which the specification reads as *stay where you
    // are* rather than as bank zero.
    if (bank.has_value())
        currentBank = juce::jlimit (0, juce::jmax (0, bankStarts.size() - 1), *bank);

    setProgram (program);
}

//==============================================================================
juce::StringArray PresetStore::availableNames() const
{
    juce::StringArray names;

    for (int i = 0; i < bankSize(); ++i)
        names.add (presets[bankStart() + i].name);

    return names;
}

void PresetStore::chooseName (int index)
{
    setProgram (index);
}

//==============================================================================
bool PresetStore::loadFile (const juce::File& file)
{
    const auto xml = juce::parseXML (file);

    if (xml == nullptr)
        return false;

    Preset preset;

    preset.name         = xml->getStringAttribute (nameAttr, file.getFileNameWithoutExtension());
    preset.meta.author  = xml->getStringAttribute (authorAttr);
    preset.meta.comment = xml->getStringAttribute (commentAttr);

    // The attributes are stripped before the tree becomes parameter state, so a
    // preset's metadata cannot be mistaken for a parameter.
    auto copy = *xml;

    copy.removeAttribute (nameAttr);
    copy.removeAttribute (authorAttr);
    copy.removeAttribute (commentAttr);

    preset.state = juce::ValueTree::fromXml (copy);

    if (! preset.state.isValid())
        return false;

    presets.add (preset);

    currentBank = juce::jmax (0, bankStarts.size() - 1);
    setProgram (presets.size() - 1 - bankStart());

    return true;
}

bool PresetStore::loadBank (const juce::File& directory)
{
    juce::Array<juce::File> files;

    for (const auto& entry : juce::RangedDirectoryIterator (directory, false, "*.xml",
                                                            juce::File::findFiles))
        files.add (entry.getFile());

    if (files.isEmpty())
        return false;

    files.sort();

    // A new bank starts where the next preset will land, so the runs stay
    // contiguous and `bankSize` is the distance to the following start.
    bankStarts.add (presets.size());

    for (const auto& file : files)
    {
        // Appended directly rather than through `loadFile`, which would move the
        // selection once per file and open a new bank each time.
        if (const auto xml = juce::parseXML (file))
        {
            Preset preset;

            preset.name         = xml->getStringAttribute (nameAttr, file.getFileNameWithoutExtension());
            preset.meta.author  = xml->getStringAttribute (authorAttr);
            preset.meta.comment = xml->getStringAttribute (commentAttr);

            auto copy = *xml;

            copy.removeAttribute (nameAttr);
            copy.removeAttribute (authorAttr);
            copy.removeAttribute (commentAttr);

            preset.state = juce::ValueTree::fromXml (copy);

            if (preset.state.isValid())
                presets.add (preset);
        }
    }

    currentBank = bankStarts.size() - 1;
    setProgram (0);

    return true;
}

bool PresetStore::save (const juce::File& file, const juce::String& name)
{
    const auto state = captureState();
    const auto xml   = state.createXml();

    if (xml == nullptr)
        return false;

    xml->setAttribute (nameAttr,    name);
    xml->setAttribute (authorAttr,  meta.author);
    xml->setAttribute (commentAttr, meta.comment);

    if (! file.replaceWithText (xml->toString()))
        return false;

    // Adopted as the loaded preset, so the edited marker clears: saving is the
    // moment the file and the live state agree.
    Preset preset;

    preset.name  = name;
    preset.meta  = meta;
    preset.state = state.createCopy();

    presets.add (preset);
    loadedState = state;

    currentBank = juce::jmax (0, bankStarts.size() - 1);
    currentProgram = presets.size() - 1 - bankStart();

    return true;
}

void PresetStore::addFromCurrentState (const juce::String& name, presets::Meta presetMeta)
{
    Preset preset;

    preset.name  = name;
    preset.meta  = std::move (presetMeta);
    preset.state = captureState();

    presets.add (preset);

    // The first one becomes the loaded preset, so the store opens *on* a preset
    // rather than on nothing with presets beside it.
    if (presets.size() == 1)
        load (0);
}

//==============================================================================
presets::Status PresetStore::getStatus() const
{
    presets::Status status;

    const auto index = bankStart() + currentProgram;

    if (juce::isPositiveAndBelow (index, presets.size()))
        status.name = presets[index].name;

    // The marker is the page's to draw, and it draws a glyph rather than a
    // character in the name — so the name stays the name.
    status.edited = isEdited();

    if (! presets.isEmpty())
    {
        // **Shown 1-based, held 0-based.** MIDI transmits a program as 0-127 and
        // documents it as 1-128 — the same split the tuning page's program and
        // bank steppers use — so the wire keeps its numbering and the end-user
        // gets theirs.
        status.program = currentProgram + 1;
        status.bank    = currentBank + 1;
    }

    return status;
}

void PresetStore::setMeta (presets::Meta newMeta)
{
    meta = std::move (newMeta);
}

bool PresetStore::isEdited() const
{
    if (! loadedState.isValid())
        return false;

    // `isEquivalentTo` compares the trees by value, so an edit that is undone
    // stops counting as one — which a dirty flag could not do.
    return ! captureState().isEquivalentTo (loadedState);
}

} // namespace microtonos::sidebar::demo
