# Quasi-Earth-fixed NTN handover resource allocation: design

**Version:** 5.1.

**Date:** 2026-11-05.

**Status:** design only. No code exists in this folder yet.

**Scope note:** this document is self-contained. It cites standards and papers, but it
does not point to local folders or files outside this study.

Where this document differs from the intent statement, this document takes precedence.

## Changes in version 5.1

1. Changed the cell circumradius default from 25 km to 41 km.
2. Updated the cluster and full profile geometry, cell counts, population, hop duty, and
   capacity values.
3. Changed the hop table to per-cell duty (`1/G`), which matches the active-cycle text.
4. Changed the access-type count in the option and complexity sections: six legal
   types, five occasion-start types plus DG. Updated the option count, pruning cap, and
   greedy cost.
5. Added the intra-procedure dwell pause rule. The dwell does not extend. The processing
   timers count active slots, and the predicted completion includes the waits.
6. Replaced the 4-step and DG completion sums with a message timeline. Grant offsets are
   transmit-to-transmit. The 4-step no-wait completion gains the CR propagation delay.
7. Treated the active-cell count as a ceiling. The hop table now reports the realized
   group size. The study renamed the `C` sweep to `C_max`.
8. Split V6 into V6a and V6b. V6a tests the information boundary. V6b tests the
   success-path replay. `plan.csv` gains `prediction_basis`.
9. Replaced the four-option exact-reference cap with per-cell enumeration of the pruned
   18-option set. The exact reference combines the per-cell frontiers for stage 2 and
   stage 3. The tiny instance is 3 UEs per cell in 2 cells.
10. Kept the CR step and CR timer for 4-step CFRA. Clarified that the DG grant is in the
    grant slot and allocates the 12 PUSCH PRB-symbols at the transmission slot.
11. Defined `Commit` as the ledger reservation with the CBRA contender update. Added the
    state update after each commit and each accepted swap.
12. Added the stage-2 pressure term to the greedy score and the lexicographic
    `SwapImproves`. Added the `mu` control.
13. Added the access policy and capability fallback. The allocator filters the option set
    and records `fallback_used`. E1 uses six strict arms plus joint on the full cohort.
14. Aligned the budget precheck with the beam-and-latency lower bound. Both use
    `MinCompletion(u)`, which includes the minimum no-wait procedure time over the allowed
    access types.
15. Defined the population total from the cell-area sum and split it over the cells by the
    largest remainder. The cluster has 131 or 132 UEs per cell. The full profile has 21 or
    22. The 6.1 table shows the mean.
16. Renamed the swap cap to the swap set size. The swap pool is the first S assigned UEs
    in the order, and the pass tests all their pairs. An unassigned UE is not swapped.
17. Added the DG decode failure and CG sync error probabilities. Removed the CG target DL
    CCE cost. The CG configuration is in the source HandoverCommand. Added the
    `spatial_distribution` key and the hotspot block.
18. Sized the horizon from the retry path. Added the censored rate threshold and the
    `horizon-limited` label.
19. Restricted stage 2 to the supported capability and budget strata. The low-support
    strata are reported separately. The pressure and swap use the supported strata.
20. Removed the stale R-5 risk row. The study folder is under version control.
21. Removed the online and initial labels from the one-batch design.
22. Added the Table 6.2 caption.
23. Bumped the config schema to 6. The new keys are the access policy, the DG and CG
    probabilities, and the spatial distribution.
24. Resolved open item 4. The full profile is the E8 scale run. The cluster is the main
    comparison.
25. Raised the scale claim to the full-profile 11,290 UEs per episode.
26. Renamed the 5.2 radius to the outer cell-center radius.
27. Explained the 5 ms dwell as a stress case below the 7.17 ms edge round trip.
28. Separated the loader budget guard from the per-UE 6.4 precheck.
29. Added the per-UE outcome file for the CG and DG outcomes.
30. Added the `preamble-exhausted` path. The per-UE reason is in `ue-outcomes.csv`.
31. Removed the stale CG configuration cost paragraph. The CG configuration is in the
    source HandoverCommand, outside the target access cost.
32. Showed the budget-infeasible skip in the 9.6 pseudocode and defined the `Feasible`
    blocking-pool return.
33. Added the header version to `run.json` and the budget share check to the loader.

## Changes in version 5

1. Removed post-decision re-allocation due to a new UE. The study is now one batch
   decision.
2. Removed the arrival process, lease states, commitment horizon, and six-policy matrix.
3. Removed re-allocation KPIs, controls, experiments, and artifacts.
4. Kept the batch allocation, the density input, and the algorithm and
   complexity outputs.
5. Kept the conditional ns-3 validation and the PHY backlog item.

## 1. Problem and objective

A target satellite serves a quasi-Earth-fixed (QEF) footprint with spot-beam cells.
At a handover epoch, a batch of incoming UEs must access the target. Each UE has an
access capability profile and a delay budget. The target activates only C cells at a
time through a fixed beam-hopping schedule.

The source and target satellites serve the same ground cell. A stationary UE keeps its
cell across the switch. Cell selection is therefore not part of the problem.

The study has one decision. For every incoming UE, choose an access type and a
time-frequency resource. The cell is the fixed ground cell of the UE. The choice must
respect the UE capability, the cell active mask, the cell bandwidth, and the
random-access resources.

The decision is made once for the batch. The study does not re-plan after the decision.
A UE that arrives after the decision is not modeled.

The target starts empty. This version does not model pre-existing traffic. A loaded
target is outside the scope.

### 1.1 Meaning of optimal

"Optimal" in this study means optimal for a declared discrete model. The model sets
four things:

1. the finite option set per UE,
2. the slot grid and the resource granularity,
3. the random seed and the resulting random draws,
4. the lexicographic objective in section 9.3.

The study does not claim optimality for the stochastic network, for the real protocol,
or for any continuous relaxation. Section 9.5 states the reference classes and their
limits.

### 1.2 Research questions

1. Which access type should each UE use when capabilities and delay budgets differ?
2. How much does joint access and resource selection improve over a fixed rule?
3. How do the active-cell count and the beam dwell change the access mix and the
   violation rate?
4. When does contention bind, and when do control and preamble constraints bind?
5. How close is the batch allocator to the exact reference on tiny instances?

### 1.3 Hypotheses

These are tests, not assumptions.

- H1: Dedicated access (CFRA and CG) reduces completion time, but it consumes scarce
  dedicated resources. The best mix changes with load.
- H2: 2-step access helps tight-budget UEs. Its fallback can increase contention when
  many UEs fail MsgA in the same occasion.
- H3: The beam schedule sets a hard lower bound on completion time. A budget below that
  bound is infeasible for some cells, whatever the access type.

## 2. Inputs, derived inputs, and outputs

### 2.1 Inputs

| Input | Symbol | Unit | Default | Role |
|---|---|---|---|---|
| Satellite altitude | `h` | km | 600 | Geometry |
| Minimum elevation | `theta_min` | deg | 30 | Full profile |
| Cell circumradius | `r_c` | km | 41 | Geometry |
| Cell layout | - | rings or derived | derived | Full profile |
| Active-cell ceiling | `C_max` | count | 19 (cluster), 64 (full) | Hop |
| Beam dwell | `D` | ms | 20 | Hop |
| Cell bandwidth | `B_c` | MHz | 15 | Radio |
| Subcarrier spacing | `SCS` | kHz | 15 | Radio |
| Population density | `rho` | persons/km^2 | 30 (cluster), 5 (full) | Population |
| Terminal penetration | `pi` | fraction | 0.001 | Population |
| Active UE fraction | `alpha` | fraction | 1.0 | Population |
| Capability profile shares | - | fractions | Table 6.2 | Population |
| Delay budget classes | `B_u` | ms | 100/200/400/800 | Population |
| Random-access resources | - | pools | Section 7.3 | Radio |

The population density is the primary population input. A person is not a UE. The
terminal penetration converts persons to terminals, and the active fraction converts
terminals to UEs that attempt access at the epoch.

### 2.2 Derived inputs

| Quantity | Formula |
|---|---|
| Footprint radius | `R_f = Re * (arccos(Re*cos(theta_min)/(Re+h)) - theta_min)` |
| Per-UE slant range | `d(r) = sqrt(Re^2 + (Re+h)^2 - 2*Re*(Re+h)*cos(r/Re))` |
| Per-UE one-way delay | `tau(r) = d(r) / c` |
| Cell count from rings | `N_cells = 1 + 3*R*(R+1)` |
| Cell count from footprint | lattice centers within `R_f` |
| Cell area | `A_c = (3*sqrt(3)/2) * r_c^2` |
| Total cell area | `A_total = sum over cells of A_c` |
| Total incoming UEs | `N_in = round(rho * pi * alpha * A_total)` |
| UEs per cell | `N_in(c) = floor(N_in * w_c * A_c / sum over cells of (w_c * A_c))` plus one for the largest remainders. `w_c = 1` for uniform. Ties go to the lower cell index |
| PRB count | `N_PRB = floor(B_c / (12 * SCS))` |
| Cell of a UE | the cell that contains the UE position |
| Incoming batch | `N_in` UEs at the epoch |

### 2.3 Outputs

| Output | Form |
|---|---|
| Per-UE assignment | `(cell, access, start slot, PRB range, occasion, preamble, CG/DG slot)` |
| Batch algorithm | pseudocode in section 9.6 |
| Batch complexity | time and space bounds in section 9.7 |
| KPIs | Section 10 |
| Exact-reference gap | Tiny instances only, section 9.8 |

The algorithms and their complexity are deliverables, not simulation artifacts. The
study reports them with the code.

## 3. Scope

### 3.1 In scope

| Item | Treatment |
|---|---|
| Target satellite | One satellite with a QEF footprint and a beam-hopping payload |
| QEF cells | Hexagonal cells fixed on the ground. A stationary UE belongs to exactly one cell |
| Incoming UEs | Batch released at the epoch, with capabilities and delay budgets |
| Access types | 4-step CBRA, 4-step CFRA, 2-step CBRA, 2-step CFRA, RACH-less CG, RACH-less DG |
| Resources | PRB by slot grid, PRACH occasions, contention and dedicated preambles, PDCCH CCEs, PUSCH and PDSCH PRB-symbols |
| Decisions | One batch decision at the epoch |
| Algorithms | Greedy marginal-benefit allocator with a swap pass |
| Outputs | Assignments, algorithm, complexity, KPIs |

### 3.2 Out of scope

| Item | Reason |
|---|---|
| Satellite movement | The study is a static snapshot at the handover epoch |
| Doppler shift | No waveform or PHY model, so no Doppler term is measurable |
| Handover preparation and CHO | The incoming context is assumed prepared. The epoch is an input |
| Source satellite and source cell | The study starts when the target must admit the batch |
| PHY, SINR, beamforming, link adaptation | Declared spectral efficiency replaces the PHY |
| Core network and application traffic | The delay budget is an access budget, not a packet budget |
| Inter-cell interference | Cells are independent for access. Reuse is a declared scenario input |
| UE mobility | UEs are static during the run |
| Multiple satellites | One target satellite. The source and target share the same ground cell identity |
| Pre-existing target traffic | The target starts empty in this version |
| Post-decision arrivals and re-allocation | The study is one batch decision. A new UE after the decision is not modeled |
| HARQ for data and RLC | Access retries are modeled. Data retransmission is not |

## 4. Decision log

| ID | Question | Decision | Why |
|---|---|---|---|
| D-1 | Where does the study live? | This folder, as new code | The study is independent |
| D-2 | What is the decision scope? | One batch decision at the epoch | The target plans once, then executes |
| D-3 | What does the delay budget bound? | Epoch to access completion | The study measures access, not application packets |
| D-4 | How detailed are the access models? | Abstract event model, calibrated | 2-step and RACH-less paths have no usable full-stack model here |
| D-5 | Does the target start empty? | Yes. Pre-existing traffic is removed in version 2 | The batch problem is isolated first |
| D-6 | What is the resource granularity? | PRB by slot per cell and direction, plus control and PRACH pools | The output must name concrete resources |
| D-7 | What is the geometry? | Static snapshot with per-UE elevation and propagation | The intent puts satellite motion out of scope |
| D-8 | How does beam hopping work? | Round-robin cell groups, `C_max` as the active-cell ceiling, fixed dwell | The intent gives the active-cell count as an input. The model treats it as a ceiling |
| D-9 | Does a UE choose a cell? | No. The cell is the fixed ground cell of the UE | QEF cells are fixed on the ground |
| D-10 | Which access types are modeled? | All six named in the intent | The comparison is the core of the study |
| D-11 | What optimizer class? | Greedy marginal-benefit heuristic plus an exact reference on tiny instances | The study needs a deployable policy and a bound |
| D-12 | How is the population sized? | Density times penetration times active fraction | The intent asks for a density input |
| D-13 | What implementation environment? | Python, not ns-3 or MATLAB | Section 11 gives the evaluation |
| D-14 | Does the study re-plan after a new UE arrives? | No. One batch decision only | The study is a single-epoch allocation |
| D-15 | Is the ns-3 validation realized? | No. It stays conditional | The user chose this scope |

## 5. Geometry and beam schedule

### 5.1 Static geometry

The model uses a spherical Earth with radius `Re = 6371 km`. The satellite altitude is
`h`. The minimum elevation is `theta_min`. All UEs are static and at height zero.

The Earth central angle from nadir to the footprint edge is

```text
psi_f = arccos( Re * cos(theta_min) / (Re + h) ) - theta_min
```

The footprint radius on the ground is `R_f = Re * psi_f`.

For a UE at ground range `r` from nadir, the central angle is `psi = r / Re`. Its
elevation is

```text
theta(r) = arctan( (cos(psi) - Re/(Re+h)) / sin(psi) )
```

and its slant range is

```text
d(r) = sqrt( Re^2 + (Re+h)^2 - 2*Re*(Re+h)*cos(psi) )
```

At `r = 0`, the elevation is 90 degrees by definition. The general formula is
undefined at that point.

The one-way propagation delay is `tau(r) = d(r) / c`, with `c = 299792.458 km/s`.
The model computes `tau` per UE. It does not use one delay for the whole footprint.

Reference values for `h = 600 km` and `theta_min = 30 deg`:

| Quantity | Value |
|---|---:|
| Earth central angle `psi_f` | 7.68 deg |
| Footprint radius `R_f` | 853.47 km |
| Nadir slant range | 600.00 km |
| Nadir one-way delay | 2.001 ms |
| Edge slant range | 1075.09 km |
| Edge one-way delay | 3.586 ms |

Sensitivity profiles: `h = 400`, `600`, `1200 km`, and `theta_min = 25`, `30`, `40 deg`.
At `h = 1200 km` and `30 deg`, `R_f = 1469.71 km` and the edge delay is 6.668 ms.

### 5.2 Cell layout

Cells are regular hexagons with circumradius `r_c`. The area is
`A_c = (3*sqrt(3)/2) * r_c^2`. The lattice spacing is `s = sqrt(3) * r_c`. Lattice
centers are

```text
x = s * (i + j/2)
y = s * (sqrt(3)/2) * j
```

Two reference profiles use 41 km cells:

| Profile | `N_cells` | Outer cell-center radius | Elevation at radius | Delay at radius | Purpose |
|---|---:|---:|---:|---:|---|
| cluster | 37 | 213.0 km | 68.6 deg | 2.135 ms | Mechanism and exact reference |
| full | 517 | 853.47 km | 30.0 deg | 3.586 ms | Scale and geometry |

The cluster is three hex rings around the center cell (`1 + 6 + 12 + 18 = 37`). It is a
reduced instance near nadir, not a miniature of the full footprint. Its elevation at the
outer radius is high and its delay range is narrow. The full profile covers the
30-degree edge.

The table reports the outer cell-center radius. The outer cell edge extends about one cell
radius beyond it. The elevation and delay in the table are evaluated at the reported
radius, not at the outer edge. The model still computes each UE delay from its own
position.

The cluster declares `rings = 3` and derives its footprint radius from the ring layout.
The full profile derives `N_cells` from the lattice count within `R_f`. The resolved
scenario records the derivation and the resulting count.

In a QEF system, each cell is a fixed ground area. A stationary UE belongs to exactly
one cell. At a satellite switch, the target satellite serves the same ground cell. The
model therefore fixes the cell from the UE position. It does not select among cells.

### 5.3 Beam-hopping schedule

The satellite activates at most `C_max` cells at a time. The model divides the `N_cells`
cells into `G = ceil(N_cells / C_max)` groups. The group size is `ceil(N_cells / G)` for
`N_cells mod G` groups and `floor(N_cells / G)` for the rest. The group size is the
realized active count. The average over a cycle is `N_cells / G`. Group `g(c) = c mod G`
is active in hop period `floor(k / D_slots) mod G`, where `k` is the slot index, `D_ms` is
the dwell in ms, and `D_slots = D_ms / slot_duration`. The schedule repeats every
`G * D_ms` ms.

The model uses a fixed round-robin order. It does not optimize the hop pattern. Group
membership follows the cell index, not a reuse pattern. Cell reuse is a declared
scenario input and does not change the access model.

| Profile | N_cells | C_max | D (ms) | G | Cycle (ms) | Group size | Per-cell duty |
|---|---:|---:|---:|---:|---:|---:|---:|
| cluster | 37 | 7 | 20 | 6 | 120 | 7 or 6 | 16.7% |
| cluster | 37 | 19 | 20 | 2 | 40 | 19 or 18 | 50.0% |
| cluster | 37 | 37 | 20 | 1 | 20 | 37 | 100% |
| full | 517 | 64 | 20 | 9 | 180 | 58 or 57 | 11.1% |

Dwell values for the sweep are 5, 10, 20, 50, and 100 ms. Active-cell ceiling values
are 7, 19, 37, and 64. The 64 level applies to the full profile. On the cluster, 37 is
the maximum. A 1 ms dwell is excluded, because it is far below the 7.17 ms edge round
trip. The 5 ms point is a stress case for the same reason. The 10 ms point is the first
value above the edge round trip.

The hop schedule is a hard constraint. A UE in an inactive cell cannot transmit or
receive. A procedure that reaches an inactive slot pauses. Its next message waits for
the first active slot of the same cell. The dwell does not extend. The processing timers
in section 8.1 count active slots of the cell, so they pause with the procedure. The
propagation delay `tau` is wall-clock time and does not pause. The wait is part of the
completion time, even when the UE has a dedicated grant.

`next_active(c, t)` is the first slot at or after `t` in which cell `c` is active.
`wait(c, t) = next_active(c, t) - t`. `advance(c, t, n)` is the slot reached after `n`
active slots of cell `c` starting at `t`. A processing timer of `n` slots uses `advance`.

The wait can exceed the procedure time. At `C_max = 19` and D=20, a 4-step procedure that
starts at slot 0 reaches the CR after the 20 ms window. The CR waits for the next window
at slot 40. At `C_max = 7` and D=20, the same procedure waits for slot 120.

The retry path is the attempt limit times the worst-case wait plus the procedure time.
Section 7.1 sizes the horizon from it.

### 5.4 UE placement

Each UE is placed uniformly at random inside its cell, using an area-uniform
draw on the hexagon. The population generator draws `N_in(c)` UEs for each cell. For an
imported population, the cell is the one with the nearest lattice center. A UE
outside every cell is not created. The generator reports the number of rejected draws.

The default placement is uniform, not edge-heavy. An edge-heavy sensitivity study uses
a separate draw toward the cell boundary.

## 6. Population

### 6.1 Population density

The population input is a density in persons per square kilometre. The generator
converts it to UEs with a terminal penetration and an active fraction:

```text
N_in = round( rho * pi * alpha * A_total )
```

where `rho` is persons per km^2, `pi` is the terminal penetration, `alpha` is the
active fraction, and `A_total = sum over cells of A_c`. The per-cell count `N_in(c)` is the
largest-remainder split of `N_in` over the cell areas, as in section 2.2.

The density is an average over the footprint. A footprint that mixes land and ocean
has a low average density. A hotspot sensitivity study uses a spatial distribution
instead of a uniform one. The distribution is a declared control. The hotspot block
declares the center cell, radius, and weight.

Reference population profiles:

| Profile | `rho` | `pi` | `alpha` | Mean UEs per cell | Total UEs |
|---|---:|---:|---:|---:|---:|
| cluster | 30 | 0.001 | 1.0 | 131.0 | 4,848 |
| full | 5 | 0.001 | 1.0 | 21.8 | 11,290 |

The per-cell counts are the largest-remainder split of the total over the cell areas. The
cluster has 36 cells with 131 UEs and one cell with 132. The full profile has 84 cells with
21 UEs and 433 cells with 22. For the hotspot distribution, the split uses the hotspot
weights `w_c`.

The cluster uses a higher density to create contention in 37 cells. The full profile
uses a lower average density to keep the episode size manageable. The density is an
input, not a fixed property of the geometry.

The load sweep uses `rho = 2, 10, 30, 100` on the cluster and `rho = 1, 5, 10` on the
full profile. At `rho = 100` on the cluster, the mean load is about 437 UEs per cell. At
the cluster default `rho = 30`, the mean load is about 131 UEs per cell.

### 6.2 Capability profiles

Every UE supports 4-step access. The other types are optional.

Table 6.2: Capability profiles. Each row lists the legal access types and the population
share.

| Profile | 4-step | 2-step | CG | DG | Share |
|---|---:|---:|---:|---:|---:|
| P1 | yes | no | no | no | 30% |
| P2 | yes | yes | no | no | 25% |
| P3 | yes | no | yes | no | 10% |
| P4 | yes | no | no | yes | 10% |
| P5 | yes | yes | yes | no | 10% |
| P6 | yes | yes | no | yes | 10% |
| P7 | yes | yes | yes | yes | 5% |

The shares are study inputs, not market estimates. The model draws them independently
of the delay budget by default. A correlated sensitivity study puts advanced
capabilities on tight-budget UEs.

CFRA is mandatory in the sense that every UE can execute it. The binding constraint is
the dedicated preamble pool, not the capability. The design therefore does not add a
`cfraSupported` flag. A network policy that withholds CFRA from a subgroup is a
declared policy restriction.

### 6.3 Delay budgets

| Class | Budget (ms) | Share |
|---|---:|---:|
| tight | 100 | 20% |
| short | 200 | 30% |
| medium | 400 | 30% |
| loose | 800 | 20% |

The budget applies from the epoch to the access completion event. It includes the
decision latency, the wait for its cell's active window, and the access procedure.

### 6.4 Feasibility precheck

The policy filter of section 9.6 runs first. The model computes `MinCompletion(u)` over
the allowed legal types. `MinCompletion(u)` is the decision latency plus the wait for the
next active slot of the UE's cell plus the minimum no-wait procedure time. The no-wait
procedure time depends on the type. It is `19 + 3*tau` for 4-step, `3 + 2*tau` for 2-step,
`1 + tau` for CG, and `9 + tau` for DG. The pause waits of section 5.3 are not included.

A UE with `MinCompletion(u) > B_u` is marked `budget-infeasible`. The mark is a necessary
condition. A UE that passes it can still fail on contention, control, or a pause wait. A
UE with no allowed type gets `no-legal-option` from section 9.6, not `budget-infeasible`.
The same `MinCompletion(u)` gives the lower bound in section 9.5.

The marked UE stays in the run and in the KPI denominator. It gets an explicit reason
code. The study does not remove it or move it to a different cell.

## 7. Resource model

### 7.1 Time base

The slot duration is `1 ms` at 15 kHz subcarrier spacing. The horizon is `K` slots and
starts at the epoch (`k = 0`). The default horizon is 2000 slots (2 s) for the main
comparison. The horizon must cover the largest budget plus the longest retry path. The
retry path is the attempt limit times the worst-case wait plus the procedure time:
`K_min = B_max + A_max * (W_max + P_max)`. `W_max` is the worst-case wait for the next
active slot over the point. `P_max` is the longest procedure, 19 ms for 4-step. A run that
reaches the horizon with pending UEs reports them as censored failures.

The model uses one time axis for all cells. The hop schedule, the PRACH occasions, and
the access timers use the same slot grid. Per-UE propagation delay shifts the arrival
instant of a transmission. It does not shift the slot grid.

### 7.2 PRB grid

Each cell has `B_c` Hz of bandwidth per direction. The nominal PRB count is

```text
N_PRB(c) = floor( B_c / (12 * SCS) )
```

This formula gives 27, 83, and 166 PRBs at 5, 15, and 30 MHz. The exact NR transmission
bandwidth configuration comes from TS 38.104 Table 5.3.2-1 and differs at some
bandwidths. The implementation must use the table when the standard value is required,
and must record which source it used.

The usable symbols per slot are `S_ul = 12` for PUSCH and `S_dl = 11` for PDSCH. The DL
value excludes two PDCCH symbols and one guard symbol. The UL value excludes two
symbols for DMRS and guard. These are declared study settings.

The PRB-symbol capacity per cell and slot is `N_PRB(c) * S_dir`. One PRB-symbol is 12
resource elements. At a declared spectral efficiency `SE` bits per resource element,
the instantaneous payload rate is

```text
rate(c, dir) = N_PRB(c) * S_dir * 12 * SE / slot_duration
```

With `SE = 4` bits per resource element and 15 MHz, the UL rate is 47.8 Mbps while the
cell is active. The table below shows the values. The average rate is the instantaneous
rate multiplied by the duty cycle.

| Bandwidth | PRB | UL PRB-symbols | Instantaneous UL rate | Full-profile average | Cluster C_max=19 average |
|---:|---:|---:|---:|---:|---:|
| 5 MHz | 27 | 324 | 15.6 Mbps | 1.73 Mbps | 7.8 Mbps |
| 15 MHz | 83 | 996 | 47.8 Mbps | 5.31 Mbps | 23.9 Mbps |
| 30 MHz | 166 | 1992 | 95.6 Mbps | 10.62 Mbps | 47.8 Mbps |

The average column uses the per-cell duty cycles from section 5.3. The realized active
count is the group size, not `C_max`. These numbers are a sanity check. They are not
measured capacity.

### 7.3 Resource pools

Each cell and slot has these pools:

| Pool | Unit | Default | Use |
|---|---|---|---|
| PUSCH | PRB-symbol | `N_PRB * 12` | Msg3, MsgA payload, CG and DG payload |
| PDSCH | PRB-symbol | `N_PRB * 11` | RAR, MsgB, contention resolution |
| PDCCH | CCE | 24 per slot | RAR grants, DG grants, MsgB |
| PRACH occasion | occasion | 1 per 10 slots | Preamble transmission |
| Contention preambles | preamble | 56 per occasion | 4-step and 2-step CBRA |
| Dedicated preambles | preamble | 8 per occasion | 4-step and 2-step CFRA |
| RAR processing | grant | 8 per slot | RAR scheduling limit |
| DG grants | grant | 8 per slot | Dynamic grant limit |

The preamble split is a declared scenario input. The total is 64, which matches the NR
maximum.

The RAR processing and DG grant limits are separate from the CCE limit. They model the
scheduler and processor budget, not the control channel. A run that binds on one of
them reports that reason.

### 7.4 Per-access resource vector

The model charges every access type with a declared resource vector. The values are
study settings. A calibration stage may replace them with measured values.

| Access | PRACH | Preamble | DL CCE | DL PRB-symbol | UL PRB-symbol | No-wait completion |
|---|---|---|---|---|---|---|
| 4-step CBRA | 1 | 1 contention | 1 RAR + 1 CR | 8 RAR + 8 CR | 12 Msg3 | `t_occ + 19 + 3*tau` |
| 4-step CFRA | 1 | 1 dedicated | 1 RAR + 1 CR | 8 RAR + 8 CR | 12 Msg3 | same as 4-step CBRA |
| 2-step CBRA | 1 | 1 contention | 2 MsgB | 8 MsgB | 12 MsgA | `t_occ + 3 + 2*tau` |
| 2-step CFRA | 1 | 1 dedicated | 2 MsgB | 8 MsgB | 12 MsgA | same as 2-step CBRA |
| CG | 0 | 0 | 0 | 0 | 12 PUSCH | first valid CG occasion + 1 + tau |
| DG | 0 | 0 | 1 grant | 0 | 12 PUSCH | grant slot + 9 + tau |

`t_occ` is the chosen PRACH occasion. `tau` is the one-way delay for that UE. Times are
in slots of 1 ms. The values 2, 8, and 1 are the detection slots, the grant offset, and
the transmission slot.

The table gives the no-wait completion. It assumes the cell is active at every step. The
timeline is

```text
4-step CBRA  PRACH tx  s1 = t_occ
             RAR tx    s2 = s1 + tau + 2
             Msg3 tx   s3 = s2 + 8
             Msg3 rx   r3 = s3 + tau + 1
             CR tx     s4 = r3 + 8
             CR rx     r4 = s4 + tau          completion
2-step CBRA  MsgA tx   s1 = t_occ
             MsgB tx   s2 = s1 + tau + 3
             MsgB rx   r2 = s2 + tau          completion
CG           PUSCH tx  s1 = first valid occasion
             PUSCH rx  r1 = s1 + 1 + tau      completion
DG           PUSCH tx  s1 = grant slot + 8
             PUSCH rx  r1 = s1 + 1 + tau      completion
```

The grant offsets are transmit-to-transmit. The offset includes the grant's downlink
propagation and the UE's preparation. If `8 < tau + 1`, the message starts at the grant
reception plus 1 slot instead. The RAR and MsgB windows start at the PRACH or MsgA
reception. The CR timer starts at the end of the 1-slot Msg3.

The model applies the pause rule of section 5.3 to each message in order. A processing
timer of `n` slots advances by `advance(c, t, n)`. The message is placed at the first
active slot at or after the resulting slot. The completion is the placed slot of the last
message plus its propagation delay. The processing timers pause, so an inactive window
does not consume them. `tau` does not pause.

The CG configuration is in the source HandoverCommand. It is outside the target access
cost. The target charges the 12 PUSCH PRB-symbols and the reserved occasion. The design
does not model a PDCCH activation, because the grant is Type-1.

The exact footprints are parameters. The design sets the accounting, not the numbers.
Section 8.7 lists the defaults and their basis.

### 7.5 Conservation rules

The engine enforces four rules:

1. Every PRB-symbol has at most one owner in a cell, slot, and direction.
2. A reservation holds its capacity until it is consumed, released, or expired. An
   unused reservation still blocks capacity.
3. A preamble index is unique inside one occasion and one cell.
4. A UE consumes resources only in slots where its cell is active.

A reservation that belongs to a paused procedure moves with its message. It does not
expire while the cell is inactive.

The engine writes a per-slot ledger for every cell. The metrics tool rebuilds the totals
from the ledger and compares them with the counters. A mismatch fails the run.

### 7.6 Worked capacity example

Take the cluster at 15 MHz, `C_max = 19`, and about 131 UEs per cell (density 30 persons per
km^2, penetration 0.001). Each cell is active for 20 ms in every 40 ms cycle. The UL
capacity is 996 PRB-symbols per active slot. One Msg3 uses 12 PRB-symbols, so the PRB
limit is 83 Msg3 per slot. The RAR processing limit is 8 per slot. The model spreads
RARs across the 10-slot RAR window, so the control capacity is 80 RARs per occasion.

With a 10 ms PRACH period, the cell has two occasions per dwell. At 131 UEs per cell and
64 preambles, the expected number of singleton preambles is

```text
n * (1 - 1/M)^(n-1) = 131 * (63/64)^130 = 16.9
```

So about 17 UEs can succeed per occasion under contention alone. That is below the
control capacity of 80 and below the PRB capacity. The first dwell is therefore
contention-limited. At 200 UEs per cell (density about 46), the expected singletons
fall to 8.7 per occasion. That is still below 80, so contention remains the binding
limit. The control limit binds only when the per-slot RAR demand exceeds 8, for example
with a short RAR window or a small `max_rar_per_slot`. The study sweeps that setting.
The crossing between contention and control is one of its testable mechanisms.

## 8. Access model

### 8.1 Common timing

| Timer | Default | Meaning |
|---|---:|---|
| Slot | 1 ms | Resource grid step |
| PRACH period | 10 slots | Occasion spacing |
| Detection | 2 slots | gNB detection and RAR preparation |
| RAR window | 10 slots | Starts at the PRACH or MsgA reception |
| RAR grant offset | 8 slots | Grant transmit start to Msg3 transmit start |
| Msg3 transmission | 1 slot | PUSCH duration |
| Contention resolution timer | 8 ms | Starts at the end of the 1-slot Msg3 |
| Processing | 1 ms | Response preparation |
| DG grant offset | 8 slots | Grant transmit start to PUSCH transmit start |
| CG period | 8 slots | Configured-grant period |
| Max attempts | 10 | Attempt limit before failure |

These timers are declared study settings. They follow the 3GPP random-access structure,
but they are not measured from a specific stack. A timer value in ms consumes the same
number of active slots at 1 ms per slot. The processing timers count active slots of the
cell, so they pause while the cell is inactive. The propagation delay does not pause.

All grant offsets are measured from the grant transmit start to the granted message
transmit start. The offset includes the grant's downlink propagation and the UE's
preparation. The model requires `offset >= tau + 1`. If the offset is shorter, the
message starts at the grant reception plus 1 slot.

The propagation delay is added once per direction. A request from the UE arrives after
`tau`. A response from the gNB arrives after `tau`. The model does not add the two
delays as one round-trip constant, because `tau` differs by UE. A grant offset is
transmit-to-transmit, so the grant's downlink delay is inside the offset. The timeline
in section 7.4 shows each leg once.

### 8.2 4-step CBRA

1. The UE waits for the next PRACH occasion of its cell.
2. The UE draws one preamble uniformly from the contention pool.
3. If two or more UEs in the same cell and occasion draw the same preamble, all of them
   collide. There is no capture effect by default.
4. A singleton preamble is detected with probability `p_detect` (default 1).
5. The gNB transmits a RAR within the RAR window. The RAR consumes 1 CCE and 8 PDSCH
   PRB-symbols.
6. The UE transmits Msg3 at the granted offset. Msg3 consumes 12 PUSCH PRB-symbols.
7. The gNB transmits contention resolution within the CR timer. It consumes 1 CCE and 8
   PDSCH PRB-symbols.
8. A collision, a missed detection, a RAR timeout, or a CR timeout ends the attempt.
   The UE waits a random backoff of 0 to 8 slots and tries the next occasion.

After 10 failed attempts the UE is a handover failure.

### 8.3 4-step CFRA

CFRA has the same message flow as 4-step CBRA. The difference is the preamble source.
The target assigns a dedicated preamble from the pool for a specific occasion. A
dedicated preamble cannot collide. The target must approve the reservation before the
UE uses it.

A CFRA reservation that is not used before its expiry still blocks the preamble. The
model charges that waste.

CFRA fails on a missed detection, a RAR timeout, or a CR timeout. It does not fail on
collision.

### 8.4 2-step CBRA and CFRA

The UE transmits MsgA in one occasion. MsgA contains the preamble and a small payload on
PUSCH. The gNB answers with MsgB within the response window.

- 2-step CBRA: the UE draws a contention preamble. Two UEs with the same preamble
  collide. The MsgA payload also fails with probability `p_msga` when the preamble is a
  singleton and the payload does not decode.
- 2-step CFRA: the target assigns a dedicated preamble. Collision is impossible. Payload
  failure remains possible.

On failure, the UE can fall back to 4-step CBRA at the next occasion. The scenario
declares the fallback switch and its cost. Fallback is not free: it consumes a second
PRACH occasion, a RAR, a Msg3, and a CR.

2-step uses 12 PUSCH PRB-symbols for MsgA and 8 PDSCH PRB-symbols for MsgB. It uses 2
DL CCEs for MsgB. Its no-wait completion is shorter because it removes the Msg3 round
trip.

### 8.5 RACH-less CG

The target configures a Type-1 configured grant with period `P_cg` and a per-UE offset.
The UE transmits at the first valid occasion at or after its configuration is usable.
RACH-less access has no preamble and no RAR.

The CG configuration is in the HandoverCommand. It is source-side signaling, outside the
target access cost. The model charges:

- 12 PUSCH PRB-symbols at the chosen occasion,
- the reserved occasion itself, even if the UE misses it.

A CG attempt fails when no valid occasion exists before `min(budget, horizon)`. It also
fails on a synchronization error with probability `sync_error_probability` when
`enable_sync_error` is true.

CG has low latency when its occasion is soon. Its cost is the unused reservation when
the UE is late or fails.

### 8.6 RACH-less DG

The UE monitors PDCCH for a dynamic grant. The target transmits one CCE grant in the
grant slot. The grant allocates 12 PUSCH PRB-symbols at the transmission slot. The UE
transmits at the granted offset.

DG fails when no grant arrives before `min(budget, horizon)`, or when the grant is not
decoded (probability `dg_decode_failure_probability`). DG has no reservation waste, but it
depends on the control pool at the moment of the grant.

### 8.7 Attempt, success, failure, and calibration

| Term | Definition |
|---|---|
| Attempt | One PRACH or MsgA transmission. CG and DG have no RACH attempts |
| Re-attempt | Any attempt after the first for the same UE |
| Access success | The UE reaches `CONNECTED_TARGET` before the horizon |
| Access failure | The attempt limit is reached, or the CG or DG deadline expires |
| Budget violation | Failure, or success after the delay budget |
| Censored | Still pending at the horizon. Counted as a failure. The censored rate is reported separately |

The censored rate is `N_censored / N_in`. A point above 1 percent is labeled
`horizon-limited` and is not used in the main claim.

A failed UE stays in the violation numerator. The model never removes it from the
denominator.

| Parameter | Default | Basis |
|---|---:|---|
| Slot | 1 ms | 15 kHz subcarrier spacing |
| RAR window | 10 slots | Declared, within the 3GPP response-window structure |
| Contention timer | 8 ms | Declared |
| RAR grant offset | 8 slots | Declared |
| DG grant offset | 8 slots | Declared |
| Backoff | 0 to 8 slots | Declared |
| Max attempts | 10 | Study value |
| Preambles | 64 total (8 dedicated) | NR maximum |
| Detection probability | 1.0 | Idealized first stage |
| MsgA payload success | 0.99 | Declared. No calibrated source in this study |
| DG grant decode failure | 0.0 | Idealized. Sensitivity 0.01 |
| CG sync error | 0.01 | Declared. Gated by `enable_sync_error` |
| MsgB window | 10 slots | Same as the RAR window |

The CG configuration is in the HandoverCommand. It is source-side signaling, outside the
target access.

The retry interval follows from the occasion period, the RAR window, and the backoff.
It is not copied from another study. A calibration stage must reproduce the analytical
singleton curve before it claims agreement with any external measurement.

## 9. Allocation problem

### 9.1 Decision variables

For UE `u`, access type `a`, and start slot `k`:

```text
x[u, a, k] = 1 if UE u uses access a starting at slot k, otherwise 0
```

A UE may also be left unassigned. That state is explicit and penalized.

The cell is fixed by the UE position. It is not a decision variable.

For CFRA, the model adds a preamble index `p` from the dedicated pool. For CG, it adds
the occasion index. For DG, the grant slot is the start slot.

For CBRA, the network does not choose the preamble. The UE draws it at random. The
allocator chooses only the occasion and the contention pool. This distinction
is important. A model that lets the allocator pick CBRA preambles is modeling CFRA.

### 9.2 Constraints

For every UE `u`:

```text
sum over legal (a, k) of x[u, a, k] + unassigned[u] = 1
```

Legal means all of the following:

- `a` is in the UE capability profile,
- the start slot is a valid PRACH occasion, CG occasion, or DG grant slot,
- the procedure's messages are placed at active slots of the cell by the pause rule in section 5.3,
- the start slot is at or after the decision end,
- the predicted completion is within the horizon.

For every cell `c`, slot `k`, and pool `r`:

```text
sum of demand(u, a, k, r) * x[u, a, k] <= capacity(c, k, r)
```

Each UE contributes only to its own cell.

For every cell `c`, occasion `o`, and preamble index `p`:

```text
sum of dedicated assignments to (c, o, p) <= 1
```

For CBRA, the collision outcome is not a constraint. It is a random outcome of the
draw. The allocator can only control the number of contenders per occasion.

### 9.3 Objective

The batch objective is lexicographic. Each stage is reported separately.

| Stage | Quantity | Definition |
|---|---|---|
| 1 | Delay-budget violation rate | Weighted count of violating UEs divided by all UEs |
| 2 | Worst supported-stratum violation rate | Maximum violation rate over the supported capability and budget strata. A low-support stratum is excluded |
| 3 | Resource cost | PRB-symbols, preambles, CCEs, CG waste, and re-attempts |

The CG configuration is in the HandoverCommand. It is outside the target access cost.

The weighted violation rate is

```text
L = sum_u w_u * V_u / sum_u w_u
```

where `V_u = 1` if the UE fails or completes after its budget. The default weight is 1
for every UE. A sensitivity study uses weights by budget class. The weights are frozen
before the evaluation runs.

Stage 2 is a declared tie-breaker. It is not a hard fairness constraint. It prevents a
policy from improving the mean by sacrificing a supported capability stratum. The
low-support strata are reported separately and are not protected by stage 2. The greedy
score uses a stratum-pressure term as a guide. The swap pass applies the exact
lexicographic order.

### 9.4 Feasible options and pruning

For UE `u`, an option is a legal `(access, start)` pair with a predicted completion and
a resource vector. The predicted completion includes the intra-procedure dwell waits of
section 5.3. The option count is bounded by

```text
(occasions within budget) * (allowed occasion-start access types) + (DG slots within budget)
```

The six access types split into five that start at a PRACH or CG occasion and DG, which
starts at any grant slot. For a 400 ms budget, a 10 ms PRACH period, five occasion-start
types, and 400 DG slots, the count is about 600 before pruning. Pruning keeps the
earliest three starts per access type, which gives at most 15 occasion-start options
plus three DG options, so 18 options per UE.

The access policy filters the access types before pruning. The 600 and the 18-option count
are for the joint policy. A fixed arm keeps three starts for each allowed type. The DG
term is zero when DG is not allowed.

### 9.5 Reference classes

| Reference | Size | What it establishes | Limit |
|---|---|---|---|
| Per-cell exhaustive enumeration | 3 UEs per cell, 2 cells, 40 slots | Exact optimum of the declared pruned model | Exponential in the UEs per cell. Uses fixed random draws per cell |
| Beam-and-latency lower bound | any size | Minimum possible violations from the hop schedule and the minimum procedure time | Ignores contention and most resources |
| CP-SAT time-indexed model | 16 to 128 UEs | Feasible plan and solver bound for a deterministic approximation | Collisions are replaced by expected values, so the bound is not certified for the simulator |

The per-cell exhaustive reference uses the allocator's pruned option set and the same
access policy. The set is at most 18 options per UE under the joint policy. It enumerates
the Cartesian product of the per-UE options in each cell independently. For each per-cell
schedule it replays the same engine with the same seeds. It recomputes collisions for
that schedule. It does not transplant outcomes between schedules.

The cell is independent for access, so the stage-1 weighted counts and the stage-3
resource costs combine by addition. Stage 2 is a max over the supported capability and
budget strata and can span cells. The reference therefore keeps the stage-1-optimal
frontier of each cell and combines the frontiers for stage 2 and stage 3. Each candidate
carries its per-stratum violation counts, so the combination computes the global stage-2
max over the supported strata. The low-support counts are kept for the descriptive report.
The combination is a small multi-choice problem over the per-cell candidates.

The beam-and-latency lower bound uses `MinCompletion(u)` from section 6.4. If
`MinCompletion(u) > B_u`, the UE must violate. The count of such UEs is a lower bound on
the violation count. It ignores contention, control, and the pause waits, so it can be
loose.

A heuristic solution is not a lower bound. The study reports `LB <= optimum <= UB` only
when it has both. A gap is valid only inside the same feasible set and the same
lexicographic stage.

### 9.6 Batch algorithm

The allocator runs once at the epoch. It has a declared decision latency of 1 ms
by default. The first access start must be after the decision end. The section 6.4
precheck runs first. A UE with `MinCompletion(u) > B_u` is marked `budget-infeasible` and
is not enumerated.

```text
ALGORITHM AllocateBatch(U, cells, hop, resources, budgets, capabilities, policy)
 1  for each u in U not marked budget-infeasible:
 2      options[u] = EnumerateOptions(u, budgets[u], hop, resources, policy)
 3      if options[u] is empty:
 4          mark u unassigned with reason no-legal-option
 5          continue
 6      T_min[u] = min completion over options[u]
 7      slack[u] = budgets[u] - T_min[u]
 8  order = sort U by (slack ascending, len(options) ascending)
 9  ledger = BuildLedger(cells, horizon)          # capacity per cell, slot, pool
10  for each u in order:
11      best = none
12      preamble_blocked = false
13      for each option o in options[u]:
14          if not Feasible(ledger, o):
15              if o needs a preamble and only the preamble pool blocks o:
16                  preamble_blocked = true
17              continue
18          s = Score(o, u, budgets[u], ledger)
19          if best is none or s < best.score:
20              best = o
21      if best is none:
22          if preamble_blocked:
23              mark u unassigned with reason preamble-exhausted
24          else:
25              mark u unassigned with reason capacity
26      else:
27          Commit(ledger, best)
28          u.plan = best
29          UpdateState(ledger, plans)
30  for pass in 1..max_swap_passes:
31      improved = false
32      for each pair (u, v) in SwapPairs(order, swap_cap):
33          if SwapImproves(u, v, ledger):
34              ApplySwap(u, v, ledger)
35              UpdateState(ledger, plans)
36              improved = true
37      if not improved:
38          break
39  return plans, ledger
```

`EnumerateOptions` applies the pause rule of section 5.3, so each option's predicted
completion includes the intra-procedure dwell waits. It also applies the access policy.
For a fixed policy, it keeps only the allowed types. If the intersection is empty, it
returns the fallback set, or it returns empty for a strict policy.

`SwapPairs(order, S)` returns every unordered pair of the first S assigned UEs in `order`.
The swap exchanges the two current options. An unassigned UE is not in the swap pool.

`Feasible(ledger, o)` returns `none` when the option fits the ledger. Otherwise it returns
the blocking pools. The greedy sets `preamble_blocked` when the option needs a preamble and
the preamble is the only blocking pool.

`BuildLedger` initializes every `price[r]`, `contention[occ]`, and stratum violation count
to zero. `Commit(ledger, best)` reserves the option's resources in the ledger. For a CBRA
option, it also increments `ledger.contention[occ]` for the chosen occasion. `u.plan =
best` records the choice. `UpdateState(ledger, plans)` recomputes `utilisation[r]`,
`price[r]`, and the stratum violation rates. The score takes `pressure[u]` as the maximum
of the supported rates for `u`. If neither of the two strata is supported,
`pressure[u] = 0`. The next UE scores against the updated prices, contender counts, and
pressure.

The score is

```text
score = 1000 * [T > B_u]
      + lambda * max(0, T - B_u)
      + mu * (1 - pressure[u])
      + sum_r price[r] * demand[r]
      + contention_penalty
```

The contention penalty for CBRA is `-log(p_singleton(n_o))`, where `n_o` is the number
of contenders already placed on that occasion. This spreads contention. The price update
is

```text
price[r] = utilisation[r]^2 / (1 - utilisation[r] + 0.01)
```

`pressure[u]` is the maximum predicted violation rate over the supported capability and
budget strata of `u`, using the plans committed so far. If neither stratum is supported,
`pressure[u] = 0`. The `mu` term gives the greedy a stage-2 signal: a UE in a high-pressure
stratum gets a lower penalty. `mu` is a declared constant.

`SwapImproves(u, v, ledger)` evaluates the batch objective before and after the swap. The
objective is the tuple (stage-1 violation rate, stage-2 worst supported-stratum rate,
stage-3 resource cost). It accepts the swap only if the tuple improves lexicographically
and the swap is feasible.

The constants are declared and tuned on a pilot, not on the evaluation seeds.

The allocator logs every rejection with a reason code. `no-legal-option` means the allowed
or feasible option set is empty. `capacity` means a non-preamble resource pool is full.
`preamble-exhausted` means the contention or dedicated preamble pool is full.
`budget-infeasible` means the 6.4 precheck. `unassigned` is the generic fallback for a UE
that no rule covers.

The allocator is deterministic for a fixed seed and input. It does not read simulator
state that a real target would not know. It sees the UE capabilities, the budgets, the
cell of each UE, the hop schedule, the access policy, and its own resource ledger. It
does not see future random draws. V6a tests this boundary.

### 9.7 Batch complexity

| Symbol | Meaning | Default scale |
|---|---|---:|
| `U` | Incoming UEs | 4,848 cluster, 11,290 full |
| `A` | Legal access types per UE after the policy filter | at most 6 |
| `O` | Feasible starts per access type after pruning | at most 3 |
| `L` | Procedure messages per option | at most 4 |
| `M_c` | Options per UE in cell `c` of the exact instance | at most 18 |
| `U_c` | UEs in cell `c` of the exact instance | at most 3 |
| `K` | Slots in the horizon | 2,000 main, computed per E3 point |
| `N_cells` | Cells | 37 or 517 |
| `R` | Resource pools per cell and slot | 6 |
| `S` | Swap set size. The first `S` assigned UEs in the order | 100 |
| `P` | Swap passes | 10 |
| `F` | Per-cell frontier combinations | product of the per-cell frontier sizes |
| `S_s` | Supported capability and budget strata | at most 11 |

| Stage | Time complexity | Notes |
|---|---|---|
| Option enumeration | `O(U * A * O * L)` with an index, `O(U * K)` without | The index is built once |
| Sorting | `O(U log U)` | Slack and option count |
| Greedy assignment | `O(U * A * O * R * L)` | Price, pressure, and per-message feasibility checks |
| Swap pass | `O(P * S^2 * (A * O + S_s))` | First `S` assigned UEs, all pairs, and the lexicographic objective |
| Ledger build | `O(N_cells * K * R)` dense, `O(occupied slots)` sparse | Sparse is the default |
| Total | `O(U*A*O*L + U log U + U*A*O*R*L + P*S^2*(A*O + S_s) + occupied slots)` | The swap pass dominates at large `S` |
| Exact reference | `O(sum_c M_c^{U_c} * U_c * L + F)` | Per-cell enumeration. `M_c <= 18`, `U_c <= 3`. `F` is the frontier combination |
| Lower bound | `O(U * A)` | Earliest dwell and minimum procedure |

With the default values, the greedy pass is about `108 * U * L` operations, or `432 * U`
at `L = 4`. The swap pass is at most `10 * 100^2 * (18 + 11) = 2.9M` basic checks, which
is why `S` and `P` are capped. The exact pair count is at most `S*(S-1)/2` per pass. The
space bound is `O(U * A * O + occupied ledger slots)`.
The ledger is sparse, so a run does not allocate `N_cells * K * R` entries for inactive
cells. The state update costs `O(R + S_s)` per commit and per accepted swap. It is
dominated by the greedy and swap terms.

### 9.8 What the gap claim means

On tiny instances, the study reports the exact optimum and the allocator result. The
gap is the additive difference in stage-1 violation rate. The allocator and the reference
use the same pruned option set and the same access policy. They also use the same stage-2
strata, the per-cell decomposition, and the same seeds. The study does not claim a gap at
full scale. At full scale it reports the lower bound and the measured outcome.

## 10. KPIs and measurement

### 10.1 KPI definitions

| KPI | Definition | Denominator |
|---|---|---|
| Handover success rate | UEs that reach `CONNECTED_TARGET` / UEs | All UEs |
| Handover failure rate | UEs that reach the attempt limit, the CG or DG deadline, or the horizon | All UEs |
| Delay-budget violation rate | UEs that fail or complete after their budget / UEs | All UEs |
| Time to completion | Epoch to `CONNECTED_TARGET` | Successful UEs only |
| RACH attempts | PRACH or MsgA transmissions per UE | All UEs |
| Re-attempts | Attempts minus one for UEs with at least one attempt | UEs with at least one attempt |
| Resource use | PRB-symbols, CCEs, preambles, CG waste | Per cell and slot |
| Access mix | Share of UEs by selected access type | All UEs |

Time to completion reports the median, p95, p99, and maximum. Failures are censored at
the horizon and reported as a separate count. A completion time without the failure
count is not a valid result.

The violation rate uses the same denominator for every configuration. The denominator is
the generated cohort, not the set of UEs that received an assignment. The summary reports
the censored rate. A censored UE is a failure in the numerator. The summary also reports
the stratum support. A low-support stratum is labeled.

### 10.2 Strata

Every KPI is reported for these strata:

- capability profile (P1 to P7),
- delay-budget class,
- cell,
- access type actually used.

A stratum with fewer than 20 UEs or 100 attempts is marked low-support. The study does
not claim population reliability from it. Stage 2 excludes the low-support strata. They
are reported separately.

### 10.3 Statistical protocol

The experimental unit is one episode. An episode is one batch at one epoch with one
seed. Positions, capabilities, budgets, and random draws all derive from that seed.

- Use paired seeds across the configurations being compared.
- Run at least 30 episodes per main scenario.
- Replicate every sweep point with at least three seeds before publishing it.
- Report 95% confidence intervals on paired differences.
- Freeze the evaluation seeds and the sample size before the main run.
- Use separate seeds for tuning, pilot, and evaluation.

A single-episode anomaly is not a result. A point that survives three seeds is a
finding. A point that disappears under replication is a retraction.

### 10.4 Artifacts

| File | Content |
|---|---|
| `run.json` | Resolved configuration, package versions, config hash, header version, seed list |
| `ues.csv` | One row per UE: id, cell, capability, budget |
| `plan.csv` | One row per assigned UE: the full assignment record |
| `access-events.csv` | One row per attempt: UE, cell, access, occasion, outcome, reason |
| `ue-outcomes.csv` | One row per UE: UE, cell, access, outcome, reason, attempts, completion_slot. The CG and DG outcomes are included. They have no attempt rows |
| `ledger.csv` | Per cell, slot, pool: capacity and use |
| `summary.json` | All KPIs, strata, confidence intervals, and feasibility flags |
| `summary.csv` | Flat KPI table for plotting, including the censored rate and the low-support flag |

The `plan.csv` columns are:

```text
ue, cell, access, start_slot, prach_occasion, preamble_pool, preamble_index,
prb_start, prb_count, symbol_count, cce_count, cg_occasion, dg_slot,
predicted_completion_slot, prediction_basis, fallback_used
```

The `preamble_index` column is empty for CBRA, because the UE chooses the preamble. It
is filled for CFRA.

The `prediction_basis` column records the basis of `predicted_completion_slot`. The
current study uses `no-failure` for every planned UE. The predicted completion is the
no-failure completion from section 7.4 plus the pause waits from section 5.3.

`fallback_used` is true when the capability fallback produced the option. It is false for
the joint arm and the strict arms.

The CSV headers are fixed in the implementation. A header change breaks the analysis
tool, so the tool version and the header version are recorded in `run.json`.

## 11. Simulation environment evaluation

This section answers one question: does the study need ns-3 and its submodules, or does
an abstract Python or MATLAB simulation suffice?

### 11.1 Criteria

| Criterion | ns-3 with NR/NTN modules | Python with SimPy | MATLAB |
|---|---|---|---|
| 4-step random access | Present. Contention resolution is not fully awaited | Modeled as declared events | Modeled as declared events |
| 2-step random access | Absent. No MsgA path | Modeled | Modeled |
| RACH-less CG/DG | Absent as a handover path | Modeled | Modeled |
| PHY, SINR, link adaptation | Present | Absent | Absent |
| Packet and core network | Present | Absent | Absent |
| Discrete-event engine | Mature | Mature enough with a small tested core | Weak |
| Determinism | Strong RNG streams | Strong with `numpy` seed sequences | Strong with `rng(seed)` |
| Iteration speed | Slow. C++ build per change | Fast | Medium |
| Scale per episode | High cost per run | 11,290 UEs per episode is feasible | Medium |
| Exact optimization | External tool | OR-Tools CP-SAT | Optimization Toolbox |
| Statistics | External scripts | NumPy, SciPy, pandas | Statistics Toolbox |
| Tests and CI | ns-3 test runner | pytest | MATLAB test framework |
| Effort to model six access types | High. New protocol code | Medium. Event models | High |
| Path to PHY validation | High | Low | Low |

### 11.2 Evidence from the available ns-3 stack

The available ns-3 NR module implements 4-step random access. It has no 2-step random
access, no MsgA or MsgB path, and no RACH-less handover path. Its UE MAC notes that
contention resolution is not awaited in principle. A model of all six access types on
that stack would be new protocol code, not reuse.

The study also has no PHY, no packets, and no core network in scope. Those are the
parts where ns-3 adds unique value. Without them, ns-3 adds build cost and protocol
code that the study does not need.

### 11.3 Recommendation

Use Python. The model is an abstract discrete-event simulation with a PRB ledger and
declared timers. Python gives exact control of the model, fast iteration, deterministic
random streams, pytest, and OR-Tools for the exact reference.

Do not use MATLAB. It is good for numerical optimization. However, its discrete-event
support is weak, and its licensing and CI story is heavier. The study needs a tested
event engine more than a matrix language.

Do not use ns-3 for version 5. Revisit this decision only if one of these conditions
holds:

1. The study adds PHY or SINR behavior.
2. The study adds packet-level QoS or core-network effects.
3. A reviewer requires full-stack 4-step protocol validation.

If a condition holds, use ns-3 for a small validation slice. Keep the main allocator and
the experiment engine in Python. Do not move the whole study.

### 11.4 Consequences

- The implementation is a Python package, not an ns-3 build target.
- Tests use pytest, not the ns-3 test runner.
- The model claims mechanism and relative access-rule performance. It does not claim
  protocol conformance.
- The ns-3 validation slice stays conditional in the backlog, section 15.

## 12. Python implementation plan

### 12.1 Package layout

```text
qef_ho_ra/
|-- __init__.py
|-- cli.py               command line and run orchestration
|-- config.py            schema, validation, resolved document
|-- geometry.py          Earth, footprint, cells, per-UE delay
|-- hop.py               groups, dwell, active mask
|-- population.py        density, penetration, capabilities, budgets
|-- resources.py         PRB and pool ledger
|-- access.py            six access procedures and retries
|-- allocator.py         options, greedy pass, swap pass
|-- engine.py            SimPy environment and episode runner
|-- metrics.py           counters, strata, CSV writers
`-- reference.py         exhaustive reference and lower bound
tests/
|-- test_geometry.py
|-- test_hop.py
|-- test_population.py
|-- test_resources.py
|-- test_access.py
|-- test_allocator.py
|-- test_reference.py
`-- test_determinism.py
scenarios/
|-- cluster-37.json
|-- footprint-517.json
`-- tiny-exact.json
tools/
|-- sweep.py
`-- report.py
requirements.txt
README.md
```

### 12.2 Event engine and determinism

The engine uses SimPy 4.1. Each episode creates a new `simpy.Environment`. The allocator
runs as a plain function at time zero after the decision latency. Each UE access is a
SimPy process.

Random draws use `numpy.random.SeedSequence`. The episode seed spawns separate streams
for positions, capabilities, budgets, preamble draws, detection outcomes, MsgA payload
outcomes, DG grant outcomes, and backoff. A configuration-specific draw must not shift
an exogenous stream. Stream IDs are recorded in `run.json`.

The ledger uses integer PRB-symbols, not floating-point units. All iteration over UEs
and resources is sorted by ID. A rerun with the same seed produces byte-identical CSV
files.

### 12.3 Configuration

The configuration is JSON with a typed schema. The schema version is 6. It is independent
of the document version. A version-5 config lacks the access policy, the DG and CG
probabilities, and the spatial distribution keys. A compact example:

```json
{
  "schema_version": 6,
  "geometry": {
    "altitude_km": 600,
    "min_elevation_deg": 30,
    "cell_radius_km": 41,
    "profile": "cluster",
    "rings": 3
  },
  "hop": {
    "active_cells": 19,
    "dwell_ms": 20,
    "phase_slots": 0
  },
  "radio": {
    "bandwidth_mhz": 15,
    "subcarrier_spacing_khz": 15,
    "spectral_efficiency_bits_per_re": 4.0,
    "prach_period_slots": 10,
    "preamble_total": 64,
    "preamble_dedicated": 8,
    "pdcch_cce_per_slot": 24,
    "max_rar_per_slot": 8,
    "max_dg_grants_per_slot": 8
  },
  "population": {
    "density_persons_per_km2": 30,
    "terminal_penetration": 0.001,
    "active_fraction": 1.0,
    "capability_shares": [0.30, 0.25, 0.10, 0.10, 0.10, 0.10, 0.05],
    "budget_shares": [0.20, 0.30, 0.30, 0.20],
    "budget_ms": [100, 200, 400, 800],
    "capability_budget_correlation": "independent",
    "spatial_distribution": "uniform",
    "hotspot": {"center_cell": 0, "radius_km": 10, "weight": 0.8}
  },
  "access": {
    "max_attempts": 10,
    "backoff_slots": 8,
    "detection_probability": 1.0,
    "msga_success_probability": 0.99,
    "dg_decode_failure_probability": 0.0,
    "sync_error_probability": 0.01,
    "enable_two_step_fallback": true,
    "cg_period_slots": 8,
    "enable_sync_error": false
  },
  "allocator": {
    "decision_latency_ms": 1,
    "options_per_access": 3,
    "lambda": 1.0,
    "mu": 1.0,
    "access_policy": "joint",
    "allowed_access": ["4step-cbra", "4step-cfra", "2step-cbra",
                       "2step-cfra", "cg", "dg"],
    "capability_fallback": "strict",
    "swap_cap": 100,
    "max_swap_passes": 10
  },
  "evaluation": {
    "horizon_slots": 2000,
    "episodes": 30,
    "first_seed": 1
  }
}
```

The loader rejects a missing key or an invalid range. It rejects `fixed` with an empty or
unknown `allowed_access`, and a `capability_fallback` outside `strict`, `4step-cbra`, and
`best-legal`. It rejects a probability outside [0, 1], an unknown `spatial_distribution`,
and a hotspot weight outside [0, 1]. It also rejects a capability or budget share that
does not sum to one. The same check rejects a preamble split that exceeds the total. It
also rejects a budget class below the 1 ms shortest no-wait procedure time. This budget
check is a configuration guard. It does not remove a UE. The 6.4 precheck marks the UEs
whose own `MinCompletion` exceeds their budget. Those UEs stay in the run. The loader
writes the resolved configuration to `run.json`. `swap_cap` is the swap set size. The pool
is the first S assigned UEs in the order.

### 12.4 Dependencies and run

Python 3.11 or newer. A virtual environment holds the dependencies.

```sh
python3 -m venv .venv
. .venv/bin/activate
python -m pip install -r requirements.txt
python -m qef_ho_ra.cli --config scenarios/cluster-37.json --output results/cluster-37
pytest -q
```

`requirements.txt` lists `numpy`, `scipy`, `pandas`, `simpy`, `ortools`, `pydantic`,
`matplotlib`, and `pytest`.

The CLI writes `run.json`, the CSV artifacts, and `summary.json`. It does not write
outside the output directory.

### 12.5 Validation gates

| Gate | Check | Failure means |
|---|---|---|
| V1 Geometry | Footprint radius, edge delay, and cell count match an independent calculation | The geometry code is wrong |
| V2 Hop | Active mask has the declared duty and repeats with the declared cycle | The schedule code is wrong |
| V3 Conservation | Every PRB-symbol has one owner. The ledger totals equal the counters | The accounting is wrong |
| V4 Analytic | Singleton count follows `n*(1-1/M)^(n-1)` within the confidence band | The contention model is wrong |
| V5 Boundary | Budget equal to completion, attempt limit, occasion boundary, inactive cell, preamble exhaustion, CG and DG timeout, 2-step fallback, access-policy gap, capability fallback | An edge case is wrong or untested |
| V6a Information boundary | Same allocator inputs, different future draw streams, identical `plan.csv` | The allocator reads future draws |
| V6b Success-path replay | For each UE whose realized draws all succeed, the actual completion equals the predicted completion with the pause waits. Each failed UE matches the expected reason and retry count | The engine or the prediction is wrong |
| V7 Exact | Per-cell independent enumeration with the same policy and the per-cell frontier combination reproduce the lexicographic optimum of a tiny case | The reference or the objective order is wrong |
| V8 Determinism | Two runs with one seed produce byte-identical CSV files | A stream or iteration order is unstable |

A gate is not a claim. V4 shows the model implements the formula, not that the formula
matches a real gNB. V7 shows the solver is right for its model, not that the model is
the network.

V6a varies only the outcome streams: preamble, detection, MsgA, DG, sync when enabled,
and backoff. The position, capability, and budget streams and the access policy stay
fixed. V6b uses one plan and one seed. It compares the completion only for UEs whose
draws all succeed. For a failed UE, it checks the failure reason and retry count.

### 12.6 Staged implementation

| Stage | Work | Exit criterion |
|---|---|---|
| S0 | Config, geometry, hop, population, resource ledger | V1, V2, V3, V8 pass |
| S1 | 4-step CBRA and CFRA engine | V4 and the CBRA and CFRA cases of V5 pass |
| S2 | 2-step access with fallback | The 2-step cases of V5 pass |
| S3 | CG and DG access | The RACH-less cases of V5 pass |
| S4 | Batch allocator and swap pass | V6a and V6b pass |
| S5 | Per-cell exhaustive reference and lower bound | V7 passes |
| S6 | Experiment harness, pilot, and main run | The pilot sets the sample size and the sweep points |

Each stage runs the full gate set for the features it adds. A stage that changes an
earlier artifact reruns the earlier gates. The study does not stack stages without a
green gate.

## 13. Control variables for repeated simulations

### 13.1 Control summary

| Group | Control | Default | Sweep or sensitivity | Role |
|---|---|---|---|---|
| Geometry | `altitude_km` | 600 | 400, 600, 1200 | Sweep |
| Geometry | `min_elevation_deg` | 30 | 25, 30, 40 | Sweep |
| Geometry | `cell_radius_km` | 41 | 25, 41, 50, 100 | Sensitivity |
| Geometry | `profile` | cluster | cluster, full | Fixed per run |
| Geometry | `rings` | 3 | 2, 3, 4 | Fixed per run |
| Population | `density_persons_per_km2` | 30 cluster, 5 full | 1, 2, 5, 10, 30, 100 | Sweep |
| Population | `terminal_penetration` | 0.001 | 0.0001, 0.001, 0.01 | Sensitivity |
| Population | `active_fraction` | 1.0 | 0.3, 0.6, 1.0 | Sensitivity |
| Population | `capability_shares` | Table 6.2 | default, no-advanced, all-advanced | Sweep |
| Population | `budget_ms` | 100/200/400/800 | tight-heavy, default, loose-heavy | Sweep |
| Population | `capability_budget_correlation` | independent | independent, advanced-on-tight | Sensitivity |
| Population | `spatial_distribution` | uniform | uniform, hotspot | Sensitivity |
| Hop | `active_cells` | 19 cluster, 64 full | 7, 19, 37, 64 | Sweep |
| Hop | `dwell_ms` | 20 | 5, 10, 20, 50, 100 | Sweep |
| Hop | `phase_slots` | 0 | 0, half cycle | Sensitivity |
| Radio | `bandwidth_mhz` | 15 | 5, 15, 30 | Sweep |
| Radio | `subcarrier_spacing_khz` | 15 | 15, 30 | Sensitivity |
| Radio | `spectral_efficiency_bits_per_re` | 4.0 | 2, 4, 6 | Sensitivity |
| Radio | `prach_period_slots` | 10 | 5, 10, 20 | Sweep |
| Radio | `preamble_total` | 64 | 32, 64 | Sensitivity |
| Radio | `preamble_dedicated` | 8 | 0, 8, 16, 32 | Sweep |
| Radio | `pdcch_cce_per_slot` | 24 | 12, 24, 48 | Sensitivity |
| Radio | `max_rar_per_slot` | 8 | 4, 8, 16 | Sweep |
| Radio | `max_dg_grants_per_slot` | 8 | 4, 8, 16 | Sweep |
| Access | `max_attempts` | 10 | 4, 10, 20 | Sweep |
| Access | `backoff_slots` | 8 | 0, 8 | Sensitivity |
| Access | `detection_probability` | 1.0 | 0.90, 0.99, 1.0 | Sensitivity |
| Access | `msga_success_probability` | 0.99 | 0.95, 0.99 | Sensitivity |
| Access | `dg_decode_failure_probability` | 0.0 | 0.0, 0.01 | Sensitivity |
| Access | `sync_error_probability` | 0.01 | 0.005, 0.01 | Sensitivity |
| Access | `enable_two_step_fallback` | true | false, true | Ablation |
| Access | `cg_period_slots` | 8 | 4, 8, 16 | Sweep |
| Access | `enable_sync_error` | false | false, true | Sensitivity |
| Allocator | `decision_latency_ms` | 1 | 0, 1, 5 | Sweep |
| Allocator | `options_per_access` | 3 | 1, 3, 5 | Sensitivity |
| Allocator | `lambda` | 1.0 | 0.1, 1, 10 | Sensitivity |
| Allocator | `mu` | 1.0 | 0, 0.5, 1, 2 | Sensitivity |
| Allocator | `swap_cap` | 100 | 0, 100, 500 | Sensitivity |
| Allocator | `max_swap_passes` | 10 | fixed | Fixed |
| Experiment | `access_policy` | joint | fixed (one allowed type) or joint | E1 |
| Experiment | `capability_fallback` | strict | strict, 4step-cbra, best-legal | E1 sensitivity |
| Evaluation | `horizon_slots` | 2000 main, computed E3 | 2000, `K_min` | Fixed per point |
| Evaluation | `episodes` | 30 | 30, 100 | Fixed after pilot |
| Evaluation | `first_seed` | 1 | fixed block | Fixed |

`active_cells` sets `C_max`. The realized active count is the group size in section 5.3.
The 64 level applies to the full profile. On the cluster, 37 is the maximum. `swap_cap` is
the swap set size. The pool is the first S assigned UEs in the order.

### 13.2 Repeated-run recipe

1. Freeze the structural controls: geometry profile, cell radius, hop pattern, and
   access timers.
2. Vary one group at a time. Do not move density and dwell together in one sweep. Their
   effects overlap and the attribution is lost.
3. Use paired seeds for every configuration at the same point.
4. Replicate each point with at least three seeds before publishing it.
5. Freeze the evaluation seed block and the sample size before the main run.
6. Record the resolved configuration and the config hash in `run.json`.

## 14. Experiment plan

| ID | Question | Factors | Levels | Purpose |
|---|---|---|---|---|
| E1 | Which access type wins? | Access policy | 4-step CBRA only, 4-step CFRA only, 2-step CBRA only, 2-step CFRA only, CG only, DG only, joint | Isolate the value of the mix |
| E2 | How does load change the answer? | Density | 2, 10, 30, 100 persons/km^2 on the cluster | Find the contention and control crossovers |
| E3 | How does hopping change the answer? | `C_max`, dwell | `C_max` 7/19/37, dwell 5/10/20/50/100 ms. The horizon is `K_min` per point | Measure the hard wait floor |
| E4 | Do budgets change the mix? | Budget shares | tight-heavy, default, loose-heavy | Test H1 and H2 |
| E5 | Does preamble scarcity bind? | Dedicated share | 0, 8, 16, 32 of 64 | Separate CFRA from CBRA capacity |
| E6 | Does bandwidth matter? | Bandwidth | 5, 15, 30 MHz | Separate PRB from control limits |
| E7 | Does correlation matter? | Capability and budget | independent, advanced-on-tight | Test fairness claims |
| E8 | Does the result scale? | Population | full profile, density 5 | Scale and runtime |
| E9 | How sensitive is the answer? | Detection, MsgA, DG decode, CG sync, fallback | 0.90/0.99, 0.0/0.01, fallback on/off | Sensitivity, not a headline result |

E3 computes `K_min` from the retry path for each point. The C_max=7 points at D=50 and
D=100 need more than 4,000 slots. If the run cap is below `K_min`, the point is labeled
`horizon-limited` and is not used in the main claim.

The stage-2 objective uses the supported strata. The low-support strata in E2 at density 2
and in any E7 point with a small marginal are reported separately. They are not optimized
and are not used for a population claim.

The main comparison uses the cluster with `C_max = 19` and density 30. The realized
active count is 19 or 18. The E1 headline runs at this point. The scale run uses the full
profile with `C_max = 64` and density 5. The realized active count is 58 or 57. E3 reports
the realized group size with each point.

The E1 headline uses the full cohort. Each strict arm allows one access type and no
capability fallback. A UE without the type is unassigned and counted in the all-UE
denominator. The six strict arms are 4-step CBRA, 4-step CFRA, 2-step CBRA, 2-step CFRA,
CG, and DG. The joint arm uses the full menu. The 4-step CBRA-only arm is the 3GPP
baseline.

The secondary E1 analyses are the four family arms, the 4-step capability fallback
variant, the eligible-only subset, and the 2-step split. The eligible-only analysis
changes the denominator and is labeled as conditional. The headline does not put an
eligible-only subset against the full cohort.

Every sweep point runs at least three seeds. The pilot run sizes the main sample. The
evaluation seeds stay untouched until the main run.

Feasibility checks run before any scoring:

- every UE has at least one allowed option, or an explicit policy or budget reason,
- the hop schedule has the declared duty,
- the PRACH and control pools fit the declared load,
- the horizon covers the retry path, or the censored rate is below 1 percent.

An infeasible point stays in the matrix as a labeled stress case. The study does not
drop it silently.

## 15. Backlog

The study no longer includes post-decision re-allocation. This backlog holds only
conditional and future items.

### B-1 ns-3 validation slice for 4-step access

**Status:** conditional. See section 11.

If a reviewer requires full-stack validation, build a small ns-3 validation that runs the
4-step path the stack supports. Compare its timing distribution with the abstract model.
State which procedure stages the stack does not model. Do not move the main engine.

### B-2 PHY and dynamic hop pattern

**Status:** not started. Outside version 5.

Add SINR, link adaptation, and a hop pattern that the allocator can optimize. This is the
point where ns-3 becomes the better engine, because PHY behavior enters the model.

## 16. Related work, similar work, and duplicate assessment

### 16.1 Search scope

Search date: 2026-11-05.

Sources: arXiv, ACM Digital Library, IEEE Xplore, ScienceDirect, MDPI, Semantic
Scholar, and DBLP.

Queries:

1. LEO satellite handover random access resource allocation delay budget
2. RACH-less handover LEO satellite resource allocation
3. 2-step random access NTN satellite handover
4. quasi-Earth-fixed cell satellite handover
5. beam hopping resource allocation LEO satellite handover
6. joint access type and resource allocation non-terrestrial network handover
7. contention-free preamble allocation handover LEO satellite
8. random access resource allocation satellite handover heterogeneous users

Most sources were inspected at abstract level. The closest paper was paywalled, so its
full text was not read. The negative duplicate result in section 16.4 is therefore
provisional.

### 16.2 Prior work

These works establish the setting or one ingredient. None solves the joint problem.

| Work | Contribution | Relation to this study |
|---|---|---|
| Hozayen, Darwish, Karabulut, Yanikomeroglu, "A Graph-Based Customizable Handover Framework for LEO Satellite Networks", IEEE GC Wkshps 2022, arXiv:2211.07872 | Handover timing and target selection in a QEFC scenario through a time-based graph and shortest path | Establishes QEFC as a studied scenario. No random-access resource allocation |
| Koutsopoulos, Tassiulas, "A Unified Framework for Handover Prediction and Resource Allocation in Non-Geostationary Mobile Satellite Networks", IEEE VTC 1999 Fall, vol. 4, pp. 2106-2110 | Joint handover prediction and resource allocation for non-geostationary networks | Early joint formulation. No 2-step or RACH-less access, no PRB grid |
| Kim, Lee, Kim, "Enhanced RACH-less conditional handover for LEO intra-satellite system", ICT Express 12(1):142-146, 2026, DOI 10.1016/j.icte.2025.08.004 | RACH-less CHO with UL grant start timing predicted from antenna gain | Establishes RACH-less access as a viable handover type. One access type, no allocation across UEs |
| Le, Hassan, Lansard, Yuen, "A Random Access Protocol for Mixed-Traffic in LEO Satellite-Based IoT Communication", IEEE VTC2024-Spring, 2024 | JRA-PA: joint random access and preamble allocation for high-priority and low-priority traffic | Establishes preamble partitioning under mixed priorities. No handover, no PRB allocation |
| Choi, "On Fast Retrial for Two-Step Random Access in MTC", IEEE Internet of Things Journal, 2021 | Analytical model of fast retrial for 2-step random access | Establishes 2-step retrial analysis. Not NTN handover |
| 3GPP TR 38.821, TS 38.321, TS 38.300 | NTN scenarios, 4-step and 2-step random access, RACH-less handover | Standard baseline for the access types and timers |

### 16.3 Similar work

These works overlap with part of the problem.

| Work | What it does | Overlap | Difference | Evidence |
|---|---|---|---|---|
| Wang, Wen, Lu, Jing, Zhang, "Random access optimization for initial access and seamless handover for 5G-satellite network", Computer Networks 214:109176, 2022, DOI 10.1016/j.comnet.2022.109176 | Proposes 2-step random access for satellite (TRAS) and an MC-controlled handover (SMHOS) that reserves PUSCH time-frequency resources for the target | 2-step access and reserved time-frequency resources for handover | Fixes 2-step for all UEs. No per-UE choice among 4-step, CFRA, and RACH-less. No shared PRB-grid allocation across mixed capabilities. No per-UE delay-budget objective | Abstract |
| Chen et al., "Incentivizing Cooperation for Handover Strategies in LEO Constellations via Fairness-guided MARL", ACM Transactions on Internet Technology 26(1), 2026, DOI 10.1145/3760527 | Fairness-guided multi-agent reinforcement learning that selects the handover target and sub-channel | Joint handover and resource selection. Fairness objective | No access-type choice. No random-access procedure. Learning, not a one-shot allocation over a PRB grid | Abstract |
| "Beam-Hopping Pattern Design for Grant-Free Random Access in LEO Satellite Communications", arXiv:2508.03391, 2025 | Binary optimization of the beam-hopping pattern to maximize the minimum successful transmission probability across cells | Beam hopping and grant-free access | No handover batch. No per-UE access-type choice. Pattern design, not resource allocation for access | Abstract |
| He, Ying, You, Gao, "PreHO: Predictive Handover for LEO Satellite Networks", arXiv:2603.07987, 2026 | Predictive handover planning with alternating optimization and dynamic programming | Plans a handover batch in advance | Timing and target planning only. No random-access resource allocation | Abstract |
| Lee, Park, Park, Molisch, "Handover Protocol Learning for LEO Satellite Networks: Access Delay and Collision Minimization", arXiv:2310.20215, 2023 | Deep reinforcement learning handover protocol that modifies measurement reporting to reduce access delay and collisions | Access delay and collision reduction | Protocol learning, not resource allocation. No 2-step or RACH-less selection | Abstract |
| Gou et al., "Beam-Hopping Scheduling for High-Priority and Time-Sensitive Services in Low-Earth-Orbit Satellite Networks", Sensors 26(19):6081, 2026, DOI 10.3390/s26196081 | Beam-hopping scheduling for high-priority and time-sensitive services | Beam-hopping scheduling under service requirements | No handover. No random-access procedure | Abstract |

### 16.4 Exact duplicate assessment

No inspected work combines all four elements:

1. a QEF handover batch at a target satellite,
2. a per-UE choice among 4-step, 2-step, and RACH-less CG/DG access,
3. a shared PRB-by-slot allocation across cells and UEs,
4. per-UE access delay budgets.

The nearest work for the batch allocation is Wang et al. 2022. It reserves PUSCH
time-frequency resources for handover and uses 2-step access. It does not choose the
access type per UE or allocate a shared PRB grid.

The nearest work for joint handover and resource allocation is Koutsopoulos and
Tassiulas 1999. It formulates the joint problem but does not model the access types or
per-UE budgets of this study.

This is a provisional negative result. The search was not exhaustive, and the closest
papers were inspected at abstract level. Absence of a duplicate is not proof of priority.

Novelty confidence: moderate for the combination, low for any single ingredient.

What would weaken the claim:

- A paper that jointly selects access types and PRBs for handover.
- A paper that proves a fixed access rule is optimal for the same objective.
- A paper that reduces the QEF batch problem to a single-cell problem with no loss.

## 17. Assumptions, risks, and open items

| ID | Item | Status | Effect if wrong |
|---|---|---|---|
| A-1 | The incoming context is prepared before the epoch | Assumption from the intent | Preparation delay would add to every budget |
| A-2 | The target knows the capabilities and budgets at the epoch | Design assumption | Stale or missing data would add a report delay |
| A-3 | Cells are independent for access | Declared | Inter-cell interference would change collision and detection rates |
| A-4 | The target starts empty | Scope choice | Pre-existing traffic would change the residual capacity |
| A-5 | Spectral efficiency is constant | Declared | Link adaptation would change the capacity table |
| A-6 | The hop pattern is fixed and known | Design choice | An optimized pattern would change the wait floor |
| A-7 | MsgA success is 0.99 | Declared | No calibrated source in this study |
| A-8 | 2-step CFRA uses the same MsgB window as CBRA | Declared | A different window would change the completion time |
| A-9 | The cell is fixed by the UE position | QEF cell model | A cell change would require a new handover decision |
| R-1 | The abstract model may not match a real gNB | Risk | The validation gates bound the claim |
| R-2 | The exact reference is exponential | Risk | Tiny cases only |
| R-3 | The heuristic may be far from the bound at scale | Risk | Report the tiny-instance gap, not a near-optimality claim at scale |
| R-4 | The density is an average over the footprint | Risk | A hotspot would change the per-cell load |

Open items for the user, with the defaults chosen here:

1. Reference bandwidth (15 MHz) and band. The intent gives no value.
2. Reference density for the main comparison (30 persons/km^2 on the cluster).
3. Terminal penetration (0.001) and active fraction (1.0).
4. Whether the ns-3 validation slice is needed for a reviewer.

## 18. Standards and references

### 18.1 Standards

| Document | Use in this design |
|---|---|
| 3GPP TR 38.821, section 6.1 | LEO-600, S-band, 5 MHz geometry sanity check |
| 3GPP TS 38.321 | 4-step and 2-step random access, RAR and contention timers |
| 3GPP TS 38.300, section 9.2.3.6 | RACH-less handover |
| 3GPP TS 38.306 | `rach-LessHandoverCG-r18` and `rach-LessHandoverDG-r18` capability names |
| 3GPP TS 38.331 | RACH configuration and preamble pools |
| 3GPP TS 38.211 and TS 38.213 | PRACH formats and response-window timing |
| 3GPP TS 38.104, table 5.3.2-1 | Exact transmission bandwidth configuration |

The timers in this design follow the structure of these documents. They are not a
conformance claim. A conformance check needs a clause-to-behavior matrix and a
full-stack reference, which this study does not build.

### 18.2 Literature

| Work | Identifier |
|---|---|
| Hozayen et al., graph-based QEFC handover framework | arXiv:2211.07872 |
| Koutsopoulos and Tassiulas, joint handover and resource allocation | IEEE VTC 1999 Fall, vol. 4, pp. 2106-2110 |
| Kim et al., enhanced RACH-less CHO | DOI 10.1016/j.icte.2025.08.004 |
| Le et al., JRA-PA mixed-traffic random access | IEEE VTC2024-Spring |
| Wang et al., random access optimization for satellite handover | DOI 10.1016/j.comnet.2022.109176 |
| Chen et al., fairness-guided MARL handover | DOI 10.1145/3760527 |
| Beam-hopping pattern design for GFRA | arXiv:2508.03391 |
| He et al., PreHO predictive handover | arXiv:2603.07987 |
| Lee et al., handover protocol learning | arXiv:2310.20215 |
| Gou et al., beam-hopping scheduling for time-sensitive services | DOI 10.3390/s26196081 |
| Choi, fast retrial for 2-step random access | IEEE Internet of Things Journal, 2021 |

## 19. Deliverables

1. This design document.
2. A Python package with six access procedures and the resource ledger.
3. The batch allocator algorithm with pseudocode and time and space complexity bounds.
4. A deterministic plan replay check.
5. A per-cell exhaustive reference and a lower bound for tiny instances.
6. A metrics and experiment toolchain with confidence intervals.
7. A validation record with the gate results and the exact limits.
8. An experiment report with the access mix, the violation rate, and the crossover
   points.

The study does not deliver a physical calibration, a full-stack reference, or a
near-optimality claim at scale. Those are separate stages.
