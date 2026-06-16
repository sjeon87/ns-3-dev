# TRIAGE.md

A rubric and procedure for AI coding agents that draft reviews of submissions to
ns-3-dev for human maintainers.

## Purpose

This document guides a coding agent through reviewing a submission to the ns-3
project — either a **Work Item** (issue / bug report / enhancement request) or a
**Merge Request** (MR) on GitLab.com — and producing a structured, draft review
that a human maintainer can read, edit, and act upon.

The agent's job is to **assist a maintainer's triage**, not to replace it. The
output of this process is always a *draft* that a human reviews before any of it
reaches the submitter or the project's digital spaces.

## Guardrails (read first)

These constraints come from the ns-3 AI tool policy
(`doc/contributing/source/ai-policy.rst`) and are non-negotiable.

1. **Human in the loop.** The agent produces a draft review only. It **must not**
   post comments, apply labels, approve, merge, close, or otherwise take any
   action in GitLab (or the mailing lists, forum, or any project space) without
   explicit, per-action human approval. Drafting is allowed; acting is not.
2. **"Good first issue" is off-limits.** If the Work Item or the MR's linked
   issue carries the `good first issue` label, the agent must **stop** and report
   that the item is reserved for human contributors as a learning opportunity.
   Do not draft a fix, and do not draft a review that does the contributor's
   thinking for them.
3. **License safety.** If the submission proposes or integrates code under a
   license incompatible with **GNU GPLv2-only** (e.g., GPLv3, Apache-2.0), the
   agent must surface this prominently and early as a blocking concern.
4. **Be constructive and welcoming.** Reviews follow the spirit of the project's
   Code of Conduct. Assume good faith, especially with new contributors. Frame
   findings as specific, actionable, and respectful. The goal is to help
   contributors learn and grow, not to gatekeep.
5. **No fabrication.** If the agent cannot verify a claim (e.g., it cannot
   reproduce a bug, cannot build the branch, or lacks context on a module),
   it must say so explicitly rather than guess. Mark unverifiable items as
   *open questions* for the maintainer.

## Inputs

To begin a triage, the agent should be given one of:

- a Work Item (issue) URL or number;
- a Merge Request URL or number; or
- a patch / diff plus a description of what it is intended to do.

Before applying the rubric, the agent should gather context:

- the full diff or issue text;
- the affected module(s), and the maintainer(s) of those modules
  (see <https://www.nsnam.org/about/governance/maintainers/>);
- the submission's labels (especially `good first issue`);
- any linked issues, prior MRs, or related discussion;
- whether the submission builds and whether tests pass, if the agent can
  determine this locally.

---

## Common criteria (both Work Items and Merge Requests)

### C1. Licensing and copyright

- Does each new source file carry an SPDX license identifier, and is it
  GPLv2-only compatible? GPLv3 and Apache-2.0 are **not** acceptable in the
  mainline.
- If code is borrowed from elsewhere, is the original copyright and license
  retained and attributed? Is the borrowed portion clearly delineated?
- Are copyright/attribution additions proportionate to the contribution
  (substantial change ~30%+ may justify a header addition; small fixes should
  not add author/copyright lines)?
- Reference: `doc/contributing/source/general.rst` (Licensing, Copyright,
  Attribution).

### C2. AI-policy compliance and disclosure

- Does the submission appear to contain substantial tool-generated content that
  is **not** disclosed (no `Assisted-by:` trailer or MR-description note)? If so,
  note it — disclosure is expected for substantial generated code, tests, ported
  implementations, or documentation.
- Does the submission read as an **extractive contribution** — i.e., apparently
  unreviewed LLM output that shifts the burden of design and review onto
  maintainers? If so, flag it for the maintainer's judgment (this is the
  maintainer's call, not the agent's).
- Reference: `doc/contributing/source/ai-policy.rst`.

### C3. Scope and value

- Is the change appropriately scoped and self-contained? Could its size or
  complexity be reduced, or its usefulness to the community increased?
- Does the submission belong in ns-3 mainline, or is it better suited to a
  contributed module, the user's own fork, or a forum/mailing-list discussion?

### C4. Tone and conduct of the draft review

- Is the drafted feedback specific, actionable, and respectful?
- Does it explain *why*, so the contributor learns, rather than just *what* to
  change?

---

## Work Item (issue) review rubric

### W1. Classification

Determine what kind of item this is, and whether it is even a bug:

- **Confirmed bug** — reproducible defect in ns-3 behavior.
- **Cannot reproduce / needs more info** — missing version/commit, platform,
  configuration, or a minimal reproducing example.
- **Not a bug** — expected behavior, user error, or a usage question better
  directed to the ns-3 users mailing list or forum.
- **Enhancement / feature request** — not a defect; assess on merit and scope.
- **Duplicate** — already tracked or already fixed (check `RELEASE_NOTES.md`
  and open issues).
- **Security vulnerability** — if the report concerns a security issue, note
  that it may warrant a private disclosure path rather than public triage, and
  defer to the maintainer.

### W2. Clarity and completeness

- Is the problem stated clearly and unambiguously?
- For a bug report, is there enough to reproduce: ns-3 version or commit hash,
  OS/compiler, build configuration, the program/script, and the observed vs.
  expected behavior?
- If a minimal reproducing example is missing, recommend requesting one.

### W3. Confirmation

- Can the agent reproduce the reported behavior? State explicitly whether
  reproduction was attempted and what the result was.
- If reproduced, identify the likely root cause and the affected module(s)/
  file(s).

### W4. Recommended solution and test approach

- If confirmed, outline one or more candidate fixes and the tradeoffs between
  them.
- Recommend a test approach that would both demonstrate the fix and guard
  against regression (unit test, system/example test, or a new test suite).
- Estimate severity (see taxonomy below) and the module(s) and maintainer(s)
  who should be routed the item.

---

## Merge Request review rubric

### M1. Intent and description

- Is it clear what the MR accomplishes — new feature, bug fix, refactoring, or
  documentation?
- Did the contributor write a description covering motivation, approach,
  expected impact, and open questions (as the AI policy asks)?
- If the MR fixes an issue, is the issue linked (e.g., `fixes #issue`)?

### M2. Design and alternatives

- What alternative approaches exist, and what are the tradeoffs (complexity,
  performance, maintainability, API surface, backward compatibility)?
- Is the chosen approach consistent with ns-3 architecture and existing
  patterns (attribute system, callbacks, `Ptr<>` smart pointers, tracing,
  logging)?

### M3. Correctness and testing

- Are there unit and/or system tests that demonstrate correctness of the new or
  modified code?
- Will the tests catch future regressions? If coverage is insufficient, suggest
  specific tests to add.
- Does the branch build with `--enable-examples --enable-tests`, and do tests
  pass? State whether this was verified.

### M4. API considerations

- Are any public APIs added, changed, or removed? If changed or removed, is the
  ns-3 deprecation process followed (deprecate, document in `RELEASE_NOTES.md`,
  remove only after a release cycle)?
- Are new attributes, trace sources, and `TypeId`s named and documented
  consistently?
- Are there backward-compatibility implications for existing users' scripts?

### M5. Documentation

- Are Sphinx model/manual docs updated where applicable
  (`doc/models/source/`, `doc/manual/source/`)?
- Is `RELEASE_NOTES.md` updated for new features and notable behavior changes,
  and `CHANGES.md` for API changes, as appropriate?
- Is Doxygen coverage complete for new classes, methods, and members
  (per `doc/contributing/source/coding-style.rst`)?

### M6. Code style and hygiene

- Does the code conform to `.clang-format` and the coding style guide
  (`check-style-clang-format.py` and `doc/contributing/source/coding-style.rst`)?
- Are logging severity levels and `NS_LOG_FUNCTION` usage appropriate
  (`doc/manual/source/logging-asserts.rst`)?
- **Commit hygiene:** present-tense imperative messages, module prefixes,
  72-char first line, `(fixes #issue)` where applicable; does each commit
  build and test independently (compile-or-die)?
- **Refactor-or-die:** is refactoring separated from functional change rather
  than mixed into the same commits
  (`doc/contributing/source/best-practices.rst`)?
- Does the branch contain only relevant changes (no stray `.vscode/`,
  build artifacts, or unrelated edits)?

---

## Severity classification for findings

Use these labels on individual findings so maintainers can prioritize:

- **Blocker** — must be resolved before merge (e.g., license incompatibility,
  build break, failing tests, correctness defect).
- **Major** — significant concern (e.g., missing tests for new behavior,
  unaddressed API break, no documentation for a new feature).
- **Minor** — should be addressed but not merge-blocking (e.g., style, naming,
  incomplete Doxygen).
- **Nit** — optional polish.
- **Question** — needs clarification from the contributor or maintainer; the
  agent could not determine the answer.

## Recommendation taxonomy

End every review with a single overall recommendation.

**Merge Requests:**

- **Approve** — ready to merge (subject to maintainer sign-off).
- **Approve with minor comments** — merge after small fixes; no re-review
  needed.
- **Request changes** — substantive changes needed; re-review expected.
- **Needs design discussion** — approach should be discussed before further
  work.
- **Out of scope** — not appropriate for mainline as proposed.
- **Possibly extractive** — may not comply with the AI policy; flag for
  maintainer judgment.

**Work Items:**

- **Confirmed — actionable** (with recommended fix and test approach).
- **Needs more information** (specify what to request).
- **Cannot reproduce.**
- **Not a bug / redirect to forum or mailing list.**
- **Duplicate** (link the original).
- **Enhancement — for maintainer prioritization.**

## Suggested GitLab labels

The review should recommend which labels to affix — but, per the guardrails,
the agent **proposes** labels for the maintainer to apply; it does not apply
them itself.

- Propose labels **only from the project's existing label set** (see the
  "ns-3-dev label set" reference below); do not invent new ones. If a useful
  label does not exist, suggest creating it as an open question for the
  maintainer.
- Labels typically span a few axes — for example a *type* (bug, feature,
  documentation, refactoring), an affected *module/component*, a *status*
  (e.g., needs more information, needs review), and policy labels
  (`good first issue`).
- Map the review outcome to labels where there is a natural correspondence —
  e.g., a *Cannot reproduce* / *Needs more information* recommendation suggests
  a "needs more information" label; a *Possibly extractive* recommendation
  suggests flagging `extractive` for maintainer judgment.
- Never propose `good first issue` for an item the agent has reviewed or drafted
  against; that label marks work reserved for humans.

## ns-3-dev label set (reference)

This is the set of labels defined on the ns-3-dev project as of 2026-06-16.
GitLab does not expose this list to the agent automatically, so it is captured
here as a convenience; **treat it as a snapshot and verify against the project's
current labels when possible.** Propose only labels from this set.

Several labels are *scoped*: GitLab renders them as `key::value`, and only one
value per scope applies to an item at a time.

- **`bug::` (severity)** — `critical` (compilation failure, no workaround),
  `high-priority` (compilation failure with a known workaround),
  `medium-priority` (wrong behaviour leading to incorrect results),
  `low-priority`.
- **`feature::` (priority)** — `high_priority` (most needed).
- **`status::`** — `unconfirmed` (not yet confirmed by a maintainer),
  `confirmed` (confirmed by a maintainer), `needinfo` (reporter must provide
  more information), `needsreview` (maintainers must review and suggest next
  steps, or approve/merge), `needsupdate` (submitter must update the patch per
  review comments or rebase), `patchpending` (an MR to fix the issue is under
  review), `patchwanted` (maintainers are requesting a patch), `blocked`
  (blocked by another issue), `reopened` (previously closed, now reopened).
- **`resolution::`** — `answered`, `fixed` (issue/MR has been fixed), `merged`
  (MR was merged), `duplicate` (duplicate issue/MR), `moved` (superseded by
  another), `wontfix` (maintainers decided not to fix or merge), `worksforme`
  (cannot reproduce or no longer relevant), `lack of interest`.
- **`module::` (affected component)** — antenna, aodv, applications, bridge,
  brite, buildings, click, config-store, core, csma, dsr, energy, fd-net-device,
  flow-monitor, internet, internet-apps, internet:tcp, lr-wpan, lte, mesh,
  mobility, mpi, netanim, network, new-module, nix-vector-routing, olsr,
  openflow, point-to-point, point-to-point-layout, propagation, sixlowpan,
  spectrum, stats, tap-bridge, topology-read, traffic-control, uan, visualizer,
  wave, wifi, wimax, zigbee.
- **Other (unscoped)** — `incident`, `build system`, `coding style`,
  `documentation`, `don't merge` (MRs not intended to be merged), `examples`,
  `feature request`, `Gitlab CI`, `good first issue`, `gsoc`, `infrastructure`
  (GitLab infrastructure), `performance`, `portability`, `project ideas`,
  `python bindings`, `tests`, `third-party`, `utils`.

---

## Output format

The agent should produce a draft review in this structure. This draft is itself
the review the maintainer will read, edit, and — once satisfied — post publicly
as an AI-assisted review on the MR or Work Item. Because the review is the
artifact that gets posted to the thread, do **not** add a separate "suggested
reply to the contributor" section; write the findings and recommendation so they
can be read directly by the contributor. (The "Suggested labels" and "Open
questions for the maintainer" sections remain maintainer-facing notes that the
maintainer may trim before posting.) It is **not** to be posted automatically; a
human reviews it first.

The agent must **identify itself** in the header — name the tool/model that
produced the review (e.g., "Claude (model name)") — and include the **calendar
date** (`YYYY-MM-DD`) on which the review was drafted, so the posted review is
transparent about its origin and currency.

```markdown
# Draft review: <MR/issue title> (<#number>)

**Type:** Merge Request | Work Item
**Reviewed by:** <agent/model identity, e.g., Claude (model name)>  **Date:** <YYYY-MM-DD>
**Module(s):** <affected modules>  **Maintainer(s):** <names>
**Labels of note:** <e.g., good first issue>

## Summary
<2-4 sentences: what this is and the headline assessment.>

## Guardrail checks
- License (GPLv2-only compatible): <pass / concern>
- good first issue: <yes/no — if yes, STOP>
- AI-policy disclosure / extractiveness: <observation>
- Build & tests verified locally: <yes/no + result>

## Findings
<one numbered item per finding, each using the format below>
1. [Blocker|Major|Minor|Nit|Question] <file:line> — <finding and why; suggested fix>
2. ...

## Recommendation
<one item from the taxonomy, with a one-paragraph rationale>

## Suggested labels (for maintainer to apply)
<labels from the project's existing set, with a short justification for each one>

## Open questions for the maintainer
- <items the agent could not verify>
```

## What the agent must NOT do

- Post, comment, label, approve, merge, or close anything in GitLab or any other
  project space without explicit per-action human approval.
- Draft a fix or a leading review for a `good first issue`.
- Present unverified claims as fact; mark them as open questions instead.
- Use any GitLab Ultimate / built-in AI automation in a mode that would publish
  output without a human reviewing it first.

---

## Appendix: existing GitLab tooling that may assist this triage

This section is **not** intended to be used by agents as part of the triage
process; it is merely some notes for maintainers, and will probably be removed
in a future revision of this document.

GitLab markets its AI features under the **GitLab Duo** umbrella, several of
which require the Ultimate tier and/or a Duo add-on. The features below were
known to exist as of this writing; availability, naming, and tier change
frequently, so **verify current status on the project's actual instance before
relying on any of them.** They are listed here as candidates a maintainer could
fold into the triage flow — not as endorsed or vetted tools.

Features that read/summarize (lower risk; output is consumed by a human):

- **Merge request summary** — Duo-generated summary of an MR's changes; useful
  as a starting point for the "Intent and description" (M1) check.
- **Code review summary** — a per-MR overview highlighting areas of change.
- **Discussion/issue summary** — condenses long comment threads on an issue or
  MR, helpful for duplicate detection and understanding prior context.
- **GitLab Duo Chat** — interactive Q&A about a file, diff, or MR; can help an
  agent or maintainer orient quickly.
- **Root cause analysis** — explains a failed CI/CD job, relevant to the
  build-and-test verification (M3).
- **Vulnerability explanation / summary** — for security-related Work Items
  (W1), though these should still follow a private disclosure path.
- **Suggested Reviewers** — an Ultimate (ML-based, not necessarily LLM) feature
  that proposes reviewers; can corroborate maintainer routing.

Features that generate, or can publish, content (higher risk under the AI policy — must keep a human in the loop):

- **GitLab Duo Code Review** — Duo can be added as a reviewer (e.g., via a
  `@GitLabDuo`/`/duo` quick action) and post review comments. Because this can
  publish comments into the MR, it is exactly the kind of automation the AI
  policy restricts: acceptable only in an **opt-in mode where a human reviews
  before anything is posted**, never as an autonomous reviewer.
- **Code Suggestions / test generation** — authoring aids; out of scope for a
  *review* but relevant if a contributor used them (disclosure under C2).

Recommendation: of the above, the **read/summarize** features are the natural
fit for assisting a first-pass triage, since their outputs are read by maintainers.
Any feature capable of posting to GitLab must be configured so that a maintainer
reviews and approves the output before it reaches the contributor.

Note: this list reflects the agent's general knowledge and is not a substitute
for checking what is actually licensed and enabled on the ns-3-dev project.
