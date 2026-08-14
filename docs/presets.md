# Presets

In Presets, the end-user ...

<table>
    <tr>
        <td><button>split</button></td>
        <td><input value="220.00 Hz" /></td>
        <td><input value="440.00 Hz" /></td>
    </tr>
    <tr>
        <td><button>on</button></td>
        <td><input name="side" type="radio" />lower</td>
        <td><input name="side" type="radio" />upper</td>
    </tr>
</table>



![](figures/presets.png)



<!-- 

> **Status.** The GUI below is built; see [What is built](#what-is-built) at the
> end. Nothing behind it is — no file is written or read, and the split control
> reports its state without anything acting on it. -->

1. When one note active both show that note's frequency.
When several notes active the left shows the lowest frequency and the right shows the highest frequency.
(Mirroring the interval in the tuning menu.)
When no note is present it shows split point+crossfade.
If a sharp slitpoint (no crossfade) both show the same value.
2. A button whether split is active or not.
Toggle whether to edit/play the upper or lower split.
If split is active it is only editing, otherwise both.
3. Status
    1. Name of current preset
    2. Preset number and bank number
    If the bank is not specified, bank is empty.
4.  Files.
    1. Load or save preset files.
    2. If save, controller settings and tuning settings can be saved with the file. If loaded, the end-user can decide whether to ignore or use controller and tuning settings.
5. Meta as in metadata.
    1. Name of the author.
    2. Any other information. <!-- can be a larger textbox with several --> E.g., usage suggestions, license.


## What is built

`pages/PresetsPage`, on the same six columns and in the same framed sections as
the other two pages, and the last of the three. Every row of the sketch is a
whole number of columns: halves for the frequencies and for load/save, thirds
for split and the layer pair, and the 2+4 label split the other pages use.

The frequency pair is always **low and high** — one note shows it twice, several
show the extremes, and with nothing sounding it shows where the crossfade begins
and ends, equal when the split is sharp. One kind of quantity in both boxes.

`lower`/`upper` is a `ChoiceStrip`, the same control the tuning page uses for its
update mode, because it is the same kind of either-or. `split` is a latching
button, and takes the sidebar's accent when on rather than
`LookAndFeel_V4`'s toggled `TextButton`, which is drawn *darker* than its
neighbours and reads as disabled on a dark theme.

### Height

The controllers page's model, not the tuning page's: everything is a fixed row
except the comment box, which takes whatever is left. So the page has a real
minimum — about 416px of editor — and the comment grows into anything above it.
`commentMinimumRows` is two rather than three for exactly that reason: three put
the page's minimum above the default window, so the box was clipped at the size
most people will see.

### One deviation from the sketch

**The two include tick boxes are on separate rows**, not side by side. A tick
box plus the word "controllers" needs about 90px and a third of this page is 69,
so the sketch's arrangement elided it to "control...". The `include` label sits
beside the first and reads as covering both.

### The page grid

Written against `pages/PageGrid`, which was extracted from the other two pages
first — three copies of the same twenty lines was past any reasonable line. It
also carries two rules that are easy to get wrong: `place` counts content
columns and steps over the gutters itself, and `performLayout` never lays out
below the page's minimum, because a `Grid` answers a flexible track it cannot
fit by pulling later rows *upwards* and overlapping them.
