# TODO

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

**D. Should the LSB column derive itself from the MSB?** The spec makes a 14-bit
pair CC *n* and CC *n+32* with n ≤ 31 and nothing else, but you note the
minilogue xd does not follow it. So: derive the LSB and forbid the rest, or leave
it free and flag a mismatch? The second tolerates real hardware.

**E. `figures/rail-compact.png` has no home.** `rail.png` is now in the README;
the compact form — the rail below `metrics::regularBreakpoint`, where the volume
control is a button rather than a strip — is still generated and unused.

**F. Build the invalid-cell colour.** Per your reply: a `pageColours` id that is
some kind of red whatever the theme is, plus the validation that turns a cell on.
Nothing validates MSB/LSB against the protected list yet. The colour table in
`appendices.md` has a comment marking where its row goes.

**High Priority**.

- I'm frequently using 'toggle' to mean both a switch and a button that can be engaged or disengaged. The former should probably just be switch and the latter a toggle. Maybe there are more GUI termonology that I've misused? Check that I'm using words consistently in docs and skills. 
- Decide on how greying out inactive components should work.
- Check that skills are organised well and make suggestions on how they could be organsed better.
    - What is a good way for publishing skills. I could publish each skill as separate git repo, but that would be too many git repos. I could have them all in the same repo, but that removes modularity I need, so I don't love either of those solutions. Ig I could have different skills for different git organisations. That could work as a middle way, but I'm not super excited about it. Would be better if the directory structure was something like `skills/skill-package/specific-skill-[0-9][0-9]`
- Does MIDI 2.0 have a way of naming tunings like MTS ESP and Sysex and Scala files?
- Attach the volume fader to its APVTS parameter. Both are already in dB over the same range with the same floor, so this is small — the sidebar just needs to expose the fader, or take a value + callback so the module stays free of `juce_audio_processors`.
**Further explanation**.
The obstacle: Sidebar owns the fader as a private member and the module has no juce_audio_processors dependency, so it cannot hold a SliderAttachment. Either the module exposes the Slider& (cheap, leaks a widget into the API) or it takes a value + onVolumeChanged callback and the demo bridges to a ParameterAttachment (keeps the seam, one more moving part). Everything else already lines up: DemoProcessor's volume parameter is dB over the same range with the same metrics::floorDb
- Apply the read-only greying rule to the tuning page. The controllers monitor now draws every row dimmed, because nothing in it can be edited from the GUI; the same should hold wherever else that is true. Which fields exactly is an interaction question rather than a layout one — the status read-outs are clearly read-only, the settings section is not — so it was pinned rather than guessed at.
- Decide how the tuning page's settings persist. The callbacks are the seam; the question is which fields become APVTS parameters and which become properties on `apvts.state`. File paths are a poor fit for parameters.
**Further explanation**.
Nothing on any page persists — this is true of all four now, not just tuning, so the item is under-scoped. The real question is a split: scheme and update mode are enumerations and want to be APVTS parameters (automatable, host-visible); file paths, channel masks and tuning names want to be properties on apvts.state (ValueTree), because a path is not a parameter. Worth restating as "decide the persistence split for all four pages"

**Low Priority**.

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
