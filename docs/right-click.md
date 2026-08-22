# Right-Click

<select>
    <option disabled>FILTER CUTOFF</option>
    <option>info ></option>
    <hr />
    <option disabled>not assigned</option>
    <option disabled>view in sidebar</option>
    <option>MIDI learn</option>
    <option disabled>unlearn</option>
</select>

'info' expands into a short text explaining what the parameter is

'not assigned' changes to the appropriate summary of the assignment, e.g.
- ch 2 MSB 80
- multiple assignments
- ch 1 polytouch

'view in sidebar' highlights the relevant row(s) in the table and expands the right page (if necessary).

'MIDI learn' automatic assignment of a control change, aftertouch or polytouch.
A window appears saying 'Move controller!'.
Once something has moved it says 'Release controller!', and the mapping is made a moment after the controller is released.
Cancel closes it without adding anything.
If a control change i is between 1 and 31 and is followed by another control change i+32 and the latter acts as an LSB (least significant byte), then they are learned as an MSB–LSB pair.
Otherwise, only the MSB (most significant byte) is learned.
See [Controllers](controllers.md).

'unlearn' removes every mapping for the parameter, not only the most recent one.
