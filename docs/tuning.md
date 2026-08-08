# Tuning

<!-- I have used allcaps bold, allcaps narrow, and all miniscule to indicate different header levels. I don't know if that make sense in the real ui. Then they should mayb all be allcaps. Is there a look and feel settings for allcaps, inital cap, all miniscule?

There is no such setting. JUCE has no text-transform anywhere in Font, Label or
LookAndFeel: a label draws the string it is given, so anything in capitals is in
capitals because someone typed it that way. What is implemented is:

  Tuning     panel title, 15px bold, title case
  STATUS     group title, uppercased, drawn into the frame's top edge
  program    field label, 13px regular, lower case

which keeps your three levels while letting the top one stay title case. The
frame carries the section far better than capitals alone did at this size, and
the two titles are indented to the same place, so "Tuning" sits above the "S" of
"STATUS" rather than above the frame around it. -->

> **Status.** The GUI described below is built; see [What is built](#what-is-built)
> at the end for what that does and does not include. Nothing behind it is —
> no MTS ESP, no sysex, no `.scl` parsing.

<table style="border: 0px">
    <!-- The table is for layout, the table borders are to be ignored for example. Sizes are not to be taken literally. -->
    <tr>
        <td colspan="6"><b>TUNING</b></td>
    </tr>
    <tr>
        <td colspan="6">
            <input type="text" value="1902.98 c" style="width: 7cm" readonly />
        </td>
    </tr>
    <tr>
        <td>
            mod
        </td>
        <td colspan="2">
            <input type="number" value="1200" style="width: 1.4cm"/>
        </td>
        <td>
            =
        </td>
        <td colspan="2">
            <input type="text" value="702.98 c" style="width: 1.5cm" readonly/>
        </td>
    </tr>
    <tr>
        <td colspan="6">
            ---- STATUS --------
        </td>
    </tr>
    <tr>
        <td colspan="6">
            <input type="text" value="Unnamed" style="width: 7cm" readonly />
        </td>
    </tr>
    <tr>
        <td colspan="2">
            program
        </td>
        <td>
            <input type="number" value="" style="width: 1.5cm" readonly>
        </td>
        <td colspan="2">
            bank
        </td>
        <td>
            <input type="number" value="" style="width: 1.5cm" readonly>
        </td>
    </tr>
    <tr>
        <td colspan="2">
            updated
        </td>
        <td colspan="4">
            <input type="text" value="21:25:38" style="width: 5cm" />
        </td>
    </tr>
    <tr>
        <td colspan="6">
            ---- PERIOD --------
        </td>
    </tr>
    <tr>
        <td colspan="3">
            <input type="text" value="1200 c" style="width: 3cm"/>
        </td>
        <td colspan="3">
            <select style="width: 3cm" readonly>
                <option>inferred</option>
                <option>specified</option>
                <option>edited</option>
            </select>
        </td>
    </tr>
    <tr>
        <td colspan="6">
            ---- SETTINGS -------
        </td>
    </tr>
    <tr>
        <td colspan="3">
            <select style="width: 3cm">
                <option>MTS ESP</option>
                <option>MTS sysex</option>
                <option>tuning file</option>
                <option>MPE</option>
                <option>MIDI 2.0</option>
                <option>standard</option>
            </select>
        </td>
        <td colspan="3">
            <button>channels</button>
        </td>
    </tr>
    <tr>
        <td colspan="3">
            <input type="file" accept=".scl" style="width: 3cm"></input>
        </td>
        <td colspan="3">
            <input type="radio" name="query">note on</input>
        </td>
    </tr>
    <tr>
        <td colspan="3">
            <input type="file" accept=".kbm" multiple style="width: 3cm"></input>
        </td>
        <td colspan="3">
            <input type="radio" name="query">always</input>
        </td>
    </tr>
</table>

**Figure 1**.


Consider the sketch above.<!--It is in the form of a table, but the table cells are only there to illustrate how the widgets should be laid out on an invisible grid. It is not to be visible in other words. The design choices of the html elements are of no importance whatsoever—the design should follow the rest of the plugin.-->
Here follows comments on the layout, row by row:

1. Title of indicating the tuning panel.
2. The interval in cents between the lowest active note and the highest active note. Empty if no active notes.
3. Modulo of the interval above. By default modulo over 1200 but can be set by the end-user. <!-- sounds correct to have the mod value unitless and the = value with unit? I mean it is a kind of division -->
4. The tuning status box starts here.
5. The name of the tuning. If unknown the name should be "Unnamed". An MTS ESP master can set a string as the name of the tuning. MTS SYSEX messages can set a 16 ASCII character name for some of the messages. `.scl` files can specify the name of a tuning at the top of the file. MPE and pitchbend do not provide a tuning name. MIDI 2.0 I don't know. STANDARD should be 12edo in general but maybe Gear60 or Gear50 for tuneBfree.
6. Tuning program number and tuning bank number. Empty if unknown/unspecified.
7. Time stamp of when tuning was last updated. If using MTS ESP queries will happen continuously (maybe several times per second), so this will look like a ticking clock. That way you see that it's active. However, it can also geneeralise to other systems. For sysex it would be when the last mts sysex message was received. for mpe when the last pitchbend was received (or really last pitchbend or note on or cc or aftertouch, ...). For tuning files, it would be when the file was loaded into the plugin.
8. End of status box, Beginning of tuning period box.
9. The scale period in cents. It can be specified in MTS ESP or `.scl` files (the last specified pitch basically acts as a specification of the period). the second wisget indicates whether it was inferred, <!-- see tuneBfree for the algorithm to infer the tuning period -->, specified, or edited, i.e. set by the user in the widget to the right.
10. Title marking the beginning of the microtuning settings section.
11. Two columns.
    - Left. Which of the microtuning encoding schemes used. For each the last state should be saved so that you can toggle back to it.
    - Right. A popup window to select channels, see below.
12. Two columns.
    - Left. Upper is uploading the scale file, e.g. `.scl`. Should probably say SCALE somewhere. Lower is the mapping file. Should probably say "MAPPING" or "MAP". Can select directory of .kbm files or multi-selection of .kbm files. if so `_<i>.kbm` files will be attached to midi channel `<i>`. If there are several different files with different names but the same `_<i>.kbm` suffix, then the most recently selected one is prioritised. If there is a `x.kbm` suffix where `x` does not indicate a midi channel, then it is taken as a "generic channel". The last selected file for a "generic channel" is the prioritised one. The "generic channel" is any channel that is not specifically assigned. `.scl` files can be used without any `.kbm` file, if the `.kbm` files are underspecified, you can fall back to this, e.g. if there are channel files but no generic file, then you can fall back on that option for the "generic channel".[^PopUpWarning]
    - Right. Here represented as radio buttons but really a toggle. If note on is checked, then pitches should only be updated on note on events. <!-- This is probably preferable for tuneBfree considering the building of the wavetable. --> If continuously is checked than a note that is already sounded can have its pitch changed. This is based after the MTS ESP convention but can be applied to mpe and midi 2.0 as well. Wrt sysex it's less clear that you need to specify this as the sysex messages themselves can specify which one it is, maybe let the note on option override continuous sysex messages? Not really applicable to tuning files.


Let us return to the popup window.
There are a number of checkbox buttons to select one or more channels. (Technically it's possible to select no channels but this is unadvisable.)
OMNI OFF is default. It means that each channel is handled separately according to it's corresponding `_<i>.kbm` file or corresponding MTS ESP channel.
All other channels that are not specified are taken to be the "generic channel".
If OMNI ON, then all channels are mapped to the "generic channel". This is `-1` in MTS ESP code—although I suspect that in the shared library, `-1` is really the same as `0` (MIDI channel 1).
SELECT ALL selects all the channels
DESELECT ALL removes all the selections.

<table>
    <tr>
        <td>
            <input type="radio" name="omni">omni on</bitton>
        </td>
        <td>
            <input type="checkbox" name="ch"> 1</input>
            <input type="checkbox" name="ch"> 2</input>
            <input type="checkbox" name="ch"> 3</input>
            <input type="checkbox" name="ch"> 4</input>
        </td>
    </tr>
    <tr>
        <td>
            <input type="radio" name="omni">omni off</input>
        </td>
        <td>
            <input type="checkbox" name="ch"> 5</input>
            <input type="checkbox" name="ch"> 6</input>
            <input type="checkbox" name="ch"> 7</input>
            <input type="checkbox" name="ch"> 8</input>
        </td>
    </tr>
    <tr>
        <td>
            <button>select all</button>
        </td>
        <td>
            <input type="checkbox" name="ch"> 9</input>
            <input type="checkbox" name="ch">10</input>
            <input type="checkbox" name="ch">11</input>
            <input type="checkbox" name="ch">12</input>
        </td>
    </tr>
    <tr>
        <td>
            <button>deselect all</button>
        </td>
        <td>
            <input type="checkbox" name="ch">13</input>
            <input type="checkbox" name="ch">14</input>
            <input type="checkbox" name="ch">15</input>
            <input type="checkbox" name="ch">16</input>
        </td>
    </tr>
</table>




The space should be consistent between the widgets.
The spaces above the two titles can be used as empty space if necessary, but it should be the same amount above each title.



**Period Inference and Specification.**
Seeing what the period is is actually not that important.
What it is used for is to tune the drawbars, or, rather, to quantize them to the closest pitch in the scale.
To do this, you simply have to merge all the midi channels into one list of frequencies.
Then, you order this list according to ascending pitch (and maybe remove duplicate pitches?).
(Now, it makes no sense to speak of a period with a negative number of cents.)
For the lower pitches, you can simply set the drawbars according to the closest pitch of the available ones in the tuning.
But for higher pitches, the drawbars may end up outside the tuning table.
If a period is found, you can use this to extend the tuning table.
Otherwise, you simply take the entire tuning table as a period.
(Now, the period would simply be the distance between the lowest frequncy in the tuning table and the highest.)
This is what I assume the code already does, but without the multichannel generalisation, is that right? Also I assume that it only works for exact periods?

<!--
Approximation. 5 cents is typically assumed to be less than the just-noticeable difference.
This is for example used in some tuners and tuner apps.
A problem in using this for periods is that the errors can compound no noticeable differences.
So, one solution would be to correct for this.
In more precise psychophysical experiments, you see that the just-noticeable difference is a function of both loadness and frequency and.
This is probably overkill from the point of view of engineering.
However, it would be good if you can look into both audio tools and psychophysics papers and geenrate a report on the matter. -->


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
something that happened. The period source is a read-only indicator, and typing
in the period field is what flips it to `edited`; anything that knows better
pushes `inferred` or `specified` back through `setPeriod`.

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

