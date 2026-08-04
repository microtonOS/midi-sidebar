// =============================================================================
//  SnapshotTool — headless PNG snapshot of a JUCE plugin editor.
//  ---------------------------------------------------------------------------
//  Renders an AudioProcessorEditor to a PNG entirely in software: no window is
//  opened, no audio or MIDI device is touched, and nothing is read off the
//  screen. That makes it safe to run inside a sandbox, and it is the intended
//  way for an agent to look at its own GUI work without asking the user for a
//  screenshot.
//
//  This file is project-agnostic. It is configured by two compile definitions,
//  normally set for you by add_snapshot_tool.cmake:
//
//      JUCE_SNAPSHOT_PROCESSOR_HEADER   e.g. "PluginProcessor.h"
//      JUCE_SNAPSHOT_PROCESSOR_CLASS    e.g. MyPluginAudioProcessor
//
//  Build it with JUCE_MODAL_LOOPS_PERMITTED=1 (the cmake helper does this).
//  Without it the message loop cannot be pumped before the snapshot is taken,
//  so timers and APVTS attachments will not have updated and the image can
//  disagree with what you see when the plugin actually runs.
//
//  Run with --help for the full option list.
// =============================================================================

#include <juce_gui_extra/juce_gui_extra.h>
#include <juce_audio_processors/juce_audio_processors.h>

#ifndef JUCE_SNAPSHOT_PROCESSOR_HEADER
 #error "Define JUCE_SNAPSHOT_PROCESSOR_HEADER, e.g. -DJUCE_SNAPSHOT_PROCESSOR_HEADER=\"PluginProcessor.h\""
#endif

#ifndef JUCE_SNAPSHOT_PROCESSOR_CLASS
 #error "Define JUCE_SNAPSHOT_PROCESSOR_CLASS, e.g. -DJUCE_SNAPSHOT_PROCESSOR_CLASS=MyPluginAudioProcessor"
#endif

#include JUCE_SNAPSHOT_PROCESSOR_HEADER

#include <algorithm>
#include <iostream>
#include <utility>

namespace
{

constexpr auto defaultSubDir = "juce-ui-snapshots";

//==============================================================================
struct Options
{
    juce::File      explicitOut;                 // --out, empty if unset
    juce::String    directory;                   // --dir
    juce::String    stem            { "snapshot" };
    float           scale           { 2.0f };
    int             width           { 0 };       // --size, 0 = leave editor's own size
    int             height          { 0 };
    int             settleMs        { 250 };
    int             keep            { 20 };
    bool            timestamp       { false };
    bool            listParams      { false };
    double          sampleRate      { 44100.0 };
    int             blockSize       { 512 };

    // id -> value, applied before the snapshot. Text values go through
    // getValueForText(); normalised ones are used as-is.
    juce::StringPairArray textParams;
    juce::StringPairArray normParams;
};

void printUsage()
{
    std::cout << R"(usage: SnapshotTool [options]

Renders the plugin editor to a PNG and prints the absolute path on stdout.

  --out <path>        Write exactly here (absolute, or relative to the CWD).
                      Overrides --dir/--name/--timestamp.
  --dir <dir>         Output directory. Default: <system temp>/)" << defaultSubDir << R"(
  --name <stem>       File stem. Default: snapshot
  --timestamp         Append -YYYYMMDD-HHMMSS to the stem instead of
                      overwriting the previous file.
  --keep <n>          Keep at most n PNGs in --dir, deleting the oldest.
                      Default: 20. Use 0 to disable pruning.
  --scale <f>         Render scale. Default: 2.0 (Retina-like).
  --size <w>x<h>      Resize the editor before rendering. Default: whatever
                      size the editor gives itself.
  --settle <ms>       Pump the message loop for this long before rendering, so
                      timers and parameter attachments can catch up.
                      Default: 250.
  --param <id>=<v>    Set parameter <id> from the text <v> (as typed into the
                      GUI, e.g. "3.5" or "Fast"). Repeatable.
  --nparam <id>=<v>   Set parameter <id> from a normalised 0..1 value.
                      Repeatable.
  --list-params       Print every parameter ID, name and current value, then
                      exit without rendering.
  --samplerate <hz>   prepareToPlay sample rate. Default: 44100.
  --blocksize <n>     prepareToPlay block size. Default: 512.
  --help              Show this message.
)";
}

//==============================================================================
bool splitKeyValue (const juce::String& arg, juce::String& key, juce::String& value)
{
    const auto eq = arg.indexOfChar ('=');

    if (eq <= 0)
        return false;

    key   = arg.substring (0, eq).trim();
    value = arg.substring (eq + 1).trim();
    return key.isNotEmpty();
}

bool parseArgs (int argc, char* argv[], Options& o, bool& showHelp)
{
    const auto needsValue = [&] (int& i, const char* what) -> juce::String
    {
        if (++i >= argc)
        {
            std::cerr << "[snapshot] " << what << " needs a value\n";
            return {};
        }

        return juce::String (juce::CharPointer_UTF8 (argv[i]));
    };

    for (int i = 1; i < argc; ++i)
    {
        const juce::String a (juce::CharPointer_UTF8 (argv[i]));

        if (a == "--help" || a == "-h")          { showHelp = true; return true; }
        else if (a == "--list-params")           { o.listParams = true; }
        else if (a == "--timestamp")             { o.timestamp = true; }
        else if (a == "--out")                   { const auto v = needsValue (i, "--out");        if (v.isEmpty()) return false; o.explicitOut = juce::File::getCurrentWorkingDirectory().getChildFile (v); }
        else if (a == "--dir")                   { const auto v = needsValue (i, "--dir");        if (v.isEmpty()) return false; o.directory = v; }
        else if (a == "--name")                  { const auto v = needsValue (i, "--name");       if (v.isEmpty()) return false; o.stem = v; }
        else if (a == "--scale")                 { const auto v = needsValue (i, "--scale");      if (v.isEmpty()) return false; o.scale = (float) v.getDoubleValue(); }
        else if (a == "--settle")                { const auto v = needsValue (i, "--settle");     if (v.isEmpty()) return false; o.settleMs = v.getIntValue(); }
        else if (a == "--keep")                  { const auto v = needsValue (i, "--keep");       if (v.isEmpty()) return false; o.keep = v.getIntValue(); }
        else if (a == "--samplerate")            { const auto v = needsValue (i, "--samplerate"); if (v.isEmpty()) return false; o.sampleRate = v.getDoubleValue(); }
        else if (a == "--blocksize")             { const auto v = needsValue (i, "--blocksize");  if (v.isEmpty()) return false; o.blockSize = v.getIntValue(); }
        else if (a == "--size")
        {
            const auto v = needsValue (i, "--size");

            if (v.isEmpty())
                return false;

            o.width  = v.upToFirstOccurrenceOf ("x", false, true).getIntValue();
            o.height = v.fromFirstOccurrenceOf ("x", false, true).getIntValue();

            if (o.width <= 0 || o.height <= 0)
            {
                std::cerr << "[snapshot] --size expects <width>x<height>, got '" << v << "'\n";
                return false;
            }
        }
        else if (a == "--param" || a == "--nparam")
        {
            const auto v = needsValue (i, a.toRawUTF8());

            if (v.isEmpty())
                return false;

            juce::String key, value;

            if (! splitKeyValue (v, key, value))
            {
                std::cerr << "[snapshot] " << a << " expects <id>=<value>, got '" << v << "'\n";
                return false;
            }

            (a == "--param" ? o.textParams : o.normParams).set (key, value);
        }
        else
        {
            std::cerr << "[snapshot] unknown option '" << a << "' (try --help)\n";
            return false;
        }
    }

    if (o.scale <= 0.0f)
    {
        std::cerr << "[snapshot] --scale must be positive\n";
        return false;
    }

    return true;
}

//==============================================================================
/** Dispatches queued messages so timers, async updates and parameter
    attachments run before we paint. Without this the editor is snapshotted in
    whatever state its constructor left it in, which is a common source of
    "the screenshot doesn't match the running plugin".
*/
void settle (int ms)
{
   #if JUCE_MODAL_LOOPS_PERMITTED
    if (ms > 0)
        juce::MessageManager::getInstance()->runDispatchLoopUntil (ms);
   #else
    juce::ignoreUnused (ms);

    static bool warned = false;

    if (! std::exchange (warned, true))
        std::cerr << "[snapshot] warning: built without JUCE_MODAL_LOOPS_PERMITTED=1, so the\n"
                     "           message loop cannot be pumped. Timers and APVTS attachments\n"
                     "           will not have updated and the image may be misleading.\n";
   #endif
}

juce::AudioProcessorParameter* findParameter (juce::AudioProcessor& proc, const juce::String& id)
{
    for (auto* p : proc.getParameters())
        if (auto* withID = dynamic_cast<juce::AudioProcessorParameterWithID*> (p))
            if (withID->paramID == id)
                return p;

    return nullptr;
}

/** Returns false if any requested parameter could not be found or parsed. We
    fail loudly rather than rendering a picture of the wrong state.
*/
bool applyParameters (juce::AudioProcessor& proc, const Options& o)
{
    auto ok = true;

    const auto apply = [&] (const juce::StringPairArray& source, bool normalised)
    {
        for (const auto& id : source.getAllKeys())
        {
            auto* param = findParameter (proc, id);

            if (param == nullptr)
            {
                std::cerr << "[snapshot] no parameter with ID '" << id << "' (try --list-params)\n";
                ok = false;
                continue;
            }

            const auto raw = source[id];
            const auto value = normalised ? (float) raw.getDoubleValue()
                                          : param->getValueForText (raw);

            if (normalised && (value < 0.0f || value > 1.0f))
            {
                std::cerr << "[snapshot] --nparam " << id << " expects 0..1, got '" << raw << "'\n";
                ok = false;
                continue;
            }

            param->setValueNotifyingHost (juce::jlimit (0.0f, 1.0f, value));
        }
    };

    apply (o.textParams, false);
    apply (o.normParams, true);
    return ok;
}

void listParameters (juce::AudioProcessor& proc)
{
    for (auto* p : proc.getParameters())
    {
        const auto id = [p]() -> juce::String
        {
            if (auto* withID = dynamic_cast<juce::AudioProcessorParameterWithID*> (p))
                return withID->paramID;

            return "(no ID)";
        }();

        std::cout << id << "\t" << p->getName (64) << "\t"
                  << p->getText (p->getValue(), 64) << "\n";
    }
}

//==============================================================================
juce::File resolveOutputDirectory (const Options& o)
{
    if (o.directory.isNotEmpty())
        return juce::File::getCurrentWorkingDirectory().getChildFile (o.directory);

    // The system temp directory: $TMPDIR on macOS (which Claude Code points at
    // the session temp dir, so snapshots are cleaned up with the session) and
    // /tmp or $TMPDIR on Linux/RPi.
    return juce::File::getSpecialLocation (juce::File::tempDirectory)
               .getChildFile (defaultSubDir);
}

juce::File resolveOutputFile (const Options& o, const juce::File& dir)
{
    if (o.explicitOut != juce::File())
        return o.explicitOut;

    auto name = o.stem;

    if (o.timestamp)
        name << "-" << juce::Time::getCurrentTime().formatted ("%Y%m%d-%H%M%S");

    return dir.getChildFile (name + ".png");
}

/** Keeps the snapshot directory from growing without bound. We do not rely on
    the OS sweeping temp files: macOS only removes files untouched for three
    days, and that behaviour is explicitly not API.
*/
void pruneOldSnapshots (const juce::File& dir, int keep)
{
    if (keep <= 0)
        return;

    auto files = dir.findChildFiles (juce::File::findFiles, false, "*.png");

    if (files.size() <= keep)
        return;

    std::sort (files.begin(), files.end(), [] (const juce::File& a, const juce::File& b)
    {
        return a.getLastModificationTime() > b.getLastModificationTime();
    });

    for (int i = keep; i < files.size(); ++i)
        files.getReference (i).deleteFile();
}

} // namespace

//==============================================================================
int main (int argc, char* argv[])
{
    Options o;
    auto showHelp = false;

    if (! parseArgs (argc, argv, o, showHelp))
        return 1;

    if (showHelp)
    {
        printUsage();
        return 0;
    }

    // Must outlive every JUCE object below.
    juce::ScopedJuceInitialiser_GUI juceInit;

    JUCE_SNAPSHOT_PROCESSOR_CLASS proc;

    if (o.listParams)
    {
        listParameters (proc);
        return 0;
    }

    // Build the engine so any live read-outs in the editor show real values.
    proc.prepareToPlay (o.sampleRate, o.blockSize);

    auto exitCode = 0;

    // Scoped so the editor is destroyed before releaseResources(): an editor
    // outliving its processor is a use-after-free.
    {
        std::unique_ptr<juce::AudioProcessorEditor> editor (proc.createEditor());

        if (editor == nullptr)
        {
            std::cerr << "[snapshot] createEditor() returned nullptr — does the processor "
                         "override hasEditor() to return true?\n";
            proc.releaseResources();
            return 2;
        }

        if (! applyParameters (proc, o))
        {
            proc.releaseResources();
            return 1;
        }

        if (o.width > 0 && o.height > 0)
            editor->setSize (o.width, o.height);

        // Let the constructor's async work, any Timer, and the parameter
        // changes above reach the editor before we paint it.
        settle (o.settleMs);

        const auto bounds = editor->getLocalBounds();

        if (bounds.isEmpty())
        {
            std::cerr << "[snapshot] editor has zero size. The editor constructor normally "
                         "calls setSize(); pass --size <w>x<h> to force one.\n";
            proc.releaseResources();
            return 2;
        }

        const auto image = editor->createComponentSnapshot (bounds, true, o.scale);

        if (! image.isValid())
        {
            std::cerr << "[snapshot] createComponentSnapshot produced an invalid image\n";
            proc.releaseResources();
            return 2;
        }

        const auto dir = resolveOutputDirectory (o);

        if (o.explicitOut == juce::File())
        {
            const auto created = dir.createDirectory();

            if (! created.wasOk())
            {
                std::cerr << "[snapshot] could not create " << dir.getFullPathName()
                          << ": " << created.getErrorMessage() << "\n";
                proc.releaseResources();
                return 2;
            }
        }

        const auto out = resolveOutputFile (o, dir);
        out.getParentDirectory().createDirectory();

        // Write to a temporary sibling and rename, so a failure part-way
        // through never leaves a half-written PNG that looks valid.
        const auto temp = out.getSiblingFile (out.getFileNameWithoutExtension() + ".partial.png");
        temp.deleteFile();

        auto wrote = false;

        {
            juce::FileOutputStream stream (temp);

            if (stream.openedOk())
            {
                juce::PNGImageFormat png;
                wrote = png.writeImageToStream (image, stream) && stream.getStatus().wasOk();
            }
        }

        if (! wrote)
        {
            temp.deleteFile();
            std::cerr << "[snapshot] could not write " << out.getFullPathName() << "\n";
            exitCode = 2;
        }
        else
        {
            out.deleteFile();

            if (! temp.moveFileTo (out))
            {
                temp.deleteFile();
                std::cerr << "[snapshot] could not move the image into place at "
                          << out.getFullPathName() << "\n";
                exitCode = 2;
            }
            else
            {
                if (o.explicitOut == juce::File())
                    pruneOldSnapshots (dir, o.keep);

                std::cerr << "[snapshot] " << image.getWidth() << "x" << image.getHeight()
                          << " px at " << o.scale << "x from a "
                          << bounds.getWidth() << "x" << bounds.getHeight() << " editor\n";

                // Last line of stdout is the path, and nothing else goes to
                // stdout, so a caller can use it directly.
                std::cout << out.getFullPathName() << std::endl;
            }
        }
    }

    proc.releaseResources();
    return exitCode;
}
