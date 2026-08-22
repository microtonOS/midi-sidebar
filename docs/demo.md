# Demo

<!-- Probably put the ./demo/ code in a ./demo/host/ directory.
Add one ./demo/tuning/ directory with some example .scl, .kbm, .sux files.
Add one ./demo/presets/ with some example .xml files -->

Not part of the MIDI Sidebar.
A demo host used for testing the GUI without embedding the sidebar in a plugin.
A "DEMO HOST" watermark is displayed in large diagonal text over the demo host, but not the MIDI Sidebar.
At the top of the page ther is a header with tabs to explore developer options and an example synth.

## Developer Options
A number of widgets (where the host plugin is supposed to be) makes it possible to test different developer settings.

Buttons for:

- Changing the theme between look and feel V4 Dark, Midnight, Grey, and Light.
- Forcing the text in slider value bubbles white or black, or leaving it as the
- Changing whether the sidebar is on the left or the right.
- Changing whether the sidebar glides on top of the header or just underneath. (A similar setting also exists for a footer if there exists one, but this is not exemplified in this demo.)



## Example Synth

A demo plugin is an alternative page to the above.
It is a simple combo organ/subtractive synthesizer.
It has:
- Two square wave oscillators.
- The volume can be set for each and each volume can be set on a per-note basis.
- For the secondary oscillator you can set the pitch compared to the primaro oscillator.
The pitch is at most ±2400c.
The pitch is per keyboard split.
- The oscillator can also be quantised to whatever the current scale is. This illustrates one use of the period setting.
Quantisation is set with an on/off switch.
- One filter with cutoff (frequency) and resonance (Q-value).
These are per keyboard-split and lack key tracking.
- One LFO with target 3-way switch for filter and pitch of the secondary oscillator (chorus) and pitch for both (vibrato). One rate control and one intensity control. Filter LFO rate, filter LFO intensity, pitch LFO rate, pitch LFO intensity, and LFO target are all different parameters. Per keyboard split.