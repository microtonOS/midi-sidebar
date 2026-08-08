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
- In temporary, there is also an audio testing file. put this in juce, well unless you think there should be a juce-dsp or whether a similar script already exists in a more developed form for some other developer.
- add the continuous controller names to the reference for midi.
- Rename the GitHub repo and this directory to `midi-sidebar`. The module inside is already renamed; this is the remaining half.
- Attach the volume fader to its APVTS parameter. Both are already in dB over the same range with the same floor, so this is small — the sidebar just needs to expose the fader, or take a value + callback so the module stays free of `juce_audio_processors`.
- Decide whether the value bubble and the volume pop-up should look different. They are currently the same colour: `BubbleComponent::backgroundColourId` and `Sidebar::backgroundColourId` both resolve to `widgetBackground`.
- Compact-mode volume pop-up has only been checked in a headless render, never by clicking it in a real host.

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