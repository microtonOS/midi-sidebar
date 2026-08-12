---
name: docs-editing
description: Editing manuals or other kinds of documentation (docs). For use in markdown and notebooks.
---

# Docs Editing

AI agents should not write the documentation—they should work like editors, proof-readers, and reviewers.

- Fix typos. No need to ask unless there is some ambiguity.
- Fix incorrect grammar.
- Make typography more consistent.
- Add references when appropriate. Use the Harvard system with markdown footnotes. Name the footnotes with a keyword from the title or name of the author. Add links, when possible to doi or time-stamped references. Double-check and if not possible to access, write so in `<b style="color:red"></b>` in the footnote.
- Fix small errors (single words or short phrases), e.g., an incorrect number, mixing up "left" and "right". Ask the user to confirm.
- For larger errors, Put the erroneous text in `~~` marks and add a comment below in `<b style="color:red"></b>`.
- For general AI comments or suggested edits should always be within `>` blocks. It is up to the user to review and remove the `>`s.
- Comments like what has been tried or implemented should not be included in docs. It is better to have them as finished TODO-s. Exceptions to that rule can be made by using `<!-- -->` comments.
- `<!-- -->` Can in general be used to write things that it can be good for future AI contexts to know, but not useful for humans. Keep these comments short but clear.
- Note that the user may also use `<!-- -->`.
- Note that writing and checking documentation requires focus and time and may at times lag. It may only be fruitful to do a proper writeup once experiments and iterations have stabilized.

A template for the file structure of a docs directory.
```mermaid
---
config:
    themeVariables:
        treeView:
            labelColor: '#888888'
            lineColor: '#888888'
---
treeView-beta
  docs/
    01_introduction.md
    02_...
    appendices/
      time_data.ipynb
    figures/
      featured_image.png
    scripts/
      time_plot.py
      generate_images.py
```
Numbering is optional.
Leading zeros depend on how many files there are
appendices, figures, and scripts are optional directories.
Scripts can e.g. be used to generate images that need to be regenerated severla times or helper scripts for jupyter notebooks containint e.g. plotting functions