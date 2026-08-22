# TODO

- The split glyphs render thinner than the notes glyph beside them in the
  controllers table: they are filled shapes on a 256 viewBox where the others are
  stroked on 48, so at `metrics::markerSize` they carry less ink. Worth either
  thickening the paths or giving them their own size.

- **A preset does not carry the controller mappings, though two comments and the
  docs say it does.** `ControllersPage.h` justifies having no files section with
  "a preset carries the whole state, mappings included", but the mappings live in
  a plain `juce::Array<controllers::Mapping>` on the processor and have never
  been in the APVTS tree — so `PresetStore` could not have saved them before the
  preset was narrowed to the synth's parameters, and cannot now. Either the
  mappings become a `ValueTree` child that the store writes beside the
  parameters, or the controllers page grows the files section its comment says it
  does not need. The first is what the docs promise.

- Nothing downstream reads `channels::Setup::pitchBendCents` yet. The page sets
  it, RPN 0 updates it and the owner is handed it, but no interval or frequency
  is computed from it — the same as when the value lived on the tuning page. It
  becomes real when the developer-facing API settles.

- improve appearance of what midi monitor looks like when learning, maybe should also be indicated elsewhere.
- there is something weird about lfo int as it goes from the middle to max but not below middle. demo synth will see an overhaul later, so wait for then.

## Double-checks you asked for in the docs

**`tuning.md:28-29` — "double check my interpretation is right".**
It is right. CC 101 = 0 then CC 100 = 3 selects Tuning Program Select; CC 6 sets
it; CC 96 and CC 97 step it; and 4 in place of 3 gives Tuning Bank Select. No
correction needed.

**`tuning.md:11-12` — "double check that 0.0061 c is less precise than MTS-ESP
and MIDI 2.0 and Scala files".** Two of the three, but **not MIDI 2.0**:

| | precision | vs 0.0061 c |
|---|---|---|
| MTS Sysex | 100/2¹⁴ = 0.0061 c | — |
| MTS-ESP | `double` frequencies in Hz | far finer |
| Scala | cents as decimal text; ratios to 2³¹−1 | far finer |
| MIDI 2.0 Per-Note Controller #3, Pitch 7.25 | 100/2²⁵ ≈ 0.000003 c | far finer **if implemented in full** |
| MIDI 2.0 Pitch 7.25, guaranteed floor | 9 fractional bits | **≈ 0.2 c — coarser** |
| MIDI 2.0 Note On Attribute #3, Pitch 7.9 | 1/512 HCU, fixed | **≈ 0.2 c — coarser** |

The catch is in UMP §7.4.15.2: "Support for all 25 bits of fractional pitch
resolution is **not mandated**. However, at least 9 bits should be supported
(strongly recommended)." And the Note On attribute form is *fixed* at 9 bits —
the spec gives its accuracy as "approximately 0.2 cents" itself (§7.4.15.3).

So "the precision is at least 0.0061 c — the limit for MTS Sysex" is not safe as
written: a conforming MIDI 2.0 sender can be twenty times coarser. Suggested:

> The precision is at least 0.0061 c, the limit for MTS Sysex. MTS-ESP and Scala
> files are finer. MIDI 2.0 can be far finer still — its per-note pitch carries
> 25 fractional bits — but only 9 of them are recommended rather than required,
> so a MIDI 2.0 source may be no better than 0.2 c.

**`tuning.md:85-86` and `channels.md` — "Can you do this?"**
Half of it. Splitting the answer, because the two halves have different answers:

- **Per-note tuning messages: yes, and they have names.** Registered Per-Note
  Controller #3 (Pitch 7.25) sets a note's pitch and *persists* for subsequent
  note ons, and the spec says outright that "a set of these messages for multiple
  Note Numbers can be used to define a complete tuning table for any and all 128
  Note Numbers" (§7.4.15.2). Note On with Attribute #3 (Pitch 7.9) does the same
  for one note only. Both **override MTS** where present.
- **Asking a device to use particular channels: no.** There is no mechanism for
  it, and no "extended channels" to ask for. Addressing has exactly four
  destinations — UMP Stream, Function Block, UMP Group, Channel (Appendix H,
  Table 35) — and a MIDI 2.0 Channel Voice Message goes to a Channel within one
  of 16 Groups. Nothing reserves Group 0 for MIDI 1.0 devices. A plugin also has
  no route to *ask*: MIDI-CI negotiation needs a bidirectional connection, which
  a plugin receiving MIDI from a host generally does not have.

What is true and might be what you meant: a device speaking MIDI 1.0 and one
speaking MIDI 2.0 can be kept apart because their **messages differ** — MIDI 1.0
Channel Voice is Message Type `0x2`, MIDI 2.0 Channel Voice is `0x4` — not
because they live on different channels. And one MTS tuning is per MIDI channel,
whereas a Pitch 7.25 table is per channel too, so 16 groups would give 256
independent tuning tables if a host ever presented them.

This is a design decision rather than a fact, so I have not touched either
sentence.

## Docs findings still open

Resolved ones have moved to [COMPLETED.md](COMPLETED.md). What is left needs a
decision or a sentence from you; nothing here has been changed in the docs.

**A. The MIDI 2.0 channel claim.** `channels.md` still ends "Channels 1 to 16 are
reserved for devices without MIDI 2.0 compatibility. MIDI Sidebar asks MIDI 2.0
devices to use the extended channels instead", and `tuning.md:85` says the same.
Neither is possible as written — see the double-checks above for why, and for
what *is* true. A design decision, so it is yours.

**B. Two drafts waiting to be pasted.** Both are in the double-checks above:
the RPN-as-control-changes sentence for `tuning.md` (you have already written
your own version at line 28, which is correct — so this one may be closed), and
the omni/MCM sentence for the `channels.md:2` comment, which you have also
already written into the body. **Check whether either is still outstanding**; I
think both are done and this item can go.

**C. MPE pitch-bend sensitivity and the tuning page.** Corrected: MPE *defaults*
sensitivity rather than forcing it (2 on the manager channel, 48 on members) and
RPN 0 can change both at any time. Your suggestion in the `tuning.md` comment — a
pitchbend section with 'global' and 'MPE member' — covers it. Say the word and I
will draft against that.

**E. `figures/rail-compact.png` has no home.** `rail.png` is now in the README;
the compact form — the rail below `metrics::regularBreakpoint`, where the volume
control is a button rather than a strip — is still generated and unused.

**High Priority**.

- **Name the `MTS Sysex` tuning standard something truer.** It now reads more
  than system exclusive: the tuning RPNs (0/3 program, 0/4 bank) are MTS but are
  not sysex, and master and channel tuning arrive there too and are not MTS at
  all — they are core MIDI and CA-025 Device Control.

  `MTS` is the smaller fix and is right about the tuning itself. **`MIDI`** is
  the honest one, and makes the four read as *sources* — MIDI, MTS ESP, files,
  standard, i.e. where the tuning comes from. `MIDI tuning` if the bare word is
  too broad inside a MIDI plugin. Touches `tuning::Scheme`, `schemeNames`,
  docs/tuning.md and the figures.

- **Pitch-bend sensitivity may want to move to the channels page.** Staying on
  the tuning page for now, by your call, and the reasoning is worth keeping
  because the two specifications pull opposite ways:

  - **MPE forbids per-channel variation.** §2.2.5: "Member Channels within the
    same Zone **shall not** have different Pitch Bend Sensitivity values. A
    receiver **shall** apply the last Pitch Bend Sensitivity message received on
    any Member Channel to all Member Channels in the Zone." So the current two
    fields are conformant rather than simplified, and a per-member-channel UI
    would let the end-user build an illegal state.
  - **Core MIDI and GM2 assume per-channel.** RPN 0 is an ordinary channel
    message with no uniformity rule. Under Omni On there is one instrument so it
    does not arise; under Omni Off each channel is its own part and per-channel
    is normal. GM2 §3.4.1 requires it per channel, default 2 semitones, "shall be
    able to accommodate at least ±12", rhythm channels excepted. Mode 4 has its
    own escape hatch — a controller on the channel *one below* the Basic Channel
    is a Global Controller affecting all voices, "though not all receivers may
    provide this function".

  So per-channel is only wanted for the non-MPE case, and the channels page is
  the only page with a per-channel UI — it also already owns the manager/member
  distinction the two tuning-page fields currently duplicate. Worth doing as **one**
  change: moving it without adding per-channel values buys nothing.

  One gap in the present fields either way: MPE's rule is **per Zone**, and there
  can be two. Strictly there are up to four values — lower manager, lower
  members, upper manager, upper members — and with both zones active the
  specification permits the two zones to differ.

- I'm frequently using 'toggle' to mean both a switch and a button that can be engaged or disengaged. The former should probably just be switch and the latter a toggle. Maybe there are more GUI termonology that I've misused? Check that I'm using words consistently in docs and skills. 
- Decide on how greying out inactive components should work.
- Check that skills are organised well and make suggestions on how they could be organsed better.
    - What is a good way for publishing skills. I could publish each skill as separate git repo, but that would be too many git repos. I could have them all in the same repo, but that removes modularity I need, so I don't love either of those solutions. Ig I could have different skills for different git organisations. That could work as a middle way, but I'm not super excited about it. Would be better if the directory structure was something like `skills/skill-package/specific-skill-[0-9][0-9]`
- Does MIDI 2.0 have a way of naming tunings like MTS ESP and Sysex and Scala files?
- Apply the read-only greying rule to the tuning page. The controllers monitor now draws every row dimmed, because nothing in it can be edited from the GUI; the same should hold wherever else that is true. Which fields exactly is an interaction question rather than a layout one — the status read-outs are clearly read-only, the settings section is not — so it was pinned rather than guessed at.
- Decide how the tuning page's settings persist. The callbacks are the seam; the question is which fields become APVTS parameters and which become properties on `apvts.state`. File paths are a poor fit for parameters.
**Further explanation**.
Nothing on any page persists — this is true of all four now, not just tuning, so the item is under-scoped. The real question is a split: scheme and update mode are enumerations and want to be APVTS parameters (automatable, host-visible); file paths, channel masks and tuning names want to be properties on apvts.state (ValueTree), because a path is not a parameter. Worth restating as "decide the persistence split for all four pages"

**Low Priority**.

- **Rank inferred periods by how simple a ratio they are.** Currently the default
  period is whichever candidate is closest to an octave, which is right for any
  equal division *of* the octave and a guess otherwise — it reports 1170 c for
  Bohlen-Pierce rather than the 1902 c tritave.

  A scale repeats when going up *n* degrees always multiplies the frequency by
  the same factor *r*, so a candidate is a pair (*n*, *r*). An equal division has
  one for **every** *n*: in 12edo one step multiplies by 2^(1/12), two steps by
  2^(2/12), and both are constant across the table. What distinguishes the real
  period is that its *r* is a simple ratio — exactly 2 for 12edo, exactly 3 for
  Bohlen-Pierce, 3/2 for 9ed(3/2) — while the intermediate candidates are
  irrational and near nothing simple.

  So: for each candidate *r*, find the simplest fraction p/q near it and score
  that by **Tenney height**, log₂(p·q), which is the standard measure of how
  consonant a ratio is. Lowest height wins. The simplest fraction in an interval
  comes from the Stern–Brocot tree or equivalently from a continued-fraction
  expansion; the Farey sequence F<sub>q</sub> is the same enumeration ordered by
  denominator, which is where the idea comes from.

  | scale | candidates | current answer | ranked answer |
  |---|---|---|---|
  | 12edo | 100 c, 200 c, … | 1200 c ✓ | 2/1 = 1200 c ✓ |
  | 13ed3 (Bohlen-Pierce) | 146 c, 293 c, … | 1170 c ✗ | 3/1 = 1902 c ✓ |
  | 9ed(3/2) | 78 c, 156 c, … | 1170 c ✗ | 3/2 = 702 c ✓ |

  Needs a tolerance — how close *r* must sit to p/q — and a ceiling on q so that
  a large denominator cannot win by brute force. Worth doing, but low priority:
  it only sets a **default** that the end-user can step away from, and it changes
  nothing that sounds. A tuning that states its own period never reaches
  inference at all.

- **Stop sounding notes when the zone changes.** §2.2.3 is a `shall`: "when a
  receiver changes its Zone configurations, the receiver shall stop all Sounding
  Notes and reset all controls to reasonable default values on each Channel
  entering or leaving MPE control." It is phrased about the *receiver's*
  configuration, so it covers the end-user moving the selector as well as an
  incoming MCM — and the plugin does neither, because it sends no MIDI at all
  yet. `onPanic` is still the stub `/* CC120 goes here once the processor sends
  MIDI. */`, and this wants the same machinery.

- Pitchbend quantization, see docs/tuning.md.
- Should `Sidebar` be changed to `SideBar` in similarity to `ToolBar` and `SidePanel`?
- The param column in the table should have some kind of header design. JUCE does not by default provide a header column. Background colour is not very informative though, as it's covered in buttons, so the border between that part of the table and the rest of the table. I have not yet decided on the design though.
- The headers in the table stll don't look the same as in the JUCE widgets demo. Arguably, they look more tasteful like this, so low priority, but the question remains why.
- Decide what the tuning page does when the panel is shorter than it needs — it wants ~394px of editor height and the sidebar's minimum is ~~212px~~ 252px, so at small sizes its lower sections are cut off. Candidates in [COMPLETED.md](COMPLETED.md#not-solved-small-heights): scroll, wrap the sections into two columns (needs a wider panel), or condense. The page is built out of section blocks so any of them is a layout change.
- Improve the design of the demo page and include further demo options.

Completed items live in [COMPLETED.md](COMPLETED.md).

## Ideas and questions moved out of the docs

These were `<!-- -->` blocks in `docs/`. They are open work rather than
documentation, so they live here and the docs no longer carry them.

**Let MIDI 2.0 use the remaining channels.** Appeared three times in
`tuning.md` as "maybe let MIDI 2.0 use remaining channels instead", plus a
"double-check MIDI 2.0" against the list of standards that support tuning
programs and banks. Now answerable in part — see the double-checks above — but
what the plugin *should* do is item A.

**Pitchbend quantization** (was `tuning.md`, IDEA). Quantize pitchbend to the
tuning table and list, on a scale from none at all to discrete steps and
everything in between. Autotune is believed to have an algorithm for the
in-between; the first thought was splines of varying degree with derivative 0 at
the tuning's frequencies, but that gets sharp too quickly as the degree rises. If
implemented, pitchbend becomes its own section. *(Already listed under Low
Priority; this is the detail behind it.)*

**Just-noticeable difference, and compounding error** (was `tuning.md`). Five
cents is the usual assumption for the JND and is what some tuner apps use. The
problem for *periods* is that individually-inaudible errors compound. One fix is
to correct for that. In careful psychophysics the JND is a function of both
loudness and frequency, which is probably overkill for engineering — but a report
surveying both audio tools and the psychophysics literature would settle it.

**Custom right-click options for particular developers** (was `right-click.md`,
inside the menu mockup). A developer might want to extend the parameter menu, for
example with `not modulated` / `add modulation from >` / `remove modulation`
below a separator. Worth deciding whether `onExtendMenu` is the whole answer.

**Two more demo settings** (was `demo.md`), in rough order of usefulness: the
animation speed, `Sidebar::setAnimationMilliseconds`, currently settable only in
code and where 0 is a legitimate value worth being able to try; and a level
generator so the meter has something to show without routing audio in.

**Does MIDI 2.0 communicate parameter names directly?** `controllers.md` says so
with an "I think?" attached. Partly: the mechanism is **MIDI-CI Property
Exchange** — "a set of MIDI-CI Transactions by which one device may access
properties from another device" (UMP §1.6.1) — not the protocol messages
themselves, and like all MIDI-CI it needs a bidirectional connection. I have not
read M2-101-UM, so the specifics are unverified; the sentence should probably
name Property Exchange rather than "MIDI 2.0".
