# JUCE Sidebar

Read the [README](README.md) and the high and low priority sections of the [TODO](TODO.md).
The README is written as if the software has already been finished even though that is not true.
After a TODO has been completed remove it from high/low priority and place it in completed.

For markdown files I urge you to read the comments in `<!-- -->`. Sometimes they are meant for you to read but are commented out because it would be too much infomration for humans. At other times they comment out something but then that still makes useful context on difficult design choices where I may not be completely settled on a design choice and would accept suggestions for something better.

Note that you are in a [sandbox](.claude/settings.json).
If there is something you need from outside, ask for it.
If there is something you need to do repeatedly, ask for it to be exempted.

Builds print `error: couldn't create cache file '/var/folders/.../xcrun_db-…'` once per
compiler invocation. The sandbox blocks `xcrun`'s cache, which lives in the Darwin
per-user temp dir and ignores $TMPDIR (so XCRUN_DB_PATH does not help). Harmless — the
build succeeds. Filter with `grep -v "cache file"` rather than widening the sandbox.

The agent skills are under construction.
Give feedback on how they can be improved.
Summarize what changes you would recommend before making any edits.
Exceptions to this rule include:
- Typos.
- Adding sources/references.
- Fixing small mistakes easily verified from references.