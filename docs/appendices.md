# Appendices

## Conventions

Pitch intervals are given in cents with two decimals.

## Glossary

**aftertouch** channel aftertouch unless stated otherwise <br />
**AT** (channel) aftertouch <br />
**CC** control change <br />
**ch** channel <br />
**edo** equally divided octave <br />
**ESP** extrasensory perception <br />
**LSB** least significant byte <br />
**manager channel** the channel carrying messages for a whole MPE zone <br />
**member channel** a channel within an MPE zone that is not its manager channel <br />
**MPE** MIDI polyphonic expression <br />
**MSB** most significant byte <br />
**MTS** MIDI tuning standard <br />
**NRPN** non-registered parameter number <br />
**omni** listening to multiple MIDI channels while ignoring channel numbers <br />
**PC** program change <br />
**PT** polyphonic aftertouch (polytouch) <br />
**polytouch** polyphonic aftertouch <br />
**RPN** registered parameter number <br />
**Sysex** system exclusive <br />
**unspecified channel** 128 frequencies in the tuning table not corresponding to any MIDI channel <br />

## Colours

Every colour the sidebar draws is derived from four of the nine in whichever
`juce::LookAndFeel_V4::ColourScheme` the developer supplies — `windowBackground`,
`widgetBackground`, `defaultText` and `defaultFill` — so a new theme means
changing the scheme and nothing else. The ids below are what the module adds on
top of JUCE's own, registered in `SidebarLookAndFeel::registerColours`.

| id | derived from | used for |
|---|---|---|
| `Sidebar::backgroundColourId` | widget background | the rail and panel surface, one object |
| `Sidebar::iconColourId` | text, dimmed | a rail icon at rest |
| `Sidebar::iconOverColourId` | text | a rail icon under the pointer |
| `Sidebar::iconActiveColourId` | accent | the icon of the open page |
| `Sidebar::activeIndicatorColourId` | accent | the mark showing which page is open |
| `Sidebar::separatorColourId` | text, hairline alpha | where the sidebar meets the host's content |
| `SidebarPanel::backgroundColourId` | widget background | the panel — deliberately the same as the rail |
| `SidebarPanel::titleColourId` | text | the page name at the top |
| `LevelMeter::trackColourId` | window background | the meter's unlit track |
| `LevelMeter::fillColourId` | accent | the lit part |
| `ChoiceStrip::selectedColourId` | accent | the chosen button of a switch |
| `ChoiceStrip::selectedTextColourId` | — | its text, which must read on that fill |
| `ReadOutField::backgroundColourId` | window background | a read-only field, recessed |
| `ReadOutField::textColourId` | text | its value |
| `ReadOutField::outlineColourId` | text, hairline alpha | its border |
| `pageColours::sectionTitleColourId` | text | a section's name |
| `pageColours::sectionOutlineColourId` | text, hairline alpha | the frame drawn around it |
| `pageColours::invalidColourId` | **not derived** — a fixed red, blended a fifth toward the text | a cell holding a controller number the plugin cannot use |

Two notes on why these are derived rather than chosen. `windowBackground` is
*darker* than `widgetBackground` in the dark, midnight and grey schemes and
lighter in the light one, so a read-out reads as a hollow either way without a
second palette. And the accent is `defaultFill`, not `highlightedFill` — in the
dark scheme the latter is the near-black surface drawn behind highlighted text,
which as an icon colour is invisible.

