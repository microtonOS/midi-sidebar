# Completed

Done and settled. Split out of [TODO.md](TODO.md) so that reading the open work
does not mean scrolling past the closed work — nothing here needs acting on, and
an agent asked to look at the TODO should not have to load this at all.

Newest at the top within each batch.

- **Master and channel tuning applies under `MTS Sysex` only.** The deciding
  argument was that a scheme carrying its own pitch reference should not have it
  silently overridden: MTS ESP has a master that owns absolute pitch, a `.kbm`
  states a reference note *and* its frequency, and "standard" means A440 by
  definition. Only the scheme that reads its tuning from the MIDI stream also
  takes its displacement from that stream.

  The case against is recorded in TODO.md rather than lost, because it is a good
  one: GM2 **requires** Scale/Octave Tuning Adjust (§4.7) *and* all four
  displacements together, so a conforming device must compose them; scale/octave
  tuning carries no absolute reference at all, being offsets from equal
  temperament; MTS frequencies are encoded as *semitone plus fraction*, relative
  to a note grid that a displacement moves; and the minilogue xd keeps a scale
  and a global Master Tune orthogonal in exactly that way — it even reduces a
  128-note bulk dump to a relative scale, discarding the absolute anchor.

- **Master and channel tuning.** The two Device Control system exclusives
  CA-025 added — Master Fine (`04 03`, 14 bits, ±100 c in steps of 0.0122 c) and
  Master Coarse (`04 04`, 7 bits, ±64 semitones, and its "LSB is always 0" is
  enforced rather than assumed) — plus RPN 01 and 02, which the same document
  renamed from *Master* to **Channel** Fine and Coarse Tuning. All four are
  displacements from A440 and are **summed**, which is CA-025's own rule.

  Applied as an offset at query time rather than folded into `TuningTable`, for
  two reasons: a displacement is not part of a tuning — the same table sounds at
  a different pitch under a different master tuning — and RPN 01 steps by
  0.0122 c where an MTS frequency field steps by 0.0061 c, so baking it in would
  lose precision.

  **Not applied under MTS ESP.** There an external master is the authority on
  absolute pitch and every other client is asking that same master, so a local
  displacement would put this plugin out of tune with all of them; a user wanting
  A=442 sets it on the master, and the frequencies handed to us already carry it.

- **The default tuning name is `12edo`**, not `12edo A4=440 Hz`, and is now the
  fallback for *every* scheme including MTS ESP — a master with no scale name and
  a plugin with no tuning are both playing equal temperament, which is more use
  than the word "Unnamed". The reference pitch left the name because it is not
  fixed there: master and channel tuning move it, and the presets page shows the
  frequencies actually sounding.

- **`checks/`, run by `ctest`.** Six console apps over the module's logic
  headers, `enable_testing()` at the top level. They exist because an earlier
  round of the same suites was written into a session temp directory, passed, and
  was gone the next day: a check that is not a build target is not a check.
  Reusable *fixtures* went to the skills instead —
  `midi-1_0/scripts/midi_vectors.py`, `midi-microtuning/scripts/mts_sysex.py`
  (which encodes as well as decodes, since nothing off the shelf does both) and
  `scales.py` — with the CMake harness in `juce-guide/scripts/`.

- **The tuning page reads real tunings.** Four pure headers in
  `sidebar/tuning/`: `TuningTable` (128 frequencies per channel plus the
  unspecified list, each `optional` so *unmapped* is distinct from silent),
  `MtsSysex` (every reception format — bulk dump, key-based dump, single-note
  with and without bank, both scale/octave forms and their dumps), `ScalaFiles`
  (over Surge's tuning-library) and `PeriodInference`.

  `demo/TuningSource` owns the sources. **ODDSound's client serves `mtsEsp`
  only**: `MTSClient::freq()` returns its sysex table solely when no master is
  online, so it cannot honour docs/tuning.md's rule that selecting MTS Sysex
  ignores the master. Nothing off the shelf reads MTS sysex the way we need —
  tuning-library has none, `tschiemer/midimessage` marks MTS TODO, and
  `kosonya/mts_dumper` is a Python generator — so that parser is ours. Tuning
  system exclusives are **consumed**, unlike Master Volume, since they are
  addressed to this instrument.

  Period inference is adapted from `../tuneBfree`'s `inferScaleSize`, minus its
  `extendFrequencies`/`gamutSize` machinery, returning cents and every candidate
  rather than one ratio. The default is the candidate nearest an octave, which
  gives 1200 c for any equal division of the octave; for a scale dividing
  something else the frequencies do not say which multiple was meant, so it is a
  guess and is documented as one. Two real bugs the checks caught: a period
  spanning all but one of the sequence was "confirmed" by a single comparison,
  so every table repeated — periods are now capped at half the sequence.

  The Scala loader follows tuneBfree's: `_i.kbm` maps channel *i*, an unsuffixed
  `.kbm` is the generic mapping, `Tunings::TuningError` is thrown and must be
  caught, `scale.description` is a better name than the filename, and a `.scl`
  states its own period so a file tuning is `specified` rather than inferred.
  Directory-as-bank is the part tuneBfree does not do.

- **C++20.** tuning-library declares `cxx_std_20` and takes
  `readSCLFile(const StreamablePath auto&)`, so the whole project moved rather
  than carrying a mixed-standard build.

- **Two skills, one new reference.** `midi-1_0/references/real-devices.md`
  collects where instruments depart from the specification — the minilogue xd's
  shared CC 63 and reversed byte order, Mixxx's "not universal" comment and its
  wraparound heuristic, Surge XT and Ardour reading plain CC as 7-bit only.
  `juce-midi` is new: what JUCE's MIDI classes provide, what they do not (no
  14-bit CC helper anywhere), `processBlock`'s pass-through contract, the
  `getSysExData` off-by-two, and that `MPEZoneLayout` still says *master* where
  MPE v1.1 says *manager*. `juce-ui/references/widgets.md` gained the fact that a
  `TableListBox` column cannot be spanned.

- **The volume fader is attached to its APVTS parameter**, which needed
  `Sidebar::getVolumeSlider()`. Both were already dB over the same range with
  the same floor.

- **The LSB column is gone, and so are the built-in rows.** A mapping now holds
  one control change number of seven bits. The specification pairs CC *n* with
  CC *n*+32 for a 14-bit value, but the minilogue xd shares CC 63 as the low byte
  of every control and sends it *first*, and a survey of what other software does
  with the same stream found almost none of it implementing 14-bit CC at all —
  Surge XT multiplies by 1/127 and stops, Ardour reads 14 bits only for bindings
  declared as RPN/NRPN, JUCE ships no helper, and Mixxx, which does implement it,
  needs two mapping rows per control plus a statistical learn wizard to make it
  usable. A plugin needing more than 128 steps exposes a coarse and a fine
  parameter instead. Finding D closed with it: neither derive nor flag, remove.

  The built-in rows went at the same time. Bank select is `unavailable` now, like
  RPN/NRPN — the plugin performs it and neither CC 0 nor CC 32 can carry a
  mapping — and CC 7, 39 and 88 turned out to be nothing special: master volume
  is a system exclusive, and high-resolution velocity is an ordinary parameter.
  So `CcStatus::reserved`, `Builtin`, `withBuiltins`, `defaultsFor` and the
  delete button's `reset` label are all deleted rather than reworked.

- **MIDI learn watches a gesture.** `MidiLearner` collects for a second and a
  half after the last message and identifies the fine half of a control by its
  behaviour — a low byte wraps 127 to 0 during a sweep, so it jumps where the
  high byte steps — which is Mixxx's heuristic and, unlike a rule about which
  numbers are reserved, works on hardware that ignores the convention. The
  blanket refusal of CC 32-63 survives only for a single lone message, where
  there is no behaviour to read. The monitor shows what is being heard while it
  happens.

- **Master volume.** `deviceControl::masterVolumeFrom` reads
  `F0 7F 7F 04 01 vv vv F7` and moves the fader, on the broadcast address only —
  a plugin has no device ID, and answering every ID would have two instances
  fight. The curve is 40·log₁₀(v/16383), which is General MIDI 2 v1.2a §3.3.4's
  square law, cited by §4.1 for this message. Passed through rather than
  consumed, since a broadcast is addressed to everything downstream too. The
  demo's fader is now attached to its `volume` parameter, which it was not
  before.

- Finding F: the invalid-cell colour is built. `pageColours::invalidColourId` is
  the module's one colour not derived from the scheme — a fixed red pulled a
  fifth of the way toward the theme's text so it belongs to the page — painted as
  a wash behind the number so it stays readable. Validation is three tiers in
  `controllers::CcStatus`: only 98-101 and 120-127 turn a cell red, and the
  MSB/LSB clash marks *both* cells and both rows, as docs/controllers.md says.
  (Superseded: see the top entry. There is one controller column now, two tiers,
  and no clash rule.)
  The colour has its row in appendices.md.

- Docs findings resolved with you: 'master channel' swept (the only other
  instance was `tuning.md:71`, now fixed); `right-click.md`'s stale omni-off
  sentence; the polytouch-on-member-channels override; the glossary additions
  (lower case, `edo` = equally divided octave); the GUI now following the
  two-decimal cents convention (`PB sensitivity` reads `200.00 c`); the monitor's
  `Sysex` casing and note numbers; the channel-mode range noted as having moved;
  the colour table added to `appendices.md`; and `rail.png` given a home in the
  README. Also fixed: `channels.md` said the lower zone has 1 as its *member*
  channel where it means *manager*.

- Audited every claim in the two new MIDI skills against the specifications, at
  your request after the MPE pitch-bend error. Corrections: Table IV's mode
  messages also perform All Notes Off, and CC 126's `M` counts channels not
  voices; Table III's Effect Control, Sound Controller and Portamento Control
  entries were missing; CC 88 is *not* in Table III (it is CA-031, unread);
  pitch-bend centre `0x2000` is not stated in Table II and was removed; SysEx ID
  `7D` and the three-byte manufacturer ID form were missing; `F4 F5 F9 FD` are
  undefined. Every RPN is now shown as the control changes it actually is.
  Citations are document title, version, date and table or section.
- Split this file out of TODO.md.

- Docs consistency pass against the specifications in `tmp/midi/`. Applied:
  typos in five files; the broken comment delimiter at `tuning.md:27`, which was
  swallowing the whole period section (lines 29-39 rendered as nothing); `toggle`
  to `switch` where the control chooses between alternatives; `menu` to `page`
  where the thing is a sidebar page; `sysex` to `Sysex` in prose. Renames the doc
  comments asked for: presets `on` to `active`, presets `load` to `open`, tuning
  `load files` to `open files`, and the demo monitor now follows the examples in
  `controllers.md` exactly. Everything erroneous or underspecified is in the
  findings block at the top of this file instead, with drafts.
- Wrote the `midi-1_0` and `midi-2_0` skills, which were empty scaffolds. Five
  reference files and four, each claim cited to a specification and page, with
  what has *not* been read marked as such rather than guessed at.

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

## Notes moved out of the docs

These were `<!-- -->` blocks in `docs/`. They record how something was built or
why a decision went the way it did — history rather than open work — so they live
here now and the docs are free of them.

### The tuning page, as built

The page in `modules/midi_sidebar/sidebar/pages/`. It draws every row above and
opens the channel selector, and it holds no tuning of its own: values are pushed
in with `setInterval`, `setStatus` and `setPeriod`, and everything the end-user
does leaves through a callback (`onSchemeChanged`, `onScaleFileRequested`, …).
So it can be looked at now and does not have to change when the MIDI side
arrives. The demo fills it with the sketch's own numbers; nothing drives it yet.

The three named sections are `juce::GroupComponent`s; the interval and modulo at
the top are not, as specified. A group is only a frame — its section's widgets
are children of the *page*, not of it, so that they stay in the page's one grid
and keep their columns. Each frame is a background item spanning its section's
rows, and the grid has a gutter track at each end that the frames span and the
widgets do not, which is what insets a group's contents from its own outline.

Two widgets came out of the page, and the other two pages should use them rather
than inventing their own: `ReadOutField` (every read-only value is one, which is
what makes "you cannot type here" legible without saying so) and `ChoiceStrip`,
which moved into the module from the demo. The colours a section's title and
frame take are in `pageColours`, not on a widget, because the pages are included
before the panel that would otherwise own them.

**The page is one grid of six equal columns**, which is the grid the sketch
above is drawn on — the finest division any of its rows uses, and each cell's
`colspan` read straight off it. Everything spans a whole number of columns, so a
row split in half really is halved, and things in different rows begin on the
same line because the layout holds them there: `program`'s field and `updated`
both start at column 3, while `bank`, the period source, the channels button and
the update choices all start at column 4 — the middle of the page.

It is deliberately *not* a grid per row. That version was written first and
looked plausible, but each row divided its own width with its own fixed label
widths, so the halves were not halves and nothing lined up between rows. See
`metrics::pageColumns`.

The bottom block follows the sketch: the two load buttons stacked in the left
half, the two update choices stacked in the right, and no labels on either — the
buttons say what they load, and the choices name themselves. The update strip is
one control spanning both rows, divided by the same gap that separates the rows,
which is what puts "note on" beside `load scale` and "always" beside
`load maps`.

**Two readings of the sketch, now settled.** `updated` is read-only — it stamps
something that happened. The period source is a read-only indicator of where the
number came from, and stays so: the end-user chooses among inferred candidates
but never states a period, which is why there is no third state.

The chooser holds an *index* into the candidate list rather than a number, so an
unoffered value is unreachable rather than rejected, and the whole list arrives
through `setPeriod`. Whatever eventually infers periods therefore decides what
is offered; the page only presents it. Note that this made
`SidebarLookAndFeel::getSliderLayout` need scoping to `LinearVertical` — it had
been discarding the text box of *every* slider, which was invisible while the
fader was the only one.

#### Not solved: small heights

The page needs `TuningPage::getNaturalHeight()` — currently 346px, derived from
its own rows rather than written down — plus the panel's title and padding, so
about 394px of editor. Below that the lower sections are simply cut off. The
sidebar's minimum height is 212px, so at its own minimum the page shows down to
`updated` and no further.

Deliberately left, not overlooked. The candidates are scrolling, wrapping the
sections into two columns (which needs a wider panel: at 248px a column is about
120px, too narrow for `program [ ] bank [ ]`), and condensing sections the way
the rail condenses its volume control. The page is built out of section blocks
so that whichever is chosen is a layout change rather than a rewrite.

#### Not solved: persistence

Nothing on the page survives closing the editor yet. The callbacks are where
that attaches; whether the settings become APVTS parameters, properties on
`apvts.state`, or both, is still open — a file path is a poor fit for a
parameter, an enumerated setting is a good one.

### The controllers and presets pages

Both carried the same status note: the GUI is built, nothing behind it is. Still
true — no MIDI is read, no preset file is written, and none of the five
controller modes does anything yet. They are choices the table can express.

### Why the demo has four themes

The four are JUCE's own (`LookAndFeel_V4::getDarkColourScheme` and friends)
rather than palettes of ours. The module derives all of its colours from the nine
in whichever scheme it is given, so switching between them is a real test of that
claim: if a theme comes out wrong, a colour has been hardcoded somewhere it
should not be. That is worth more than the ability to switch themes.
