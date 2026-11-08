# Quasi-Earth-fixed NTN handover resource allocation

Python implementation of `DESIGN.md` version 5.1. The study allocates, for one
handover batch at a target satellite, an access type and a time-frequency
resource to every incoming UE. The six access types are 4-step CBRA, 4-step
CFRA, 2-step CBRA, 2-step CFRA, RACH-less CG and RACH-less DG.

## Quick start

```sh
python3 -m venv .venv
. .venv/bin/activate
python -m pip install -r requirements.txt
python scenarios/generate.py
pytest -q
python -m qef_ho_ra.cli --config scenarios/cluster-37.json --output results/cluster-37 --episodes 1
python tools/sweep.py --experiment E1 --out results --resume
python tools/report.py --results results
```

The CLI writes one directory per seed under `--output`:

| File | Content |
|---|---|
| `run.json` | resolved configuration, config hash, header and document versions, seed |
| `ues.csv` | one row per UE: cell, capability profile, budget class, delay |
| `plan.csv` | one row per assigned UE, fixed 16-column layout of DESIGN.md 10.4 |
| `access-events.csv` | one row per PRACH or MsgA attempt |
| `ue-outcomes.csv` | one row per UE, including CG and DG, with the outcome reason |
| `ledger.csv` | per cell, slot and pool: used and capacity |
| `summary.json` | KPIs, strata, objective stages, conservation and replay checks |
| `summary.csv` | flat KPI table including the censored rate and low-support flag |

## Package layout

```text
qef_ho_ra/
  config.py      JSON schema 6, validation, resolved document
  geometry.py    spherical Earth, footprint, hexagonal cells, per-UE delay
  hop.py         beam-hopping groups, active mask, pause algebra
  population.py  density, placement, capabilities, budgets, seed streams
  resources.py   integer PRB-symbol and pool ledger
  access.py      six procedures, message timeline, option enumeration
  allocator.py   policy filter, greedy pass, swap pass
  engine.py      SimPy episode engine with a per-cell occasion process
  metrics.py     KPIs, strata, conservation, artifact writers
  reference.py   lower bound, per-cell exhaustive reference, frontier DP
  run.py         Runner: one seed from configuration to artifacts
  cli.py         command line
tests/           V1..V8 gate fixtures
scenarios/       cluster-37, footprint-517, tiny-exact, tiny-joint
tools/           sweep.py and report.py
```

## Model decisions that the design leaves implicit

These are stated here and in `VALIDATION.md`; they do not change the design's
objective or scope.

1. **Slot quantization.** Transmissions start at whole-millisecond slots. A
   message sent in slot `s` is received at `s + tau_ms`. A processing timer that
   starts at an arrival instant begins in the first whole slot at or after it and
   counts active slots of the cell. The completion instant keeps the fractional
   propagation delay. The design's no-wait expressions therefore act as lower
   bounds, which is how section 6.4 uses them.
2. **Response spreading.** A RAR or MsgB is placed at the first slot of its
   ten-slot window with free control capacity, in both the plan and the engine.
   This realises the "80 RARs per occasion" statement of section 7.6.
3. **Preamble split.** Contention draws use the 56-preamble contention pool, not
   the 64-preamble total.
4. **Swap.** An option's timeline depends on its own UE's propagation delay, so
   options cannot be transplanted between UEs. The swap exchanges the two start
   slots and each UE keeps its access type.
5. **CG offset.** The per-UE configured-grant offset is `(ue*5 + 3) mod P_cg`.
6. **Hotspot.** The hotspot cells share `w*N_cells` of weight equally and the
   other cells share `(1-w)*N_cells`; `w = 0` is uniform.
7. **Retry access.** A CFRA grant covers one attempt; a retry uses the contention
   pool. A 2-step retry falls back to 4-step CBRA when the fallback is enabled.
8. **Stage-3 cost.** `PRB-symbols + 12 * (CCEs + preambles + RAR grants + DG
   grants)`, plus CG waste in the runtime summary.
9. **Exact instance.** The reference uses the ring-1 cluster (7 cells) with three
   UEs per cell for the tiny instance and two UEs per cell for the joint
   decomposition check. The design's two-cell figure is a size declaration; the
   method is unchanged.
