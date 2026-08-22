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

'MIDI learn' automatic assignment of an MSB, aftertouch or polytouch.
A window appears saying 'Move controller!'.
Once something has moved it says 'Release controller!', and the mapping is made a moment after the controller is released.
Cancel closes it without adding anything.
The monitor keeps showing incoming messages throughout, so the stream stays readable while the gesture is made.

> It waits for a whole gesture rather than taking the first message, because the first message is often the wrong one.
> A control sending both a coarse and a fine message offers both, and on at least one instrument the fine one arrives first.
> The fine message gives itself away by how it behaves: it wraps from its maximum back to zero over and over during a sweep, so it jumps where the coarse one steps, and the one that jumps is discarded.
> This needs no assumption about which numbers are meant to be fine ones, which is just as well, since instruments disagree about that.

> Only the MSB is learned. An LSB is a refinement the end-user states in the table, not something to guess from a gesture.

> Notes and pitchbend are not learnable: a keyboard sends a note whenever it is played, so learning would catch the first key pressed rather than the control the end-user reached for, and pitchbend belongs to the [tuning](tuning.md) page.
> Numbers the table refuses are not learnable either, so a gesture never produces a row that is red the moment it appears.
> A *single* message on a CC between 32 and 63 is also ignored, since one message shows no behaviour at all and MIDI reserves those numbers as the fine halves of controllers 0 to 31 — but a control that sweeps there is learned like any other.

'unlearn' removes every mapping for the parameter, not only the most recent one.

> A parameter that has been learned several times is one the end-user wants to stop responding, and clearing one of three assignments leaves it responding.