# Controllers

The outer table is for layout.
The inner tables are actual JUCE tables.

> **Status.** The GUI below is built; see [What is built](#what-is-built) at the
> end. Nothing behind it is — no MIDI is read, and none of the five modes does
> anything yet. They are choices the table can express.

<table>
    <tr>
        <td colspan="2">
            <b>CONTROLLERS</b>
        </td>
    </tr>
    <tr>
        <td colspan="2">
            <table>
                <!--
                <tr>
                    <th>type</th>
                    <th>channel</th>
                    <th>note/cc</th>
                    <th>value</th>
                </tr>
                -->
                <tr>
                    <td>control</td>
                    <td>16</td>
                    <td>11</td>
                    <td>98</td>
                </tr>
                <tr>
                    <td>sysex</td>
                    <td></td>
                    <td></td>
                    <td></td>
                </tr>
                <tr>
                    <td>note on</td>
                    <td>15</td>
                    <td>A4</td>
                    <td>102</td>
                </tr>
            </table>
        </td>
    </tr>
    <tr>
        <td colspan="2">FILES</td>
    </tr>
    <tr>
        <td><button>load</button></td>
        <td><button>save</button><td>
    </tr>
    <tr>
        <td colspan="2">EDITING</td>
    </tr>
    <tr>
        <td colspan="2">
            another table goes here,<br />see Figure 2 below
        </td>
    </tr>
    <tr>
        <td><button>add</button></td>
        <td><button>remove</button><td>
    </tr>
</table>

**Figure 1**.

For recent messages at the top, only the last 3 messages are shown.


<table>
    <tr>
        <th>param</th>
        <th>chan</th>
        <th>MSB</th>
        <th>LSB</th>
        <th>mode</th>
        <th>min</th>
        <th>max</th>
    </tr>
    <tr>
        <th>
            <select>
                <option selected>swell</option>
                <option>rotary</option>
            </select>
        </th>
        <td><select>
            <option selected>omni</option>
            <option>1</option>
            <option>2</option>
            <option>4</option>
            <option>5</option>
            <option>6</option>
            <option>7</option>
            <option>8</option>
            <option>9</option>
            <option>10</option>
            <option>11</option>
            <option>12</option>
            <option>13</option>
            <option>14</option>
            <option>15</option>
            <option>16</option>
        </select></td>
        <td><input type="number" value="11" style="width:1cm"/></td>
        <td><input type="number" value="43" style="width:1cm"/></td>
        <td>
            <select>
                <option selected>jump</option>
                <option>catch</option>
                <option>scale</option>
                <hr />
                <option >toggle</option>
                <option>inc</option>
            </select>
        </td>
        <td>
            <input type="text" value="10 %" style="width:1cm"/>
        </td>
        <td>
            <input type="text" value="100 %" style="width:1cm"/>
        </td>
    </tr>
    <!----------------------------------->
    <tr>
        <th>
            <select>
                <option>swell</option>
                <option selected>rotary</option>
            </select>
        </th>
        <td><select>
            <option>omni</option>
            <option>1</option>
            <option>2</option>
            <option>4</option>
            <option>5</option>
            <option>6</option>
            <option>7</option>
            <option>8</option>
            <option>9</option>
            <option>10</option>
            <option>11</option>
            <option>12</option>
            <option>13</option>
            <option>14</option>
            <option selected>15</option>
            <option>16</option>
        </select></td>
        <td><input type="number" value="64" style="width:1cm"/></td>
        <td><input type="number" value="" style="width:1cm"/></td>
        <td>
            <select>
                <option>jump</option>
                <option>catch</option>
                <option>scale</option>
                <hr />
                <option selected>toggle</option>
                <option>inc</option>
            </select>
        </td>
        <td>
            <input type="text" value="1" style="width:1cm"/>
        </td>
        <td>
            <input type="text" value="3" style="width:1cm"/>
        </td>
    </tr>
</table>

**Figure 2**.

In the editing table, the leftmost header cells—the parameter names—should always be visible as you scroll in the left–right directions.
Likewise, the topmost headers should always be visible when you scroll in the up–down directions.
This does not mean that all header cells are always visible. An up–down scroll may change the visible parameter names and vice versa.


The front panel knobs can operate in one of three modes:[^korg]
- Jump: When you turn the knob, the parameter value will jump to the value indicated by the knob.
Since this makes it easy to hear the results while editing, we recommend that you use this setting.
- Catch: Turning the knob will not change the parameter value until the knob position matches the
stored value. We recommend that you use this setting when you don’t want the sound to change
abruptly, such as while performing.
- Scale: When you turn the knob, the parameter value will increase or decrease in a relative manner in the direction that it is turned. When you turn the knob and it reaches the full extent of its
motion, it will operate proportionate to the maximum or minimum value of the parameter. Once
the knob position matches the parameter value, the knob position and parameter value will subsequently be linked.


Two more options ignore LSB:
- Toggle: Whenever a controller emits a value at least 64, the toggle switches. min and max can be swapped for a polarity change.
- Inc(rement): Whenever the controller emits a value of at least 64, it is interpreted as going from CC value x to x+1 (at most 127). min and max can be swapped for decrement.

[^korg]: [Korg. *Minilogue XD—Owner's Manual*.](https://cdn.korg.com/us/support/download/files/efbf7ff0140570942060130b28f96ae6.pdf?response-content-disposition=inline%3Bfilename%2A%3DUTF-8%27%27minilogue_xd_xdMod_OM_E9.pdf&response-content-type=application%2Fpdf%3B)


## What is built

`pages/ControllersPage` and `pages/ControllerTable`, on the same six-column grid
and in the same framed sections as the tuning page. The demo fills both tables
with the figures above; nothing generates them.

### The frozen column

**JUCE has no frozen column.** `TableListBox` pins the header *row* — that is
what `TableHeaderComponent` is, and it gives the up–down half of the requirement
for free — but nothing anywhere pins a column. So the editing table is two views
of one list side by side: a `ListBox` holding the parameter names and a
`TableListBox` holding the six columns that scroll, tied together through the
table's vertical scrollbar.

Three details make the pair behave as one table rather than two:

- The scrollbar of a `Viewport` is ranged over its content in **pixels**, not
  rows, so the offset copies straight across; both lists hold the same rows at
  the same height, so the same offset means the same row.
- The frozen column shows no scrollbars and takes no mouse clicks. Its
  ComboBoxes still do — children are unaffected — while a wheel over its
  background falls through to the table. Without that, spinning the wheel over
  the names would slide them out of step with the rows beside them.
- `ControllerTable` implements *both* models. `ListBoxModel` and
  `TableListBoxModel` are unrelated interfaces that happen to declare the same
  `getNumRows()`, so one implementation serves both — which is exactly the
  invariant wanted: the two views cannot disagree about how many rows there are.

### Menus are buttons, not ComboBoxes

Every choice — the parameter, the channel, the mode, and the tuning page's
scheme — is a `ChoiceButton`: a plain button showing the current value that
opens a `PopupMenu` of the alternatives, with the current one ticked. This is
the pattern JUCE's own Widgets demo uses on its Menus page.

A `ComboBox` draws its arrow inside its own bounds, and it costs about twenty
pixels. Affordable on a page, ruinous in a table cell forty-odd pixels wide,
where it rendered "omni" as "o...". Removing it gave the scrolling columns
twenty-six pixels back, which is a whole extra column visible at a time.

Note that **the snapshot tool cannot click these**: `PopupMenu::getParentArea`
dereferences `getDisplayForPoint(...)` without a null check
(`juce_PopupMenu.cpp:920`), and a headless process has no displays, so any menu
segfaults there. A JUCE bug, unrelated to this project, and only reachable
without a display. The menus have to be checked in the standalone.

### Sorting

The parameter column has no title, so that corner of the header holds a two-way
toggle instead: a clock icon, and `abc`.

- **Clock** orders by when the mapping was added, newest at the top. `add` then
  puts the new row where you are looking, and selects it.
- **`abc`** orders by parameter name, a at the top. The sort is *stable*, so
  several mappings onto one parameter keep the order they were added in rather
  than shuffling whenever the table is rebuilt.

Only the *view* is sorted. `getMappings()` always returns them in the order they
were added, so an owner's indices never move under it; a display row is
translated through `mappingIndexFor` before anything touches the data. The clock
is the default, since insertion order is what you typed.

It is not a `ChoiceStrip` — one of the two is an icon and a strip builds
`TextButton`s — but it takes the same accent for "this one is on", and the clock
uses `DrawableButton::ImageOnButtonBackground` so that it is drawn by the same
`drawButtonBackground` as every other button. The other image styles go to
`drawDrawableButton` instead, which fills a plain rectangle and ignores
connected edges, leaving a square-cornered button beside a rounded one.

### Selecting a row

A cell's widget swallows the click that would otherwise reach the table, so
without help a row could only be selected by hitting the few pixels between the
widgets — which made `remove` feel broken. Each cell therefore listens to its
own widget and selects its row on `mouseDown` before the click does whatever it
was for; JUCE's WidgetsDemo solves it the same way, with
`selectRowsBasedOnModifierKeys`. Clicking a parameter name, a menu or a number
now selects the row it is in.

### Decisions the sketch left open

- **min/max are plain numbers and the unit belongs to the parameter.** A row
  retargeted at another parameter relabels its limits with nothing to keep in
  step: `swell` shows `10 %`, `rotary` shows `1`. Typing strips the unit and puts
  it back on commit.
- **The two threshold modes disable that row's LSB cell.** The doc says toggle
  and increment ignore the LSB, so the cell is greyed rather than hidden — the
  number is still part of the mapping, it just has no effect.
- **`add` inserts an empty mapping** and selects it; `remove` takes the selected
  row, or the last one when nothing is selected, so the button is never dead
  while there is something to remove.
- The `chan` menu lists 1–16. Figure 2 skips 3, which is taken as a typo.

### This page owns its data

Unlike the tuning page, which holds nothing and pushes every edit out for the
owner to push back, the mappings live in `ControllerTable`. A table cannot be
rebuilt from outside on every keystroke without fighting the editor being typed
into. `onMappingsChanged` fires afterwards and `getMappings()` returns the
result; an owner may still replace the whole list at any time.

### Height

The reverse of the tuning page, and the better pattern for the presets page to
copy. Everything except the editing table is a fixed height — three monitor
rows, two rows of buttons, two frames — and the table is the single flexible
track, so the page has a genuine minimum (about 314px of editor) and grows into
anything above it rather than being cut off.

Below that minimum the page is laid out at its minimum and clipped, rather than
letting the flexible track go negative: a `Grid` answers a negative track by
pulling the rows after it *upwards*, which put `add` and `remove` on top of the
FILES frame. Clipping is the honest failure. What to do about small heights
generally is still the open item in [TODO.md](../TODO.md).