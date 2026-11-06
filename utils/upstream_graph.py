#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-only
"""Render an upstream audit report as a self-contained HTML page."""

import argparse
import collections
import html
import json
import re
from pathlib import Path

WIDTH = 760
HEIGHT = 200
LANE_LEFT = 240
LANE_RIGHT = 680
DOT_LEFT = 360
DOT_RIGHT = 680
LOCAL_Y = 72
BASE_Y = 100
UPSTREAM_Y = 128

STATUS_CLASS = {
    "up-to-date": "good",
    "reviewed": "good",
    "content-identical": "good",
    "fast-forward": "info",
    "merge-clean": "info",
    "merge-review": "warn",
    "manual-port": "warn",
    "port-update": "warn",
    "merge-conflicts": "bad",
    "error": "bad",
    "unknown-history": "bad",
    "covered-by-declared-or-local": "muted",
}

SEVERITY = {
    "error": 5,
    "merge-conflicts": 5,
    "unknown-history": 4,
    "manual-port": 3,
    "port-update": 3,
    "merge-review": 3,
    "fast-forward": 2,
    "merge-clean": 2,
    "up-to-date": 1,
    "reviewed": 1,
    "content-identical": 1,
    "covered-by-declared-or-local": 0,
}

STYLE = """
:root {
  color-scheme: light dark;
  --bg: #ffffff;
  --fg: #1f2328;
  --muted: #59636e;
  --line: #d1d9e0;
  --card: #f6f8fa;
  --accent: #0969da;
  --good: #1a7f37;
  --info: #0969da;
  --warn: #9a6700;
  --bad: #cf222e;
  --gray: #6e7781;
  --local: #0969da;
  --upstream: #bc4c00;
  --base: #6e7781;
}
@media (prefers-color-scheme: dark) {
  :root {
    --bg: #0d1117;
    --fg: #e6edf3;
    --muted: #9198a1;
    --line: #3d444d;
    --card: #151b23;
    --accent: #4493f8;
    --good: #3fb950;
    --info: #4493f8;
    --warn: #d29922;
    --bad: #f85149;
    --gray: #9198a1;
    --local: #4493f8;
    --upstream: #f0883e;
    --base: #9198a1;
  }
}
* { box-sizing: border-box; }
html { scroll-behavior: smooth; }
body {
  margin: 0;
  background: var(--bg);
  color: var(--fg);
  font: 14px/1.55 -apple-system, BlinkMacSystemFont, "Segoe UI", Helvetica, Arial, sans-serif;
}
a { color: var(--accent); text-decoration: none; }
a:hover { text-decoration: underline; }
code, .mono {
  font-family: ui-monospace, SFMono-Regular, "SF Mono", Menlo, Consolas, monospace;
  font-size: 12px;
}
.page { max-width: 1400px; margin: 0 auto; padding: 28px 24px 8px; }
.page h1 { margin: 0 0 4px; font-size: 26px; }
.meta { margin: 0; color: var(--muted); }
.summary { display: flex; flex-wrap: wrap; gap: 8px; align-items: center; margin: 14px 0 0; }
.summary .stat { display: inline-flex; align-items: center; gap: 6px; }
.summary .count { margin-left: 0; }
.layout {
  display: grid;
  grid-template-columns: 250px minmax(0, 1fr);
  gap: 28px;
  max-width: 1400px;
  margin: 0 auto;
  padding: 16px 24px 48px;
}
.sidebar {
  position: sticky;
  top: 0;
  align-self: start;
  max-height: 100vh;
  overflow: auto;
  padding: 12px 16px 12px 0;
  border-right: 1px solid var(--line);
}
.sidebar h2 {
  margin: 0 0 8px;
  font-size: 12px;
  text-transform: uppercase;
  letter-spacing: .06em;
  color: var(--muted);
}
ul.tree { list-style: none; margin: 0; padding-left: 0; }
ul.tree ul.tree { padding-left: 14px; }
ul.tree li { margin: 2px 0; }
ul.tree a {
  display: flex;
  align-items: center;
  gap: 6px;
  padding: 2px 4px;
  border-radius: 4px;
  color: var(--fg);
}
ul.tree a:hover { background: var(--card); text-decoration: none; }
.mark { width: 8px; height: 8px; border-radius: 50%; flex: none; background: var(--gray); }
.mark.good { background: var(--good); }
.mark.info { background: var(--info); }
.mark.warn { background: var(--warn); }
.mark.bad { background: var(--bad); }
.count { margin-left: auto; font-size: 11px; color: var(--muted); }
main { min-width: 0; }
section { margin: 0 0 28px; }
h2 { font-size: 20px; margin: 0 0 6px; }
h3 { font-size: 15px; margin: 18px 0 8px; }
.overview table { width: 100%; border-collapse: collapse; font-size: 13px; }
.overview th, .overview td {
  text-align: left;
  padding: 6px 8px;
  border-bottom: 1px solid var(--line);
  vertical-align: top;
}
.overview th { color: var(--muted); font-weight: 600; }
.overview td:last-child, .overview th:last-child { text-align: right; }
.chip {
  display: inline-block;
  padding: 0 7px;
  border: 1px solid currentColor;
  border-radius: 999px;
  font-size: 11px;
  font-weight: 600;
  line-height: 18px;
  white-space: nowrap;
}
.chip.good { color: var(--good); }
.chip.info { color: var(--info); }
.chip.warn { color: var(--warn); }
.chip.bad { color: var(--bad); }
.chip.muted { color: var(--gray); }
.badge {
  display: inline-block;
  padding: 0 7px;
  border-radius: 4px;
  font-size: 11px;
  font-weight: 600;
  border: 1px solid currentColor;
}
.badge.warn { color: var(--warn); }
.badge.bad { color: var(--bad); }
.repo { border-top: 1px solid var(--line); padding-top: 18px; }
.repo-meta { margin: 0 0 10px; color: var(--muted); font-size: 13px; }
.connection {
  border: 1px solid var(--line);
  border-radius: 8px;
  padding: 12px 14px;
  margin: 12px 0;
  background: var(--card);
}
.connection-head { display: flex; flex-wrap: wrap; gap: 8px; align-items: center; }
.connection-head .source { font-weight: 600; }
.tag {
  font-size: 11px;
  color: var(--muted);
  border: 1px solid var(--line);
  border-radius: 4px;
  padding: 0 5px;
}
.tag.muted { color: var(--gray); }
.arrow { color: var(--muted); }
.facts { margin: 6px 0 0; color: var(--muted); font-size: 12px; }
svg { display: block; width: 100%; max-width: 760px; height: auto; margin: 8px 0 4px; overflow: visible; }
.lane { stroke: var(--line); stroke-width: 2; }
.lane.dashed { stroke-dasharray: 3 5; }
.edge { fill: none; stroke-width: 2; }
.edge.local { stroke: var(--local); }
.edge.upstream { stroke: var(--upstream); }
.base { fill: var(--base); }
.dot { stroke: var(--bg); stroke-width: 1.5; }
.dot.local { fill: var(--local); }
.dot.upstream { fill: var(--upstream); }
.dot.tip { stroke-width: 3; }
.dot.empty { fill: none; stroke: var(--gray); stroke-width: 2; }
.divider { stroke: var(--gray); stroke-width: 1.5; stroke-dasharray: 4 4; }
text.label { font-size: 12px; font-weight: 600; fill: var(--fg); }
text.mono { font-size: 11px; fill: var(--muted); }
text.elide {
  font-size: 11px;
  fill: var(--muted);
  paint-order: stroke;
  stroke: var(--card);
  stroke-width: 3px;
  stroke-linejoin: round;
}
details { margin: 8px 0; }
summary { cursor: pointer; }
details.commits summary, details.files summary { color: var(--muted); font-size: 12px; }
details.commits ul, details.files ul { margin: 6px 0 0; padding-left: 18px; font-size: 12px; }
details.commits li, details.files li { margin: 2px 0; }
.status { display: inline-block; width: 16px; font-weight: 700; }
.finding {
  margin: 6px 0 0;
  padding: 6px 8px;
  border-left: 3px solid var(--warn);
  background: var(--bg);
  font-size: 12px;
}
.receipt { margin: 6px 0 0; font-size: 12px; color: var(--muted); }
.kind { font-weight: 700; }
.muted { color: var(--muted); }
details.branch { border-top: 1px solid var(--line); padding: 6px 0; }
details.branch > summary { padding: 4px 0; }
details.branch > summary > * { margin-right: 8px; }
details.covered ul, details.findings ul { margin: 6px 0 0; padding-left: 18px; font-size: 12px; }
details.covered li, details.findings li { margin: 3px 0; }
.legend ul { margin: 6px 0 0; padding-left: 18px; }
.legend li { margin: 5px 0; }
.swatch {
  display: inline-block;
  width: 18px;
  height: 4px;
  border-radius: 2px;
  vertical-align: middle;
  margin-right: 6px;
}
.swatch.local { background: var(--local); }
.swatch.upstream { background: var(--upstream); }
.swatch.base { width: 9px; height: 9px; background: var(--base); transform: rotate(45deg); }
footer {
  max-width: 1400px;
  margin: 0 auto;
  padding: 0 24px 40px;
  color: var(--muted);
  font-size: 12px;
}
@media (max-width: 860px) {
  .layout { grid-template-columns: 1fr; }
  .sidebar {
    position: static;
    max-height: none;
    border-right: 0;
    border-bottom: 1px solid var(--line);
    padding-bottom: 12px;
  }
}
@media print {
  .sidebar { display: none; }
  .layout { display: block; }
  .connection { break-inside: avoid; }
}
"""


def esc(value):
    """Escape a value for HTML text and attribute contexts."""
    return html.escape("" if value is None else str(value), quote=True)


def short(value):
    """Return the abbreviated object name shown in labels."""
    return str(value)[:8] if value else ""


def stamp(value):
    """Format an ISO 8601 timestamp for display without its seconds."""
    text = "" if value is None else str(value)
    if "T" in text:
        date, _, rest = text.partition("T")
        return f"{date} {rest[:5]}"
    return text


def anchor(path):
    """Return a stable HTML identifier for a repository path."""
    if path == ".":
        return "repo-root"
    return "repo-" + re.sub(r"[^a-z0-9]+", "-", path.lower()).strip("-")


def chip(status):
    """Render a status chip with its severity color."""
    return f'<span class="chip {STATUS_CLASS.get(status, "bad")}">{esc(status)}</span>'


def severity_class(value):
    """Map a numeric severity to a tree marker class."""
    if value >= 4:
        return "bad"
    if value >= 3:
        return "warn"
    if value >= 2:
        return "info"
    return "good"


def commit_count(count, adjective=""):
    """Render a commit count with a singular or plural noun."""
    noun = "commit" if count == 1 else "commits"
    prefix = f"{adjective} " if adjective else ""
    return f"{count} {prefix}{noun}"


def worst(sources):
    """Return the highest severity among a repository's sources."""
    return max((SEVERITY.get(source.get("status"), 0) for source in sources), default=0)


def dot(x, y, css, commit, radius=4.5):
    """Render one commit dot with a native tooltip."""
    title = "{} {} {}".format(
        str(commit.get("commit", ""))[:12],
        stamp(commit.get("date")),
        commit.get("subject", ""),
    )
    return (
        f'<circle cx="{x:.1f}" cy="{y}" r="{radius}" class="dot {css}">'
        f"<title>{esc(title)}</title></circle>"
    )


def lane(y, samples, count, css):
    """Render one horizontal lane of sampled commits, newest at the right."""
    shown = len(samples)
    remaining = (count or 0) - shown
    elided = remaining > 0
    start = DOT_LEFT if elided else LANE_LEFT + 40
    parts = []
    if elided:
        parts.append(
            f'<line x1="{LANE_LEFT}" y1="{y}" x2="{start - 30}" y2="{y}" class="lane dashed"/>'
        )
        parts.append(
            f'<line x1="{start - 30}" y1="{y}" x2="{LANE_RIGHT}" y2="{y}" class="lane"/>'
        )
    else:
        parts.append(f'<line x1="{LANE_LEFT}" y1="{y}" x2="{LANE_RIGHT}" y2="{y}" class="lane"/>')
    if not shown:
        parts.append(f'<circle cx="{DOT_RIGHT}" cy="{y}" r="5.5" class="dot empty"/>')
        if count:
            parts.append(
                f'<text x="{start - 26}" y="{y + 4}" class="elide" text-anchor="end">'
                f"+{count} more</text>"
            )
        return parts
    step = (DOT_RIGHT - start) / (shown - 1) if shown > 1 else 0
    for index, commit in enumerate(samples):
        x = DOT_RIGHT - index * step
        radius = 6 if index == 0 else 4.5
        parts.append(dot(x, y, css + (" tip" if index == 0 else ""), commit, radius))
    if elided:
        parts.append(
            f'<text x="{start - 26}" y="{y + 4}" class="elide" text-anchor="end">'
            f"+{remaining} more</text>"
        )
    return parts


def labels(title, oid, date, note, y):
    """Render the three-line label at the right edge of a lane."""
    return [
        f'<text x="{WIDTH - 16}" y="{y}" class="label" text-anchor="end">{esc(title)}</text>',
        f'<text x="{WIDTH - 16}" y="{y + 16}" class="mono" text-anchor="end">'
        f"{esc(short(oid))} &middot; {esc(stamp(date))}</text>",
        f'<text x="{WIDTH - 16}" y="{y + 32}" class="elide" text-anchor="end">{esc(note)}</text>',
    ]


def connection_svg(project, source):
    """Render the branch connection diagram for one source mapping."""
    relation = source.get("relation")
    tip = source.get("tip")
    merge_base = source.get("merge_base")
    head = project.get("head", "")
    head_date = project.get("head_date", "")
    local = source.get("local_commits") or []
    upstream = source.get("commits") or []
    local_only = source.get("local_only")
    local_total = source.get("local_total")
    local_count = local_only if local_only is not None else local_total
    pending = source.get("pending_commits")
    scoped = bool(source.get("path") or source.get("destination"))

    label = f"Commit connection between local HEAD and {source.get('source', 'source')}"
    parts = [f'<svg viewBox="0 0 {WIDTH} {HEIGHT}" role="img" aria-label="{esc(label)}">']

    if source.get("status") in ("error", "unknown-history") or not tip:
        parts.append(
            f'<text x="{WIDTH // 2}" y="{HEIGHT // 2}" class="label" text-anchor="middle">'
            f"No commit graph: {esc(source.get('status', 'unavailable'))}</text>"
        )
        parts.append("</svg>")
        return "".join(parts)

    if relation not in ("diverged", "ahead", "behind", "equal", "unrelated"):
        parts.append(
            f'<text x="{WIDTH // 2}" y="{HEIGHT // 2}" class="label" text-anchor="middle">'
            "No commit graph: relation unknown</text>"
        )
        parts.append("</svg>")
        return "".join(parts)

    pending_note = f"{pending} pending" if pending is not None else "pending unknown"
    if source.get("pending_commits_proven") is False:
        pending_note += " (unverified)"
    if scoped:
        pending_note += " (path-scoped)"
    local_note = (
        f"{local_count} local-only"
        if local_only is not None
        else commit_count(local_count, "local")
        if local_count is not None
        else "local history"
    )

    if relation == "equal":
        parts.append(f'<circle cx="200" cy="{BASE_Y}" r="9" class="dot tip"/>')
        parts.append(
            f'<text x="228" y="{BASE_Y - 4}" class="label">'
            "local HEAD and tip are the same commit</text>"
        )
        parts.append(
            f'<text x="228" y="{BASE_Y + 14}" class="mono">'
            f"{esc(short(head))} &middot; {esc(stamp(head_date))}</text>"
        )
        parts.append("</svg>")
        return "".join(parts)

    if relation == "unrelated":
        parts.append('<line x1="216" y1="48" x2="216" y2="152" class="divider"/>')
        parts.append('<text x="196" y="96" class="label" text-anchor="end">no common</text>')
        parts.append('<text x="196" y="112" class="label" text-anchor="end">ancestor</text>')
        parts.extend(lane(LOCAL_Y, local, local_count, "local"))
        parts.extend(lane(UPSTREAM_Y, upstream, pending, "upstream"))
        parts.extend(labels("local HEAD", head, head_date, local_note, 24))
        parts.extend(labels("tip", tip, source.get("tip_date"), pending_note, 146))
        parts.append("</svg>")
        return "".join(parts)

    parts.append(
        f'<rect x="192" y="92" width="16" height="16" '
        f'transform="rotate(45 {200} {BASE_Y})" class="base"/>'
    )
    parts.append('<text x="184" y="96" class="label" text-anchor="end">merge base</text>')
    parts.append(
        f'<text x="184" y="112" class="mono" text-anchor="end">{esc(short(merge_base))}</text>'
    )

    if relation in ("diverged", "ahead"):
        parts.append(
            f'<path d="M 200 {BASE_Y} C 216 {BASE_Y} 216 {LOCAL_Y} {LANE_LEFT} {LOCAL_Y} '
            f'H {LANE_RIGHT}" class="edge local"/>'
        )
        parts.extend(lane(LOCAL_Y, local, local_count, "local"))
        parts.extend(labels("local HEAD", head, head_date, local_note, 24))
    else:
        parts.append('<text x="184" y="128" class="elide" text-anchor="end">= local HEAD</text>')

    if relation in ("diverged", "behind"):
        parts.append(
            f'<path d="M 200 {BASE_Y} C 216 {BASE_Y} 216 {UPSTREAM_Y} {LANE_LEFT} {UPSTREAM_Y} '
            f'H {LANE_RIGHT}" class="edge upstream"/>'
        )
        parts.extend(lane(UPSTREAM_Y, upstream, pending, "upstream"))
        parts.extend(labels("tip", tip, source.get("tip_date"), pending_note, 146))
    else:
        parts.append('<text x="184" y="128" class="elide" text-anchor="end">= tip</text>')

    parts.append("</svg>")
    return "".join(parts)


def commit_list(title, samples, count, opened=False):
    """Render a collapsible list of commit samples."""
    samples = samples or []
    if not samples and not count:
        return ""
    summary = title
    if count is not None:
        summary += f" ({len(samples)} of {count} shown)"
    elif samples:
        summary += f" ({len(samples)} shown)"
    body = "".join(
        f'<li><code>{esc(short(commit.get("commit")))}</code> '
        f'<span class="mono">{esc(stamp(commit.get("date")))}</span> '
        f'{esc(commit.get("subject", ""))}</li>'
        for commit in samples
    )
    if not body:
        body = '<li class="muted">No commit samples.</li>'
    return (
        f'<details class="commits"{" open" if opened else ""}>'
        f"<summary>{esc(summary)}</summary><ul>{body}</ul></details>"
    )


def file_list(source):
    """Render changed paths and predicted conflicts."""
    files = source.get("files") or []
    conflicts = (source.get("merge") or {}).get("conflicts") or []
    if not files and not conflicts:
        return ""
    parts = ["<details class=\"files\"><summary>Changed paths"]
    if files:
        parts.append(f" ({len(files)})")
    parts.append("</summary>")
    if files:
        items = "".join(
            f'<li><span class="status">{esc(item.get("status"))}</span> '
            f'<code>{esc(item.get("path"))}</code></li>'
            for item in files[:20]
        )
        parts.append(f"<ul>{items}</ul>")
        if len(files) > 20:
            parts.append(
                f'<p class="muted">+{len(files) - 20} more paths in the JSON report.</p>'
            )
    if conflicts:
        parts.append(f'<p class="label">Predicted conflicts ({len(conflicts)})</p><ul>')
        parts.append("".join(f"<li><code>{esc(path)}</code></li>" for path in conflicts[:20]))
        parts.append("</ul>")
    parts.append("</details>")
    return "".join(parts)


def connection_card(project, source, expanded=True):
    """Render one source mapping as a header, a graph, and its evidence."""
    status = source.get("status", "error")
    pending = source.get("pending_commits")
    files = source.get("files") or []
    conflicts = (source.get("merge") or {}).get("conflicts") or []

    facts = []
    if pending is not None:
        text = f"pending {pending}"
        if source.get("pending_commits_proven") is False:
            text += " (unverified)"
        facts.append(text)
    local_count = source.get("local_only")
    if local_count is None:
        local_count = source.get("local_total")
    if source.get("local_only") is not None:
        facts.append(f"local-only {source['local_only']}")
    elif local_count is not None:
        facts.append(commit_count(local_count, "local"))
    if source.get("upstream_only") is not None:
        facts.append(f"upstream-only {source['upstream_only']}")
    if files:
        facts.append(f"{len(files)} changed files")
    if conflicts:
        facts.append(f"{len(conflicts)} conflicts")
    if source.get("movement"):
        facts.append(f"branch {source['movement']}")

    parts = ['<article class="connection">', '<header class="connection-head">']
    parts.append(chip(status))
    parts.append(f'<code class="source">{esc(source.get("source", ""))}</code>')
    parts.append('<span class="tag">declared</span>' if source.get("declared") else '<span class="tag muted">other branch</span>')
    if source.get("destination"):
        parts.append(f'<span class="arrow">-&gt;</span><code>{esc(source["destination"])}</code>')
    parts.append("</header>")
    if facts:
        parts.append('<p class="facts">' + " &middot; ".join(esc(fact) for fact in facts) + "</p>")
    parts.append(connection_svg(project, source))
    parts.append(commit_list("Pending upstream commits", source.get("commits"), pending, expanded))
    parts.append(
        commit_list(
            "Local commits" if source.get("local_only") is None else "Local-only commits",
            source.get("local_commits"),
            local_count,
        )
    )
    parts.append(file_list(source))
    for detail in source.get("residues") or []:
        parts.append(f'<p class="finding">{esc(detail)}</p>')
    review = source.get("review")
    if review:
        parts.append(
            f'<p class="receipt">Reviewed {esc(stamp(review.get("reviewed_at")))}: '
            f'{esc(review.get("decision"))} &mdash; {esc(review.get("reason"))}</p>'
        )
    parts.append("</article>")
    return "".join(parts)


def repo_section(project, residues):
    """Render one repository with its declared and other branches."""
    path = project.get("project", ".")
    sources = project.get("sources") or []
    declared = [source for source in sources if source.get("declared")]
    others = [source for source in sources if not source.get("declared")]
    attention = [source for source in others if source.get("attention")]
    covered = [source for source in others if not source.get("attention")]
    pinned = project.get("pinned")
    head = project.get("head")

    parts = [f'<section class="repo" id="{anchor(path)}">']
    badges = []
    if project.get("dirty"):
        badges.append('<span class="badge warn">dirty worktree</span>')
    if project.get("shallow"):
        badges.append('<span class="badge bad">shallow history</span>')
    if pinned and pinned != head:
        badges.append('<span class="badge bad">gitlink drift</span>')
    parts.append(f'<h2>{esc(path)}</h2>')
    if badges:
        parts.append('<p class="facts">' + " ".join(badges) + "</p>")

    meta = [
        f'HEAD <code>{esc(short(head))}</code>',
        esc(stamp(project.get("head_date"))),
        f"{len(declared)} declared",
        f"{len(others)} other branches",
    ]
    if pinned:
        meta.append(
            f'pinned <code>{esc(short(pinned))}</code> '
            + ("(matches)" if pinned == head else "(drift)")
        )
    parts.append('<p class="repo-meta">' + " &middot; ".join(meta) + "</p>")

    for source in declared:
        parts.append(connection_card(project, source, expanded=True))

    if others:
        parts.append('<details class="others" open>')
        parts.append(
            f"<summary>Other branches ({len(others)}; {len(attention)} need review)</summary>"
        )
        for source in attention:
            parts.append('<details class="branch">')
            parts.append("<summary>")
            parts.append(chip(source.get("status", "error")))
            parts.append(f'<code>{esc(source.get("branch", ""))}</code>')
            exclusive = source.get("exclusive_commits")
            if exclusive is not None:
                parts.append(f'<span class="muted">{exclusive} exclusive commits</span>')
            parts.append("</summary>")
            parts.append(connection_card(project, source, expanded=False))
            parts.append("</details>")
        if covered:
            parts.append('<details class="covered"><summary>')
            parts.append(f"{len(covered)} branches covered by local or declared history")
            parts.append("</summary><ul>")
            for source in covered:
                parts.append(
                    f'<li><code>{esc(source.get("branch", ""))}</code> '
                    f'<span class="muted">{esc(short(source.get("tip")))}</span></li>'
                )
            parts.append("</ul></details>")
        parts.append("</details>")

    if residues:
        parts.append('<details class="findings"><summary>')
        parts.append(f"Findings ({len(residues)})")
        parts.append("</summary><ul>")
        for item in residues:
            origin = (
                f' <span class="muted">({esc(item["source"])})</span>'
                if item.get("source")
                else ""
            )
            parts.append(
                f'<li><span class="kind">{esc(item.get("kind"))}</span>{origin} '
                f'{esc(item.get("detail"))}</li>'
            )
        parts.append("</ul></details>")

    parts.append("</section>")
    return "".join(parts)


def overview(report):
    """Render the repository summary table."""
    parts = ['<section class="overview" id="overview">', "<h2>Overview</h2>"]
    parts.append(
        "<table><thead><tr><th>Repository</th><th>HEAD</th><th>Declared</th>"
        "<th>Attention</th><th>Statuses</th><th>Pending</th></tr></thead><tbody>"
    )
    for project in report.get("projects") or []:
        sources = project.get("sources") or []
        declared = [source for source in sources if source.get("declared")]
        attention = [source for source in sources if source.get("attention")]
        statuses = collections.Counter(source.get("status") for source in declared)
        pending = sum(source.get("pending_commits") or 0 for source in declared)
        status_text = " ".join(
            f'{chip(status)}<span class="count">{count}</span>'
            for status, count in sorted(statuses.items())
        )
        parts.append(
            f'<tr><td><a href="#{anchor(project.get("project", "."))}">'
            f'{esc(project.get("project", "."))}</a></td>'
            f'<td><code>{esc(short(project.get("head")))}</code></td>'
            f"<td>{len(declared)}</td><td>{len(attention)}</td>"
            f"<td>{status_text}</td><td>{pending}</td></tr>"
        )
    parts.append("</tbody></table></section>")
    return "".join(parts)


def legend():
    """Render the reading guide."""
    return """
<section class="legend" id="legend">
<h2>How to read the graphs</h2>
<ul>
<li><span class="swatch local"></span><b>Blue lane</b>: commits reachable from local HEAD
but not from the merge base. The report samples them; the count is complete.
An unrelated history has no merge base, so both lanes are separate and the blue
lane counts all local commits.</li>
<li><span class="swatch upstream"></span><b>Orange lane</b>: upstream commits pending
against local HEAD, a declared branch, or a review baseline. The count is path-scoped
when the mapping selects a source path.</li>
<li><span class="swatch base"></span><b>Diamond</b>: merge base of local HEAD and the
source tip. A single lane means the other tip is that merge base.</li>
<li><b>Hollow dot</b>: the tip has no sampled commits, usually because no commit is
pending. <b>+N more</b>: commits not drawn; the label keeps the full count.</li>
<li>Hover a dot to see the commit identifier, date, and subject. The lists below each
graph carry the same samples as text.</li>
<li>Status colors: green needs no action, blue is a clean advance or merge, amber needs
review, red is a conflict or an incomplete audit.</li>
</ul>
</section>
"""


def project_tree(projects):
    """Build a nested tree from repository paths."""
    root = {"project": None, "children": {}}
    for project in projects:
        path = project.get("project", ".")
        node = root
        if path != ".":
            for part in path.split("/"):
                node = node["children"].setdefault(part, {"project": None, "children": {}})
        node["project"] = project
    return root


def tree_items(node):
    """Render nested repository links."""
    parts = []
    for name in sorted(node["children"]):
        child = node["children"][name]
        project = child["project"]
        if project:
            sources = project.get("sources") or []
            attention = sum(1 for source in sources if source.get("attention"))
            marker = severity_class(worst(sources))
            count = f'<span class="count">{attention}</span>' if attention else ""
            parts.append(
                f'<li><a href="#{anchor(project.get("project", "."))}">'
                f'<span class="mark {marker}"></span>{esc(name)}{count}</a>'
            )
        else:
            parts.append(f"<li>{esc(name)}")
        if child["children"]:
            parts.append('<ul class="tree">' + tree_items(child) + "</ul>")
        parts.append("</li>")
    return "".join(parts)


def sidebar(report):
    """Render the repository tree navigation."""
    tree = project_tree(report.get("projects") or [])
    parts = ['<nav class="sidebar">', "<h2>Repositories</h2>", '<ul class="tree">']
    root_project = tree["project"]
    if root_project:
        sources = root_project.get("sources") or []
        attention = sum(1 for source in sources if source.get("attention"))
        count = f'<span class="count">{attention}</span>' if attention else ""
        parts.append(
            f'<li><a href="#{anchor(".")}"><span class="mark {severity_class(worst(sources))}">'
            f'</span>ns-3-dev <span class="muted">(root)</span>{count}</a></li>'
        )
    parts.append(tree_items(tree))
    parts.append("</ul>")
    parts.append('<p class="meta"><a href="#legend">Legend</a></p>')
    parts.append("</nav>")
    return "".join(parts)


def render(report):
    """Render a complete self-contained HTML document."""
    summary = report.get("summary") or {}
    projects = report.get("projects") or []
    residues = collections.defaultdict(list)
    for item in report.get("residues") or []:
        residues[item.get("project", ".")].append(item)

    parts = [
        "<!DOCTYPE html>",
        '<html lang="en">',
        "<head>",
        '<meta charset="utf-8">',
        '<meta name="viewport" content="width=device-width, initial-scale=1">',
        "<title>Upstream audit graph</title>",
        f"<style>{STYLE}</style>",
        "</head>",
        "<body>",
        '<header class="page">',
        "<h1>Upstream audit</h1>",
        f'<p class="meta">Generated {esc(stamp(report.get("generated_at")))} '
        f'&middot; {esc(report.get("mode", ""))} &middot; '
        f'{summary.get("projects", len(projects))} repositories &middot; '
        f'{summary.get("urls", 0)} upstream URLs &middot; '
        f'{summary.get("branches", 0)} branches</p>',
        '<p class="summary">',
    ]
    for status, count in sorted((summary.get("statuses") or {}).items()):
        parts.append(
            f'<span class="stat">{chip(status)}<span class="count">{count}</span></span>'
        )
    parts.append(
        '<span class="stat"><span class="chip warn">other branches needing review</span>'
        f'<span class="count">{summary.get("other_branches_needing_review", 0)}</span></span>'
    )
    parts.append(
        '<span class="stat"><span class="chip bad">findings</span>'
        f'<span class="count">{summary.get("residues", 0)}</span></span>'
    )
    parts.append("</p></header>")
    parts.append('<div class="layout">')
    parts.append(sidebar(report))
    parts.append("<main>")
    parts.append(overview(report))
    parts.append(legend())
    for project in projects:
        parts.append(repo_section(project, residues.get(project.get("project", "."), [])))
    parts.append("</main></div>")
    parts.append(
        "<footer>Generated from the JSON report by <code>utils/upstream_graph.py</code>. "
        "The page is self-contained and makes no network requests. Counts and statuses "
        "are evidence for review, not proof of integration.</footer>"
    )
    parts.append("</body></html>")
    return "\n".join(parts)


def write(report, path):
    """Write the rendered document, creating its parent directory."""
    path = Path(path)
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(render(report), encoding="utf-8")


def main(argv=None):
    """Render a saved report without fetching or analyzing repositories."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--json", type=Path, default=Path(".cache/upstream/status.json"), help="Input report"
    )
    parser.add_argument("--html", type=Path, help="Output page; default beside the JSON report")
    args = parser.parse_args(argv)
    report = json.loads(args.json.read_text())
    output = args.html or args.json.with_suffix(".html")
    write(report, output)
    print(f"Wrote {output}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
