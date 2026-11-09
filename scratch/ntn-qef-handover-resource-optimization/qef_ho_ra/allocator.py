"""Batch allocator: options, greedy pass, swap pass (DESIGN.md section 9).

The allocator sees only what a real target knows at the epoch: capabilities,
budgets, the cell of each UE, the hop schedule, the access policy and its own
ledger. It never reads a random draw.

Deviation recorded in the validation record: an option's timeline depends on the
UE's own propagation delay, so options cannot be transplanted between UEs. The
swap pass therefore exchanges the two start slots while each UE keeps its access
type; a pair whose exchanged start is not in the UE's pruned option set is
skipped.
"""

from __future__ import annotations

import itertools
import math
from dataclasses import dataclass, field

from . import ACCESS_TYPES, BUDGET_CLASSES, CAPABILITY_PROFILES
from .access import (
    MSGB_WINDOW_SLOTS,
    RAR_WINDOW_SLOTS,
    Option,
    OptionEnumerator,
    budget_infeasible,
    min_completion_ms,
    with_response,
)
from .resources import Ledger, build_capacities

STRATA_PER_UE = 2


@dataclass
class PlanRecord:
    ue: int
    cell: int
    option: Option | None = None
    reason: str = ""
    dedicated_index: int | None = None
    prb_start: int = 0
    predicted_completion: float | None = None
    fallback_used: bool = False


@dataclass
class AllocationResult:
    records: dict[int, PlanRecord]
    ledger: Ledger
    order: list[int]
    stats: dict = field(default_factory=dict)


def policy_types(cfg, ue) -> tuple[tuple[str, ...], bool]:
    """Allowed access types of one UE and whether the capability fallback fired."""
    cap = CAPABILITY_PROFILES[f"P{ue.capability + 1}"]
    allowed_by_policy = set(cfg.allocator.allowed_access)
    inter = tuple(a for a in ACCESS_TYPES if a in cap and a in allowed_by_policy)
    if inter:
        return inter, False
    fallback = cfg.allocator.capability_fallback
    if fallback == "strict":
        return (), False
    if fallback == "4step-cbra":
        return (("4step-cbra",) if "4step-cbra" in cap else ()), True
    return cap, True


class Allocator:
    """Greedy marginal-benefit allocator with a lexicographic swap pass."""

    def __init__(self, cfg, resolved: dict, layout, hop, ues, enumerator: OptionEnumerator,
                 weights: dict[int, float] | None = None):
        self.cfg = cfg
        self.resolved = resolved
        self.layout = layout
        self.hop = hop
        self.ues = ues
        self.by_id = {u.uid: u for u in ues}
        self.enum = enumerator
        self.ledger = Ledger(build_capacities(resolved), cfg.evaluation.horizon_slots)
        self.weights = {i: 1.0 for i in range(4)}
        if weights:
            self.weights.update(weights)
        self.total_weight = sum(self.weights[u.budget_class] for u in ues)
        self.records: dict[int, PlanRecord] = {}
        self.options: dict[int, list[Option]] = {}
        self.pressure_counts = [0] * (7 + 4)
        self.stratum_n = [0] * (7 + 4)
        for u in ues:
            self.stratum_n[u.capability] += 1
            self.stratum_n[7 + u.budget_class] += 1
        self.stratum_supported = [n >= 20 for n in self.stratum_n]
        self.violation_weight = 0.0
        self.cost = 0
        self.cg_waste = 0
        self.swap_pairs = 0
        self.swap_missing = 0
        self.swap_candidates = 0
        self.swap_accepted = 0

    # -- objective -------------------------------------------------------
    def objective(self) -> tuple[float, float, int]:
        stage1 = self.violation_weight / self.total_weight if self.total_weight else 0.0
        stage2 = 0.0
        for idx, supported in enumerate(self.stratum_supported):
            if not supported or self.stratum_n[idx] == 0:
                continue
            rate = self.pressure_counts[idx] / self.stratum_n[idx]
            if rate > stage2:
                stage2 = rate
        return (round(stage1, 12), round(stage2, 12), self.cost)

    def pressure(self, ue) -> float:
        idxs = (ue.capability, 7 + ue.budget_class)
        rates = [self.pressure_counts[i] / self.stratum_n[i]
                 for i in idxs if self.stratum_supported[i] and self.stratum_n[i]]
        return max(rates) if rates else 0.0

    def violates(self, ue, option: Option) -> bool:
        return option.predicted_completion > ue.budget_ms

    # -- score -----------------------------------------------------------
    def score(self, ue, option: Option) -> float:
        t = option.predicted_completion
        budget = ue.budget_ms
        s = 0.0
        if t > budget:
            s += 1000.0
            s += self.cfg.allocator.lambda_ * (t - budget)
        # DESIGN.md 9.6 declares this stage-2 term. pressure[u] is a property of
        # the UE, not of the option, so the term is constant across the UE's
        # candidates and cannot change this argmin (VALIDATION.md G1). It stays
        # to match the declared score and as the hook for a later revision.
        s += self.cfg.allocator.mu * (1.0 - self.pressure(ue))
        for m in option.demands:
            util = self.ledger.utilisation(m.pool, option.cell, m.slot)
            price = util * util / (1.0 - util + 0.01)
            s += price * m.amount
        if option.access in ("4step-cbra", "2step-cbra"):
            n_o = self.ledger.used.get((option.cell, option.start_slot, "preamble-contention"), 0)
            m_pool = self.ledger.capacities["preamble-contention"]
            s += n_o * math.log(m_pool / (m_pool - 1))
        return s

    # -- commit ----------------------------------------------------------
    def _effective(self, ue, option: Option) -> Option | None:
        """Place the response in the RAR/MsgB window where capacity exists.

        The design spreads RARs and MsgBs over the ten-slot window, so a plan
        that checks only the nominal slot under-counts the control capacity by a
        factor of ten (DESIGN.md section 7.6). The engine uses the same scan.
        """
        if option.access not in ("4step-cbra", "4step-cfra", "2step-cbra", "2step-cfra"):
            return option
        tl = option.timeline
        if tl.response_slot is None:  # pragma: no cover - PRACH types always have one
            return option
        two_step = option.access.startswith("2step")
        window = MSGB_WINDOW_SLOTS if two_step else RAR_WINDOW_SLOTS
        arrival = int(math.ceil(option.start_slot + ue.delay_ms - 1e-12))
        for slot in range(tl.response_slot, arrival + window + 1):
            if not self.hop.is_active(ue.cell, slot):
                continue
            if two_step:
                ok = (self.ledger.remaining("cce", ue.cell, slot) >= 2
                      and self.ledger.remaining("pdsch", ue.cell, slot) >= 8)
            else:
                ok = (self.ledger.remaining("rar-grant", ue.cell, slot) >= 1
                      and self.ledger.remaining("cce", ue.cell, slot) >= 1
                      and self.ledger.remaining("pdsch", ue.cell, slot) >= 8)
            if not ok:
                continue
            if slot == tl.response_slot:
                return option
            return with_response(option, slot, ue.delay_ms, ue.tau_slots, self.hop,
                                 self.enum.contention)
        return None

    def _reserve(self, ue, option: Option) -> int | None:
        self.ledger.reserve(option.demands)
        index = None
        if option.dedicated and option.needs_preamble:
            index = self.ledger.free_dedicated_index(option.cell, option.start_slot)
            if index is None:
                self.ledger.release(option.demands)
                raise RuntimeError("dedicated preamble pool inconsistent with ledger")
            self.ledger.mark_dedicated(option.cell, option.start_slot, index)
        return index

    def _release(self, ue, record: PlanRecord) -> None:
        if record.option is None:
            return
        self.ledger.release(record.option.demands)
        if record.dedicated_index is not None:
            self.ledger.unmark_dedicated(
                record.cell, record.option.start_slot, record.dedicated_index
            )

    def _apply(self, ue, option: Option, reason: str) -> PlanRecord | None:
        if self.ledger.blocking_pools(option.demands):
            effective = self._effective(ue, option)
            if effective is None:
                return None
            if self.ledger.blocking_pools(effective.demands):
                return None
        else:
            effective = option
        index = self._reserve(ue, effective)
        prb_start = self.ledger.used.get((ue.cell, effective.start_slot, "pusch"), 0) // 12
        rec = PlanRecord(
            ue=ue.uid,
            cell=ue.cell,
            option=effective,
            reason=reason,
            dedicated_index=index,
            prb_start=prb_start % max(1, self.resolved["radio"]["prb"]),
            predicted_completion=effective.predicted_completion,
            fallback_used=effective.fallback_used,
        )
        self.records[ue.uid] = rec
        self.cost += effective.direct_cost
        if self.violates(ue, effective):
            self.violation_weight += self.weights[ue.budget_class]
            self.pressure_counts[ue.capability] += 1
            self.pressure_counts[7 + ue.budget_class] += 1
        return rec

    # -- main ------------------------------------------------------------
    def run(self) -> AllocationResult:
        decisions = []
        for ue in self.ues:
            allowed, fallback = policy_types(self.cfg, ue)
            if not allowed:
                self.records[ue.uid] = PlanRecord(ue.uid, ue.cell, reason="no-legal-option")
                continue
            if budget_infeasible(ue, allowed, self.hop,
                                 self.cfg.allocator.decision_latency_ms):
                self.records[ue.uid] = PlanRecord(
                    ue.uid, ue.cell, reason="budget-infeasible"
                )
                continue
            options = self.enum.enumerate_for(ue, allowed, fallback)
            if not options:
                self.records[ue.uid] = PlanRecord(ue.uid, ue.cell, reason="no-legal-option")
                continue
            self.options[ue.uid] = options
            t_min = min(o.predicted_completion for o in options)
            decisions.append((ue.budget_ms - t_min, len(options), ue.uid))

        decisions.sort(key=lambda t: (t[0], t[1], t[2]))
        order = [uid for _slack, _n, uid in decisions]

        for uid in order:
            ue = self.by_id[uid]
            best = None
            best_score = math.inf
            best_tie = (math.inf, math.inf, math.inf)
            preamble_blocked = False
            other_blocked = False
            for index, option in enumerate(self.options[uid]):
                if self.ledger.blocking_pools(option.demands):
                    effective = self._effective(ue, option)
                else:
                    effective = option
                if effective is None:
                    other_blocked = True
                    continue
                blocking = self.ledger.blocking_pools(effective.demands)
                if blocking:
                    if blocking <= {"preamble-contention", "preamble-dedicated"}:
                        preamble_blocked = True
                    else:
                        other_blocked = True
                    continue
                s = self.score(ue, effective)
                # DESIGN.md 9.6 leaves an equal score to the enumeration order,
                # which made the mix depend on the ACCESS_TYPES order (G2). On a
                # tie, take the cheaper option, then the earlier completion, then
                # the enumeration order.
                tie = (effective.direct_cost, effective.predicted_completion, index)
                if (best is None or s < best_score - 1e-12
                        or (abs(s - best_score) <= 1e-12 and tie < best_tie)):
                    best_score = s
                    best = effective
                    best_tie = tie
            if best is None:
                reason = "preamble-exhausted" if (preamble_blocked and not other_blocked) else "capacity"
                self.records[uid] = PlanRecord(uid, ue.cell, reason=reason)
            else:
                self._apply(ue, best, "assigned")

        if self.cfg.allocator.swap_cap > 0:
            self._swap_pass(order)

        stats = {
            "assigned": sum(1 for r in self.records.values() if r.option is not None),
            "objective": self.objective(),
            "stratum_counts": list(self.pressure_counts),
            "stratum_n": list(self.stratum_n),
            "stratum_supported": list(self.stratum_supported),
            "cg_waste": self.cg_waste,
            "swap": {
                "pairs": self.swap_pairs,
                "missing": self.swap_missing,
                "candidates": self.swap_candidates,
                "accepted": self.swap_accepted,
            },
        }
        return AllocationResult(self.records, self.ledger, order, stats)

    # -- swap ------------------------------------------------------------
    def _option_lookup(self, uid: int, access: str, start: int) -> Option | None:
        for o in self.options.get(uid, ()):
            if o.access == access and o.start_slot == start:
                return o
        return None

    def _swap_pass(self, order: list[int]) -> None:
        pool = [uid for uid in order if self.records[uid].option is not None]
        pool = pool[: self.cfg.allocator.swap_cap]
        for _ in range(self.cfg.allocator.max_swap_passes):
            improved = False
            for u_id, v_id in itertools.combinations(pool, 2):
                if self._try_swap(u_id, v_id):
                    improved = True
            if not improved:
                break

    def _remove(self, uid: int) -> None:
        rec = self.records.get(uid)
        if rec is None or rec.option is None:
            return
        ue = self.by_id[uid]
        self._release(ue, rec)
        self.cost -= rec.option.direct_cost
        if self.violates(ue, rec.option):
            self.violation_weight -= self.weights[ue.budget_class]
            self.pressure_counts[ue.capability] -= 1
            self.pressure_counts[7 + ue.budget_class] -= 1
        del self.records[uid]

    def _try_swap(self, u_id: int, v_id: int) -> bool:
        ue_u, ue_v = self.by_id[u_id], self.by_id[v_id]
        rec_u, rec_v = self.records[u_id], self.records[v_id]
        o_u, o_v = rec_u.option, rec_v.option
        assert o_u is not None and o_v is not None
        self.swap_pairs += 1
        if o_u.access == o_v.access:
            return False
        # Exchange the two access types at the current start slots, inside the
        # pruned set. An option's timeline belongs to its own UE's propagation
        # delay, so the option is looked up in the UE's own set, never copied.
        new_u = self._option_lookup(u_id, o_v.access, o_u.start_slot)
        new_v = self._option_lookup(v_id, o_u.access, o_v.start_slot)
        if new_u is None or new_v is None:
            self.swap_missing += 1
            return False
        self.swap_candidates += 1

        before = self.objective()
        self._remove(u_id)
        self._remove(v_id)
        applied_u = self._apply(ue_u, new_u, "assigned")
        applied_v = self._apply(ue_v, new_v, "assigned")
        if applied_u is not None and applied_v is not None:
            if self.objective() < before:
                self.swap_accepted += 1
                return True
        if applied_u is not None:
            self._remove(u_id)
        if applied_v is not None:
            self._remove(v_id)
        self._apply(ue_u, o_u, "assigned")
        self._apply(ue_v, o_v, "assigned")
        return False


def stratum_labels() -> list[str]:
    return [f"cap:{p}" for p in CAPABILITY_PROFILES] + [
        f"budget:{b}" for b in BUDGET_CLASSES
    ]
