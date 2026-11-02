#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-only
"""Exercise the upstream auditor against real, isolated, local Git repositories."""

import contextlib
import importlib.util
import io
import json
import tempfile
import unittest
from pathlib import Path

spec = importlib.util.spec_from_file_location(
    "upstream_status", Path(__file__).resolve().parents[1] / "upstream-status.py"
)
upstream = importlib.util.module_from_spec(spec)
spec.loader.exec_module(upstream)


class UpstreamStatusTest(unittest.TestCase):
    """Verify classifications, scoped history, persistent reviews, and read-only behavior."""

    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory()
        self.addCleanup(self.temporary.cleanup)
        self.base = Path(self.temporary.name)
        self.source = self.new_repo("source")
        self.commit(self.source, {"code": "base\n", ".gitignore": ".cache/\n"})
        self.root = self.base / "project"
        upstream.git(self.base, "clone", "--quiet", self.source, self.root)
        self.manifest(self.source)

    def new_repo(self, name):
        """Create a repository with a deterministic initial branch."""
        path = self.base / name
        upstream.git(self.base, "init", "--quiet", "--initial-branch=main", path)
        return path

    def commit(self, repo, files, message="change"):
        """Write fixture files and commit all changes with a local test identity."""
        for name, content in files.items():
            path = repo / name
            path.parent.mkdir(parents=True, exist_ok=True)
            if content is None:
                path.unlink()
            else:
                path.write_text(content)
        upstream.git(repo, "add", "--all")
        upstream.git(
            repo,
            "-c",
            "user.name=Test",
            "-c",
            "user.email=test@example.invalid",
            "commit",
            "--quiet",
            "-m",
            message,
        )
        return upstream.git(repo, "rev-parse", "HEAD").stdout.strip()

    def manifest(self, repo, branch="main", scope=""):
        """Set the project's manifest without changing its committed history."""
        (self.root / "upstream").write_text(
            f"{repo}#{branch}" + (f" {scope}" if scope else "") + "\n"
        )

    def scan(self, *options, expected=0):
        """Run the public CLI and load its JSON output."""
        with contextlib.redirect_stdout(io.StringIO()), contextlib.redirect_stderr(io.StringIO()):
            code = upstream.main(["--root", str(self.root), "scan", *options])
        self.assertEqual(code, expected)
        return json.loads((self.root / ".cache/upstream/status.json").read_text())

    def declared(self, report):
        """Return the first declared mapping for the root project."""
        return next(s for s in report["projects"][0]["sources"] if s["declared"])

    def test_fast_forward_and_no_mutations(self):
        self.commit(self.source, {"code": "updated\n"})
        before = {
            command: upstream.git(self.root, *command.split()).stdout
            for command in ("rev-parse HEAD", "status --porcelain", "remote -v", "ls-files --stage")
        }
        report = self.scan()
        source = self.declared(report)
        self.assertEqual(source["status"], "fast-forward")
        self.assertEqual(source["pending_commits"], 1)
        self.assertEqual(source["files"], [{"status": "M", "path": "code"}])
        for command, value in before.items():
            self.assertEqual(upstream.git(self.root, *command.split()).stdout, value)

    def test_clean_and_conflicting_divergence(self):
        self.commit(self.root, {"local": "local\n"})
        self.commit(self.source, {"remote": "remote\n"})
        self.assertEqual(self.declared(self.scan())["status"], "merge-clean")
        self.commit(self.root, {"code": "local version\n"})
        self.commit(self.source, {"code": "source version\n"})
        source = self.declared(self.scan())
        self.assertEqual(source["status"], "merge-conflicts")
        self.assertEqual(source["merge"]["conflicts"], ["code"])

    def test_other_branches_are_not_duplicated(self):
        upstream.git(self.source, "branch", "same-tip")
        upstream.git(self.source, "checkout", "--quiet", "-b", "feature")
        self.commit(self.source, {"feature": "candidate\n"})
        upstream.git(self.source, "checkout", "--quiet", "main")
        report = self.scan()
        sources = {s["branch"]: s for s in report["projects"][0]["sources"]}
        self.assertFalse(sources["same-tip"]["attention"])
        self.assertEqual(sources["feature"]["exclusive_commits"], 1)
        self.assertTrue(sources["feature"]["attention"])

    def test_offline_and_fetch_failure(self):
        online = self.scan()
        self.commit(self.source, {"code": "not yet fetched\n"})
        offline = self.scan("--offline")
        self.assertEqual(offline["mode"], "offline-cache")
        self.assertEqual(self.declared(online)["tip"], self.declared(offline)["tip"])
        self.source.rename(self.base / "source-unavailable")
        failed = self.scan(expected=2)
        self.assertIn("fetch-error", {r["kind"] for r in failed["residues"]})
        self.assertEqual(self.declared(failed)["status"], "error")

    def test_rewritten_and_deleted_branches(self):
        self.commit(self.source, {"code": "advance\n"})
        upstream.git(self.source, "branch", "temporary")
        self.scan()
        upstream.git(self.source, "reset", "--hard", "HEAD~1")
        upstream.git(self.source, "branch", "-D", "temporary")
        report = self.scan()
        self.assertEqual(self.declared(report)["movement"], "rewritten-or-unavailable")
        self.assertIn("deleted-branch", {r["kind"] for r in report["residues"]})

    def test_invalid_branch_and_path(self):
        self.manifest(self.source, branch="missing")
        self.assertIn("branch does not exist", self.declared(self.scan(expected=2))["residues"][0])
        self.manifest(self.source, scope="contribt/missing")
        self.assertIn("does not exist", self.declared(self.scan(expected=2))["residues"][0])
        self.manifest(self.source, scope="../escape")
        self.assertIn("invalid-manifest", {r["kind"] for r in self.scan(expected=2)["residues"]})

    def test_path_scope_and_explicit_port_baseline(self):
        source = self.new_repo("monorepo")
        self.commit(source, {"contrib/module/code": "ported\n", "unrelated": "one\n"})
        self.manifest(source, scope="contrib/module")
        self.commit(self.root, {"code": "ported with local customization\n"})
        initial = self.declared(self.scan())
        self.assertEqual(initial["status"], "manual-port")
        self.assertFalse(initial["pending_commits_proven"])
        with contextlib.redirect_stdout(io.StringIO()):
            self.assertEqual(
                upstream.main(
                    [
                        "--root",
                        str(self.root),
                        "review",
                        "--project",
                        ".",
                        "--key",
                        initial["key"],
                        "--decision",
                        "ported",
                        "--reason",
                        "Ported and validated the module",
                    ]
                ),
                0,
            )
        self.commit(self.root, {})
        self.commit(source, {"unrelated": "two\n"})
        self.assertEqual(self.declared(self.scan())["status"], "reviewed")
        self.commit(source, {"contrib/module/code": "next\n"})
        update = self.declared(self.scan())
        self.assertEqual(update["status"], "port-update")
        self.assertEqual(update["pending_commits"], 1)
        self.assertEqual(update["files"], [{"status": "M", "path": "code"}])
        self.assertEqual(update["comparison"], "reviewed-source-to-upstream")

    def test_baseline_invalid_after_source_rewrite(self):
        source = self.new_repo("unrelated")
        self.commit(source, {"code": "one\n"})
        self.commit(source, {"code": "two\n"})
        self.manifest(source)
        self.commit(self.root, {})
        initial = self.declared(self.scan())
        with contextlib.redirect_stdout(io.StringIO()):
            self.assertEqual(
                upstream.main(
                    [
                        "--root",
                        str(self.root),
                        "review",
                        "--project",
                        ".",
                        "--key",
                        initial["key"],
                        "--decision",
                        "ignored",
                        "--reason",
                        "Review fixture",
                    ]
                ),
                0,
            )
        upstream.git(source, "reset", "--hard", "HEAD~1")
        result = self.declared(self.scan())
        self.assertTrue(any("baseline is not an ancestor" in r for r in result["residues"]))
        self.assertEqual(result["status"], "manual-port")

    def test_recursive_and_uninitialized_submodules(self):
        child = self.new_repo("child")
        self.commit(child, {"upstream": f"{self.source}#main\n"})
        upstream.git(
            self.root,
            "-c",
            "protocol.file.allow=always",
            "submodule",
            "add",
            "--quiet",
            child,
            "modules/child",
        )
        self.commit(self.root, {})
        report = self.scan()
        self.assertEqual({p["project"] for p in report["projects"]}, {".", "modules/child"})
        upstream.git(self.root, "submodule", "deinit", "--force", "modules/child")
        self.assertIn(
            "uninitialized-submodule", {r["kind"] for r in self.scan(expected=2)["residues"]}
        )

    def test_fail_on_attention(self):
        self.commit(self.source, {"code": "new\n"})
        self.scan("--fail-on-attention", expected=1)

    def test_unrelated_covered_branches_do_not_need_baselines(self):
        source = self.new_repo("unrelated")
        self.commit(source, {"module/code": "one\n"})
        upstream.git(source, "branch", "old")
        self.commit(source, {"module/code": "two\n"})
        self.manifest(source, scope="module")
        report = self.scan()
        old = next(s for s in report["projects"][0]["sources"] if s["branch"] == "old")
        self.assertEqual(old["status"], "covered-by-declared-or-local")
        self.assertFalse(old["attention"])

    def test_paths_missing_on_other_branches_and_deleted_paths(self):
        upstream.git(self.source, "branch", "no-module")
        self.commit(self.source, {"module/code": "one\n"})
        upstream.git(self.source, "checkout", "--quiet", "-b", "remove-module")
        self.commit(self.source, {"module/code": None})
        upstream.git(self.source, "checkout", "--quiet", "main")
        self.manifest(self.source, scope="module")
        sources = {s["branch"]: s for s in self.scan()["projects"][0]["sources"]}
        self.assertFalse(sources["no-module"]["attention"])
        self.assertTrue(sources["remove-module"]["attention"])
        self.assertTrue(
            any("deleted the selected path" in r for r in sources["remove-module"]["residues"])
        )

    def test_source_file_mapping_and_quoted_path(self):
        source = self.new_repo("file-source")
        self.commit(source, {"folder with spaces/code": "base\n"})
        self.manifest(source, scope='"folder with spaces/code"')
        result = self.declared(self.scan())
        self.assertEqual(result["path"], "folder with spaces/code")
        self.assertNotIn("code", {f["path"] for f in result["files"]})

    def test_shallow_history_is_incomplete(self):
        self.commit(self.source, {"code": "next\n"})
        shallow = self.base / "shallow"
        upstream.git(self.base, "clone", "--quiet", "--depth=1", self.source.as_uri(), shallow)
        self.root = shallow
        self.manifest(self.source)
        result = self.declared(self.scan(expected=2))
        self.assertEqual(result["status"], "unknown-history")

    def test_ignored_snapshot_with_shared_history(self):
        self.commit(self.source, {"code": "reviewed upstream\n"})
        self.commit(self.root, {})
        source = self.declared(self.scan())
        with contextlib.redirect_stdout(io.StringIO()):
            self.assertEqual(
                upstream.main(
                    [
                        "--root",
                        str(self.root),
                        "review",
                        "--project",
                        ".",
                        "--key",
                        source["key"],
                        "--decision",
                        "ignored",
                        "--reason",
                        "Incompatible API reviewed",
                    ]
                ),
                0,
            )
        self.assertEqual(self.declared(self.scan())["status"], "reviewed")
        self.commit(self.source, {"new": "unreviewed\n"})
        result = self.declared(self.scan())
        self.assertNotEqual(result["status"], "reviewed")
        self.assertEqual(result["pending_commits"], 1)

    def test_dirty_review_is_rejected(self):
        source = self.declared(self.scan())
        with contextlib.redirect_stdout(io.StringIO()), contextlib.redirect_stderr(io.StringIO()):
            self.assertEqual(
                upstream.main(
                    [
                        "--root",
                        str(self.root),
                        "review",
                        "--project",
                        ".",
                        "--key",
                        source["key"],
                        "--decision",
                        "ignored",
                        "--reason",
                        "Cannot acknowledge dirty checkout",
                    ]
                ),
                2,
            )
        self.assertFalse((self.root / "upstream-state.json").exists())

    def test_destination_override_and_mapping_change(self):
        source = self.new_repo("mapped-source")
        self.commit(source, {"module/code": "incoming\n"})
        self.manifest(source, scope="module")
        mapping = {
            "project": ".",
            "url": str(source),
            "path": "module",
            "destination": "src/module",
        }
        self.commit(
            self.root,
            {
                "src/module/code": "local\n",
                "upstream-map.json": json.dumps({"schema": 1, "mappings": [mapping]}),
            },
        )
        result = self.declared(self.scan())
        self.assertEqual(result["destination"], "src/module")
        self.assertEqual(result["files"], [{"status": "M", "path": "code"}])
        mapping["destination"] = ""
        (self.root / "upstream-map.json").write_text(
            json.dumps({"schema": 1, "mappings": [mapping]})
        )
        self.assertNotEqual(self.declared(self.scan())["key"], result["key"])

    def test_multiple_reviews_can_update_the_ledger(self):
        self.commit(self.root, {})
        source = self.declared(self.scan())
        for reason in ("First decision", "Amended decision"):
            with contextlib.redirect_stdout(io.StringIO()):
                self.assertEqual(
                    upstream.main(
                        [
                            "--root",
                            str(self.root),
                            "review",
                            "--project",
                            ".",
                            "--key",
                            source["key"],
                            "--decision",
                            "ignored",
                            "--reason",
                            reason,
                        ]
                    ),
                    0,
                )
        ledger = json.loads((self.root / "upstream-state.json").read_text())
        self.assertEqual(ledger["reviews"][source["key"]]["reason"], "Amended decision")


if __name__ == "__main__":
    unittest.main()
