# Tuning

In Tuning, the end-user can monitor tunings of individual notes, see the name, program number, and bank number, as well as the period.
The end-user can edit which tuning standard to use.
The end-user can choose between MTS ESP, MTS Sysex, <!-- MIDI 2.0, (maybe let MIDI 2.0 use remaining channels instead) --> tuning files, or standard.
The user can set associated parameters as well as pitchbend sensitivity.

MIDI Sidebar saves a table of frequencies per note per channel.
In addition, there is a list frequencies per note for an *unspecified channel*.
There can be multiple table+list pairs arranged in tuning programs and tuning banks.

![](figures/tuning.png)

A value in cents (two decimals) indicating the interval between the lowest and highest currently active notes.
0 c if a signle note and 'all notes off' if none.
For large values a modulo over 1200 is handy to quickly identify the interval.
1200 is the default value but can be edited.
(The post-modulo indicator is empty if all notes off.)

The status section shows the name of the tuning.
Not all tuning standards allow naming (MTS Sysex has only partial support) and, if so, it says 'no name' (standard is '12edo A4=440 Hz').
For tuning standards that allow tuning programs (and tuning banks) are MTS Sysex, <!-- MIDI 2.0, (maybe let MIDI 2.0 use remaining channels instead) --> <!-- TODO: double-check MIDI 2.0 --> and tuning files.
Tunings files can be arranged in a directory to form a bank, and several such directories can be opened together.
For these tuning standards the name is clickable and other tuning programs (and banks) are selectable.
If so, tuning programs and banks can also be explored numerically.
A time stamp when the tuning was last updated is useful for seeing whether the plugin is connected to a tuning master of one sort or another.
<!-- tuning program and bank can also be changed with (N)RPN messages somehow, but I don't recall exactly how, the sidebar should respond to these. >

The period section shows the period of a tuning.
For an equal division tuning, it is trivial—the step between two notes count as one period.
So does the distance between three, and four, and so on.
Therefore, the period indicator can be incremented or decremented among acceptable choices.
For tuning with uneven step sizes that are nonetheless arranged in a pattern, e.g. repeating every 12th note, the period is the interval across the pattern, e.g. 1200 c.
MTS ESP and some tuning files can specify the period.
If they do, 'specified' is indicated (and the period cannot be incremented/decremented).
Otherwise, the period is 'inferred'.
Period inference merges all the channels and sorts the frequencies.
By default the smallest possible period is shown.
If no period is found the entire set of frequencies is taken as the period.
<!-- TODO: specify the error in inference. think it should be 128**3 as per MTS Sysex standard but worth double-checking against the other standards. -->
Use cases:
A tonewheel organ has its drawbars tuned according to an underlying scale. To get the correct pitches for the higher notes, a period has to be inferred.
Similar ideas could be applied to any synthesizer with numerous oscillators.

In the settings page, the end-user sets up what tuning standard to use.
The name of the tuning standard is selected from a menu.
MTS ESP is used by default and then the plugin acts as an MTS ESP client and ignores other tuning data.
If MTS Sysex is selected the plugin listens to the relevant sysex messages but ignores the MTS ESP master.
If tuning files or standard is selected data of either kind is ignored.

The 'open files' <!-- probably replace load with open consistently --> button is active for the tuning files. <!-- this I also want to rename to use plural consistently -->
(It can also be used in MTS Sysex for `.syx` files.)
A single `.scl` file sets the tuning for the unspecified channel.
The end-user can select one `.scl` file and one or several `.kbm` files at the same time.
The suffix `_i.kbm` is the mapping for the ith channel.
Selecting one directory creates a bank with all the tuning files in that directory.
`.scl` and `.kbm` files with the same prefix are taken to belong to the same program.
As mentioned, a selection of several directories generates a set of banks.
If the end-user want to check what files are selected, they can press 'open files' and see them marked.

Tuning can be changed for a currently sounding note (always) or only applied to the next note on.
This is a relevant setting for MTS ESP.
For MTS Sysex messages the toggle cannot be set but works as an indicator.
Otherwise, it has no effect.

Pitchbend messages are never ignored, but sensitivity can be set, and a sensitivity of 0 is effectively ignoring them.
<!-- IDEA
Add a pitchbend quantization option.
Quantization is to the tuning table+list.
Quantisation can go from none at all to discrete steps and everything in between.
I believe Autotune has an algorithm for the in-between, but I don't know how it works.
My first thought is to use splines of different degrees and with derivative 0 at the frequencies in the tuning, but that seems to get sharp to quickly as you increase the degrees.
If implemented, pitchbned should become its own section. -->

MIDI 2.0 can send per-note tuning messages and is free to set those for the extended channels beyond the first 16. <!-- I don't feel like I know enough about midi 2.0 to really know how to implement it. -->

<!--
Approximation. 5 cents is typically assumed to be less than the just-noticeable difference.
This is for example used in some tuners and tuner apps.
A problem in using this for periods is that the errors can compound no noticeable differences.
So, one solution would be to correct for this.
In more precise psychophysical experiments, you see that the just-noticeable difference is a function of both loadness and frequency and.
This is probably overkill from the point of view of engineering.
However, it would be good if you can look into both audio tools and psychophysics papers and geenrate a report on the matter. -->

<!--

## What is built

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

### Not solved: small heights

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

### Not solved: persistence

Nothing on the page survives closing the editor yet. The callbacks are where
that attaches; whether the settings become APVTS parameters, properties on
`apvts.state`, or both, is still open — a file path is a poor fit for a
parameter, an enumerated setting is a good one.

-->