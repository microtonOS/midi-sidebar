# TODO

**High Priority**.

- Decide on how greying out inactive components should work.
- Check that skills are organised well and make suggestions on how they could be organsed better.
    - What is a good way for publishing skills. I could publish each skill as separate git repo, but that would be too many git repos. I could have them all in the same repo, but that removes modularity I need, so I don't love either of those solutions. Ig I could have different skills for different git organisations. That could work as a middle way, but I'm not super excited about it. Would be better if the directory structure was something like `skills/skill-package/specific-skill-[0-9][0-9]`
- Does MIDI 2.0 have a way of naming tunings like MTS ESP and Sysex and Scala files?
- In tmp, there is also an audio testing file. put this in juce, well unless you think there should be a juce-dsp or whether a similar script already exists in a more developed form for some other developer.
- add the continuous controller names to the reference for midi.
- Attach the volume fader to its APVTS parameter. Both are already in dB over the same range with the same floor, so this is small — the sidebar just needs to expose the fader, or take a value + callback so the module stays free of `juce_audio_processors`.
**Further explanation**.
The obstacle: Sidebar owns the fader as a private member and the module has no juce_audio_processors dependency, so it cannot hold a SliderAttachment. Either the module exposes the Slider& (cheap, leaks a widget into the API) or it takes a value + onVolumeChanged callback and the demo bridges to a ParameterAttachment (keeps the seam, one more moving part). Everything else already lines up: DemoProcessor's volume parameter is dB over the same range with the same metrics::floorDb
- Apply the read-only greying rule to the tuning page. The controllers monitor now draws every row dimmed, because nothing in it can be edited from the GUI; the same should hold wherever else that is true. Which fields exactly is an interaction question rather than a layout one — the status read-outs are clearly read-only, the settings section is not — so it was pinned rather than guessed at.
- Decide how the tuning page's settings persist. The callbacks are the seam; the question is which fields become APVTS parameters and which become properties on `apvts.state`. File paths are a poor fit for parameters.
**Further explanation**.
Nothing on any page persists — this is true of all four now, not just tuning, so the item is under-scoped. The real question is a split: scheme and update mode are enumerations and want to be APVTS parameters (automatable, host-visible); file paths, channel masks and tuning names want to be properties on apvts.state (ValueTree), because a path is not a parameter. Worth restating as "decide the persistence split for all four pages"

**Low Priority**.

- Should `Sidebar` be changed to `SideBar` in similarity to `ToolBar` and `SidePanel`?
- The param column in the table should have some kind of header design. JUCE does not by default provide a header column. Background colour is not very informative though, as it's covered in buttons, so the border between that part of the table and the rest of the table. I have not yet decided on the design though.
- The headers in the table stll don't look the same as in the JUCE widgets demo. Arguably, they look more tasteful like this, so low priority, but the question remains why.
- Decide what the tuning page does when the panel is shorter than it needs — it wants ~394px of editor height and the sidebar's minimum is ~~212px~~ 252px, so at small sizes its lower sections are cut off. Candidates in [tuning.md](docs/tuning.md#not-solved-small-heights): scroll, wrap the sections into two columns (needs a wider panel), or condense. The page is built out of section blocks so any of them is a layout change.
- Improve the design of the demo page and include further demo options.

<details>
<summary>
<b>Completed.</b>
</summary>


- From the presets page, rename the FILES group into FILE. remove the include tuning and include controllers options.
- From the controllers page, remove the FILES section altogether. Replace it with an INSERT section. There should be 3 buttons: control change or CC is one button and is a rename of 'add' The other two or aftertouch and polytouch. Rename the editing section into EDIT. rename the remove button into 'delete'. The 'add' button is no longer necessary. Replace it with a redo and undo button. if those words cannot fit use these icons (but rotate them appropriately):
```
<svg xmlns="http://www.w3.org/2000/svg" width="1em" height="1em" viewBox="0 0 48 48">
	<path d="M0 0h48v48H0z" fill="none" />
	<path fill="none" stroke="currentColor" stroke-linecap="round" stroke-linejoin="round" d="m18.629 32.542l9.958 9.958l9.958-9.958" />
	<path fill="none" stroke="currentColor" stroke-linecap="round" stroke-linejoin="round" d="M28.587 42.5V20.431c0-8.246-6.685-14.931-14.932-14.931h-4.2" />
</svg>
<svg xmlns="http://www.w3.org/2000/svg" width="1em" height="1em" viewBox="0 0 48 48">
	<path d="M0 0h48v48H0z" fill="none" />
	<path fill="none" stroke="currentColor" stroke-linecap="round" stroke-linejoin="round" d="M18.629 15.458L28.587 5.5l9.958 9.958" />
	<path fill="none" stroke="currentColor" stroke-linecap="round" stroke-linejoin="round" d="M28.587 5.5v22.069c0 8.246-6.685 14.931-14.932 14.931h-4.2" />
</svg>
```

- The channels page is actually incorrect I realised. You should be able to have an upper or a lower zone MPE and still adjust the other channels with omni on and omni off. So, the first toglle should simply be omni and MPE. The second toggle on or off. The buttons underneath the channels should be select all or mute al while omni and lower zone or upper zone while MPE.
- To not reinvent the wheel, add external content for juce development with agents. Some are suggested iin the Resources maybe there are more. Link to these in some reasonable way, is there a "git submodule" for skills? Can you include MCP tools in skills?
- Give feedback on how it is written for an agent. what is clear what is not.
- Doublecheck that claims are actually correct.
    - Add suitable references to the skill files.
- Check updates from juce 8 to juce 9.
- If this project is a submodule to something already having JUCE as a submodule, then it shouldn't need a JUCE submodule of its own, right?
- Make sure that the skills are shareable with repect to licensing and containing all the relevant information.
- Rename the GitHub repo and this directory to `midi-sidebar`. The module inside is already renamed; this is the remaining half.
- Decide whether the value bubble and the volume pop-up should look different. They are currently the same colour: `BubbleComponent::backgroundColourId` and `Sidebar::backgroundColourId` both resolve to `widgetBackground`.
    - Related, and now documented as a gotcha in the juce-ui skill rather than fixed here: the bubble's *text* comes from `TooltipWindow::textColourId` (`highlightedText`), so in the Light scheme it is white on white and invisible. A JUCE bug — it reproduces in the DemoRunner. The demo has a Bubble text switch for trying the workaround. Giving the bubble `highlightedFill` as a background would fix the contrast *and* settle this item in one move, if that is the direction you want.
- Compact-mode volume pop-up has only been checked in a headless render, never by clicking it in a real host.
- The presets page is the one page left. It should reuse `ReadOutField`, `ChoiceStrip`, `juce::GroupComponent` sections and the six-column page grid rather than growing its own — that is what keeps the three from looking like three plugins — and follow the controllers page's height model, everything fixed except one flexible track, rather than the tuning page's all-fixed one.
- What do `juce::TabbedComponent`s actually look like?
- Add a tool for testing the gui. Now in `.claude/skills/juce-ui/scripts`: a project-agnostic `SnapshotTool.cpp`, a `add_snapshot_tool.cmake` helper, and a `snapshot.sh` wrapper. The `temporary` version was tuneBfree-specific, wrote into the CWD, and never pumped the message loop before painting.
- Rename the JUCE module to `midi_sidebar` (was `microtonos_sidebar`). The C++ namespace stays `microtonos::sidebar` and the CMake alias stays `microtonos::`, because JUCE's own convention puts the *vendor* in the namespace — `juce::juce_gui_basics` — not the module name.
- Check whether I specified the allows for the system's `/tmp` directory correctly in `settings.json`. They were removed: `permissions.allow` does not extend the OS sandbox that Bash runs under, so `Write(//tmp/**)` had no effect on a compiled tool. The session `$TMPDIR` is writable by default and is what the snapshot tool uses via `juce::File::getSpecialLocation (juce::File::tempDirectory)`.
- Extract the page grid scaffolding. `TuningPage::resized` and `ControllersPage::resized` now contain the same twenty lines: the six columns with gutters, the row counter, `place` by content column, and `frame`. Two copies is the point at which the shape is known and a third would be careless, so this is worth doing before or with the presets page.

</details>