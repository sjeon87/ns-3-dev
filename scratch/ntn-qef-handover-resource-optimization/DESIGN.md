# Quasi-Earth-fixed NTN handover resource allocation: design

**Version:** 3.

**Date:** 2026-11-05.

**Status:** design only. No code exists in this folder yet.

**Scope note:** this document is self-contained. It cites standards and papers, but it
does not point to local folders or files outside this study.

Where this document differs from the intent statement, this document takes precedence.

## Changes in version 3

1. Realized the post-decision re-allocation problem from the version 2 backlog.
2. Realized multi-cell candidate selection on one satellite.
3. Kept the ns-3 validation slice conditional.
4. Added two preemption policies and three balancing policies, all compared.
5. Added the arrival process and the commitment horizon as inputs.
6. Added re-allocation and multi-cell KPIs, artifacts, and experiments.
7. Extended the algorithm and complexity for candidate cells and re-planning.
8. Added related work on re-allocation and arrival-aware management.
9. Left only conditional and future items in the backlog.

## 1. Problem and objective

A target satellite serves a quasi-Earth-fixed (QEF) footprint with spot-beam cells.
At a handover epoch, a batch of incoming UEs must access the target. Each UE has an
access capability profile and a delay budget. The target activates only C cells at a
time through a fixed beam-hopping schedule.

The study has two decisions.

The first decision is the initial batch plan. For every incoming UE, choose a candidate
cell, an access type, and a time-frequency resource. The choice must respect the UE
capability, the candidate set, the cell active mask, the cell bandwidth, and the
random-access resources.

The second decision is the re-allocation after a new UE arrives. A new UE can attach or
connect after the initial plan. The target must decide whether to re-plan, which leases
to move, and how to charge the disruption to the UEs that already hold a plan.

The study also compares re-plan policies. Two preemption policies and three balancing
policies form a six-policy matrix. All six are implemented and compared.

The target starts empty. This version does not model pre-existing traffic. A loaded
target is outside the scope.

### 1.1 Meaning of optimal

"Optimal" in this study means optimal for a declared discrete model. The model sets
five things:

1. the finite option set per UE,
2. the slot grid and the resource granularity,
3. the random seed and the resulting random draws,
4. the arrival trace and the commitment horizon,
5. the lexicographic objective in sections 9.3 and 9.12.

The study does not claim optimality for the stochastic network, for the real protocol,
or for any continuous relaxation. Section 9.5 states the reference classes and their
limits.

### 1.2 Research questions

1. Which access type and candidate cell should each UE use when capabilities and delay
   budgets differ?
2. How much does joint access, cell, and resource selection improve over a fixed rule?
3. How do the active-cell count and the beam dwell change the access mix and the
   violation rate?
4. When does contention bind, and when do control and preamble constraints bind?
5. When a new UE arrives, which re-plan policy gives the best service without
   excessive churn?
6. Does allowing preemption of prepared or committed leases beat a policy that moves
   only uncommitted leases?
7. How close is the online allocator to the exact reference on tiny instances?

### 1.3 Hypotheses

These are tests, not assumptions.

- H1: Dedicated access (CFRA and CG) reduces completion time, but it consumes scarce
  dedicated resources. The best mix changes with load.
- H2: 2-step access helps tight-budget UEs. Its fallback can increase contention when
  many UEs fail MsgA in the same occasion.
- H3: The beam schedule sets a hard lower bound on completion time. A budget below that
  bound is infeasible for some cells, whatever the access type.
- H4: Candidate-cell selection helps edge UEs, because a neighboring cell can be active
  sooner. The gain grows with the duty-cycle imbalance across cells.
- H5: Allowing preemption improves the new UE's success rate. The gain falls as the
  commitment horizon grows, because more leases are frozen.

## 2. Inputs, derived inputs, and outputs

### 2.1 Inputs

| Input | Symbol | Unit | Default | Role |
|---|---|---|---|---|
| Satellite altitude | `h` | km | 600 | Geometry |
| Minimum elevation | `theta_min` | deg | 30 | Full profile |
| Cell circumradius | `r_c` | km | 25 | Geometry |
| Cell layout | - | rings or derived | derived | Full profile |
| Candidate cells per UE | `Q` | count | 3 | Multi-cell |
| Candidate edge margin | `m_e` | km | 0 | Multi-cell |
| Active cells | `C` | count | 19 (cluster), 64 (full) | Hop |
| Beam dwell | `D` | ms | 20 | Hop |
| Cell bandwidth | `B_c` | MHz | 15 | Radio |
| Subcarrier spacing | `SCS` | kHz | 15 | Radio |
| Population density | `rho` | persons/km^2 | 30 (cluster), 5 (full) | Population |
| Terminal penetration | `pi` | fraction | 0.001 | Population |
| Active UE fraction | `alpha` | fraction | 1.0 | Population |
| Capability profile shares | - | fractions | Table 6.2 | Population |
| Delay budget classes | `B_u` | ms | 100/200/400/800 | Population |
| New-UE arrival mode | - | poisson or fixed | poisson | Re-allocation |
| Arrival rate | `lambda_f` | fraction of `N_in` per s | 0.05 | Re-allocation |
| First arrival time | `t_first` | ms | 100 | Re-allocation |
| Commitment horizon | `H_c` | ms | 20 | Re-allocation |
| Preemption policy | - | two values | both compared | Re-allocation |
| Balancing policy | - | three values | all compared | Re-allocation |
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
| UEs per cell | `N_in(c) = round(rho * pi * alpha * A_c)` |
| Total incoming UEs | `N_in = sum over cells of N_in(c)` |
| PRB count | `N_PRB = floor(B_c / (12 * SCS))` |
| Candidate set | cells whose center distance to the UE is at most `r_c + m_e`, capped at `Q` |
| Initial batch | `N_in` UEs at the epoch |
| Arrival process | Poisson with rate `lambda_f * N_in` per second, or one fixed batch |

### 2.3 Outputs

| Output | Form |
|---|---|
| Initial per-UE assignment | `(cell, access, start slot, PRB range, occasion, preamble, CG/DG slot)` |
| Re-allocation decision | accept or reject, moved leases, new-UE placement |
| Policy comparison | six-policy matrix results, section 14 |
| Batch algorithm | pseudocode in section 9.6 |
| Batch complexity | time and space bounds in section 9.7 |
| Re-allocation algorithm | pseudocode in section 9.10 |
| Re-allocation complexity | time and space bounds in section 9.11 |
| KPIs | Section 10 |
| Exact-reference gap | Tiny instances only, section 9.8 |

The algorithms and their complexity are deliverables, not simulation artifacts. The
study reports them with the code.

## 3. Scope

### 3.1 In scope

| Item | Treatment |
|---|---|
| Target satellite | One satellite with a QEF footprint and a beam-hopping payload |
| Cells | Hexagonal cells inside the footprint, fixed on the ground |
| Candidate cells | Up to `Q = 3` cells that cover the UE, one chosen per UE |
| Incoming UEs | Batch released at the epoch, with capabilities and delay budgets |
| New UEs after the plan | Poisson or fixed-batch arrivals, each may trigger a re-plan |
| Access types | 4-step CBRA, 4-step CFRA, 2-step CBRA, 2-step CFRA, RACH-less CG, RACH-less DG |
| Resources | PRB by slot grid, PRACH occasions, contention and dedicated preambles, PDCCH CCEs, PUSCH and PDSCH PRB-symbols |
| Decisions | One initial batch decision, then an event-driven re-plan at each arrival |
| Re-plan policies | Two preemption policies and three balancing policies, all compared |
| Algorithms | Greedy marginal-benefit allocator with a swap pass, plus incremental re-plan |
| Outputs | Assignments, re-plan decisions, algorithm, complexity, KPIs |

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
| Multiple satellites | Version 3 adds candidate cells on one satellite, not multi-satellite selection |
| Pre-existing target traffic | The target starts empty in this version |
| HARQ for data and RLC | Access retries are modeled. Data retransmission is not |
| Multi-satellite visibility | Deferred to the backlog, section 15 |

## 4. Decision log

| ID | Question | Decision | Why |
|---|---|---|---|
| D-1 | Where does the study live? | This folder, as new code | The study is independent |
| D-2 | What is the initial decision scope? | One batch decision at the epoch | The target plans once, then executes |
| D-3 | What does the delay budget bound? | Arrival or epoch to access completion | The study measures access, not application packets |
| D-4 | How detailed are the access models? | Abstract event model, calibrated | 2-step and RACH-less paths have no usable full-stack model here |
| D-5 | Does the target start empty? | Yes. Pre-existing traffic is removed in version 2 | The batch problem is isolated first |
| D-6 | What is the resource granularity? | PRB by slot per cell and direction, plus control and PRACH pools | The output must name concrete resources |
| D-7 | What is the geometry? | Static snapshot with per-UE elevation and propagation | The intent puts satellite motion out of scope |
| D-8 | How does beam hopping work? | Round-robin cell groups, C active, fixed dwell | The intent gives the active-cell count as an input |
| D-9 | Does a UE choose a cell? | Yes, from up to three candidate cells | Realized from the version 2 backlog |
| D-10 | Which access types are modeled? | All six named in the intent | The comparison is the core of the study |
| D-11 | What optimizer class? | Greedy marginal-benefit heuristic plus an exact reference on tiny instances | The study needs a deployable policy and a bound |
| D-12 | How is the population sized? | Density times penetration times active fraction | The intent asks for a density input |
| D-13 | What implementation environment? | Python, not ns-3 or MATLAB | Section 11 gives the evaluation |
| D-14 | What happens after a new UE arrives? | Event-driven re-plan, local first, global fallback | Realized from the version 2 backlog |
| D-15 | Which leases may move? | Both policies are compared: uncommitted only, and any lease with compensation | The user asked for the comparison |
| D-16 | How is the new UE balanced against existing UEs? | All three policies are compared: total-first, existing-first, stability-first | The user asked for the comparison |
| D-17 | How many candidate cells? | Up to three on one satellite | The user chose this scope |
| D-18 | Is the ns-3 validation realized? | No. It stays conditional | The user chose this scope |
| D-19 | How do new UEs arrive? | Poisson by default, with a fixed-batch test mode | A general process plus a deterministic test |

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

Two reference profiles use 25 km cells:

| Profile | `N_cells` | Footprint radius | Edge elevation | Edge delay | Purpose |
|---|---:|---:|---:|---:|---|
| cluster | 37 | 129.9 km | 76.6 deg | 2.052 ms | Mechanism, exact reference, re-allocation |
| full | 1373 | 853.47 km | 30.0 deg | 3.586 ms | Scale and geometry |

The cluster is three hex rings around the center cell (`1 + 6 + 12 + 18 = 37`). It is a
reduced instance near nadir, not a miniature of the full footprint. Its edge elevation
is high and its delay range is narrow. The full profile covers the 30-degree edge.

The table reports the radius to the outermost cell center. The outer cell edge extends
about one cell radius beyond it. The edge elevation and delay are evaluated at the
reported radius, not at the outer edge. The model still computes each UE delay from its
own position.

The cluster declares `rings = 3` and derives its footprint radius from the ring layout.
The full profile derives `N_cells` from the lattice count within `R_f`. The resolved
scenario records the derivation and the resulting count.

### 5.3 Beam-hopping schedule

The satellite activates `C` cells at a time. The model divides the `N_cells` cells
into `G = ceil(N_cells / C)` groups. Group `g(c) = c mod G` is active in hop period
`floor(k/D) mod G`, where `k` is the slot index and `D` is the dwell in slots. The
schedule repeats every `G * D` slots.

The model uses a fixed round-robin order. It does not optimize the hop pattern. Group
membership follows the cell index, not a reuse pattern. Cell reuse is a declared
scenario input and does not change the access model.

| Profile | N_cells | C | D (ms) | G | Cycle (ms) | Duty |
|---|---:|---:|---:|---:|---:|---:|
| cluster | 37 | 7 | 20 | 6 | 120 | 18.9% |
| cluster | 37 | 19 | 20 | 2 | 40 | 51.4% |
| cluster | 37 | 37 | 20 | 1 | 20 | 100% |
| full | 1373 | 64 | 20 | 22 | 440 | 4.66% |

Dwell values for the sweep are 5, 10, 20, 50, and 100 ms. Active-cell values are 7,
19, 37, and 64. A 1 ms dwell is not a default, because the round-trip propagation delay
is longer than the dwell.

The hop schedule is a hard constraint. A UE in an inactive cell cannot transmit. It
waits for the next active dwell of its chosen cell. That wait is part of the access
latency, even when the UE has a dedicated grant.

### 5.4 UE placement

Each UE is placed uniformly at random inside its serving cell, using an area-uniform
draw on the hexagon. The population generator draws `N_in(c)` UEs for each cell. For an
imported population, the serving cell is the cell with the nearest lattice center. A UE
outside every cell is not created. The generator reports the number of rejected draws.

The default placement is uniform, not edge-heavy. An edge-heavy sensitivity study uses
a separate draw toward the cell boundary, which also raises the candidate-set size.

### 5.5 Candidate cells

A UE may be covered by more than one cell. The model computes a candidate set at the
decision instant:

```text
CandidateCells(u) = { c : distance(u, center(c)) <= r_c + m_e }, capped at Q
```

The serving cell is the nearest center. It is always in the set. A UE in the interior
has one candidate. A UE on an edge has two. A UE on a vertex has three.

`Q` defaults to 3. `Q = 1` reproduces the single-cell version. `m_e` defaults to 0,
which means strict hexagonal coverage. A positive `m_e` adds overlapping cells and can
raise the candidate count, but the cap `Q` still applies.

The candidate set is static during a procedure, because the UE and the cells are
static. The propagation delay does not depend on the chosen cell, because one satellite
serves all cells. The cell choice changes the hop wait and the per-cell resources.

A cell switch is a plan change. The model counts it as churn. A switch to a cell that
is active sooner can reduce the completion time. A switch to a loaded cell can raise
contention. Both effects are measured.

## 6. Population

### 6.1 Population density

The population input is a density in persons per square kilometre. The generator
converts it to UEs with a terminal penetration and an active fraction:

```text
N_in(c) = round( rho * pi * alpha * A_c )
```

where `rho` is persons per km^2, `pi` is the terminal penetration, `alpha` is the
active fraction, and `A_c` is the cell area in km^2.

The density is an average over the footprint. A footprint that mixes land and ocean
has a low average density. A hotspot sensitivity study uses a spatial distribution
instead of a uniform one. The distribution is a declared control.

Reference population profiles:

| Profile | `rho` | `pi` | `alpha` | UEs per cell | Total UEs |
|---|---:|---:|---:|---:|---:|
| cluster | 30 | 0.001 | 1.0 | 48.7 | 1,802 |
| full | 5 | 0.001 | 1.0 | 8.1 | 11,147 |

The cluster uses a higher density to create contention in 37 cells. The full profile
uses a lower average density to keep the episode size manageable. The density is an
input, not a fixed property of the geometry.

The load sweep uses `rho = 2, 10, 30, 100` on the cluster and `rho = 1, 5, 10` on the
full profile. At `rho = 100` on the cluster, the load is about 162 UEs per cell.

### 6.2 Capability profiles

Every UE supports 4-step access. The other types are optional.

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

For an initial UE, the budget applies from the epoch to the access completion event.
For a new UE, it applies from its arrival to the access completion event. Both include
the decision latency, the wait for the chosen cell's active window, and the access
procedure.

### 6.4 Feasibility precheck

The model checks one condition before it runs the allocator. A UE whose budget is below
its minimum possible completion time is marked `budget-infeasible`. The minimum includes
the decision latency and the wait for the next active dwell of any candidate cell.

The marked UE stays in the run and in the KPI denominator. It gets an explicit reason
code. The study does not remove it or move it to a different cell.

### 6.5 Arrival process

New UEs arrive after the initial plan. The model supports two modes.

| Mode | Behaviour | Use |
|---|---|---|
| poisson | Independent arrivals at rate `lambda_f * N_in` per second, starting at `t_first` | Main study |
| fixed | One batch of `N_arrivals` UEs at `t_first` | Deterministic tests and exact cases |

A new UE gets a cell from its position, a candidate set, a capability profile, and a
budget. Its budget starts at its arrival time. Its context creation costs one control
message and `context_creation_ms` (default 1 ms).

The arrival trace is generated from the episode seed before the run starts. The trace is
recorded, so a re-plan comparison uses the same arrivals for every policy.

The default `lambda_f` is 0.05. That is 5% of the initial population per second. For
the cluster (1,802 UEs), it gives about 90 new UEs per second. That is one arrival every
11 ms. The sweep uses 0.01, 0.05, and 0.20.

## 7. Resource model

### 7.1 Time base

The slot duration is `1 ms` at 15 kHz subcarrier spacing. The horizon is `K` slots and
starts at the epoch (`k = 0`). The default horizon is 2000 slots (2 s). The horizon
must cover the largest budget plus the longest access procedure. A run that reaches the
horizon with pending UEs reports them as censored failures.

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

| Bandwidth | PRB | UL PRB-symbols | Instantaneous UL rate | Full-profile average | Cluster C=19 average |
|---:|---:|---:|---:|---:|---:|
| 5 MHz | 27 | 324 | 15.6 Mbps | 0.73 Mbps | 8.0 Mbps |
| 15 MHz | 83 | 996 | 47.8 Mbps | 2.23 Mbps | 24.5 Mbps |
| 30 MHz | 166 | 1992 | 95.6 Mbps | 4.45 Mbps | 49.1 Mbps |

The average column uses the duty cycles from section 5.3. These numbers are a sanity
check. They are not measured capacity.

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

| Access | PRACH | Preamble | DL CCE | DL PRB-symbol | UL PRB-symbol | Nominal completion |
|---|---|---|---|---|---|---|
| 4-step CBRA | 1 | 1 contention | 1 RAR + 1 CR | 8 RAR + 8 CR | 12 Msg3 | `t_occ + 2 + tau + 8 + 1 + tau + 8` |
| 4-step CFRA | 1 | 1 dedicated | 1 RAR + 1 CR | 8 RAR + 8 CR | 12 Msg3 | same minus contention timer |
| 2-step CBRA | 1 | 1 contention | 2 MsgB | 8 MsgB | 12 MsgA | `t_occ + 2 + tau + 1 + tau` |
| 2-step CFRA | 1 | 1 dedicated | 2 MsgB | 8 MsgB | 12 MsgA | same as 2-step CBRA |
| CG | 0 | 0 | config cost | 0 | 12 PUSCH | first valid CG occasion + 1 + tau |
| DG | 0 | 0 | 1 grant | 0 | 12 PUSCH | grant slot + 8 + 1 + tau |

`t_occ` is the chosen PRACH occasion. `tau` is the one-way delay for that UE. Times are
in slots of 1 ms. The values 2, 8, and 1 are the detection slots, the grant offset, and
the transmission slot.

The CG configuration cost is a declared number of control bytes. The model charges it
once per UE at the decision epoch. It also charges the PDCCH occasion that activates
the grant when the scenario enables activation signaling.

The exact footprints are parameters. The design sets the accounting, not the numbers.
Section 8.7 lists the defaults and their basis.

### 7.5 Conservation rules

The engine enforces four rules:

1. Every PRB-symbol has at most one owner in a cell, slot, and direction.
2. A lease holds its capacity until it is consumed, released, or expired. An unused
   lease still blocks capacity.
3. A preamble index is unique inside one occasion and one cell.
4. A UE consumes resources only in slots where its chosen cell is active.

The engine writes a per-slot ledger for every cell. The metrics tool rebuilds the totals
from the ledger and compares them with the counters. A mismatch fails the run.

### 7.6 Worked capacity example

Take the cluster at 15 MHz, `C = 19`, and about 49 UEs per cell (density 30 persons per
km^2, penetration 0.001). Each cell is active for 20 ms in every 40 ms cycle. The UL
capacity is 996 PRB-symbols per active slot. One Msg3 uses 12 PRB-symbols, so the PRB
limit is 83 Msg3 per slot. The RAR processing limit is 8 per slot. The model spreads
RARs across the 10-slot RAR window, so the control capacity is 80 RARs per occasion.

With a 10 ms PRACH period, the cell has two occasions per dwell. At 49 UEs per cell and
64 preambles, the expected number of singleton preambles is

```text
n * (1 - 1/M)^(n-1) = 49 * (63/64)^48 = 23.0
```

So about 23 UEs can succeed per occasion under contention alone. That is below the
control capacity of 80 and below the PRB capacity. The first dwell is therefore
contention-limited. At 200 UEs per cell (density about 123), the expected singletons
fall to 8.7 per occasion. That is still below 80, so contention remains the binding
limit. The control limit binds only when the per-slot RAR demand exceeds 8, for example
with a short RAR window or a small `max_rar_per_slot`. The study sweeps that setting.
The crossing between contention and control is one of its testable mechanisms.

### 7.7 Lease states and commitment

A lease carries a version and a state. The state controls whether a re-plan may move it.

| State | Meaning | Move under uncommitted-only | Move under any-with-compensation |
|---|---|---|---|
| planned | Allocated, no configuration sent | Yes | Yes |
| prepared | Configuration sent to the UE | No | Yes, with cost |
| committed | Start slot is inside the commitment horizon | No | Yes, with cost |
| consumed | The UE has transmitted | No | No |
| expired | The lease expired | No | No |

A lease becomes `committed` when its start slot is at most `H_c` slots ahead. The
default `H_c` is 20 ms. A lease outside the horizon stays `prepared` or `planned`.

The reconfiguration cost is one control message and `reconfig_delay_ms` (default 5 ms).
A moved `prepared` or `committed` lease releases its old resources at the re-plan
instant. The UE's new start is at or after `now + reconfig_delay_ms + preparation`. The
old resources may be reused by another UE. The wasted reservation is counted.

Each move increments the lease version. The engine rejects a stale version. The old
configuration is cancelled with a reconfiguration message.

## 8. Access model

### 8.1 Common timing

| Timer | Default | Meaning |
|---|---:|---|
| Slot | 1 ms | Resource grid step |
| PRACH period | 10 slots | Occasion spacing |
| Detection | 2 slots | gNB detection and RAR preparation |
| RAR window | 10 slots | Maximum wait for a RAR or MsgB |
| RAR grant offset | 8 slots | Grant transmit start to Msg3 receive start |
| Msg3 transmission | 1 slot | PUSCH duration |
| Contention resolution timer | 8 ms | CR wait after Msg3 |
| Processing | 1 ms | Response preparation |
| DG grant offset | 8 slots | Grant to PUSCH receive start |
| CG period | 8 slots | Configured-grant period |
| Max attempts | 10 | Attempt limit before failure |

These timers are declared study settings. They follow the 3GPP random-access structure,
but they are not measured from a specific stack.

The propagation delay is added once per direction. A request from the UE arrives after
`tau`. A response from the gNB arrives after `tau`. The model does not add the two
delays as one round-trip constant, because `tau` differs by UE.

### 8.2 4-step CBRA

1. The UE waits for the next PRACH occasion of its chosen cell.
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
DL CCEs for MsgB. Its nominal completion is shorter because it removes the Msg3 round
trip.

### 8.5 RACH-less CG

The target configures a Type-1 configured grant with period `P_cg` and a per-UE offset.
The UE transmits at the first valid occasion at or after its configuration is usable.
RACH-less access has no preamble and no RAR.

The model charges:

- one configuration message at the decision, with a declared byte count,
- 12 PUSCH PRB-symbols at the chosen occasion,
- the reserved occasion itself, even if the UE misses it.

A CG attempt fails when no valid occasion exists before `min(budget, horizon)`. It also
fails on a declared synchronization error with probability `p_sync`.

CG has low latency when its occasion is soon. Its cost is the unused reservation when
the UE is late or fails.

### 8.6 RACH-less DG

The UE monitors PDCCH for a dynamic grant. The target must allocate one CCE and 12
PUSCH PRB-symbols in the grant slot. The UE transmits at the granted offset.

DG fails when no grant arrives before `min(budget, horizon)`, or when the grant is not
decoded (probability `p_dg`). DG has no reservation waste, but it depends on the control
pool at the moment of the grant.

### 8.7 Attempt, success, failure, and calibration

| Term | Definition |
|---|---|
| Attempt | One PRACH or MsgA transmission. CG and DG have no RACH attempts |
| Re-attempt | Any attempt after the first for the same UE |
| Access success | The UE reaches `CONNECTED_TARGET` before the horizon |
| Access failure | The attempt limit is reached, or the CG or DG deadline expires |
| Budget violation | Failure, or success after the delay budget |
| Censored | Still pending at the horizon. Counted as a failure and reported separately |

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
| MsgB window | 10 slots | Same as the RAR window |
| Context creation | 1 ms | Declared. For a new UE after the initial plan |
| Reconfiguration delay | 5 ms | Declared. For a moved prepared or committed lease |

The retry interval follows from the occasion period, the RAR window, and the backoff.
It is not copied from another study. A calibration stage must reproduce the analytical
singleton curve before it claims agreement with any external measurement.

## 9. Allocation problem

### 9.1 Decision variables

For UE `u`, candidate cell `c`, access type `a`, and start slot `k`:

```text
x[u, c, a, k] = 1 if UE u uses cell c, access a, starting at slot k, otherwise 0
```

A UE may also be left unassigned. That state is explicit and penalized.

`c` must belong to `CandidateCells(u)`. For `Q = 1`, the model reduces to the
single-cell case.

For CFRA, the model adds a preamble index `p` from the dedicated pool. For CG, it adds
the occasion index. For DG, the grant slot is the start slot.

For CBRA, the network does not choose the preamble. The UE draws it at random. The
allocator chooses only the cell, the occasion, and the contention pool. This distinction
is important. A model that lets the allocator pick CBRA preambles is modeling CFRA.

### 9.2 Constraints

For every UE `u`:

```text
sum over legal (c, a, k) of x[u, c, a, k] + unassigned[u] = 1
```

Legal means all of the following:

- `a` is in the UE capability profile,
- `c` is in `CandidateCells(u)`,
- the start slot is a valid PRACH occasion, CG occasion, or DG grant slot,
- the cell of `u` is active at every slot in which the procedure transmits or receives,
- the start slot is at or after the decision end,
- the predicted completion is within the horizon.

For every cell `c`, slot `k`, and pool `r`:

```text
sum of demand(u, c, a, k, r) * x[u, c, a, k] <= capacity(c, k, r)
```

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
| 2 | Worst-stratum violation rate | Maximum violation rate over capability and budget strata |
| 3 | Resource cost | PRB-symbols, preambles, CCEs, CG waste, and re-attempts |

The weighted violation rate is

```text
L = sum_u w_u * V_u / sum_u w_u
```

where `V_u = 1` if the UE fails or completes after its budget. The default weight is 1
for every UE. A sensitivity study uses weights by budget class. The weights are frozen
before the evaluation runs.

Stage 2 is a declared tie-breaker. It is not a hard fairness constraint. It prevents a
policy from improving the mean by sacrificing a small capability stratum.

The re-allocation objective uses the same quantities on the current plan. It adds a
churn stage. Section 9.12 defines the six policies.

### 9.4 Feasible options and pruning

For UE `u`, an option is a legal `(cell, access, start)` triple with a predicted
completion and a resource vector. The option count is bounded by

```text
Q * (occasions within budget) * (legal access types) + Q * (DG slots within budget)
```

For a 400 ms budget, `Q = 3`, a 10 ms PRACH period, five access types, and 400 DG slots,
the count can exceed 1,500 before pruning. Pruning keeps the earliest three starts per
access type and per cell, which gives at most 45 random-access and CG options plus nine
DG options.

### 9.5 Reference classes

| Reference | Size | What it establishes | Limit |
|---|---|---|---|
| Exhaustive schedule enumeration | up to 6 UEs, 2 cells, 40 slots | Exact optimum of the declared finite model | Exponential. Uses fixed random draws per schedule |
| Beam-and-latency lower bound | any size | Minimum possible violations from the hop schedule and the minimum procedure time | Ignores contention and most resources |
| CP-SAT time-indexed model | 16 to 128 UEs | Feasible plan and solver bound for a deterministic approximation | Collisions are replaced by expected values, so the bound is not certified for the simulator |

The exhaustive reference enumerates the Cartesian product of per-UE options. It caps the
option set at four per UE to keep the product tractable. For each schedule it replays
the same engine with the same seeds. It recomputes collisions for that schedule. It does
not transplant outcomes between schedules.

The beam-and-latency lower bound is simple and valid. For each UE, compute the earliest
active dwell over its candidate cells and add the minimum procedure time of any legal
access type. If that time exceeds the budget, the UE must violate. The count of such UEs
is a lower bound on the violation count. It ignores contention, so it can be loose.

A heuristic solution is not a lower bound. The study reports `LB <= optimum <= UB` only
when it has both. A gap is valid only inside the same feasible set and the same
lexicographic stage.

### 9.6 Batch algorithm

The initial allocator runs once at the epoch. It has a declared decision latency of 1 ms
by default. The first access start must be after the decision end.

```text
ALGORITHM AllocateBatch(U, cells, hop, resources, budgets, capabilities, frozen)
 1  for each u in U:
 2      options[u] = EnumerateOptions(u, candidates[u], budgets[u], hop, resources)
 3      if options[u] is empty:
 4          mark u unassigned with reason no-legal-option
 5          continue
 6      T_min[u] = min completion over options[u]
 7      slack[u] = budgets[u] - T_min[u]
 8  order = sort U by (slack ascending, len(options) ascending)
 9  ledger = BuildLedger(cells, horizon)          # capacity per cell, slot, pool
10  ledger = ApplyFrozen(ledger, frozen)           # committed leases are fixed load
11  for each u in order:
12      best = none
13      for each option o in options[u]:
14          if not Feasible(ledger, o):
15              continue
16          s = Score(o, budgets[u], ledger.prices, ledger.contention)
17          if best is none or s < best.score:
18              best = o
19      if best is none:
20          mark u unassigned with reason capacity
21      else:
22          Commit(ledger, best)
23          u.plan = best
24  for pass in 1..max_swap_passes:
25      improved = false
26      for each pair (u, v) in SwapCandidates(order, swap_cap):
27          if SwapImproves(u, v, ledger):
28              ApplySwap(u, v, ledger)
29              improved = true
30      if not improved:
31          break
32  return plans, ledger
```

The score is

```text
score = 1000 * [T > B_u]
      + lambda * max(0, T - B_u)
      + sum_r price[r] * demand[r]
      + contention_penalty
```

The contention penalty for CBRA is `-log(p_singleton(n_o))`, where `n_o` is the number
of contenders already placed on that occasion. This spreads contention. The price update
is

```text
price[r] = utilisation[r]^2 / (1 - utilisation[r] + 0.01)
```

The constants are declared and tuned on a pilot, not on the evaluation seeds.

The allocator logs every rejection with a reason code: `no-legal-option`, `capacity`,
`preamble-exhausted`, `budget-infeasible`, or `unassigned`.

The allocator is deterministic for a fixed seed and input. It does not read simulator
state that a real target would not know. It sees the UE capabilities, the budgets, the
candidate cells, the hop schedule, and its own resource ledger. It does not see future
random draws.

### 9.7 Batch complexity

| Symbol | Meaning | Default scale |
|---|---|---:|
| `U` | Incoming UEs | 1,802 cluster, 11,147 full |
| `Q` | Candidate cells per UE | at most 3 |
| `A` | Legal access types per UE | at most 5 |
| `O` | Candidate starts per access type and cell after pruning | at most 3 |
| `K` | Slots in the horizon | 2,000 |
| `C` | Cells | 37 or 1,373 |
| `R` | Resource pools per cell and slot | 6 |
| `S` | Swap candidate cap | 100 |
| `P` | Swap passes | 10 |

| Stage | Time complexity | Notes |
|---|---|---|
| Option enumeration | `O(U * Q * A * O)` with an index, `O(U * Q * K)` without | The index is built once |
| Sorting | `O(U log U)` | Slack and option count |
| Greedy assignment | `O(U * Q * A * O * R)` | Price and feasibility checks |
| Swap pass | `O(P * S^2 * Q * A * O)` | Bounded candidate pairs |
| Ledger build | `O(C * K * R)` dense, `O(occupied slots)` sparse | Sparse is the default |
| Total | `O(U*Q*A*O + U log U + U*Q*A*O*R + P*S^2*Q*A*O + occupied slots)` | The swap pass dominates at large `S` |
| Exact reference | `O(M^U * U * L)` | Exponential. `M <= 4`, `U <= 6` |
| Lower bound | `O(U * Q * A)` | Earliest dwell and minimum procedure |

With the default values and `Q = 3`, the greedy pass is about `270 * U` operations. The
swap pass is at most `10 * 100^2 * 45 = 4.5M` basic checks, which is why `S` and `P` are
capped. The space bound is `O(U * Q * A * O + occupied ledger slots)`. The ledger is
sparse, so a run does not allocate `C * K * R` entries for inactive cells.

### 9.8 What the gap claim means

On tiny instances, the study reports the exact optimum and the allocator result. The
gap is the additive difference in stage-1 violation rate, with the same option set and
the same seeds. The study does not claim a gap at full scale. At full scale it reports
the lower bound and the measured outcome.

### 9.9 Re-allocation problem

A new UE arrives at `t_a`. The target holds a current plan with leases in the states of
section 7.7. The re-allocation must decide four things:

1. whether to re-plan at all,
2. which leases may move,
3. which scope to use,
4. how to balance the new UE against the existing UEs.

The re-plan runs at `t_a + decision_latency`. The decision latency is 1 ms by default.
The new UE's context creation costs 1 ms. Its first access start is after both.

The no-re-plan baseline places the new UE only on residual resources. It moves no
existing lease. If no residual option fits, the new UE fails. The no-re-plan baseline is
the reference for the re-plan benefit.

The local-first policy tries the new UE's candidate cells first. If the local attempt
places the new UE and improves the selected objective, the target accepts it. Otherwise,
the target runs the global fallback, capped at `max_replan_ues` UEs by slack order. If
the global attempt also fails, the target keeps the current plan and marks the new UE
unassigned.

The commitment horizon `H_c` protects leases close to transmission. A lease whose start
is at most `H_c` ahead is `committed`. The default is 20 ms, one dwell. A longer horizon
freezes more leases and reduces the benefit of preemption. A shorter horizon exposes
more transmissions to change.

### 9.10 Re-allocation algorithm

```text
ALGORITHM ReplanOnArrival(u_new, now, plan, ledger, policy)
 1  candidates = CandidateCells(u_new)
 2  attempts = [local, global] if policy.scope == local-first else [global]
 3  frozen = leases that policy.preempt cannot move
 4  base = Objective(plan, policy.balance)
 5  for attempt in attempts:
 6      if attempt == local:
 7          conflict = UEs in candidates whose leases overlap u_new's feasible options
 8          subset = {u_new} + conflict
 9      else:
10          subset = {u_new} + first max_replan_ues UEs by slack
11      candidate_plan = AllocateBatch(subset, frozen, policy.balance)
12      candidate_plan = Merge(plan, candidate_plan, frozen)
13      value = Objective(candidate_plan, policy.balance)
14      if value < base:
15          return candidate_plan
16  return plan with u_new unassigned
```

`Merge` keeps every frozen lease and every lease outside the subset unchanged. It
releases the moved leases and applies the new assignments. A moved lease increments its
version. The engine rejects a stale version.

The re-plan does not change the hop schedule, the access timers, or the traffic. It
changes only the cell, access type, start slot, and resource of the UEs in the subset.

A re-plan is accepted only when the selected objective is strictly better. A tie keeps
the current plan. This prevents churn without a service gain.

### 9.11 Re-allocation complexity

| Symbol | Meaning | Default scale |
|---|---|---:|
| `N_new` | New UEs in the decision | 1 for event-driven |
| `N_conflict` | Existing UEs in the local conflict set | bounded by `max_local_ues` |
| `N_global` | Existing UEs in the global fallback | bounded by `max_replan_ues` |
| `N_arr` | Arrivals in the episode | about 90 per second at the default rate |
| `G_att` | Global fallback attempts in the episode | bounded by `N_arr` |

| Stage | Time complexity |
|---|---|
| Local attempt | `O((N_new + N_conflict) * Q * A * O * R + P * S^2 * Q * A * O + C_local * K * R)` |
| Global fallback | `O((N_new + N_global) * Q * A * O * R + P * S^2 * Q * A * O + C * K * R)` |
| Per episode | `O(N_arr * local_cost + G_att * global_cost)` |
| Space | `O((N_new + N_global) * Q * A * O + occupied ledger slots)` |

With `N_new = 1`, `N_conflict <= 20`, `Q = 3`, and `S = 100`, the local attempt is
cheap. The global fallback costs about the same as a batch run on the capped subset. The
design reports the measured re-plan latency in the experiment report.

### 9.12 Re-plan policy matrix

The study implements and compares six policies. The preemption axis controls which
leases may move. The balance axis controls the objective order.

| ID | Preemption | Balance | Objective order |
|---|---|---|---|
| R1 | uncommitted-only | total-first | total violations, then churn, then resource cost |
| R2 | uncommitted-only | existing-first | existing-UE violations, then new-UE violations, then churn, then resource cost |
| R3 | uncommitted-only | stability-first | new-UE failure, then churn, then total violations, then resource cost |
| R4 | any-with-compensation | total-first | total violations, then churn, then resource cost |
| R5 | any-with-compensation | existing-first | existing-UE violations, then new-UE violations, then churn, then resource cost |
| R6 | any-with-compensation | stability-first | new-UE failure, then churn, then total violations, then resource cost |

The preemption policies are:

- `uncommitted-only`: only `planned` leases move. `prepared` and `committed` leases are
  frozen. This is the low-churn policy.
- `any-with-compensation`: `planned`, `prepared`, and `committed` leases move. Each moved
  lease pays one control message and `reconfig_delay_ms`. `consumed` and `expired`
  leases never move. This is the high-flexibility policy.

The balance policies are:

- `total-first`: minimize the weighted violations over all UEs, then churn.
- `existing-first`: minimize the weighted violations over existing UEs first, then the
  new UE, then churn. This protects UEs that already hold a plan.
- `stability-first`: minimize new-UE failure first. If the new UE can be served, then
  minimize churn, then total violations. This keeps the plan stable unless the new UE
  would fail.

All six policies use the same arrivals, capabilities, budgets, hop schedule, and random
streams. They may differ only in the legal moves and the objective order.

## 10. KPIs and measurement

### 10.1 KPI definitions

| KPI | Definition | Denominator |
|---|---|---|
| Handover success rate | UEs that reach `CONNECTED_TARGET` / UEs | All UEs |
| Handover failure rate | UEs that reach the attempt limit, the CG or DG deadline, or the horizon | All UEs |
| Delay-budget violation rate | UEs that fail or complete after their budget / UEs | All UEs |
| New-UE violation rate | New UEs that fail or exceed budget / new UEs | New UEs |
| Existing-UE violation delta | Existing-UE violation rate after re-plan minus the no-re-plan baseline | Existing UEs |
| Time to completion | Arrival or epoch to `CONNECTED_TARGET` | Successful UEs only |
| RACH attempts | PRACH or MsgA transmissions per UE | All UEs |
| Re-attempts | Attempts minus one for UEs with at least one attempt | UEs with at least one attempt |
| Churn | Moved leases / existing UEs with a lease | Existing UEs with a lease |
| Preempted resources | PRB-symbols, preambles, and CCEs released by moved leases | Per re-plan |
| Re-plan count | Accepted and rejected re-plans | Per episode |
| Plan age | Time since the last change for a UE | Existing UEs |
| Cell load balance | Maximum per-cell PRB utilization / mean | Per episode |
| Cell switch count | UEs that changed cell in a re-plan | Existing UEs |
| Resource use | PRB-symbols, CCEs, preambles, CG waste | Per cell and slot |
| Access mix | Share of UEs by selected access type | All UEs |

Time to completion reports the median, p95, p99, and maximum. Failures are censored at
the horizon and reported as a separate count. A completion time without the failure
count is not a valid result.

The violation rate uses the same denominator for every policy. The denominator is the
generated cohort, not the set of UEs that received an assignment.

### 10.2 Strata

Every KPI is reported for these strata:

- capability profile (P1 to P7),
- delay-budget class,
- cell and candidate-set size,
- initial UE versus new UE,
- access type actually used,
- re-plan policy (R1 to R6).

A stratum with fewer than 20 UEs or 100 attempts is marked low-support. The study does
not claim population reliability from it.

### 10.3 Statistical protocol

The experimental unit is one episode. An episode is one batch at one epoch plus its
arrival trace, with one seed. Positions, capabilities, budgets, arrivals, and random
draws all derive from that seed.

- Use paired seeds across policies.
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
| `run.json` | Resolved configuration, package versions, config hash, seed list |
| `ues.csv` | One row per UE: id, arrival time, cell, candidates, capability, budget |
| `plan.csv` | One row per assigned UE: the full assignment record |
| `access-events.csv` | One row per attempt: UE, cell, access, occasion, outcome, reason |
| `leases.csv` | One row per lease: UE, cell, access, state, version, start, resources |
| `replans.csv` | One row per re-plan: arrival, policy, scope, attempt, objective before and after, moved leases, preempted resources |
| `ledger.csv` | Per cell, slot, pool: capacity and use |
| `summary.json` | All KPIs, strata, confidence intervals, and feasibility flags |
| `summary.csv` | Flat KPI table for plotting |

The `plan.csv` columns are:

```text
ue, cell, access, start_slot, prach_occasion, preamble_pool, preamble_index,
prb_start, prb_count, symbol_count, cce_count, cg_occasion, dg_slot,
predicted_completion_slot, lease_version
```

The `replans.csv` columns are:

```text
time_ms, ue_new, policy, scope, attempt, accepted, objective_before, objective_after,
moved_leases, preempted_prb_symbols, preempted_preambles, preempted_cce
```

The `preamble_index` column is empty for CBRA, because the UE chooses the preamble. It
is filled for CFRA.

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
| Scale per episode | High cost per run | 10,000 UEs per episode is feasible | Medium |
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

Do not use ns-3 for version 3. Revisit this decision only if one of these conditions
holds:

1. The study adds PHY or SINR behavior.
2. The study adds packet-level QoS or core-network effects.
3. A reviewer requires full-stack 4-step protocol validation.

If a condition holds, use ns-3 for a small validation slice. Keep the main allocator and
the experiment engine in Python. Do not move the whole study.

### 11.4 Consequences

- The implementation is a Python package, not an ns-3 build target.
- Tests use pytest, not the ns-3 test runner.
- The model claims mechanism and relative policy performance. It does not claim
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
|-- cells.py             candidate sets and cell distance
|-- hop.py               groups, dwell, active mask
|-- population.py        density, penetration, capabilities, budgets
|-- arrivals.py          Poisson and fixed-batch traces
|-- resources.py         PRB and pool ledger
|-- access.py            six access procedures and retries
|-- allocator.py         options, greedy pass, swap pass
|-- replan.py            lease states, six policies, incremental re-plan
|-- engine.py            SimPy environment and episode runner
|-- metrics.py           counters, strata, CSV writers
`-- reference.py         exhaustive reference and lower bound
tests/
|-- test_geometry.py
|-- test_cells.py
|-- test_hop.py
|-- test_population.py
|-- test_arrivals.py
|-- test_resources.py
|-- test_access.py
|-- test_allocator.py
|-- test_replan.py
|-- test_reference.py
`-- test_determinism.py
scenarios/
|-- cluster-37.json
|-- footprint-1373.json
|-- replan-matrix.json
`-- tiny-exact.json
tools/
|-- sweep.py
`-- report.py
requirements.txt
README.md
```

### 12.2 Event engine and determinism

The engine uses SimPy 4.1. Each episode creates a new `simpy.Environment`. The initial
allocator runs as a plain function at time zero after the decision latency. Each arrival
runs the re-plan function and then starts the new UE's access process.

Random draws use `numpy.random.SeedSequence`. The episode seed spawns separate streams
for positions, capabilities, budgets, arrivals, preamble draws, detection outcomes,
MsgA payload outcomes, DG grant outcomes, and backoff. A policy-specific draw must not
shift an exogenous stream. Stream IDs are recorded in `run.json`.

The arrival trace is generated before the run and recorded. Every policy in a comparison
uses the same trace.

The ledger uses integer PRB-symbols, not floating-point units. All iteration over UEs
and resources is sorted by ID. A rerun with the same seed produces byte-identical CSV
files.

### 12.3 Configuration

The configuration is JSON with a typed schema. A compact example:

```json
{
  "schema_version": 3,
  "geometry": {
    "altitude_km": 600,
    "min_elevation_deg": 30,
    "cell_radius_km": 25,
    "profile": "cluster",
    "rings": 3,
    "candidate_cells": 3,
    "edge_margin_km": 0.0
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
    "capability_budget_correlation": "independent"
  },
  "arrivals": {
    "mode": "poisson",
    "fraction_per_s": 0.05,
    "first_arrival_ms": 100,
    "context_creation_ms": 1
  },
  "access": {
    "max_attempts": 10,
    "backoff_slots": 8,
    "detection_probability": 1.0,
    "msga_success_probability": 0.99,
    "enable_two_step_fallback": true,
    "cg_period_slots": 8,
    "enable_sync_error": false
  },
  "allocator": {
    "decision_latency_ms": 1,
    "options_per_access": 3,
    "lambda": 1.0,
    "swap_cap": 100,
    "max_swap_passes": 10
  },
  "replan": {
    "scope": "local-first",
    "preempt_policy": "uncommitted-only",
    "balance_policy": "total-first",
    "commitment_horizon_ms": 20,
    "reconfig_delay_ms": 5,
    "max_local_ues": 20,
    "max_replan_ues": 200,
    "cooldown_ms": 10
  },
  "evaluation": {
    "horizon_slots": 2000,
    "episodes": 30,
    "first_seed": 1
  }
}
```

The loader rejects a missing key or an invalid range. It also rejects a capability share
that does not sum to one. The same check rejects a preamble split that exceeds the total
and a budget below the minimum procedure time. The loader writes the resolved
configuration to `run.json`.

### 12.4 Dependencies and run

Python 3.11 or newer. A virtual environment holds the dependencies.

```sh
python3 -m venv .venv
. .venv/bin/activate
python -m pip install -r requirements.txt
python -m qef_ho_ra.cli --config scenarios/cluster-37.json --output results/cluster-37
python -m qef_ho_ra.cli --config scenarios/replan-matrix.json --output results/replan-matrix
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
| V5 Boundary | Budget equal to completion, attempt limit, occasion boundary, inactive cell, preamble exhaustion, CG and DG timeout, 2-step fallback | An edge case is wrong or untested |
| V6 Replay | The plan replayed through the engine reproduces its predicted outcomes with the same seed | The allocator reads hidden state |
| V7 Exact | Independent enumeration reproduces the lexicographic optimum of a tiny case | The reference or the objective order is wrong |
| V8 Determinism | Two runs with one seed produce byte-identical CSV files | A stream or iteration order is unstable |
| V9 Candidate set | A vertex UE has three candidates, an edge UE has two, an interior UE has one | The cell geometry is wrong |
| V10 Lease state | `consumed` and `expired` leases never move. Versions increase on a move | The re-plan changes committed work |
| V11 Re-plan policy | R1 to R6 are selectable, and a tie keeps the current plan | The policy matrix or the acceptance rule is wrong |

A gate is not a claim. V4 shows the model implements the formula, not that the formula
matches a real gNB. V7 shows the solver is right for its model, not that the model is
the network.

### 12.6 Staged implementation

| Stage | Work | Exit criterion |
|---|---|---|
| S0 | Config, geometry, candidate cells, hop, population, resource ledger | V1, V2, V3, V8, V9 pass |
| S1 | 4-step CBRA and CFRA engine | V4 and the CBRA and CFRA cases of V5 pass |
| S2 | 2-step access with fallback | The 2-step cases of V5 pass |
| S3 | CG and DG access | The RACH-less cases of V5 pass |
| S4 | Batch allocator with candidate cells and swap pass | V6 passes |
| S5 | Arrivals, lease states, and the six re-plan policies | V10 and V11 pass |
| S6 | Exhaustive reference and lower bound | V7 passes |
| S7 | Experiment harness, pilot, and main run | The pilot sets the sample size and the sweep points |

Each stage runs the full gate set for the features it adds. A stage that changes an
earlier artifact reruns the earlier gates. The study does not stack stages without a
green gate.

## 13. Control variables for repeated simulations

### 13.1 Control summary

| Group | Control | Default | Sweep or sensitivity | Role |
|---|---|---|---|---|
| Geometry | `altitude_km` | 600 | 400, 600, 1200 | Sweep |
| Geometry | `min_elevation_deg` | 30 | 25, 30, 40 | Sweep |
| Geometry | `cell_radius_km` | 25 | 25, 50, 100 | Sensitivity |
| Geometry | `profile` | cluster | cluster, full | Fixed per run |
| Geometry | `rings` | 3 | 2, 3, 4 | Fixed per run |
| Multi-cell | `candidate_cells` | 3 | 1, 2, 3 | Sweep |
| Multi-cell | `edge_margin_km` | 0 | 0, 5, 10 | Sensitivity |
| Population | `density_persons_per_km2` | 30 cluster, 5 full | 1, 2, 5, 10, 30, 100 | Sweep |
| Population | `terminal_penetration` | 0.001 | 0.0001, 0.001, 0.01 | Sensitivity |
| Population | `active_fraction` | 1.0 | 0.3, 0.6, 1.0 | Sensitivity |
| Population | `capability_shares` | Table 6.2 | default, no-advanced, all-advanced | Sweep |
| Population | `budget_ms` | 100/200/400/800 | tight-heavy, default, loose-heavy | Sweep |
| Population | `capability_budget_correlation` | independent | independent, advanced-on-tight | Sensitivity |
| Population | `spatial_distribution` | uniform | uniform, hotspot | Sensitivity |
| Arrivals | `mode` | poisson | poisson, fixed | Fixed per run |
| Arrivals | `fraction_per_s` | 0.05 | 0.01, 0.05, 0.20 | Sweep |
| Arrivals | `first_arrival_ms` | 100 | 50, 100, 500 | Sensitivity |
| Arrivals | `context_creation_ms` | 1 | fixed | Fixed |
| Re-plan | `scope` | local-first | local-first, global-only | Sweep |
| Re-plan | `preempt_policy` | uncommitted-only | uncommitted-only, any-with-compensation | Sweep |
| Re-plan | `balance_policy` | total-first | total-first, existing-first, stability-first | Sweep |
| Re-plan | `commitment_horizon_ms` | 20 | 0, 20, 100 | Sweep |
| Re-plan | `reconfig_delay_ms` | 5 | 1, 5, 20 | Sensitivity |
| Re-plan | `max_local_ues` | 20 | 5, 20, 50 | Sensitivity |
| Re-plan | `max_replan_ues` | 200 | 50, 200, 1000 | Sensitivity |
| Re-plan | `cooldown_ms` | 10 | 0, 10, 50 | Sensitivity |
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
| Access | `enable_two_step_fallback` | true | false, true | Ablation |
| Access | `cg_period_slots` | 8 | 4, 8, 16 | Sweep |
| Access | `enable_sync_error` | false | false, true | Sensitivity |
| Allocator | `decision_latency_ms` | 1 | 0, 1, 5 | Sweep |
| Allocator | `options_per_access` | 3 | 1, 3, 5 | Sensitivity |
| Allocator | `lambda` | 1.0 | 0.1, 1, 10 | Sensitivity |
| Allocator | `swap_cap` | 100 | 0, 100, 500 | Sensitivity |
| Allocator | `max_swap_passes` | 10 | fixed | Fixed |
| Evaluation | `horizon_slots` | 2000 | 1000, 2000, 4000 | Fixed per run |
| Evaluation | `episodes` | 30 | 30, 100 | Fixed after pilot |
| Evaluation | `first_seed` | 1 | fixed block | Fixed |

### 13.2 Repeated-run recipe

1. Freeze the structural controls: geometry profile, cell radius, hop pattern, and
   access timers.
2. Freeze the arrival trace for a comparison. Every policy uses the same trace.
3. Vary one group at a time. Do not move density, dwell, and arrival rate together in one
   sweep. Their effects overlap and the attribution is lost.
4. Use paired seeds for every policy at the same point.
5. Replicate each point with at least three seeds before publishing it.
6. Freeze the evaluation seed block and the sample size before the main run.
7. Record the resolved configuration and the config hash in `run.json`.

## 14. Experiment plan

| ID | Question | Factors | Levels | Purpose |
|---|---|---|---|---|
| E1 | Which access type wins? | Fixed access rule | CBRA only, CFRA only, CG only, DG only, joint | Isolate the value of the mix |
| E2 | How does load change the answer? | Density | 2, 10, 30, 100 persons/km^2 on the cluster | Find the contention and control crossovers |
| E3 | How does hopping change the answer? | `C`, dwell | `C` 7/19/37, dwell 5/10/20/50/100 ms | Measure the hard wait floor |
| E4 | Do budgets change the mix? | Budget shares | tight-heavy, default, loose-heavy | Test H1 and H2 |
| E5 | Does preamble scarcity bind? | Dedicated share | 0, 8, 16, 32 of 64 | Separate CFRA from CBRA capacity |
| E6 | Does bandwidth matter? | Bandwidth | 5, 15, 30 MHz | Separate PRB from control limits |
| E7 | Does correlation matter? | Capability and budget | independent, advanced-on-tight | Test fairness claims |
| E8 | Does the result scale? | Population | full profile, density 5 | Scale and runtime |
| E9 | How sensitive is the answer? | Detection, MsgA, fallback | 0.90/0.99, fallback on/off | Sensitivity, not a headline result |
| E10 | Which re-plan policy is best? | Preemption and balance | R1 to R6, plus a no-re-plan baseline | Compare service, churn, and latency |
| E11 | How does the arrival rate change it? | Arrival fraction | 0.01, 0.05, 0.20 per s | Find the re-plan saturation point |
| E12 | How does the horizon change it? | Commitment horizon | 0, 20, 100 ms | Measure the cost of frozen leases |
| E13 | Is local-first enough? | Scope | local-first, global-only | Measure the value of the fallback |
| E14 | Does multi-cell help? | Candidate cells | 1, 2, 3 | Test H4 |
| E15 | Do multi-cell and re-plan interact? | Candidates, arrival rate | 1/3, 0.01/0.05 | Joint effect on edge UEs |

The main comparison uses the cluster with `C = 19` and density 30. The scale run uses
the full profile with `C = 64` and density 5.

The fixed-access arms in E1 run on a matched eligible population. A UE without the
required capability stays in the population with an explicit fallback. The comparison
does not put an advanced-only subset against the full mixed cohort.

Every sweep point runs at least three seeds. The pilot run sizes the main sample. The
evaluation seeds stay untouched until the main run.

Feasibility checks run before any scoring:

- every UE has at least one legal option,
- the hop schedule has the declared duty,
- the PRACH and control pools fit the declared load,
- the horizon covers the largest budget,
- every candidate set is nonempty.

An infeasible point stays in the matrix as a labeled stress case. The study does not
drop it silently.

## 15. Backlog

The version 2 backlog items on re-allocation and multi-cell are now design sections.
This backlog holds only conditional and future items.

### B-1 ns-3 validation slice for 4-step access

**Status:** conditional. See section 11.

If a reviewer requires full-stack validation, build a small ns-3 validation that runs the
4-step path the stack supports. Compare its timing distribution with the abstract model.
State which procedure stages the stack does not model. Do not move the main engine.

### B-2 Multi-satellite candidate selection

**Status:** not started. Outside version 3.

Version 3 adds candidate cells on one satellite. A later stage adds several candidate
satellites with a visibility model. The allocator then chooses a satellite and a cell.
This needs per-satellite propagation and a candidate-satellite set.

### B-3 PHY and dynamic hop pattern

**Status:** not started. Outside version 3.

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
9. dynamic resource allocation LEO satellite new user arrival re-planning
10. handover re-planning LEO satellite resource allocation

Most sources were inspected at abstract level. The closest paper was paywalled, so its
full text was not read. The negative duplicate result in section 16.5 is therefore
provisional.

### 16.2 Prior work

These works establish the setting or one ingredient. None solves the joint problem.

| Work | Contribution | Relation to this study |
|---|---|---|
| Hozayen, Darwish, Karabulut, Yanikomeroglu, "A Graph-Based Customizable Handover Framework for LEO Satellite Networks", IEEE GC Wkshps 2022, arXiv:2211.07872 | Handover timing and target selection in a QEFC scenario through a time-based graph and shortest path | Establishes QEFC as a studied scenario. No random-access resource allocation |
| Koutsopoulos, Tassiulas, "A Unified Framework for Handover Prediction and Resource Allocation in Non-Geostationary Mobile Satellite Networks", IEEE VTC 1999 Fall, vol. 4, pp. 2106-2110 | Joint handover prediction and resource allocation for non-geostationary networks | Early joint formulation. No 2-step or RACH-less access, no PRB grid, no re-plan policy comparison |
| Kim, Lee, Kim, "Enhanced RACH-less conditional handover for LEO intra-satellite system", ICT Express 12(1):142-146, 2026, DOI 10.1016/j.icte.2025.08.004 | RACH-less CHO with UL grant start timing predicted from antenna gain | Establishes RACH-less access as a viable handover type. One access type, no allocation across UEs |
| Le, Hassan, Lansard, Yuen, "A Random Access Protocol for Mixed-Traffic in LEO Satellite-Based IoT Communication", IEEE VTC2024-Spring, 2024 | JRA-PA: joint random access and preamble allocation for high-priority and low-priority traffic | Establishes preamble partitioning under mixed priorities. No handover, no PRB allocation |
| Zhu, Sun, Peng, "Beam Management in LEO Satellite Networks With Random Traffic Arrival and Time-Varying Topology", IEEE TVT 73(9):13352-13367, 2024, DOI 10.1109/TVT.2024.3393924 | Beam management with random traffic arrivals and time-varying topology | Arrival-aware beam management. No per-UE access-type choice, no re-plan policy matrix |
| Choi, "On Fast Retrial for Two-Step Random Access in MTC", IEEE Internet of Things Journal, 2021 | Analytical model of fast retrial for 2-step random access | Establishes 2-step retrial analysis. Not NTN handover |
| 3GPP TR 38.821, TS 38.321, TS 38.300 | NTN scenarios, 4-step and 2-step random access, RACH-less handover | Standard baseline for the access types and timers |

### 16.3 Similar work

These works overlap with part of the problem.

| Work | What it does | Overlap | Difference | Evidence |
|---|---|---|---|---|
| Wang, Wen, Lu, Jing, Zhang, "Random access optimization for initial access and seamless handover for 5G-satellite network", Computer Networks 214:109176, 2022, DOI 10.1016/j.comnet.2022.109176 | Proposes 2-step random access for satellite (TRAS) and an MC-controlled handover (SMHOS) that reserves PUSCH time-frequency resources for the target | 2-step access and reserved time-frequency resources for handover | Fixes 2-step for all UEs. No per-UE choice among 4-step, CFRA, and RACH-less. No shared PRB-grid allocation across mixed capabilities. No per-UE delay-budget objective. No re-plan policy comparison | Abstract |
| Chen et al., "Incentivizing Cooperation for Handover Strategies in LEO Constellations via Fairness-guided MARL", ACM Transactions on Internet Technology 26(1), 2026, DOI 10.1145/3760527 | Fairness-guided multi-agent reinforcement learning that selects the handover target and sub-channel | Joint handover and resource selection. Fairness objective | No access-type choice. No random-access procedure. Learning, not a one-shot allocation over a PRB grid. No post-decision re-allocation | Abstract |
| "Beam-Hopping Pattern Design for Grant-Free Random Access in LEO Satellite Communications", arXiv:2508.03391, 2025 | Binary optimization of the beam-hopping pattern to maximize the minimum successful transmission probability across cells | Beam hopping and grant-free access | No handover batch. No per-UE access-type choice. Pattern design, not resource allocation for access | Abstract |
| He, Ying, You, Gao, "PreHO: Predictive Handover for LEO Satellite Networks", arXiv:2603.07987, 2026 | Predictive handover planning with alternating optimization and dynamic programming | Plans a handover batch in advance | Timing and target planning only. No random-access resource allocation. No late-arrival re-plan | Abstract |
| Lee, Park, Park, Molisch, "Handover Protocol Learning for LEO Satellite Networks: Access Delay and Collision Minimization", arXiv:2310.20215, 2023 | Deep reinforcement learning handover protocol that modifies measurement reporting to reduce access delay and collisions | Access delay and collision reduction | Protocol learning, not resource allocation. No 2-step or RACH-less selection. No re-plan policy matrix | Abstract |
| Gou et al., "Beam-Hopping Scheduling for High-Priority and Time-Sensitive Services in Low-Earth-Orbit Satellite Networks", Sensors 26(19):6081, 2026, DOI 10.3390/s26196081 | Beam-hopping scheduling for high-priority and time-sensitive services | Beam-hopping scheduling under service requirements | No handover. No random-access procedure. No re-allocation after an arrival | Abstract |

### 16.4 Multi-cell and re-allocation work

| Work | Overlap | Difference |
|---|---|---|
| Koutsopoulos and Tassiulas, 1999 | Joint handover prediction and resource allocation | Aggregate channel allocation, not per-UE access type, PRB grid, or policy comparison |
| Chen et al., 2026 | Handover target and sub-channel selection | Learning policy, no RA procedure, no churn or preemption comparison |
| Zhu, Sun, Peng, 2024 | Random traffic arrival and beam management | Beam-level management, not UE-level access and re-plan decisions |
| Hozayen et al., 2022 | QEFC handover sequence and target choice | Sequence planning, not resource allocation or re-planning |

### 16.5 Exact duplicate assessment

No inspected work combines all seven elements:

1. a QEF handover batch at a target satellite,
2. a per-UE choice among 4-step, 2-step, and RACH-less CG/DG access,
3. a per-UE choice among up to three candidate cells,
4. a shared PRB-by-slot allocation across cells and UEs,
5. per-UE access delay budgets,
6. event-driven re-allocation after a new UE arrives,
7. a comparison of preemption and balancing policies with churn accounting.

The nearest work for the initial allocation is Wang et al. 2022. It reserves PUSCH
time-frequency resources for handover and uses 2-step access. It does not choose the
access type per UE, allocate a shared PRB grid, or re-plan after an arrival.

The nearest work for joint handover and resource allocation is Koutsopoulos and
Tassiulas 1999. It formulates the joint problem but does not model the access types,
per-UE budgets, candidate cells, or re-plan policies of this study.

This is a provisional negative result. The search was not exhaustive, and the closest
papers were inspected at abstract level. Absence of a duplicate is not proof of priority.

Novelty confidence: moderate for the combination, low for any single ingredient.

What would weaken the claim:

- A paper that jointly selects access types, cells, and PRBs for handover.
- A paper that compares re-plan preemption and balancing policies for NTN handover.
- A paper that proves a fixed access or re-plan rule is optimal for the same objective.
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
| A-9 | New UEs arrive as a Poisson process | Declared default | A bursty arrival would change the re-plan load |
| A-10 | The target re-plans within 1 ms of an arrival | Declared | A slower decision would miss the new UE's options |
| A-11 | A moved lease pays 5 ms and one control message | Declared | A larger cost would reduce the value of preemption |
| A-12 | The candidate set is static during a procedure | Design choice | A moving UE or a moving cell would change the set |
| R-1 | The abstract model may not match a real gNB | Risk | The validation gates bound the claim |
| R-2 | The exact reference is exponential | Risk | Tiny cases only |
| R-3 | The heuristic may be far from the bound at scale | Risk | Report the tiny-instance gap, not a near-optimality claim at scale |
| R-4 | The density is an average over the footprint | Risk | A hotspot would change the per-cell load |
| R-5 | The study folder may be outside version control | Fact | The study has no repository history unless it moves |
| R-6 | The re-plan comparison depends on the arrival rate | Risk | Report the rate and do not generalize across rates |
| R-7 | Churn depends on the lease-state definition | Risk | State the definitions and keep them fixed |
| R-8 | The multi-cell gain depends on the overlap model | Risk | A different overlap would change the candidate sets |

Open items for the user, with the defaults chosen here:

1. Reference bandwidth (15 MHz) and band. The intent gives no value.
2. Reference density for the main comparison (30 persons/km^2 on the cluster).
3. Terminal penetration (0.001) and active fraction (1.0).
4. Whether the full-footprint run is required or the cluster is enough.
5. Whether the ns-3 validation slice is needed for a reviewer.

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
| Zhu et al., beam management with random arrivals | DOI 10.1109/TVT.2024.3393924 |
| Wang et al., random access optimization for satellite handover | DOI 10.1016/j.comnet.2022.109176 |
| Chen et al., fairness-guided MARL handover | DOI 10.1145/3760527 |
| Beam-hopping pattern design for GFRA | arXiv:2508.03391 |
| He et al., PreHO predictive handover | arXiv:2603.07987 |
| Lee et al., handover protocol learning | arXiv:2310.20215 |
| Gou et al., beam-hopping scheduling for time-sensitive services | DOI 10.3390/s26196081 |
| Choi, fast retrial for 2-step random access | IEEE Internet of Things Journal, 2021 |

## 19. Deliverables

1. This design document.
2. A Python package with six access procedures, candidate-cell selection, and the
   resource ledger.
3. The batch allocator algorithm with pseudocode and time and space complexity bounds.
4. The re-allocation algorithm with pseudocode and time and space complexity bounds.
5. Six re-plan policies: two preemption policies times three balancing policies.
6. A deterministic plan replay check and a lease-version audit.
7. An arrival generator with Poisson and fixed-batch modes.
8. An exhaustive reference and a lower bound for tiny instances.
9. A metrics and experiment toolchain with confidence intervals.
10. A validation record with the gate results and the exact limits.
11. An experiment report with the access mix, the violation rate, the re-plan policy
    comparison, and the candidate-cell comparison.

The study does not deliver a physical calibration, a full-stack reference, a
near-optimality claim at scale, or a multi-satellite policy. Those are separate stages.
