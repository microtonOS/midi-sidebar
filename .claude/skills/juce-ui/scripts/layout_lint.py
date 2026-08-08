#!/usr/bin/env python3
"""layout_lint.py — check JUCE resized() bodies against the Layout mechanics rules.

Layout is the part of a GUI that cannot be verified by reading the code, so the
few rules that *can* be checked mechanically are worth checking every time. This
script finds every resized() body in the given files and reports:

  rule 1  constants declared inside resized() instead of in the look and feel
          file, which is how the same measurement ends up defined twice and
          drifting apart between pages
  rule 3  a region divided by an item count (getWidth() / n), which truncates
          and abandons the remainder at one edge
  rule 6  unnamed numeric literals greater than 2, each of which is a design
          decision with no name and no single place to change it

Usage:
    layout_lint.py <file-or-directory>...      # defaults to the CWD
    layout_lint.py --max 20 plugin/            # tolerate a legacy baseline
    layout_lint.py --quiet plugin/             # counts only, no per-line detail

Exit status is 1 when the rule-6 count exceeds --max (default 0), so this can
gate a change; the other rules are always reported but never fail the run, since
both have legitimate exceptions.

Suppress a line with a trailing `// layout-lint: allow` comment.
"""

from __future__ import annotations

import argparse
import re
import sys
from pathlib import Path

SUPPRESS = "layout-lint: allow"
SOURCE_SUFFIXES = {".cpp", ".h", ".hpp", ".cc", ".mm"}

# A numeric literal not glued to an identifier or a member access. Captures
# decimals so 2.5f is judged on its value rather than its spelling.
NUMBER = re.compile(r"(?<![\w.])(\d+\.?\d*)[fFuUlL]*(?![\w.])")
HEX = re.compile(r"0[xX][0-9a-fA-F]+")
SUBSCRIPT = re.compile(r"\[\s*\d+\s*\]")
FOR_HEADER = re.compile(r"\bfor\s*\(")
# Rule 1 is about measurements that should live in the look and feel file, so
# only arithmetic types count. A `const auto` holding a Rectangle or an
# iterator is an ordinary local, not a layout constant.
CONST_DECL = re.compile(
    r"\b(?:static\s+)?(?:const|constexpr)\s+"
    r"(?:unsigned\s+|signed\s+|long\s+|short\s+)*"
    r"(?:int|float|double|char|size_t|int8_t|int16_t|int32_t|int64_t"
    r"|uint8_t|uint16_t|uint32_t|uint64_t)\s+\w+\s*[=({]"
)
DIVIDE_BY_COUNT = re.compile(r"\.get(?:Width|Height)\s*\(\s*\)\s*/\s*\d+")
RESIZED_SIG = re.compile(r"(?:(\w+)\s*::\s*)?\bresized\s*\(\s*\)")
ENCLOSING_TYPE = re.compile(r"\b(?:class|struct)\s+(\w+)")


def blank_noise(src: str) -> str:
    """Replace comment and string-literal content with spaces.

    Character positions and newlines are preserved so that offsets computed on
    the result still map onto the original line numbers.
    """
    out = list(src)
    i, n = 0, len(src)
    state = None  # None | "//" | "/*" | '"' | "'"

    while i < n:
        c = src[i]
        nxt = src[i + 1] if i + 1 < n else ""

        if state is None:
            if c == "/" and nxt == "/":
                state = "//"
                out[i] = out[i + 1] = " "
                i += 2
                continue
            if c == "/" and nxt == "*":
                state = "/*"
                out[i] = out[i + 1] = " "
                i += 2
                continue
            if c in ('"', "'"):
                state = c
                out[i] = " "
                i += 1
                continue
        elif state == "//":
            if c == "\n":
                state = None
            else:
                out[i] = " "
        elif state == "/*":
            if c == "*" and nxt == "/":
                out[i] = out[i + 1] = " "
                i += 2
                state = None
                continue
            if c != "\n":
                out[i] = " "
        else:  # inside a string or char literal
            if c == "\\":
                out[i] = " "
                if i + 1 < n and src[i + 1] != "\n":
                    out[i + 1] = " "
                i += 2
                continue
            if c == state:
                state = None
            elif c != "\n":
                out[i] = " "

        i += 1

    return "".join(out)


def blank_for_headers(line: str) -> str:
    """Blank out the parenthesised part of any `for (...)` on this line."""
    out = list(line)

    for m in FOR_HEADER.finditer(line):
        depth, i = 0, m.end() - 1
        while i < len(line):
            if line[i] == "(":
                depth += 1
            elif line[i] == ")":
                depth -= 1
                if depth == 0:
                    break
            out[i] = " "
            i += 1

    return "".join(out)


def find_resized_bodies(clean: str) -> list[tuple[str, int, int]]:
    """Return (owner, start_offset, end_offset) for each resized() definition."""
    bodies = []

    for match in RESIZED_SIG.finditer(clean):
        # An out-of-line definition names its owner; an inline one does not, so
        # fall back to the nearest preceding class or struct.
        owner = match.group(1)
        if owner is None:
            enclosing = ENCLOSING_TYPE.findall(clean, 0, match.start())
            owner = enclosing[-1] if enclosing else None

        j = match.end()

        # Skip qualifiers, then require a body rather than a declaration.
        while j < len(clean) and (clean[j].isspace() or clean[j].isalpha()):
            j += 1
        if j >= len(clean) or clean[j] != "{":
            continue

        depth, k = 0, j
        while k < len(clean):
            if clean[k] == "{":
                depth += 1
            elif clean[k] == "}":
                depth -= 1
                if depth == 0:
                    break
            k += 1

        bodies.append((owner, match.start(), min(k, len(clean) - 1)))

    return bodies


def line_of(offsets: list[int], pos: int) -> int:
    lo, hi = 0, len(offsets) - 1
    while lo < hi:
        mid = (lo + hi + 1) // 2
        if offsets[mid] <= pos:
            lo = mid
        else:
            hi = mid - 1
    return lo + 1


def check_file(path: Path, quiet: bool) -> tuple[int, int, int, list[str]]:
    raw = path.read_text(encoding="utf-8", errors="replace")
    clean = blank_noise(raw)
    raw_lines = raw.split("\n")

    offsets, pos = [0], 0
    for line in raw_lines:
        pos += len(line) + 1
        offsets.append(pos)

    suppressed = {i + 1 for i, l in enumerate(raw_lines) if SUPPRESS in l}
    report: list[str] = []
    totals = [0, 0, 0]  # rule 6, rule 1, rule 3

    for owner, start, end in find_resized_bodies(clean):
        body = clean[start : end + 1]
        first, last = line_of(offsets, start), line_of(offsets, end)

        literals: list[tuple[int, str]] = []
        consts: list[int] = []
        divisions: list[int] = []

        for rel_no, line in enumerate(body.split("\n")):
            lineno = first + rel_no
            if lineno in suppressed:
                continue

            if CONST_DECL.search(line):
                consts.append(lineno)
            if DIVIDE_BY_COUNT.search(line):
                divisions.append(lineno)

            # Loop headers hold item counts, not measurements; array subscripts
            # and hex constants are not layout numbers either. Only the loop
            # header itself is skipped, so a literal in a single-line loop body
            # is still reported.
            scan = SUBSCRIPT.sub(lambda m: " " * len(m.group()), line)
            scan = HEX.sub(lambda m: " " * len(m.group()), scan)
            scan = blank_for_headers(scan)

            for m in NUMBER.finditer(scan):
                try:
                    if float(m.group(1)) > 2:
                        literals.append((lineno, m.group(1)))
                except ValueError:
                    pass

        if not (literals or consts or divisions):
            continue

        totals[0] += len(literals)
        totals[1] += len(consts)
        totals[2] += len(divisions)

        label = f"{owner}::resized()" if owner else "resized()"
        report.append(f"  {label:<44} lines {first}-{last}")

        if divisions:
            report.append(f"    rule 3  {len(divisions)} region(s) divided by an item count")
            if not quiet:
                for n in divisions:
                    report.append(f"      {n}: {raw_lines[n - 1].strip()}")
        if consts:
            report.append(f"    rule 1  {len(consts)} constant(s) declared inside resized()")
            if not quiet:
                for n in consts:
                    report.append(f"      {n}: {raw_lines[n - 1].strip()}")
        if literals:
            report.append(f"    rule 6  {len(literals)} unnamed literal(s) > 2")
            if not quiet:
                for n, val in literals:
                    report.append(f"      {n}: [{val}] {raw_lines[n - 1].strip()}")

    return totals[0], totals[1], totals[2], report


def collect(targets: list[str]) -> list[Path]:
    files: list[Path] = []
    for t in targets:
        p = Path(t)
        if p.is_dir():
            files += [f for f in sorted(p.rglob("*")) if f.suffix in SOURCE_SUFFIXES]
        elif p.is_file():
            files.append(p)
        else:
            print(f"layout_lint: no such file or directory: {t}", file=sys.stderr)
    return files


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    ap.add_argument("targets", nargs="*", default=["."], help="files or directories")
    ap.add_argument("--max", type=int, default=0,
                    help="tolerated rule-6 literals before failing (default 0)")
    ap.add_argument("--quiet", action="store_true", help="counts only, no per-line detail")
    args = ap.parse_args()

    files = collect(args.targets or ["."])
    if not files:
        print("layout_lint: nothing to check")
        return 0

    grand = [0, 0, 0]
    any_output = False

    for f in files:
        r6, r1, r3, report = check_file(f, args.quiet)
        if not report:
            continue
        any_output = True
        print(f"\n{f}")
        print("\n".join(report))
        grand[0] += r6
        grand[1] += r1
        grand[2] += r3

    if not any_output:
        print(f"layout_lint: {len(files)} file(s) checked, nothing to report")
        return 0

    print(f"\n{'-' * 60}")
    print(f"rule 6  unnamed literals > 2 in resized():   {grand[0]}")
    print(f"rule 1  constants declared inside resized(): {grand[1]}")
    print(f"rule 3  regions divided by an item count:    {grand[2]}")

    if grand[0] > args.max:
        print(f"\nFAIL: {grand[0]} unnamed literals exceeds --max {args.max}.")
        print("Name them in the look and feel file, or suppress a deliberate")
        print(f"case with a trailing `// {SUPPRESS}` comment.")
        return 1

    return 0


if __name__ == "__main__":
    sys.exit(main())
