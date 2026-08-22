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

### macOS ships bash 3.2, where an empty array under `set -u` is fatal

Build and tooling scripts hit this constantly, because `set -euo pipefail` is the
right default and an optional list of arguments is the obvious way to write a
flag:

```bash
MODULES=()                      # nothing was passed
for m in "${MODULES[@]}"; do    # bash 3.2: "MODULES[@]: unbound variable"
```

Bash fixed it in 4.4. Apple will not ship 4.4 — 3.2 is the last GPLv2 release —
so a script that must run on a stock Mac needs the `+` form, which expands to
nothing when the array is unset and to its elements otherwise:

```bash
for m in ${MODULES[@]+"${MODULES[@]}"}; do
```

The failure is easy to misread: it names the array rather than the emptiness, and
it only fires on the path where the flag was *not* given — so it survives every
test that passes one.

### Builds print an `xcrun` cache-file error under a sandbox

```
error: couldn't create cache file '/var/folders/.../xcrun_db-…' (errno=Operation not permitted)
```

Once per compiler invocation, and harmless — the build succeeds. `xcrun`'s cache
lives in the Darwin per-user temp directory and ignores `$TMPDIR`, so
`XCRUN_DB_PATH` does not move it. Filter it with `grep -v "cache file"` rather
than widening the sandbox to make it go away.
