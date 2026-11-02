#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-only
"""Audit upstream files in a project and its recursive Git submodules."""

import argparse
import collections
import datetime
import difflib
import hashlib
import json
import os
import shlex
import signal
import subprocess
import sys
from pathlib import Path, PurePosixPath


class MissingPath(ValueError):
    """Indicate that a selected source path is absent from a commit."""


def timestamp():
    """Return an ISO 8601 UTC timestamp."""
    return datetime.datetime.now(datetime.timezone.utc).isoformat(timespec="seconds")


def identifier(value):
    """Return a stable identifier for a URL or a source mapping."""
    return hashlib.sha256(value.encode()).hexdigest()[:20]


def git(directory, *args, allowed=(0,), timeout=180, stdin=None):
    """Run Git without a shell, returning its captured result."""
    env = dict(os.environ, GIT_TERMINAL_PROMPT="0", GIT_LITERAL_PATHSPECS="1")
    process = subprocess.Popen(
        ["git", "-C", str(directory), *map(str, args)],
        stdin=subprocess.PIPE if stdin is not None else subprocess.DEVNULL,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        encoding="utf-8",
        errors="replace",
        env=env,
        start_new_session=os.name != "nt",
    )
    try:
        stdout, stderr = process.communicate(stdin, timeout=timeout)
    except subprocess.TimeoutExpired:
        if os.name != "nt":
            os.killpg(process.pid, signal.SIGKILL)
        else:
            process.kill()
        process.communicate()
        raise
    result = subprocess.CompletedProcess(process.args, process.returncode, stdout, stderr)
    if result.returncode not in allowed:
        raise RuntimeError(f"git {args[0]}: {result.stderr.strip() or result.stdout.strip()}")
    return result


def write_json(path, value):
    """Atomically replace a JSON document."""
    path.parent.mkdir(parents=True, exist_ok=True)
    temporary = path.with_suffix(path.suffix + ".tmp")
    temporary.write_text(json.dumps(value, indent=2, ensure_ascii=True) + "\n")
    temporary.replace(path)


def load_json(path, default):
    """Load a JSON document or return a default for a missing file."""
    return json.loads(path.read_text()) if path.exists() else default


def residue(report, project, kind, detail, source=None):
    """Append an actionable finding to the report."""
    item = {"project": project, "kind": kind, "detail": detail}
    if source:
        item["source"] = source
    report["residues"].append(item)


def parse_upstream(path):
    """Parse URL#branch [path] lines, preserving individual line errors."""
    entries, errors = [], []
    for number, line in enumerate(path.read_text().splitlines(), 1):
        if not line.strip() or line.lstrip().startswith("#"):
            continue
        try:
            fields = shlex.split(line)
            if len(fields) not in (1, 2) or "#" not in fields[0]:
                raise ValueError("expected URL#branch [path]")
            url, branch = fields[0].rsplit("#", 1)
            if not url or url.startswith("-") or not branch:
                raise ValueError("URL and branch must be nonempty and cannot be options")
            git(path.parent, "check-ref-format", "refs/heads/" + branch)
            scope = fields[1] if len(fields) == 2 else ""
            if scope:
                parsed = PurePosixPath(scope)
                if parsed.is_absolute() or ".." in parsed.parts or scope.startswith(":"):
                    raise ValueError("path must stay within the upstream repository")
                scope = str(parsed)
                if scope == ".":
                    scope = ""
            entry = {"url": url, "branch": branch, "path": scope, "line": number}
            if any(all(old[k] == entry[k] for k in ("url", "branch", "path")) for old in entries):
                raise ValueError("duplicate source mapping")
            entries.append(entry)
        except (ValueError, RuntimeError) as error:
            errors.append(f"upstream:{number}: {error}")
    return entries, errors


def discover(root, report):
    """Discover initialized repositories and report missing submodules and manifests."""
    projects = []

    def visit(directory, pinned=None):
        relative = directory.relative_to(root).as_posix() or "."
        if not (directory / ".git").exists():
            residue(
                report,
                relative,
                "uninitialized-submodule",
                "Run git submodule update --init --recursive.",
            )
            return
        head = git(directory, "rev-parse", "HEAD").stdout.strip()
        project = {
            "project": relative,
            "head": head,
            "head_date": git(directory, "show", "-s", "--format=%cI", head).stdout.strip(),
            "pinned": pinned,
            "shallow": git(directory, "rev-parse", "--is-shallow-repository").stdout.strip()
            == "true",
            "dirty": bool(
                git(directory, "status", "--porcelain", "--untracked-files=normal").stdout
            ),
            "entries": [],
        }
        if project["dirty"]:
            residue(
                report,
                relative,
                "dirty-worktree",
                "Review local changes before integration; comparisons use committed HEAD.",
            )
        if pinned and head != pinned:
            residue(
                report,
                relative,
                "gitlink-drift",
                f"Checkout {head} differs from parent index gitlink {pinned}.",
            )
        if project["shallow"]:
            residue(
                report,
                relative,
                "shallow-history",
                "Unshallow this repository before trusting ancestry results.",
            )
        manifest = directory / "upstream"
        if manifest.is_file():
            project["entries"], errors = parse_upstream(manifest)
            for error in errors:
                residue(report, relative, "invalid-manifest", error)
            if not project["entries"]:
                residue(report, relative, "empty-manifest", "No valid upstream mappings.")
        else:
            residue(
                report,
                relative,
                "missing-manifest",
                "Add an upstream file; origin is not assumed to be upstream.",
            )
        projects.append(project)
        for record in git(directory, "ls-files", "--stage", "-z").stdout.split("\0"):
            if record.startswith("160000 "):
                metadata, name = record.split("\t", 1)
                if metadata.split()[2] != "0":
                    residue(
                        report,
                        relative,
                        "unmerged-gitlink",
                        f"Resolve the index conflict for {name}.",
                    )
                    continue
                visit(directory / name, metadata.split()[1])

    visit(root)
    return projects


class Audit:
    """Analyze fetched sources in a disposable bare repository."""

    def __init__(self, root, cache, projects, timeout):
        """Prepare a cache that borrows local objects without modifying local repositories."""
        self.root, self.cache, self.timeout = root, cache, timeout
        self.repo = cache / "analysis.git"
        if not self.repo.exists():
            self.repo.parent.mkdir(parents=True, exist_ok=True)
            git(self.repo.parent, "init", "--bare", self.repo)
        alternates = []
        for project in projects:
            directory = root / project["project"]
            objects = git(directory, "rev-parse", "--git-path", "objects").stdout.strip()
            objects = (directory / objects).resolve()
            if not project["shallow"]:
                alternates.append(str(objects))
        (self.repo / "objects/info/alternates").write_text("\n".join(alternates) + "\n")
        for project in projects:
            if not project["shallow"]:
                self.run(
                    "update-ref", "refs/local/" + identifier(project["project"]), project["head"]
                )

    def run(self, *args, **kwargs):
        """Run Git in the analysis cache."""
        return git(self.repo, *args, timeout=self.timeout, **kwargs)

    def heads(self, url):
        """Return cached advertised branch heads for one source URL."""
        prefix = "refs/upstreams/" + identifier(url) + "/"
        output = self.run("for-each-ref", "--format=%(refname) %(objectname)", prefix).stdout
        return {line.split()[0][len(prefix) :]: line.split()[1] for line in output.splitlines()}

    def fetch(self, url):
        """Fetch and prune all heads of one source into its private namespace."""
        self.run(
            "-c",
            "gc.auto=0",
            "fetch",
            "--quiet",
            "--atomic",
            "--prune",
            "--no-tags",
            "--no-recurse-submodules",
            "--no-write-fetch-head",
            url,
            "+refs/heads/*:refs/upstreams/" + identifier(url) + "/*",
        )

    def ancestor(self, older, newer):
        """Test reachability, raising errors rather than interpreting them as unrelatedness."""
        return self.run("merge-base", "--is-ancestor", older, newer, allowed=(0, 1)).returncode == 0

    def history(self, local, source):
        """Classify whole repository ancestry and count commits on both sides."""
        base = self.run("merge-base", local, source, allowed=(0, 1)).stdout.strip()
        if not base:
            return {"relation": "unrelated", "merge_base": None}
        ahead, behind = map(
            int,
            self.run("rev-list", "--left-right", "--count", f"{local}...{source}").stdout.split(),
        )
        relation = (
            "diverged"
            if ahead and behind
            else "behind" if behind else "ahead" if ahead else "equal"
        )
        return {
            "relation": relation,
            "merge_base": base,
            "local_only": ahead,
            "upstream_only": behind,
        }

    def tree(self, commit, scope):
        """Resolve a source directory, or map a source file to its basename at the target root."""
        if not scope:
            return self.run("rev-parse", commit + "^{tree}").stdout.strip()
        listing = self.run("ls-tree", "-z", commit, "--", scope).stdout
        if not listing:
            parent = PurePosixPath(scope).parent
            siblings = self.run(
                "ls-tree",
                "--name-only",
                "-z",
                commit if str(parent) == "." else f"{commit}:{parent}",
                allowed=(0, 128),
            ).stdout.split("\0")
            matches = difflib.get_close_matches(PurePosixPath(scope).name, siblings, n=3)
            suggestion = (
                " Similar paths: " + ", ".join(str(parent / match) for match in matches)
                if matches
                else ""
            )
            raise MissingPath(f"Path {scope!r} does not exist at {commit}.{suggestion}")
        metadata, _ = listing.rstrip("\0").split("\t", 1)
        mode, kind, oid = metadata.split()
        if kind == "tree":
            return oid
        if kind == "blob":
            return self.run(
                "mktree", "-z", stdin=f"{mode} blob {oid}\t{PurePosixPath(scope).name}\0"
            ).stdout.strip()
        raise ValueError(
            f"Path {scope!r} is a gitlink; declare its repository as an upstream instead."
        )

    def changes(self, older_tree, newer_tree):
        """Return all incoming file differences, with no rename heuristics or truncation."""
        fields = self.run(
            "diff",
            "--no-ext-diff",
            "--no-renames",
            "--name-status",
            "-z",
            older_tree,
            newer_tree,
            "--",
        ).stdout.split("\0")
        return [{"status": fields[i], "path": fields[i + 1]} for i in range(0, len(fields) - 1, 2)]

    def commits(self, tip, exclusions, scope, limit):
        """Count missing commits and sample them, limiting history to the configured source path."""
        revisions = [tip, *("^" + oid for oid in sorted(set(exclusions)))]
        paths = ["--", scope] if scope else ["--"]
        count = int(self.run("rev-list", "--full-history", "--count", *revisions, *paths).stdout)
        newer = self.run(
            "log", "--full-history", f"-{limit}", "--format=%H%x09%cI%x09%s", *revisions, *paths
        ).stdout
        sample = []
        for line in newer.splitlines():
            oid, date, subject = line.split("\t", 2)
            sample.append({"commit": oid, "date": date, "subject": subject})
        return count, sample

    def merge_check(self, local, source):
        """Predict merge conflicts without writing an index, worktree, or commit."""
        result = self.run(
            "merge-tree", "--write-tree", "--name-only", "-z", local, source, allowed=(0, 1)
        )
        paths = result.stdout.split("\0\0", 1)[0].split("\0")[1:]
        return {
            "clean": result.returncode == 0,
            "conflicts": [p for p in paths if p],
            "diagnostic": result.stderr.strip(),
        }


def mapping_key(project, url, branch, scope, destination=""):
    """Identify an exact project, source URL, branch, and path mapping."""
    return identifier(json.dumps([project, url, branch, scope, destination]))


def analyze_source(
    audit, project, entry, branch, tip, declared, declared_tips, ledger, previous, args
):
    """Analyze one declared or discovered branch for one path mapping."""
    url, scope, local = entry["url"], entry["path"], project["head"]
    destination = entry.get("destination", "")
    label = f"{url}#{branch}" + (f" {scope}" if scope else "")
    key = mapping_key(project["project"], url, branch, scope, destination)
    result = {
        "key": key,
        "source": label,
        "url": url,
        "branch": branch,
        "path": scope,
        "destination": destination,
        "declared": declared,
        "tip": tip,
        "tip_date": audit.run("show", "-s", "--format=%cI", tip).stdout.strip(),
        "residues": [],
    }
    old_tip = previous.get(branch)
    if old_tip and old_tip != tip:
        exists = audit.run("cat-file", "-e", old_tip, allowed=(0, 1, 128)).returncode == 0
        result["movement"] = (
            "advanced" if exists and audit.ancestor(old_tip, tip) else "rewritten-or-unavailable"
        )
        if result["movement"] != "advanced":
            result["residues"].append(
                "Branch history was rewritten, or the old object is unavailable; recheck port baselines."
            )
    if project["shallow"]:
        result.update(status="unknown-history", attention=True)
        return result
    history = audit.history(local, tip)
    result.update(history)
    receipt = ledger.get(key)
    baseline = None
    if receipt:
        try:
            valid = audit.ancestor(receipt["local_head"], local) and audit.ancestor(
                receipt["upstream_commit"], tip
            )
            if valid:
                baseline = receipt["upstream_commit"]
                result["review"] = receipt
            else:
                result["residues"].append(
                    "Review baseline is not an ancestor of both current heads; review it again."
                )
        except RuntimeError:
            result["residues"].append("Review baseline objects are unavailable; review it again.")
    exclusions = [local]
    if baseline:
        exclusions.append(baseline)
    count, sample = audit.commits(tip, exclusions, scope, args.sample)
    result.update(pending_commits=count, commits=sample)
    if not declared:
        exclusive, exclusive_sample = audit.commits(
            tip, [*exclusions, *declared_tips], scope, args.sample
        )
        result["exclusive_commits"] = exclusive
        result["exclusive_commit_samples"] = exclusive_sample
        if exclusive == 0:
            result.update(status="covered-by-declared-or-local", attention=bool(result["residues"]))
            return result
    try:
        source_tree = audit.tree(tip, scope)
    except MissingPath:
        if declared or not scope:
            raise
        source_tree = audit.run("mktree", stdin="").stdout.strip()
        result["residues"].append(
            "This branch deleted the selected path; review whether that deletion should be ported."
        )
    if scope or destination or history["relation"] == "unrelated":
        before = audit.tree(baseline, scope) if baseline else audit.tree(local, destination)
        changes = audit.changes(before, source_tree)
        result.update(
            files=changes,
            comparison=(
                "reviewed-source-to-upstream" if baseline else "local-destination-to-upstream-scope"
            ),
        )
        result["pending_commits_proven"] = bool(baseline)
        if baseline:
            result["status"] = "reviewed" if count == 0 else "port-update"
        else:
            result["status"] = "content-identical" if not changes else "manual-port"
        if result["status"] == "manual-port":
            result["residues"].append(
                "Establish a reviewed source baseline; ancestry cannot prove which scoped changes were ported."
            )
    else:
        result["pending_commits_proven"] = True
        relation = history["relation"]
        result["status"] = {
            "equal": "up-to-date",
            "ahead": "up-to-date",
            "behind": "fast-forward",
            "diverged": "merge-review",
        }[relation]
        if relation == "diverged" and declared and not args.no_merge_check:
            result["merge"] = audit.merge_check(local, tip)
            result["status"] = "merge-clean" if result["merge"]["clean"] else "merge-conflicts"
        if count:
            result["files"] = audit.changes(audit.tree(history["merge_base"], ""), source_tree)
        if baseline and count == 0:
            result["status"] = "reviewed"
    result["attention"] = result["status"] not in (
        "up-to-date",
        "reviewed",
        "content-identical",
    ) or bool(result["residues"])
    return result


def markdown(report):
    """Render a concise inventory and the full list of manual findings."""

    def cell(value):
        return str(value).replace("|", "\\|").replace("\n", " ").replace("\r", " ")

    lines = [
        "# Upstream audit",
        "",
        f"Generated: {report['generated_at']}",
        "",
        f"Mode: {report['mode']}. Comparisons use committed HEAD; a clean merge prediction still needs build/test review.",
        "",
        "| Project | Declared source | Status | Pending commits | Files / conflicts |",
        "| --- | --- | --- | ---: | ---: |",
    ]
    for project in report["projects"]:
        for source in project.get("sources", []):
            if not source["declared"]:
                continue
            count = source.get("pending_commits", "?")
            if source.get("pending_commits_proven") is False:
                count = f"{count} (unverified)"
            files = len(source.get("files", []))
            conflicts = len(source.get("merge", {}).get("conflicts", []))
            lines.append(
                f"| {cell(project['project'])} | {cell(source['source'])}{' -> ' + cell(source['destination']) if source.get('destination') else ''} | {cell(source['status'])} | {count} | {files} / {conflicts} |"
            )
    lines += [
        "",
        "## Other branches needing review",
        "",
        "Counts exclude commits already in local HEAD, declared branches, or a valid review baseline.",
        "",
    ]
    for project in report["projects"]:
        for source in project.get("sources", []):
            if not source["declared"] and source.get("attention"):
                lines.append(
                    f"- `{cell(project['project'])}`: {cell(source['source'])}: **{source['status']}**, {source.get('exclusive_commits', '?')} exclusive commits."
                )
    lines += [
        "",
        "## Fetch observations",
        "",
        "| Repository | Last successful fetch | Branches |",
        "| --- | --- | ---: |",
    ]
    for inventory in report["upstreams"]:
        lines.append(
            f"| {cell(inventory['url'])} | {cell(inventory.get('fetched_at', 'FAILED'))} | {len(inventory['heads'])} |"
        )
    lines += ["", "## Manual attention", ""]
    if not report["residues"]:
        lines.append("No outstanding findings.")
    for item in report["residues"]:
        label = f" ({cell(item['source'])})" if "source" in item else ""
        lines.append(
            f"- **{cell(item['kind'])}** `{cell(item['project'])}`{label}: {cell(item['detail'])}"
        )
    lines += [
        "",
        "The JSON report contains complete branch inventories, changed paths, commit samples, source hashes, review receipts, and fetch errors.",
        "",
    ]
    return "\n".join(lines)


def scan(args):
    """Fetch each distinct URL once and write machine and human readable reports."""
    root, cache = args.root.resolve(), args.cache.resolve()
    report = {
        "schema": 1,
        "generated_at": timestamp(),
        "mode": "offline-cache" if args.offline else "live-fetch",
        "residues": [],
    }
    projects = discover(root, report)
    mappings = load_json(args.mappings, {"schema": 1, "mappings": []})["mappings"]
    for project in projects:
        for entry in project["entries"]:
            matches = [
                m
                for m in mappings
                if (m["project"], m["url"], m["path"])
                == (project["project"], entry["url"], entry["path"])
            ]
            if len(matches) > 1:
                raise ValueError(
                    f"Duplicate destination mappings for {project['project']} {entry['url']} {entry['path']}"
                )
            destination = matches[0]["destination"] if matches else ""
            parsed = PurePosixPath(destination)
            if parsed.is_absolute() or ".." in parsed.parts or destination.startswith(":"):
                raise ValueError(
                    f"Destination must stay within its local repository: {destination}"
                )
            entry["destination"] = "" if str(parsed) == "." else str(parsed)
    audit = Audit(root, cache, projects, args.timeout)
    observations = load_json(cache / "observations.json", {"sources": {}})
    ledger = load_json(args.state, {"schema": 1, "reviews": {}})["reviews"]
    urls = sorted({entry["url"] for p in projects for entry in p["entries"]})
    inventories = {}
    for url in urls:
        print(
            f"{'Reading cache' if args.offline else 'Fetching'} {url}", file=sys.stderr, flush=True
        )
        previous = observations["sources"].get(url, {})
        inventory = {"url": url, "previous": previous.get("heads", {})}
        try:
            if not args.offline:
                audit.fetch(url)
            inventory["heads"] = audit.heads(url)
            if args.offline:
                inventory["fetched_at"] = previous.get("fetched_at")
                if not inventory["fetched_at"]:
                    raise ValueError("No successful fetch recorded; run an online scan first.")
            else:
                inventory["fetched_at"] = timestamp()
                observations["sources"][url] = {
                    "heads": inventory["heads"],
                    "fetched_at": inventory["fetched_at"],
                }
        except (RuntimeError, ValueError, subprocess.TimeoutExpired) as error:
            inventory["error"] = str(error)
            inventory["heads"] = {}
        inventories[url] = inventory
    for project in projects:
        project["sources"] = []
        grouped = collections.defaultdict(list)
        for entry in project["entries"]:
            grouped[(entry["url"], entry["path"])].append(entry)
        for (url, scope), entries in grouped.items():
            inventory = inventories[url]
            declared = {e["branch"] for e in entries}
            if "error" in inventory:
                residue(report, project["project"], "fetch-error", inventory["error"], url)
            for removed in (
                sorted(set(inventory["previous"]) - set(inventory["heads"]))
                if "error" not in inventory
                else []
            ):
                residue(
                    report,
                    project["project"],
                    "deleted-branch",
                    f"Previously observed branch {removed!r} disappeared.",
                    url,
                )
            tips = [
                inventory["heads"][branch] for branch in declared if branch in inventory["heads"]
            ]
            for branch in sorted(declared | set(inventory["heads"])):
                label = f"{url}#{branch}" + (f" {scope}" if scope else "")
                try:
                    if branch not in inventory["heads"]:
                        raise ValueError(
                            "Source unavailable after fetch failure."
                            if "error" in inventory
                            else "Declared branch does not exist; correct the manifest."
                        )
                    source = analyze_source(
                        audit,
                        project,
                        entries[0],
                        branch,
                        inventory["heads"][branch],
                        branch in declared,
                        tips,
                        ledger,
                        inventory["previous"],
                        args,
                    )
                except (RuntimeError, ValueError, subprocess.TimeoutExpired) as error:
                    source = {
                        "source": label,
                        "declared": branch in declared,
                        "branch": branch,
                        "url": url,
                        "path": scope,
                        "status": "error",
                        "attention": True,
                        "residues": [str(error)],
                    }
                project["sources"].append(source)
                for detail in source["residues"]:
                    residue(report, project["project"], source["status"], detail, label)
                if source["attention"] and not source["residues"]:
                    detail = f"Review {source['status']} candidate at {source.get('tip', '?')}."
                    if source.get("merge", {}).get("conflicts"):
                        conflicts = source["merge"]["conflicts"]
                        detail += f" {len(conflicts)} conflicts; sample: " + ", ".join(
                            conflicts[:8]
                        )
                    residue(report, project["project"], source["status"], detail, label)
    report["projects"] = projects
    report["upstreams"] = list(inventories.values())
    report["summary"] = {
        "projects": len(projects),
        "urls": len(urls),
        "branches": sum(len(inventory["heads"]) for inventory in inventories.values()),
        "other_branches_needing_review": sum(
            not source["declared"] and source["attention"]
            for project in projects
            for source in project["sources"]
        ),
        "residues": len(report["residues"]),
        "statuses": dict(
            collections.Counter(
                s["status"] for p in projects for s in p["sources"] if s["declared"]
            )
        ),
        "errors": sum(
            r["kind"]
            in (
                "fetch-error",
                "error",
                "invalid-manifest",
                "uninitialized-submodule",
                "missing-manifest",
                "empty-manifest",
                "shallow-history",
                "unmerged-gitlink",
            )
            for r in report["residues"]
        ),
    }
    write_json(args.json, report)
    args.markdown.parent.mkdir(parents=True, exist_ok=True)
    args.markdown.write_text(markdown(report))
    if not args.offline:
        write_json(cache / "observations.json", observations)
    print(json.dumps(report["summary"], indent=2))
    print(f"Reports: {args.markdown}\n         {args.json}")
    return (
        2
        if report["summary"]["errors"]
        else 1 if args.fail_on_attention and report["residues"] else 0
    )


def record_review(args):
    """Record an explicit review decision for a source snapshot and local HEAD."""
    report = load_json(args.json, {})
    project = next((p for p in report.get("projects", []) if p["project"] == args.project), None)
    if not project:
        raise ValueError("Project not present in report; scan first.")
    source = next((s for s in project["sources"] if s.get("key") == args.key), None)
    if not source or "tip" not in source:
        raise ValueError(
            "Source key not present or source unavailable; use a successful report entry's key."
        )
    directory = args.root / args.project
    head = git(directory, "rev-parse", "HEAD").stdout.strip()
    changes = iter(
        git(directory, "status", "--porcelain", "-z", "--untracked-files=normal").stdout.split("\0")
    )
    dirty = False
    for change in changes:
        if not change:
            continue
        names = [change[3:]]
        if "R" in change[:2] or "C" in change[:2]:
            names.append(next(changes))
        dirty |= any((directory / name).resolve() != args.state.resolve() for name in names)
    if head != project["head"] or dirty:
        raise ValueError(
            "Commit the integration and local changes, then scan again before recording review."
        )
    if not args.reason.strip():
        raise ValueError("A review reason is required.")
    ledger = load_json(args.state, {"schema": 1, "reviews": {}})
    ledger["reviews"][args.key] = {
        "project": args.project,
        "url": source["url"],
        "branch": source["branch"],
        "path": source["path"],
        "destination": source.get("destination", ""),
        "upstream_commit": source["tip"],
        "local_head": head,
        "decision": args.decision,
        "reason": args.reason,
        "reviewed_at": timestamp(),
    }
    write_json(args.state, ledger)
    print(
        f"Recorded {args.decision} review in {args.state}; commit this ledger for repeatable audits."
    )
    return 0


def main(argv=None):
    """Parse audit options and convert operational failures to a nonzero status."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", type=Path, default=Path(__file__).resolve().parents[1])
    parser.add_argument("--cache", type=Path, help="Disposable cache; default ROOT/.cache/upstream")
    parser.add_argument(
        "--mappings", type=Path, help="Destination overrides; default ROOT/upstream-map.json"
    )
    parser.add_argument(
        "--state", type=Path, help="Reviewed port ledger; default ROOT/upstream-state.json"
    )
    parser.add_argument("--json", type=Path, help="JSON report; default CACHE/status.json")
    parser.add_argument("--markdown", type=Path, help="Markdown report; default CACHE/status.md")
    subparsers = parser.add_subparsers(dest="command", required=True)
    scan_parser = subparsers.add_parser(
        "scan", help="Fetch all source heads and report their status"
    )
    scan_parser.add_argument(
        "--offline", action="store_true", help="Use previously fetched heads; report their age"
    )
    scan_parser.add_argument(
        "--no-merge-check", action="store_true", help="Skip merge-tree conflict prediction"
    )
    scan_parser.add_argument(
        "--timeout", type=int, default=180, help="Timeout in seconds per Git operation"
    )
    scan_parser.add_argument(
        "--sample",
        type=int,
        default=8,
        help="Commit samples per source; counts and paths remain complete",
    )
    scan_parser.add_argument(
        "--fail-on-attention",
        action="store_true",
        help="Return 1 for findings (operational errors return 2)",
    )
    review_parser = subparsers.add_parser(
        "review", help="Explicitly record a completed port or an ignored snapshot"
    )
    review_parser.add_argument(
        "--project", required=True, help="Repository path as shown in the report, or ."
    )
    review_parser.add_argument(
        "--key", required=True, help="Exact source mapping key from the JSON report"
    )
    review_parser.add_argument("--decision", choices=("ported", "ignored"), required=True)
    review_parser.add_argument("--reason", required=True)
    args = parser.parse_args(argv)
    args.root = args.root.resolve()
    args.cache = args.cache or args.root / ".cache/upstream"
    args.state = args.state or args.root / "upstream-state.json"
    args.mappings = args.mappings or args.root / "upstream-map.json"
    args.json = args.json or args.cache / "status.json"
    args.markdown = args.markdown or args.cache / "status.md"
    if args.command == "scan" and (args.sample < 0 or args.timeout < 1):
        parser.error("sample must be nonnegative and timeout must be positive")
    try:
        return scan(args) if args.command == "scan" else record_review(args)
    except (OSError, RuntimeError, ValueError, KeyError, subprocess.TimeoutExpired) as error:
        print(f"Upstream audit failed: {error}", file=sys.stderr)
        return 2


if __name__ == "__main__":
    sys.exit(main())
