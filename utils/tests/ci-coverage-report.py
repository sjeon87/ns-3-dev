#!/usr/bin/env python3
# Copyright (c) 2026 Tom Henderson
#
# SPDX-License-Identifier: GPL-2.0-only
#
# Authored by Claude Code (opus-4.7); reviewed by Tom Henderson
#
"""Render CI coverage report from build/ci-env-*.json files.

Each CI job writes one such JSON file via ci-capture-env.sh. Emits two outputs:
  - ci-coverage.md  : markdown table + summary, for wiki/MR/docs embedding.
  - ci-coverage.txt : fixed-width ASCII table + summary, for the job console.

Both place the summary AFTER the table so a streaming console log lands on
the summary at the bottom -- no scroll-back to see the bird's-eye view.
"""

import argparse
import glob
import json
import sys
from collections import Counter
from pathlib import Path

FIELDS = ("job", "distro", "compiler_var", "compiler_version", "mode", "kernel")
HEADERS = ("Job", "Distro", "Compiler", "Version", "Mode", "Kernel")
WIDTHS = (38, 28, 8, 38, 9, 30)


def load(patterns):
    rows = []
    for pat in patterns:
        for p in sorted(glob.glob(pat)):
            try:
                rows.append(json.loads(Path(p).read_text()))
            except Exception as e:
                print(f"warning: could not read {p}: {e}", file=sys.stderr)
    rows.sort(
        key=lambda r: (
            r.get("distro", ""),
            r.get("compiler_version", ""),
            r.get("mode", ""),
            r.get("job", ""),
        )
    )
    return rows


def trunc(s, n):
    s = s or ""
    return s if len(s) <= n else s[: n - 1] + "~"


def summary(rows):
    if not rows:
        return "(no jobs reported)\n"
    distros = sorted({r.get("distro", "") for r in rows})
    versions = sorted({r.get("compiler_version", "") for r in rows})
    modes = Counter(r.get("mode", "") for r in rows)
    n = len(rows)
    lines = [
        f"=== CI coverage: {n} {'job' if n == 1 else 'jobs'} ===",
        f"distros ({len(distros)}):   " + ", ".join(distros),
        f"compilers ({len(versions)}): " + ", ".join(versions),
        "modes:        " + ", ".join(f"{m} ({n})" for m, n in sorted(modes.items())),
    ]
    return "\n".join(lines) + "\n"


def ascii_table(rows):
    if not rows:
        return "(no jobs reported)\n"
    sep = "+" + "+".join("-" * (w + 2) for w in WIDTHS) + "+"
    head = "| " + " | ".join(h.ljust(w) for h, w in zip(HEADERS, WIDTHS)) + " |"
    out = [sep, head, sep]
    for r in rows:
        cells = [trunc(r.get(f, ""), w).ljust(w) for f, w in zip(FIELDS, WIDTHS)]
        out.append("| " + " | ".join(cells) + " |")
    out.append(sep)
    return "\n".join(out) + "\n"


def md_table(rows):
    if not rows:
        return "_No jobs reported._\n"
    lines = ["| " + " | ".join(HEADERS) + " |", "|" + "|".join("---" for _ in HEADERS) + "|"]
    for r in rows:
        lines.append("| " + " | ".join(r.get(f, "") for f in FIELDS) + " |")
    return "\n".join(lines) + "\n"


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("paths", nargs="+", help="glob patterns for ci-env-*.json")
    ap.add_argument("--md", default="ci-coverage.md")
    ap.add_argument("--txt", default="ci-coverage.txt")
    args = ap.parse_args()

    rows = load(args.paths)
    s = summary(rows)

    Path(args.md).write_text(
        "# CI coverage report\n\n" f"{md_table(rows)}\n" "## Summary\n\n" f"```\n{s}```\n"
    )
    Path(args.txt).write_text(f"{ascii_table(rows)}\n{s}")


if __name__ == "__main__":
    main()
