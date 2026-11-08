"""Exact reference and lower bound (DESIGN.md sections 9.5, 9.8 and gate V7).

The reference enumerates the pruned option set of every UE in a cell, replays
each per-cell schedule through the real engine with the same seed, keeps the
stage-1-optimal frontier of the cell, and combines the frontiers by dynamic
programming over the per-stratum violation counts. Stage 1 and stage 3 add over
cells; stage 2 is a maximum over the supported strata, so the combination keeps
the count vector, not only the cost.
"""

from __future__ import annotations

import itertools
from dataclasses import dataclass, replace

from . import BUDGET_CLASSES, CAPABILITY_PROFILES
from .access import budget_infeasible, min_completion_ms
from .allocator import AllocationResult, PlanRecord, policy_types
from .engine import Episode
from .resources import Ledger, build_capacities

STRATUM_KEYS = [f"cap:{p}" for p in CAPABILITY_PROFILES] + [
    f"budget:{b}" for b in BUDGET_CLASSES
]


@dataclass(frozen=True)
class Candidate:
    counts: tuple[int, ...]
    cost: int

    @property
    def violations(self) -> int:
        return sum(self.counts)


def lower_bound(cfg, ues, hop) -> dict:
    """Necessary-condition lower bound from the beam-and-latency expression."""
    forced = []
    for ue in ues:
        allowed, _fallback = policy_types(cfg, ue)
        if not allowed:
            forced.append(ue.uid)
            continue
        if budget_infeasible(ue, allowed, hop, cfg.allocator.decision_latency_ms):
            forced.append(ue.uid)
    return {
        "forced_violations": len(forced),
        "forced_rate": len(forced) / len(ues) if ues else 0.0,
        "ues": forced,
    }


def _counts_for(ues, outcomes) -> tuple[int, ...]:
    counts = [0] * len(STRATUM_KEYS)
    for ue in ues:
        if outcomes[ue.uid].budget_violation:
            counts[ue.capability] += 1
            counts[7 + ue.budget_class] += 1
    return tuple(counts)


def replay_cell(cfg, resolved, layout, hop, ues, enumerator, assignment, seed) -> Candidate:
    """Replay one per-cell schedule through the engine and score it."""
    records = {}
    cost = 0
    for ue in ues:
        option = assignment[ue.uid]
        records[ue.uid] = PlanRecord(ue.uid, ue.cell, option=option, reason="assigned")
        cost += option.direct_cost
    allocation = AllocationResult(records, Ledger(build_capacities(resolved),
                                                 cfg.evaluation.horizon_slots), [], {})
    episode = Episode(cfg, resolved, layout, hop, ues, enumerator, allocation, seed,
                      active_cells=[ues[0].cell])
    result = episode.run()
    return Candidate(_counts_for(ues, result.outcomes), cost)


def per_cell_frontier(cfg, resolved, layout, hop, ues, enumerator, cell: int,
                      seed: int) -> list[Candidate]:
    """Stage-1-optimal schedules of one cell, deduplicated by count vector."""
    cell_ues = [u for u in ues if u.cell == cell]
    option_sets = []
    for ue in cell_ues:
        allowed, fallback = policy_types(cfg, ue)
        if not allowed:
            raise ValueError(f"UE {ue.uid} has no legal access type")
        options = enumerator.enumerate_for(ue, allowed, fallback)
        if not options:
            raise ValueError(f"UE {ue.uid} has no option inside the horizon")
        option_sets.append(options)
    best_violations = None
    best: dict[tuple[int, ...], int] = {}
    for combo in itertools.product(*option_sets):
        assignment = {ue.uid: opt for ue, opt in zip(cell_ues, combo)}
        cand = replay_cell(cfg, resolved, layout, hop, cell_ues, enumerator,
                           assignment, seed)
        if best_violations is None or cand.violations < best_violations:
            best_violations = cand.violations
            best = {cand.counts: cand.cost}
        elif cand.violations == best_violations:
            if cand.counts not in best or cand.cost < best[cand.counts]:
                best[cand.counts] = cand.cost
    return [Candidate(counts, cost) for counts, cost in sorted(best.items())]


def combine(frontiers: list[list[Candidate]], stratum_n: list[int],
            total_weight: float, supported: list[bool]) -> dict:
    """Combine per-cell frontiers into the lexicographic batch optimum."""
    states: dict[tuple[int, ...], int] = {tuple([0] * len(STRATUM_KEYS)): 0}
    for frontier in frontiers:
        new: dict[tuple[int, ...], int] = {}
        for vec, cost in states.items():
            for cand in frontier:
                merged = tuple(a + b for a, b in zip(vec, cand.counts))
                total = cost + cand.cost
                if merged not in new or total < new[merged]:
                    new[merged] = total
        states = new
    best_tuple = None
    best_state = None
    for vec, cost in states.items():
        stage1 = sum(vec) / total_weight if total_weight else 0.0
        stage2 = 0.0
        for idx, flag in enumerate(supported):
            if flag and stratum_n[idx]:
                stage2 = max(stage2, vec[idx] / stratum_n[idx])
        tup = (round(stage1, 12), round(stage2, 12), cost)
        if best_tuple is None or tup < best_tuple:
            best_tuple = tup
            best_state = vec
    return {
        "tuple": list(best_tuple) if best_tuple else [0.0, 0.0, 0],
        "counts": list(best_state) if best_state else [],
        "n_states": len(states),
    }


def stratum_sizes(ues) -> tuple[list[int], float]:
    """Marginal stratum sizes (7 capability + 4 budget) and the total weight."""
    sizes = [0] * len(STRATUM_KEYS)
    for ue in ues:
        sizes[ue.capability] += 1
        sizes[7 + ue.budget_class] += 1
    return sizes, float(len(ues))


def reference_optimum(cfg, resolved, layout, hop, ues, enumerator, seed: int) -> dict:
    """Per-cell enumeration plus frontier combination on a tiny instance."""
    frontiers = []
    for cell in range(layout.n_cells):
        frontiers.append(per_cell_frontier(cfg, resolved, layout, hop, ues,
                                           enumerator, cell, seed))
    sizes, total = stratum_sizes(ues)
    supported = [n >= 20 for n in sizes]
    return {
        "frontiers": frontiers,
        "cells": layout.n_cells,
        "optimum": combine(frontiers, sizes, total, supported),
        "frontier_sizes": [len(f) for f in frontiers],
    }
