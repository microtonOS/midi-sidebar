# TODO

**High Priority**.

- Check that skills are organised well and make suggestions on how they could be organsed better.
    - What is a good way for publishing skills. I could publish each skill as separate git repo, but that would be too many git repos. I could have them all in the same repo, but that removes modularity I need, so I don't love either of those solutions. Ig I could have different skills for different git organisations. That could work as a middle way, but I'm not super excited about it. Would be better if the directory structure was something like `skills/skill-package/specific-skill-[0-9][0-9]`
- To not reinvent the wheel, add external content for juce development with agents. Some are suggested iin the Resources maybe there are more. Link to these in some reasonable way, is there a "git submodule" for skills? Can you include MCP tools in skills?
- Give feedback on how it is written for an agent. what is clear what is not.
- Doublecheck that claims are actually correct.
    - Add suitable references to the skill files.
- Check updates from juce 8 to juce 9.
- Does MIDI 2.0 have a way of naming tunings like MTS ESP and Sysex and Scala files?
- Make sure that the skills are shareable with repect to licensing and containing all the relevant information.
- If this project is a submodule to something already having JUCE as a submodule, then it shouldn't need a JUCE submodule of its own, right?
- In tmp, there is also an audio testing file. put this in juce, well unless you think there should be a juce-dsp or whether a similar script already exists in a more developed form for some other developer.
- add the continuous controller names to the reference for midi.
- Rename the GitHub repo and this directory to `midi-sidebar`. The module inside is already renamed; this is the remaining half.
- Attach the volume fader to its APVTS parameter. Both are already in dB over the same range with the same floor, so this is small — the sidebar just needs to expose the fader, or take a value + callback so the module stays free of `juce_audio_processors`.
- Decide whether the value bubble and the volume pop-up should look different. They are currently the same colour: `BubbleComponent::backgroundColourId` and `Sidebar::backgroundColourId` both resolve to `widgetBackground`.
    - Related, and now documented as a gotcha in the juce-ui skill rather than fixed here: the bubble's *text* comes from `TooltipWindow::textColourId` (`highlightedText`), so in the Light scheme it is white on white and invisible. A JUCE bug — it reproduces in the DemoRunner. The demo has a Bubble text switch for trying the workaround. Giving the bubble `highlightedFill` as a background would fix the contrast *and* settle this item in one move, if that is the direction you want.
- Compact-mode volume pop-up has only been checked in a headless render, never by clicking it in a real host.
- Decide what the tuning page does when the panel is shorter than it needs — it wants ~394px of editor height and the sidebar's minimum is 212px, so at small sizes its lower sections are cut off. Candidates in [tuning.md](docs/tuning.md#not-solved-small-heights): scroll, wrap the sections into two columns (needs a wider panel), or condense. The page is built out of section blocks so any of them is a layout change.
- Decide how the tuning page's settings persist. The callbacks are the seam; the question is which fields become APVTS parameters and which become properties on `apvts.state`. File paths are a poor fit for parameters.
- The tuning page is the first page. The presets and controllers pages should reuse `ReadOutField`, `ChoiceStrip`, `juce::GroupComponent` sections and the six-column page grid rather than growing their own — that is what keeps the three from looking like three plugins. The grid scaffolding in `TuningPage::resized` (row counter, `place` by content column, `frame`) is the piece most likely worth extracting once there is a second page to check it against.

**Low Priority**.

- What do `juce::TabbedComponent`s actually look like?


<details>
<summary>
<b>Completed.</b>
</summary>

- Add a tool for testing the gui. Now in `.claude/skills/juce-ui/scripts`: a project-agnostic `SnapshotTool.cpp`, a `add_snapshot_tool.cmake` helper, and a `snapshot.sh` wrapper. The `temporary` version was tuneBfree-specific, wrote into the CWD, and never pumped the message loop before painting.
- Rename the JUCE module to `midi_sidebar` (was `microtonos_sidebar`). The C++ namespace stays `microtonos::sidebar` and the CMake alias stays `microtonos::`, because JUCE's own convention puts the *vendor* in the namespace — `juce::juce_gui_basics` — not the module name.
- Check whether I specified the allows for the system's `/tmp` directory correctly in `settings.json`. They were removed: `permissions.allow` does not extend the OS sandbox that Bash runs under, so `Write(//tmp/**)` had no effect on a compiled tool. The session `$TMPDIR` is writable by default and is what the snapshot tool uses via `juce::File::getSpecialLocation (juce::File::tempDirectory)`.

</details>