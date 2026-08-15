# Presets

A preset is a collection of parameter values in the host plugin.
In Presets, the end-user can get two presets in one by arranging a keyboard split.
They can navigate presets by name, program number and bank number.
They can open and save presets.
They can view and write metadata.

![](figures/presets.png)

The end-user can create a keyboard split by pressing the split button.
When engaged, notes are turned off and muted.
The indicated frequencies are those of the lowest and highest active notes.
If the indicated frequencies are equal, then that is the frequency for a hard split point.
If they are different, they indicate the lower and upper bounds of a crossfade.
The frequencies can also be set directly in the text fields.

When the 'active' button is engaged, the host plugin plays the split.
The lower and upper switch controls what side of the split is being edited.
Some developers may choose to have an entire duplicate of the plugin, and the switch controls which copy is being edited.
Some developers may choose to only duplicate a subset of parameters.
Some plugins, like the lower and upper manuals of a clonewheels, may expose all copied parameters at once in the plugin.
In that case, the lower and upper switch has no effect when the 'active' button is engaged.
When it is disengaged, lower and upper is simply switches the entire keyboard between the parameter settings for lower and upper.
MIDI does not have a standard for keyboard splits, but all of the split parameters can be [MIDI-learned](right-click.md).

In the status section, the name of the preset is indicated.
If the preset has been edited, an asterisk appears to the right.
If the preset name is clicked, a menu appears to choose between different preset programs (and banks).
(The name can be edited whenever the file is saved, see below.)
In addition, preset program numbers and preset bank numbers can be incremented and decremented.
These numbers also respond to program change messages and bank select control change (CC0 and CC32).

In the file section, the end-user can open or save a preset file.
The default format is `.xml` but the developer may use their own format.
Opening a directory of preset files interprets the directory as a bank (the developer may also have their own file format for banks).
Opening several such directories opens several banks.
When saving a preset, the end-user is prompted to name the preset.

In the metadata section, the end-user can optionally add any other relevant information about the preset.
The name of the author can be set.
Comments may include a license, a suggestion on how to use the preset, or any other information.
