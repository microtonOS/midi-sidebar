# JUCE Sidebar

Read the [README](README.md) and the high and low priority sections of the [TODO](TODO.md).
The README is written as if the software has already been finished even though that is not true.
After a TODO has been completed remove it from high/low priority and move it to [COMPLETED.md](COMPLETED.md). Do not read COMPLETED.md unless you need the history — the point of the split is that TODO.md is the file that needs acting on.

GUI mockups in [docs](docs/) are HTML tables. The table is a layout device only —
ignore its borders, its widget styling and its `cm` widths, all of which are
artefacts of writing a sketch in HTML. What *is* specification is the structure:
the number of columns and each cell's `colspan` are the invisible grid the
widgets sit on, and they translate directly into `juce::Grid` columns and spans.
A row whose cells are `colspan="3"` twice is halved, exactly, and should come out
halved.

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
Suggest when you think a new skill should be created or when an existing should be reorganized, e.g. changing headings or adding references files.
After edits to skills, check that they have reasonable token counts.
Summarize what changes you would recommend before making any edits.
Exceptions to this rule include:
- Typos.
- Adding sources/references.
- Fixing small mistakes easily verified from references.

The same set of rules as for skills apply to docs.

Temporary files that the user needs to see can be placed in `./tmp`.
Only use this if the user's feedback is necessary for e.g. A/B testing.
If the user doesn't have to see them, remove them once done with them.
The user may also add files Claude needs to see there.
Or any other file that is temporary for one reason or another.
