# Controllers

The outer table is for layout.
The inner tables are actual JUCE tables.

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