# Validation record

**Study:** quasi-Earth-fixed NTN handover resource allocation (DESIGN.md v5.1).
**Environment:** macOS, Python 3.11.8, simpy 4.1.2, numpy 2.4.6, pydantic 2.14.3,
scipy 1.16, matplotlib 3.10; repository revision `50e9c701b` plus the studied
sources in this folder.
**Test command:** `pytest -q` from this directory (90 tests).
**Experiment command:** `python tools/sweep.py --experiment <E1..E9|C1> --out results --resume`.

This record separates what the model verifies from what it does not. It is a
development-only validation: no physical calibration, no full-stack protocol
conformance, no near-optimality claim at scale.

## 1. Gate results

| Gate | Fixture | What it asserts | Evidence |
|---|---|---|---|
| V1 Geometry | `tests/test_geometry.py` | Footprint radius, edge delay, cell count and cell area against the closed forms and the design table | Pass. 853.47 km, 2.001 / 3.586 ms, 37 and 517 cells, 4367.366 km2. A second, independent formulation agrees to 1e-12. |
| V2 Hop | `tests/test_hop.py` | Group sizes, duty, cycle, phase shift, `next_active`, `advance` | Pass. All four design rows reproduce (7/6, 19/18, 37, 58/57). The mask repeats with the declared cycle and the duty matches over 1,200 slots. |
| V3 Conservation | `tests/test_resources.py` and every episode | Every PRB-symbol has one owner; ledger totals equal the sum of per-UE consumption | Pass in all 369 experiment episodes (`resources.conservation_ok`) and in the fixture. Reservations are atomic and never exceed capacity. |
| V4 Analytic | `tests/test_analytic.py` | Singleton count follows `n*(1-1/M)^(n-1)` | Pass. At n=131 and M=56 the measured mean is 12.6 per occasion over 400 seeds, within 5 percent of the closed form. |
| V5 Boundary | `tests/test_access.py`, `tests/test_engine.py` | Budget equal to completion, attempt limit, occasion boundary, inactive cell, preamble exhaustion, CG and DG deadline, 2-step fallback, policy gap, capability fallback | Pass. 12 fixtures; each one exercises the named branch. |
| V6a Information boundary | `tests/test_engine.py` | Same cohort, different outcome streams, identical `plan.csv` | Pass. `plan.csv` is byte-identical under two outcome seeds; `ue-outcomes.csv` differs. |
| V6b Success-path replay | `tests/test_engine.py` and every episode summary | A first-attempt success whose messages land at the planned slots completes at the predicted instant | Pass. Across 369 episodes: 948,121 first-attempt successes, 930,714 with every message at the planned slot, **0 mismatches**, 17,407 delayed by a control-pool spill (excluded with the reason recorded). |
| V7 Exact | `tests/test_reference.py` | Per-cell enumeration plus frontier combination equals joint enumeration; the reference bounds the allocator | Pass. The decomposition equals a brute force over 3x3x3 candidate sets. On the tiny instance the optimum is `(0.0, 0.0, 520)` and the allocator returns `(0.0, 0.0, 520)` after the G2 tie-break, so the gap is zero on this instance. Before the tie-break the allocator returned 1,064. |
| V8 Determinism | `tests/test_engine.py` | Two runs of one seed produce byte-identical CSV | Pass. All eight artifacts hash equal. |

V4 shows that the model implements the formula, not that the formula matches a
real gNB. V7 shows the reference is right for the declared model, not that the
model is the network. V6b is a consistency check between the planner and the
engine, not an independent protocol trace.

## 2. Model decisions that the design leaves implicit

These were declared before the runs and cost nothing to reverse.

1. **Slot quantization.** Transmissions start at whole slots. A message sent in
   slot `s` is received at `s + tau_ms`; a processing timer starts in the first
   whole slot at or after the arrival and counts active slots of the cell. The
   completion instant keeps the fractional delay. Consequence: the design's
   no-wait expressions are lower bounds, which is how section 6.4 uses them. The
   observed excess over the continuous bound is at most two slots per procedure.
2. **Response spreading.** A RAR or MsgB goes to the first slot of its ten-slot
   window with free control capacity, in both the plan and the engine. Without
   this the plan blocks at 8 RARs per occasion instead of the 80 of section 7.6;
   enabling it moved the cluster default from 676 unassigned UEs to zero.
3. **Preamble split.** Contention draws use the 56-preamble contention pool, not
   the 64-preamble total (see N1).
4. **Swap scope.** An option's timeline depends on its own UE's propagation
   delay, so two options are not interchangeable objects. The swap exchanges the
   two start slots and keeps each UE's access type (see G3).
5. **CG offset** `(ue*5 + 3) mod P_cg`; **hotspot** weight split as declared in
   README.md; **CFRA retry** uses the contention pool; **DG** has one grant and no
   retry after a decode failure.
6. **Stage-3 cost** `PRB-symbols + 12*(CCEs + preambles + RAR grants + DG grants)`.
7. **Exact instance** ring-1 cluster (7 cells), three UEs per cell for V7 and two
   per cell for the joint-vs-decomposition check.
8. **Tie-break.** On an exact score tie the allocator takes the cheaper option,
   then the earlier predicted completion, then the enumeration order. DESIGN.md
   9.6 leaves the tie to the enumeration order; without this rule the access mix
   depends on the order of `ACCESS_TYPES` (G2).

## 3. Findings against the design

**G1 - `mu` is inert.** `mu*(1-pressure[u])` is constant across a UE's option
set, so it cannot change an option choice. `C1/mu-0` and `C1/mu-2` produce
identical plans and identical KPIs (violation 0.0154 in both, 3 seeds). The
stage-2 signal exists only in the swap's lexicographic test. Decision: the term
stays as declared, with a comment in `allocator.score` that records the reason,
so the configured score and the design agree. Moving it to the ordering key or
the acceptance rule remains a design revision, not a code fix.

**G2 - stage 3 was unreachable from the greedy.** *(Tie fixed; the cost term is
still absent.)* The score has no cost term, and `score < best - 1e-12` kept the
first enumerated option on a tie. At low utilisation every price is zero, so the
first option won: before the fix all 14 tiny-instance UEs took 4-step and the
cost was 1,064 against an optimum of 520. At the cluster default the mix was
always sensible (P1 UEs take the mandatory 4-step, CG-capable UEs take CG), and
the measured total is within 4.5 percent of the per-UE cheapest-option bound
(239,352 against 229,112, seed 1). Status: an exact tie now takes
`(direct_cost, predicted_completion, enumeration index)`, declared in section 2
item 8. The tiny instance reaches 520, the exact optimum; the cluster headline is
unchanged. Stage 3 remains *reported, not optimized* (decision D): the score
still has no cost term, so `epsilon*direct_cost` remains an option if the claim
must hold at low load.

**G3 - the swap pass is inert at the headline point.** A start-slot exchange
leaves the access type - and therefore the stage-3 cost - unchanged, and it
rarely changes a violation, because no planned UE violates at the default
operating point. `C1/swap-0` and `C1/swap-500` give identical summaries, plans and
costs (239,148). The tiny-instance gap in V7 survives the swap for the same
reason. Fix: let the swap exchange access types and rebuild both timelines, or
drop the swap from the claim.

**N1 - singleton arithmetic uses the wrong pool.** *(Resolved in design v5.1 item 34.)*
Section 7.6 evaluated `131*(63/64)^130 = 16.9` with the 64-preamble total, while section
8.2 draws from the 56-preamble contention pool. The section now defines
`M = preamble_total - preamble_dedicated` and gives 12.6 and 5.5. The code and
`tests/test_analytic.py` always used the pool; the fixture now derives it from the
configuration instead of naming a constant. Kept for the record.

**N2 - the full profile's "outer cell-center radius".** The table reports
853.47 km, which is the footprint radius `R_f`; the outermost lattice centre is
at 852.17 km, and the outermost cell edge at about 893 km. The elevation and
delay in the row are evaluated at `R_f`, so they remain correct for the edge.

**N3 - RAR and CR timeouts are unreachable.** Once processing timers count
active slots, the 2-slot detection always fits the 10-slot RAR window and the
8-slot CR timer is the scheduling offset itself. The only attempt failures are a
preamble collision, a missed detection, an MsgA payload failure, a control-pool
spill and the attempt limit. The design lists RAR and CR timeouts as failure
causes; the implementation keeps the checks but no fixture can reach them.

**N4 - the CG sync-error control has no outcome effect at 1 percent.** The path
fires (12, 13 and 16 sync errors in the three seeds at p=0.01, and 73 retries at
p=0.05 in the probe), and the retry at the next CG occasion recovers every UE
within the declared 10 attempts, so the violation rate is unchanged (0.0154) and
only the CG waste (144-192 PRB-symbols) moves. A sensitivity claim needs a
tighter deadline or a larger probability.

## 4. Limits

- The model is an abstract event model with declared timers. It is not a
  conformance test and is not calibrated to a gNB or a satellite link.
- The cluster is a near-nadir instance: its delay range is 2.001-2.19 ms, so the
  edge geometry of the full footprint is exercised only by E8.
- Sweep points use three seeds; the E1 headline uses thirty. A three-seed
  interval that crosses zero (E2 density 10) is reported as computed and is not
  a claim.
- E3 at `C_max = 37` makes the hop schedule irrelevant (`G = 1`); the five dwell
  rows are one configuration and are labelled as such.
- The full profile is a scale run, not a second mechanism instance: 22 UEs per
  cell do not create contention, so its 20.5 percent violation rate is the beam
  wait against tight budgets, not a contention result.
- No CP-SAT reference is implemented. Section 9.5 lists it as a reference class;
  V7 is satisfied by the exhaustive per-cell reference and the lower bound.

## 5. What would close the open items

1. Decide G1 and G3 (G2's tie-break is applied and its tiny gap is closed): give
   `mu` a real lever (ordering or acceptance) and either exchange access types in
   the swap or drop it from the claim. Re-run E1 and C1 and require the swap to
   change something.
2. Correct N2 in the full-profile caption (N1 is fixed in v5.1 item 34).
3. State in section 8.2 that RAR and CR timeouts cannot occur under active-slot
   timers, or restore wall-clock windows and re-measure the pause cases.
4. Add a CG sensitivity point that can bind (deadline below the first retry or
   sync probability at 0.1) before any claim about CG reliability.
