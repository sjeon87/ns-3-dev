# Tracking upstream repositories

Run the auditor from any directory with Python 3.9 or later and Git 2.38 or later:

```sh
python3 utils/upstream-status.py scan
```

The default outputs are `.cache/upstream/status.md` and
`.cache/upstream/status.json`. The Markdown report lists declared upstreams,
other branches requiring review, and outstanding manual work. JSON contains
all advertised branches, full commit IDs, commit counts, commit samples, all
changed paths, conflict paths, errors, and fetch timestamps. The cache is
already excluded by the project's `.gitignore`.

The auditor reads the root repository and every initialized recursive Git
submodule, including `contrib/satellite/data`. An uninitialized submodule or
missing `upstream` file is reported rather than silently omitted. Initialize
missing submodules separately with `git submodule update --init --recursive`.
The parent index gitlink and actual submodule HEAD are both recorded; drift
requires a deliberate choice of the commit to integrate and pin.

## Source declarations

Each repository owns an `upstream` file with one mapping per line:

```text
https://example.org/project.git#main
https://example.org/monorepo.git#integration contrib/module
```

Blank lines and lines starting with `#` are accepted. Quote paths containing
spaces. Branch names may contain slashes. A directory maps its contents to the
local repository root; an individual file maps to its basename at the local
root. Absolute paths, paths escaping the source repository, malformed branches,
duplicate mappings, and nonexistent declared branches or paths produce findings.
Gitlinks cannot be used as source directories; declare the underlying repository
instead. The auditor reads the current working copies of manifests, so uncommitted
declarations are included and a dirty worktree is reported.

If a selected path belongs inside the local repository rather than at its root,
declare the destination in the root `upstream-map.json`. Its `mappings` array
contains `project`, `url`, `path`, and `destination` fields. The included mmwave
mapping compares toolkit `contrib/mmwave` with `external/ns3-mmwave/src/mmwave`.
An override applies to all branches of that exact project, URL and source path.
Relative destinations cannot escape the local repository. Changing a destination
changes its review key, so an old port receipt cannot silently acknowledge a
different mapping. A source file mapped into a destination directory retains
its basename. Destination directories must already exist in committed local HEAD.

URLs are explicit source identities: no repository is inferred from `origin`,
and distinct URL spellings are not automatically treated as aliases. No remote
configuration, local branches, index entries, working files, or submodule pins
are changed. Fetching, merge simulations, and report generation write only to
the disposable cache. The separate `review` command writes the review ledger.

## Comparison and integration strategies

| Situation | Automated evidence | Integration work |
| --- | --- | --- |
| Whole repository, upstream contained in HEAD | Reachability and ahead/behind counts | No upstream integration required |
| Whole repository, local HEAD contained in upstream | `fast-forward`, missing commits and changed paths | Integrate the pinned source commit with `git merge --ff-only` in a clean worktree; build and test |
| Whole repository, diverged history | Merge base, counts, `merge-tree` result, conflict paths | Review a merge in an integration worktree; resolve conflicts; build and test |
| Selected path, regardless of whole repository ancestry | Scoped commit history and a source tree mapped to the local root | Review the scoped delta; port or use a deliberately established subtree workflow |
| Whole repository, unrelated history | Source snapshot compared with local root | Identify a trusted source baseline, review differences and port deliberately |
| Previously reviewed port or ignored snapshot | Source changes since that exact source commit, validated against current local history | Review only subsequent source changes; update the receipt after completing review |
| Unavailable source, invalid declaration, shallow history | Explicit error, without claiming that the project is current | Correct the declaration, credentials, network access or clone depth; rerun |

For related whole repositories, “missing” means commits reachable from upstream
but not local HEAD. Commit dates do not decide whether an update exists: rebases,
cherry-picks and clock skew make date cutoffs unreliable. Dates and subjects are
included as review context. A cherry-pick can have equivalent content while its
commit remains absent by ancestry; use a reasoned review receipt when appropriate.

For a path mapping, a shared whole repository merge base is not permission to
merge the entire source repository. Only the indicated path is considered. Until
a reviewed baseline exists, the report labels counts as unverified: it cannot
infer whether previous changes were copied, rewritten or partially ported. A
snapshot diff is evidence for review, not a patch to apply indiscriminately.
`A` means present only in the incoming source, `D` means present only in the
comparison baseline/local root, and `M` means changed. Local customization and
tracking metadata may intentionally account for these differences. Exact tree
equality is reported as `content-identical`; it does not establish an ongoing
port baseline.

`git merge-tree --write-tree` predicts a merge of committed snapshots without
changing an index or working tree. A clean prediction does not establish API,
behavioral or build compatibility. See the [Git merge-tree reference](https://git-scm.com/docs/git-merge-tree).
Predictions run in the cache and may differ from integrations using custom
repository merge drivers/configuration. Use `scan --no-merge-check` to skip them;
diverged sources then remain `merge-review` findings.

## Report statuses

Statuses describe the relationship between committed local HEAD and a source
snapshot, or the outcome of a simulated integration. They do not indicate that
the scanner applied changes. Working manifests select the sources to audit;
uncommitted source changes are excluded from content and history comparisons.

| Status | Meaning | Next action |
| --- | --- | --- |
| `up-to-date` | All commits from the tracked upstream branch are contained in local history. Local commits may exist on top. | No upstream integration is currently identified. |
| `fast-forward` | Local HEAD is an ancestor of upstream HEAD, which has additional commits. | Review the pinned source commit, advance with a fast-forward integration, then build and test. |
| `merge-clean` | The histories diverged, but the simulated merge found no Git conflicts. | Review the merged result and test compatibility before integrating. |
| `merge-conflicts` | The histories diverged and the simulated merge found conflicts. | Review the listed conflict paths, resolve them during integration, then build and test. |
| `merge-review` | The histories diverged and no merge simulation was performed. This is the default for discovered branches and for declared branches scanned with `--no-merge-check`. | Decide whether the branch is relevant and investigate its integration. |
| `manual-port` | A selected source or destination path, or a repository with unrelated history, differs from local content and has no valid review baseline. | Establish what was already incorporated, review the differences, and record a baseline after completing the port or deciding to ignore the snapshot. |
| `port-update` | A valid review baseline exists for a path mapping or unrelated history, and additional relevant upstream commits remain unreviewed. | Review the source changes since the baseline, port or reject them, and update the receipt. |
| `reviewed` | A valid receipt accounts for all relevant commits through the current source snapshot. Its decision can be `ported` or `ignored`. | No additional review is currently identified for this mapping. |
| `content-identical` | The selected source tree exactly matches the committed local destination tree. | No current content difference exists. An ongoing port baseline is not established automatically. |
| `covered-by-declared-or-local` | An undeclared branch adds no scoped candidate commits beyond local HEAD, declared branches for the same URL and path, or its valid review baseline. | No separate branch review is identified. Declared sources may still need integration. |
| `unknown-history` | The local repository is shallow, preventing a reliable ancestry assessment. | Obtain complete local history and rescan. |
| `error` | A comparison could not be completed, for example because fetching failed, a declared branch or path is missing, or a Git operation failed. | Correct the reported cause and rescan. |

`manual-port` expresses uncertainty about previous integration. Some source
changes may already have been copied, cherry-picked, rewritten or partially
ported locally. A differing snapshot also includes intentional local changes
and tracking metadata. The status alone cannot establish that every reported
commit or file difference needs porting. For example, a toolkit module selected
with `contrib/mmwave` is compared with the configured local `src/mmwave`
destination; its source history cannot establish which changes were previously
copied there without a reviewed baseline.

The exact status for diverged histories awaiting merge simulation is
`merge-review`. There is no separate `manual-review` status. The report's
**Manual attention** section collects outstanding findings across statuses,
repositories and source mappings. Additional findings, such as a rewritten
branch or an invalid receipt, can require attention even when the source status
would otherwise need no action.

### Counts and report fields

| Field or column | Interpretation |
| --- | --- |
| Pending commits / `pending_commits` | Source commits not reachable from local HEAD or a valid review baseline, limited to the selected source path when present. The JSON `commits` field contains samples rather than the entire list. |
| `(unverified)` / `pending_commits_proven: false` | No validated review baseline establishes which scoped or unrelated source changes were already handled. The count represents candidate history, potentially including the entire source history. |
| `exclusive_commits` | For an undeclared branch, candidate commits remaining after also excluding declared branches for the same URL and source path. `exclusive_commit_samples` samples these commits. |
| Files / conflicts | The number of differing file paths and predicted conflict paths. A zero conflict count only establishes a clean simulation when the status is `merge-clean`; a simulation may not have run for other statuses. |
| `attention` | Whether this source entry needs review because of its status or additional findings. This flag does not describe repository findings such as a dirty worktree. |
| `summary.statuses` | Counts of statuses for declared mappings only. One local repository can have multiple mappings. |
| `summary.other_branches_needing_review` | The number of undeclared branch entries requiring attention across project/source/path mappings. The same remote branch may be counted for multiple local projects or source paths. |
| `summary.residues` | The total number of outstanding findings, including repository conditions and source integration work. This is not a count of unique repositories or commits. |
| `summary.errors` | Findings that prevent a complete audit, such as fetch failures, invalid declarations, shallow history or unavailable submodules. Zero means the audit completed without these errors; review and integration findings can remain. |

For manual ports, the file comparison initially runs from the committed local
destination to the current source snapshot. After a valid baseline is recorded,
it runs from the reviewed source snapshot to the current source snapshot. Thus
the later report isolates subsequent upstream changes instead of repeatedly
listing intentional local differences. Shared-history whole-repository
integration candidates show upstream file changes since the merge base.

### Other manual findings

Findings can reuse a source status, such as `manual-port` or `merge-conflicts`.
The following finding kinds describe repository, declaration or observation
conditions separately from source integration status:

| Finding | Meaning and action |
| --- | --- |
| `dirty-worktree` | Tracked or untracked local files have changes. Review them before integrating; the audit compares committed HEAD. |
| `gitlink-drift` | A submodule's checked-out HEAD differs from its parent index gitlink. Deliberately select the checkout and parent pin to retain. |
| `unmerged-gitlink` | The parent index contains an unresolved submodule conflict. Resolve the pin before reassessing that submodule. |
| `uninitialized-submodule` | A discovered submodule is not initialized and could not be audited. Initialize it and rescan. |
| `missing-manifest` | A local repository has no `upstream` file. Add its intended sources; the auditor does not infer them from `origin`. |
| `empty-manifest` | An `upstream` file has no valid source mappings. Add or correct its entries. |
| `invalid-manifest` | An `upstream` entry is malformed, duplicated, or otherwise invalid. Correct the reported line. |
| `shallow-history` | Local ancestry is incomplete. Unshallow the repository and rescan. |
| `fetch-error` | A source could not be fetched or its offline cache is unavailable. Restore access or populate the cache, then rescan. |
| `deleted-branch` | A previously observed upstream branch disappeared. Review whether its declaration or any associated baseline should be retained. |

Branch rewrites and invalid review baselines are described in an entry's
`residues` messages; they are not additional source status names. A rewrite is
also recorded as `movement: rewritten-or-unavailable`. These findings require
review before relying on previous observations or receipts.

## Other branches and recurring scans

Every distinct URL is fetched once per scan, including **all** advertised branch
heads, into `refs/upstreams/<url-id>/*` in the cache's bare repository. Explicit
refspecs and pruning keep URL namespaces separate; see the
[Git fetch reference](https://git-scm.com/docs/git-fetch). Deleted branches and
non-fast-forward movements relative to the last successful observation are
reported. New URLs have no prior observation, so rewrites before the first scan
cannot be detected.

For each source/path mapping the auditor also examines undeclared branches.
The `exclusive_commits` count excludes commits already reachable from local
HEAD, any declared branch **for that same URL and path**, and that branch's valid
review baseline. This exposes branch work without repeating commits already
covered by declared branches. All branches remain in JSON, including older heads
and branches covered by a declared source. Paths that never existed on another
branch are irrelevant to that mapping; a branch that deletes an existing path
requires review. Commits on an unrelated branch are candidates for review,
not proof that their changes have never been ported.

Only commits reachable from advertised `refs/heads/*` are covered. Unadvertised
or unreachable commits, tag-only histories, pull-request refs, and other forks
absent from the manifests are outside this inventory. Git cannot enumerate
unadvertised objects as a general upstream monitoring operation.

```sh
# No network calls; the report explicitly identifies cached observations.
python3 utils/upstream-status.py scan --offline

# CI: 0 = operational success, 1 = manual findings, 2 = incomplete/error.
python3 utils/upstream-status.py scan --fail-on-attention

# Increase the timeout for large initial fetches; it applies per Git operation.
python3 utils/upstream-status.py scan --timeout 600

# Limit commit samples without truncating counts, inventories or changed paths.
python3 utils/upstream-status.py scan --sample 20
```

A failed live fetch produces an error instead of substituting stale heads. An
offline scan requires a previous successful fetch and preserves its timestamp;
it cannot assert current remote status. `observations.json` records observation,
not review or integration, and a successful scan never acknowledges changes.
Reports are snapshots of the separately fetched repositories, not a transaction
across all upstream servers.

The bare cache borrows objects from the current local repositories via Git
alternates to avoid copying their histories. It fetches complete source histories;
large initial downloads are possible. It is disposable and specific to this
checkout: remove `.cache/upstream` and rescan if repositories have been removed,
garbage collected, or relocated. Do not run overlapping scans against one cache.
Authentication uses the existing Git credential configuration without interactive
terminal prompts. No credentials are written to the reports by the auditor;
do not embed credentials in manifest URLs, which are included verbatim.

## Explicit review receipts

After porting, cherry-picking, or deciding to ignore an exact snapshot:

1. Commit and validate the integration in the affected repository. For a
   submodule, also update and commit its parent gitlink when appropriate.
2. Rescan so the report identifies the final local HEAD and the exact source tip
   that was reviewed. Check that this source tip is the snapshot actually handled.
3. Record the decision using the mapping's `key` from JSON:

   ```sh
   python3 utils/upstream-status.py review \
     --project contrib/nr --key SOURCE_KEY \
     --decision ported --reason 'Ported the reviewed source snapshot; nr tests pass'
   ```

   Use `--decision ignored` with a concrete reason for an intentionally rejected
   snapshot. `--project .` selects the root. A receipt applies to exactly one URL,
   branch and path, so ignoring another branch cannot hide declared updates.
4. Commit the root `upstream-state.json` ledger. It records source commit, local
   commit, decision, reason and review time. This ledger is shareable; fetched
   observations and reports belong in cache/artifacts.

Receipts require a clean repository (apart from the review ledger itself) and a
local HEAD matching the report. Multiple receipts can be recorded before
committing the ledger. They
are human assertions, not proof that code was integrated correctly. A receipt
remains valid only while its local commit is an ancestor of current local HEAD
and its source commit is an ancestor of the current source tip. A rewrite or
missing object invalidates it and restores manual review. Changes elsewhere in
a monorepo do not create pending work for a reviewed path. A revert or selective
removal after a receipt still needs normal code review: ancestry cannot establish
that ported behavior continues to exist.

For longstanding subtree integrations, a one-time migration to a source split
history (for example `git subtree split --prefix=...`) can reduce future manual
porting. Establish its prefix, source baseline and destination mapping explicitly;
the auditor never guesses a subtree lineage or merges unrelated full histories.

## Scheduling and validation

`.github/workflows/upstream-status.yml` runs weekly, on manual dispatch, and on
tracker changes. It publishes Markdown and JSON artifacts and a job summary;
incomplete audits or manual findings fail the status check. It does not merge,
push, open issues, or publish messages. Submodule manifest edits become visible
when their commits and parent gitlinks are pushed. Schedule notifications follow
the repository's existing GitHub Actions notification settings.

The same command can be run by cron or another CI system. Persist or archive
`.cache/upstream/observations.json` and the matching cache between runs to detect
branch movement. Keep `upstream-state.json` in version control. Process submodule
integrations from the deepest repositories upward, preserving each reviewed pin.
Finish integration work with examples and tests enabled, relevant module tests,
and then the required project checks before committing/publishing integrations.

Run the tracker tests independently of the simulator build:

```sh
python3 utils/tests/test-upstream-status.py -v
```

They use temporary local Git repositories and require no network access.
