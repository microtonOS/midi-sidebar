# Right-Click

<select>
    <option disabled>FILTER CUTOFF</option>
    <option>info ></option>
    <hr />
    <option disabled>not assigned</option>
    <option disabled>view in sidebar</option>
    <option>MIDI learn</option>
    <option disabled>unlearn</option>
    <!-- particular developers might want cusom options like
    <hr />
    <option disabled>not modulated</option>
    <option>add modulation from ></option>
    <option disabled>remove modulation</option>
    -->
</select>

'info' expands into a short text explaining what the parameter is

'not assigned' changes to the appropriate summary of the assignment, e.g.
- channel 2 MSB 80
- omni on MSB 11 LSB 43
- multiple assignments
- channel 1 polytouch

'view in sidebar' highlights the relevant row(s) in the menu and expands the right page (if necessary).

'MIDI learn' automatic assignment of MSB for CC or aftertouch/polytouch. If MPE is on, assigned to omni off, else to the recorded channel.

'unlearn' remove (latest) mapping.