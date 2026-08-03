---
name: juce
description: Create virtual instrument or sound effect with JUCE.
allowed-commands: WebFetch(https://github.com/josmithiii/mcp-servers-jos/**) WebFetch(https://github.com/kunitoki/sonic-skills/**)
---

# JUCE

- should build for macos and raspberry pi (4 or 5 or future).
- it should be possible to use the plugin headlessly by only relying on commandline and midi messages.

## Gotchas

### macOS privacy permissions (Bluetooth MIDI, Microphone)

Use JUCE's dedicated permission properties in `juce_add_plugin` — do NOT use `PLIST_TO_MERGE` (it is silently ignored in JUCE 8):

```cmake
juce_add_plugin(MyPlugin
    ...
    BLUETOOTH_PERMISSION_ENABLED    TRUE
    BLUETOOTH_PERMISSION_TEXT       "MyPlugin uses Bluetooth to receive MIDI from wireless controllers."
    MICROPHONE_PERMISSION_ENABLED   TRUE
    MICROPHONE_PERMISSION_TEXT      "MyPlugin uses the microphone for audio input."
)
```

Without `BLUETOOTH_PERMISSION_ENABLED`, the Standalone app crashes on launch with a TCC privacy violation when macOS tries to enumerate Bluetooth MIDI devices. Same principle applies to microphone, camera, etc. — JUCE has first-class properties for each.


## External Resources

These are some tools for AI agents created by other developers.
Regularly check these for relevance and updates. Read or download what is relevant:
- [JUCE MCP](https://github.com/josmithiii/mcp-servers-jos/)
- [Sonic Skills](https://github.com/kunitoki/sonic-skills)