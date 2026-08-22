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
- ch 2 CC 80
- multiple assignments
- ch 1 polytouch

'view in sidebar' highlights the relevant row(s) in the table and expands the right page (if necessary).

'MIDI learn' automatic assignment of a CC, aftertouch or polytouch.
Choosing it opens the [controllers](controllers.md) page and takes the monitor over, which then asks the end-user to move a control and says what it has heard so far.
Move the control through its range rather than nudging it: learning watches the whole gesture and decides a second and a half after it stops.

> It watches rather than taking the first message because the first message is often the wrong one.
> A control sending two messages per movement — a coarse one and a fine one — offers both, and on at least one instrument the fine one arrives first.
> The fine message gives itself away by how it behaves: it wraps from its maximum back to zero over and over during a sweep, so it jumps where the coarse one steps, and the one that jumps is discarded.
> This needs no assumption about which numbers are meant to be fine ones, which is just as well, since instruments disagree about that.

> Notes and pitchbend are not learnable: a keyboard sends a note whenever it is played, so learning would catch the first key pressed rather than the control the end-user reached for, and pitchbend belongs to the [tuning](tuning.md) page.
> Numbers the table refuses are not learnable either, so a gesture never produces a row that is red the moment it appears.
> A *single* message on a CC between 32 and 63 is also ignored, since one message shows no behaviour at all and MIDI reserves those numbers as the fine halves of controllers 0 to 31 — but a control that sweeps there is learned like any other.

'unlearn' removes every mapping for the parameter, not only the most recent one.

> A parameter that has been learned several times is one the end-user wants to stop responding, and clearing one of three assignments leaves it responding.