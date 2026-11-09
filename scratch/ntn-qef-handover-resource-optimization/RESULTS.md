# Experiment results

**Design:** DESIGN.md v5.1. **Validation:** VALIDATION.md (gates and findings).
**Runs:** 60 points, 369 episodes, 1,818,900 UE-episodes, 1,207,504 successes;
`results/report.md` holds the raw tables and `results/figures/` the figures.

These tables come from the run with the G2 tie-break applied (`results/`). The
pre-fix run is kept in `results-prefix-tiebreak/`. The E1 headline is unchanged
to four decimals; other points move by at most 0.001, except the low-load
medians and the full-profile cost.

Setup unless stated otherwise: cluster of 37 cells, 131 or 132 UEs per cell
(4,848 UEs), `C_max = 19`, dwell 20 ms, 15 MHz, density 30 persons/km2, joint
access menu, 30 paired seeds. Every rate uses the generated cohort as the
denominator; an unassigned UE counts as a failure and a violation.

## E1 - which access type wins (30 seeds)

| point | success | budget violation (95% CI) | median ms | p95 ms |
|---|---|---|---|---|
| joint | 1.0000 | 0.0159 [0.0154, 0.0164] | 53.4 | 127.1 |
| 4-step CBRA only | 0.7921 | 0.2834 [0.2819, 0.2850] | 107.4 | 155.9 |
| 2-step CBRA only | 0.5015 | 0.5252 [0.5226, 0.5279] | 59.3 | 132.3 |
| CG only | 0.2495 | 0.7505 [0.7484, 0.7527] | 25.4 | 67.2 |
| DG only | 0.1827 | 0.8173 [0.8171, 0.8175] | 14.1 | 33.1 |
| 4-step CFRA only | 0.1832 | 0.8435 [0.8430, 0.8441] | 87.0 | 107.1 |
| 2-step CFRA only | 0.1832 | 0.8172 [0.8171, 0.8173] | 48.0 | 68.1 |

Paired against joint, every arm is worse with a 95 percent interval that
excludes zero: 4-step +0.2676 [+0.2660, +0.2691], 2-step +0.5093, CG +0.7347,
DG +0.8014, CFRA +0.828.

The strict arms are a *policy* comparison: a UE without the type is unassigned
and counted. Five of the six types are optional, so the headline mixes capability
share with access performance. The eligible-only secondary analysis (3 seeds)
separates them:

| arm | all-cohort violation | eligible-only violation | eligible success | eligible share |
|---|---:|---:|---:|---:|
| joint | 0.0153 | 0.0153 | 1.0000 | 1.000 |
| 4-step CBRA | 0.2798 | 0.2798 | 0.7921 | 1.000 |
| 2-step CBRA | 0.5249 | 0.0529 | 1.0000 | 0.502 |
| CG | 0.7494 | 0.0000 | 1.0000 | 0.251 |
| DG | 0.8183 | 0.2390 | 0.7610 | 0.239 |
| 4-step CFRA | 0.8435 | 0.8435 | 0.1832 | 1.000 |
| 2-step CFRA | 0.8170 | 0.6352 | 0.3652 | 0.502 |
| 4-step family | 0.2544 | 0.2544 | 0.7921 | 1.000 |
| 2-step family | 0.5089 | 0.0210 | 1.0000 | 0.502 |
| CBRA family | 0.1157 | 0.1157 | 0.9909 | 1.000 |
| CFRA family | 0.8184 | 0.8184 | 0.1832 | 1.000 |
| RACH-less family | 0.5762 | 0.0424 | 0.9576 | 0.443 |

Reading:

- The mix is worth 26.8 points against the 3GPP baseline (4-step CBRA, the only
  type every UE must support). That is H1: a single access rule is far from the
  joint allocation.
- CG is the best single type for the UEs that have it (0.0 percent eligible
  violation, 25.4 ms median). Every CG-capable UE takes CG under the joint menu,
  so CG is the default whenever a UE supports it.
- CFRA alone strands UEs: 8 dedicated preambles per occasion cap the arm at 18
  percent success even though every UE is CFRA-capable. Dedicated access lowers
  completion but the pool is the scarce resource, not the capability.
- DG alone is worse than 2-step CBRA (eligible 23.9 vs 5.3 percent) because the
  pruned option set offers only three grant slots and the DG grant pool is 8 per
  slot; UEs beyond that are unassigned.
- Joint mix: 30 percent 4-step CBRA (equal to the P1 share), 25 percent CG, 16
  percent DG, 18 percent 2-step CBRA, 10 percent CFRA.

## E2 - load sweep (3 seeds)

| density | success | violation (95% CI) | median ms | p95 ms |
|---|---|---|---|---|
| 2 | 1.0000 | 0.0000 [0.0000, 0.0000] | 33.8 | 98.0 |
| 10 | 1.0000 | 0.0045 [0.0022, 0.0069] | 38.1 | 107.1 |
| 30 | 1.0000 | 0.0153 [0.0125, 0.0182] | 54.0 | 127.1 |
| 100 | 0.7441 | 0.3278 [0.3234, 0.3323] | 67.3 | 198.4 |

The curve is flat to density 10 and rises steeply between 30 (131 UEs/cell) and
100 (437 UEs/cell). At density 100 one quarter of UEs exhaust the 10 attempts.
The lever is the pruned three-start option set: `C1/options-1` and
`C1/options-5` move the same point from 0.2230 to 0.0074. The design's
contention-versus-control crossover is not reached on the cluster: the control
pools are spread over the ten-slot window and do not bind before the preamble
pool does.

## E3 - hop sweep (3 seeds, horizon = K_min per point)

| point | success | violation | median ms | p95 ms |
|---|---|---|---|---|
| C_max=7, dwell 5 | 0.4222 | 0.5778 | 50.1 | 138.1 |
| C_max=7, dwell 10 | 1.0000 | 0.1274 | 135.7 | 344.1 |
| C_max=7, dwell 20 | 0.9591 | 0.1628 | 134.0 | 345.4 |
| C_max=7, dwell 50 | 0.7767 | 0.2560 | 138.1 | 337.1 |
| C_max=7, dwell 100 | 0.5436 | 0.4564 | 128.1 | 507.1 |
| C_max=19, dwell 5 | 0.7216 | 0.2786 | 30.4 | 84.1 |
| C_max=19, dwell 10 | 1.0000 | 0.0152 | 48.1 | 124.1 |
| C_max=19, dwell 20 | 1.0000 | 0.0153 | 54.0 | 127.1 |
| C_max=19, dwell 50 | 1.0000 | 0.0146 | 61.1 | 157.0 |
| C_max=19, dwell 100 | 0.9020 | 0.0980 | 61.1 | 147.4 |
| C_max=37, any dwell | 1.0000 | 0.0000 | 28.8 | 67.7 |

- A 5 ms window is below every RACH procedure span: only RACH-less access can
  finish inside one window. At C_max=7, dwell 5 the mix collapses to CG and DG
  (2,039 of 2,077 capable UEs) and 1,692 UEs are unassigned; P1 and P2 UEs
  violate at 100 percent. This is the hard floor of H3, not contention.
- Dwell 10 beats dwell 20 at C_max=7 (12.7 vs 16.3 percent) although the duty is
  the same (1/6). The cycle is 60 ms against 120 ms, so the wait floor halves;
  the 19-slot procedure spans more windows, and the shorter cycle still wins.
  This non-monotonic pair survives three seeds and is reported as a finding, not
  smoothed.
- At C_max=37 the group count is one, so the schedule is always on and the dwell
  has no meaning; the five rows are one configuration and are labelled as such.
- All points run at their own K_min (up to 6,010 slots at C_max=7, dwell 100)
  and censored is 0 everywhere. The design's warning that those points need more
  than 4,000 slots is confirmed.

## E4 - budget shares (3 seeds)

| shares | success | violation | median ms |
|---|---|---|---|
| loose-heavy | 1.0000 | 0.0063 | 54.0 |
| default | 1.0000 | 0.0153 | 54.0 |
| tight-heavy | 1.0000 | 0.0314 | 54.1 |

The violation rate follows the tight share almost linearly (35, 20, 10 percent of
UEs at 100 ms give 3.1, 1.5, 0.6 percent violations) while the access mix and the
median completion barely move. The allocator does not re-mix under budget
pressure: the tie-break now prices resources on an exact score tie, but stage 3
still does not enter the score, so a budget change cannot make the policy prefer
a cheaper mix.

## E5 - dedicated preambles (3 seeds)

| dedicated | success | violation | median ms |
|---|---|---|---|
| 0 | 1.0000 | 0.0299 | 54.1 |
| 8 | 1.0000 | 0.0153 | 54.0 |
| 16 | 1.0000 | 0.0084 | 49.4 |
| 32 | 1.0000 | 0.0047 | 48.1 |

Monotone: each doubling of the dedicated pool roughly halves the violation rate
as CFRA substitutes for contention. The median also falls, because a CFRA UE
skips the collision risk rather than retrying. Preamble scarcity binds, and it
binds on the *split* between the two pools, not on the 64 total.

## E6 - bandwidth (3 seeds)

| MHz | PRB | success | violation | median ms |
|---|---|---|---|---|
| 5 | 27 | 1.0000 | 0.0127 | 54.1 |
| 15 | 83 | 1.0000 | 0.0153 | 54.0 |
| 30 | 166 | 1.0000 | 0.0164 | 54.1 |

Flat within three seeds. At 131 UEs per cell the PRB grid is not the binding
resource: the delay comes from the hop wait and the access procedure, and the
scarce pools are preambles and control. A bandwidth claim needs a load point
where PUSCH saturates, which E2's density 100 does not reach either.

## E7 - capability-budget correlation (3 seeds)

| correlation | success | violation | median ms |
|---|---|---|---|
| independent | 1.0000 | 0.0153 | 54.0 |
| advanced-on-tight | 1.0000 | 0.0127 | 48.4 |

Putting advanced capabilities on tight-budget UEs lowers the violation rate by
0.26 points and the median by 5.6 ms, because the tight UEs then have CG/DG/2-step
available. The effect is small: fairness claims need the low-support strata of
E7 to be large, and 3 seeds cannot support a stratum claim.

## E8 - full profile (3 seeds)

| success | violation (95% CI) | median ms | p95 ms | cost |
|---|---|---|---|---|
| 0.9057 | 0.2046 [0.1966, 0.2126] | 151.8 | 421.8 | 490,212 |

By budget class (one probe episode): tight 60.3 percent violation and 52.3
percent success, short 27.6 percent violation with 100 percent success, medium
0.5 percent, loose 0.0. By capability: P1 46.4 percent, P2-P7 between 8 and 10
percent. 9.3 percent of UEs are unassigned at plan time.

The full footprint has 22 UEs per cell, so contention is not the mechanism. With
`C_max = 64` the group count is 9 and the cycle is 180 ms, so a cell is served 20
ms in every 180 ms; a 100 ms budget cannot survive the wait for most cells, and a
200 ms budget survives it but not the procedure. This is H3 at scale. The P1
excess is the mandatory 4-step path meeting the same wait with a 19-slot
procedure.

## E9 - sensitivity (3 seeds)

| point | success | violation | note |
|---|---|---|---|
| detection 1.00 | 1.0000 | 0.0153 | baseline |
| detection 0.99 | 1.0000 | 0.0162 | +0.09 pt |
| detection 0.90 | 1.0000 | 0.0232 | +0.79 pt |
| MsgA 0.99 | 1.0000 | 0.0153 | baseline |
| MsgA 0.95 | 1.0000 | 0.0167 | +0.14 pt |
| DG decode 0.00 | 1.0000 | 0.0153 | baseline |
| DG decode 0.01 | 0.9981 | 0.0172 | 8-11 failures per episode, no DG retry |
| sync off | 1.0000 | 0.0153 | baseline |
| sync 0.005 | 1.0000 | 0.0153 | 4-7 sync errors, all recovered |
| sync 0.01 | 1.0000 | 0.0153 | 12-16 sync errors, 144-192 PRB-symbols wasted |
| fallback off | 1.0000 | 0.0069 | less than half the baseline |

The CG sync path fires and the next CG occasion recovers it, so the KPI does not
move while the waste does (N4). The DG decode failure is terminal by design and
costs 0.19 points. Disabling the 2-step fallback *halves* the violation rate: a
2-step retry at the next occasion completes in about 8-11 slots, while the 4-step
fallback needs a RAR, a Msg3 and a CR, roughly 28-37 slots. This is a reversal of
H2 at the default point - the fallback helps success but costs the delay budget.

## C1 - control sensitivity (3 seeds)

| control | violation | reading |
|---|---|---|
| mu 0 / mu 2 | 0.0153 / 0.0153 | inert (G1) |
| lambda 0.1 / 10 | 0.0153 / 0.0153 | inert at this point: no predicted violation |
| decision latency 0 | 0.0091 | an earlier decision helps |
| decision latency 5 | 0.0153 | same as 1 ms at this load |
| options 1 | 0.2230 | pruning depth is the dominant lever |
| options 5 | 0.0074 | 5 starts per type nearly removes the gap |
| swap 0 / 500 | 0.0153 / 0.0153 | inert (G3); cost identical at 239,148, 0 of 2,500 candidates accepted |

The swap result deserves one line. With the access-type exchange at the fixed
start, 4,950 pairs are tested per episode and 2,500 of them have both lookups
present, yet none improves the tuple. That is structural, not an implementation
gap: a pairwise exchange preserves the pair's cost multiset, so stage 3 cannot
move, and the greedy already assigns in slack order, so the fast types are
already on the tight UEs. The pass is a repair pass for stages 1-2 and is empty
at every tested point. A stage-3 improvement needs a *move* (one UE to another of
its own options, including the unassigned), not a swap.

## Cost, after the tie-break

Stage 3 is the sum over assigned UEs of the option cost (1 per PRB-symbol, 12 per
control unit): CG 12, DG 36, 2-step 56, 4-step 76.

| point | total | per UE |
|---|---:|---:|
| cluster joint, 30 seeds | 238,793 | 49.26 |
| full profile, 3 seeds | 490,212 | 43.42 |
| tiny instance, allocator | 520 | 37.14 |
| tiny instance, exact reference | 520 | 37.14 |

The tie-break reaches the exact reference cost on the tiny instance, where the
pre-fix run took 1,064. At the cluster point the total barely moves (239,352 for
seed 1, before and after) because the congestion price already separated the
options; the seed-1 per-UE cheapest-option lower bound is 229,112, so stage 3 is
within 4.5 percent of a bound that ignores capacity. Stage 3 is reported, not
optimized: the score still has no cost term, and the swap cannot change an access
type.

## Answers to the research questions

1. **Which access type per UE?** CG whenever it is available (0.0 percent
   eligible violation), 2-step for the rest of the 2-step-capable UEs, CFRA where
   the dedicated pool allows, and 4-step only for the UEs that have nothing else.
   The joint mix cuts the violation rate from 28.3 to 1.6 percent against the
   4-step baseline.
2. **How much does joint selection gain?** 26.8 points against the 3GPP
   baseline, 10.1 points against the best family arm (CBRA family), and 1.3
   points against the RACH-less family on eligible UEs. The gain is not uniform:
   it is concentrated in P1 UEs, which the mix cannot help at all.
3. **How do `C_max` and dwell change the answer?** They set a floor. Below the
   procedure span (5 ms) only RACH-less access survives; at `C_max = 19` the
   answer is flat from 10 to 50 ms and degrades at 100 ms; at `C_max = 37` the
   schedule is irrelevant. The C_max=7 pair at 10 and 20 ms is non-monotonic, and
   the cycle length explains it.
4. **When does contention bind?** Above roughly 130 UEs per cell with a 56
   preamble pool and three-start pruning; at 437 UEs per cell it costs a quarter
   of the cohort. Control pools do not bind before the preamble pool. PRB
   capacity does not bind at any tested point.
5. **How close is the allocator to the exact reference?** On the tiny instance it
   matches all three stages, 520 against 520, after the tie-break. At the
   headline point stage 3 is within 4.5 percent of a per-UE lower bound.
   Stage 3 remains reported, not optimized.

## Ranked next actions

1. **Decide G1 (G2's tie-break is applied and G3 is closed as a null result).**
   G2 is closed at the option level: an exact tie takes the cheaper option and the
   tiny-instance stage-3 gap is zero. G3 is closed structurally: a pairwise
   exchange preserves the cost multiset, so the swap can only repair stages 1-2,
   and it accepted 0 of 2,500 candidates. What remains is `mu` (give it a real
   lever or drop it) and, if a stage-3 local search is wanted, a *move* pass over
   each UE's own options including the unassigned.
2. **Fix N2.** The full-profile row still needs a caption that 853.47 km is the
   footprint radius and the outermost centre is 852.17 km.
3. **Decide the timeout semantics (N3).** Either state that RAR and CR timeouts
   cannot occur under active-slot timers, or restore wall-clock windows and
   re-measure the pause cases (the C_max=7 rows would change).
4. **Add one CG sensitivity that can bind (N4).** Deadline below the first retry,
   or sync probability 0.1, before any reliability claim.
5. **Optional: CP-SAT reference.** Section 9.5 lists it; V7 is already satisfied
   by the exhaustive reference. Do it only if a certified bound is needed.

## Reproduction

```sh
. .venv/bin/activate
pytest -q                                        # 91 tests, V1-V8
python tools/sweep.py --experiment E1 --out results
python tools/sweep.py --experiment E2 --out results   # ... E9, C1
python tools/report.py --results results
```
