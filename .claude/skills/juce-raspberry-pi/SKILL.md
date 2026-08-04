---
name: juce-raspberry-pi
description: Build JUCE plugins and standalones for RPI (Raspberry Pi) Debian.
allowed-tools: WebFetch(domain:docs.juce.com) WebFetch(domain:forum.juce.com)
---

# JUCE Raspberry Pi

It should be possible to build JUCE projects for Raspberry Pi Debian.

## CLI options
Commandline options for standalones:
- Add a `--headless` flag that is false by default. Implement the corresponging GUI/headless functionality.
- Make it possible to connect audio and MIDI inputs and outputs over JACK and Pipewire. Audio out is mandatory, the others depend on the application. `--audio-in`, `--audio-out`, `--midi-in`, `--midi-out`. Each can accept a list of choices to enable e.g. stereo and multiple simultaneous MIDI controllers. Audio and MIDI ports can be specified by using regex expressions of their names.
- Make it possible to load associated files. A state file is mandatory. Tuning files, presets files and MIDI mapping files depends on the plugin.