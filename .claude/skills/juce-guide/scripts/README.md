# Checking a JUCE module without a plugin

A JUCE module's logic headers can usually be exercised on their own: they are
headers, and a console app can link them. No host, no window, no plugin wrapper.
What stops people doing it is that the CMake takes twenty minutes to write and
the result gets thrown away — so the check does not get written at all.

These two are that twenty minutes, once.

| file | for |
|---|---|
| `add_check_app.cmake` | a check you want to **keep**, as a target in your own repository |
| `check.sh` | a check you want to **run now**, against a module sitting anywhere |

## Keeping one

```cmake
enable_testing()
include(/path/to/add_check_app.cmake)

juce_add_check_app(
    TARGET   MidiParserCheck
    SOURCES  MidiParserCheck.cpp
    MODULES  mycompany::my_module
    LINK     juce::juce_audio_basics)
```

Then `cmake --build build && ctest --test-dir build --output-on-failure`.

The target is registered with CTest automatically (pass `NO_TEST` to skip that).
A check should **return its failure count from `main()`**, which is what CTest
reads as a non-zero exit — no framework needed, and the whole harness is three
functions:

```cpp
int failures = 0;

void check (bool ok, const juce::String& what)
{
    std::cout << (ok ? "  ok    " : "  FAIL  ") << what << std::endl;
    if (! ok) ++failures;
}

int main() { /* … */ return failures; }
```

Print on **pass as well as fail**. A green run that says nothing tells you only
that nothing crashed; a green run that lists what it checked is a description of
the contract, and reading it is how you notice a rule that stopped being tested.

## Running one now

```bash
./check.sh MyCheck.cpp --module ~/code/my_module --link juce::juce_audio_basics
```

Writes a throwaway project to a temp directory, builds, runs, and forwards the
exit code. `--juce` points at a JUCE checkout (default `~/JUCE`), `--std` sets
the language standard, `--keep` leaves the scratch project for inspection.

## The failure this was extracted from

Six suites were written this way into a **session temp directory**. They passed.
They were gone the next day, and the code they covered was left unprotected
without anyone noticing.

So: `check.sh` is for the thirty seconds where you want to know whether a parser
works. Anything you would be sorry to lose belongs in your repository as a real
target — which is what `add_check_app.cmake` is for.

## Notes

- **Keep check descriptions ASCII.** `juce::String (const char*)` asserts that
  the literal is **ASCII**, not UTF-8 — `jassert (CharPointer_ASCII::isValidString
  (...))` in `juce_String.cpp`. An em dash in a test message fires an assertion
  on every run and renders mangled, which is a poor thing to have between you and
  a real failure. Non-ASCII needs `CharPointer_UTF8` or `String::fromUTF8`; in a
  check it is simpler not to.
- **`juce_add_console_app` requires a project VERSION.** Without one it fails
  with an error naming your target rather than the cause.
- **`JUCE_STANDALONE_APPLICATION=1`** is what lets a console app link without the
  plugin client wrappers.
- **JUCE puts the binary under `<target>_artefacts/`**, sometimes with a
  configuration directory inside it. Use `$<TARGET_FILE:target>` in CMake and
  `find` in a shell rather than assuming the path.
- On macOS, builds print `couldn't create cache file '.../xcrun_db-…'` once per
  compiler invocation when the Darwin per-user temp dir is not writable. Harmless;
  filter it with `grep -v "cache file"` rather than widening anything.

## Reusable fixtures live elsewhere

The *harness* is here because it is general JUCE tooling. The **inputs** — byte
sequences, synthetic gestures, tuning files — are facts about a protocol rather
than about JUCE, so they live with the skill that documents that protocol:
`midi-1_0/scripts/midi_vectors.py`, `midi-microtuning/scripts/mts_sysex.py` and
`midi-microtuning/scripts/scales.py`.
