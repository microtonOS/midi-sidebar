---
name: juce-macos
description: Build JUCE plugins and standalones for MacOS
allowed-tools: WebFetch(domain:docs.juce.com) WebFetch(domain:forum.juce.com)
---

# JUCE MacOS

It should be possible to build JUCE projects for MacOS.

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
