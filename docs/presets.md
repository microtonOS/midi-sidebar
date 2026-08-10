# Presets

<table>
    <tr>
        <td colspan="6"><b>PRESETS</b></td>
    </tr>
    <tr>
        <td colspan="3"><input type="text" value="220.00 Hz" style="width:2cm" /></td>
        <td colspan="3"><input type="text" value="440.00 Hz" style="width:2cm" /></td>
    </tr>
    <tr>
        <td colspan="2">
            <button>split</button>
        </td>
        <td colspan="2">
            <input type="radio" name="layer">lower</input>
        </td>
        <td colspan="2">
            <input type="radio" name="layer">upper</input>
        </td>
    </tr>
    <tr>
        <td colspan="6">STATUS</td>
    </tr>
    <tr>
        <td colspan="6"><input type="text" value="Jimmie Smith" />
    </tr>
    <tr>
        <td colspan="2">program</td>
        <td><input type="number" value="1" style="width:1cm" /></td>
        <td colspan="2">bank</td>
        <td><input type="number" value="" style="width:1cm" /></td>
    </tr>
    <tr>
        <td colspan="6">FILES</td>
    </tr>
    <tr>
        <td colspan="3"><button style="width:3cm">load</button></td>
        <td colspan="3"><button style="width:3cm">save</button></td>
    </tr>
    <tr>
        <td colspan="2">include</td>
        <td colspan="2"><input type="checkbox">controllers</input></td>
        <td colspan="2"><input type="checkbox">tuning</input></td>
    </tr>
    <tr>
        <td colspan="6">META</td>
    </tr>
    <tr>
        <td colspan="2">author</td>
        <td colspan="4"><input /></td>
    </tr>
    <tr>
        <td colspan="2">comment</td>
        <td colspan="4"><input /></td>
    </tr>
</table>

**Figure 1**.

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
