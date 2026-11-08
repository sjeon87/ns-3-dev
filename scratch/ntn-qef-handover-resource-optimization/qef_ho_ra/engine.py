"""Episode engine on SimPy 4.1 (DESIGN.md sections 8 and 12.2).

One process per cell walks the PRACH occasions of that cell. Planned CG and DG
transmissions are separate processes at their slots. Every attempt consumes the
run ledger message by message, so a retry that finds a full pool is delayed or
fails instead of overbooking the calendar.

Random draws use order-independent streams ``(seed, stream, ue, attempt)``, so a
change in one UE never shifts another UE's draw.
"""

from __future__ import annotations

import math
from bisect import bisect_left
from collections import defaultdict
from dataclasses import dataclass, field

import simpy

from .access import (
    CR_TIMER_SLOTS,
    DETECTION_SLOTS,
    DG_GRANT_OFFSET,
    MSGB_WINDOW_SLOTS,
    RAR_WINDOW_SLOTS,
    RESPONSE_SLOTS,
    TWO_STEP_RESPONSE_SLOTS,
    build_timeline,
    four_step_tail,
)
from .population import (
    STREAM_BACKOFF,
    STREAM_DETECTION,
    STREAM_DG,
    STREAM_MSGA,
    STREAM_PREAMBLE,
    STREAM_SYNC,
    ue_rng,
)
from .resources import Demand, Ledger, build_capacities


@dataclass
class AttemptRow:
    ue: int
    cell: int
    access: str
    attempt: int
    slot: int
    preamble: int | None
    outcome: str
    reason: str


@dataclass
class Outcome:
    ue: int
    cell: int
    access: str
    realised_access: str
    outcome: str
    reason: str
    attempts: int
    completion_ms: float | None
    planned_completion: float | None
    budget_violation: bool
    censored: bool
    matched_prediction: bool
    response_slot: int | None = None
    msg3_slot: int | None = None
    cr_slot: int | None = None


@dataclass
class EpisodeResult:
    outcomes: dict[int, Outcome]
    attempts: list[AttemptRow]
    ledger: Ledger
    consumed: dict[int, list[Demand]]
    stats: dict = field(default_factory=dict)


class Episode:
    """One batch decision and its execution for a single seed."""

    def __init__(self, cfg, resolved: dict, layout, hop, ues, enumerator,
                 allocation, seed: int, outcome_seed: int | None = None,
                 active_cells: list[int] | None = None):
        self.cfg = cfg
        self.resolved = resolved
        self.layout = layout
        self.hop = hop
        self.ues = ues
        self.by_id = {u.uid: u for u in ues}
        self.enum = enumerator
        self.allocation = allocation
        self.seed = seed
        self.outcome_seed = seed if outcome_seed is None else outcome_seed
        self.active_cells = active_cells
        self.horizon = cfg.evaluation.horizon_slots
        self.ledger = Ledger(build_capacities(resolved), self.horizon)
        self.consumed: dict[int, list[Demand]] = {u.uid: [] for u in ues}
        self.outcomes: dict[int, Outcome] = {}
        self.attempts: list[AttemptRow] = []
        self.queue: dict[int, dict[int, list[tuple]]] = defaultdict(lambda: defaultdict(list))
        self.occasion_slots = {c: list(slots) for c, slots in enumerate(self.enum.occasions)}
        self.cell_of = {u.uid: u.cell for u in ues}
        self.reason_counts: dict[str, int] = defaultdict(int)
        self.counters: dict[str, int] = defaultdict(int)
        self.mu = self.cfg.allocator.mu  # unused here; kept for symmetry
        self._build_first_attempts()

    # -- planning --------------------------------------------------------
    def _build_first_attempts(self) -> None:
        for uid, rec in self.allocation.records.items():
            ue = self.by_id[uid]
            if rec.option is None:
                self._finish(uid, None, "failure", rec.reason or "unassigned",
                             realised_access="", completion=None, attempts=0)
                continue
            opt = rec.option
            if opt.access in ("4step-cbra", "4step-cfra", "2step-cbra", "2step-cfra"):
                self.queue[ue.cell][opt.start_slot].append(
                    (uid, opt.access, 0, rec.dedicated_index)
                )
            # CG and DG are scheduled by their own processes in run().

    # -- helpers ---------------------------------------------------------
    def _consume(self, uid: int, cell: int, slot: int, pool: str, amount: int) -> bool:
        if self.ledger.remaining(pool, cell, slot) < amount:
            return False
        demand = Demand(pool, cell, slot, amount)
        self.ledger.reserve([demand])
        self.consumed[uid].append(demand)
        return True

    def _finish(self, uid: int, option, outcome: str, reason: str,
                realised_access: str, completion: float | None, attempts: int,
                timeline=None) -> None:
        ue = self.by_id[uid]
        planned = None if option is None else option.predicted_completion
        matched = bool(
            outcome == "success" and option is not None and timeline is not None
            and timeline[0] == option.timeline.response_slot
            and timeline[1] == option.timeline.msg3_slot
            and timeline[2] == option.timeline.cr_slot
            and abs(completion - option.predicted_completion) < 1e-9
        )
        violation = outcome != "success" or (completion is not None
                                             and completion > ue.budget_ms)
        self.outcomes[uid] = Outcome(
            ue=uid,
            cell=ue.cell,
            access="" if option is None else option.access,
            realised_access=realised_access,
            outcome=outcome,
            reason=reason,
            attempts=attempts,
            completion_ms=completion,
            planned_completion=planned,
            budget_violation=violation,
            censored=(reason == "horizon-censored"),
            matched_prediction=matched,
            response_slot=None if timeline is None else timeline[0],
            msg3_slot=None if timeline is None else timeline[1],
            cr_slot=None if timeline is None else timeline[2],
        )
        self.reason_counts[reason] += 1

    def _record_attempt(self, uid: int, cell: int, access: str, attempt: int,
                        slot: int, preamble: int | None, outcome: str, reason: str) -> None:
        self.attempts.append(AttemptRow(uid, cell, access, attempt, slot, preamble,
                                        outcome, reason))

    def _next_occasion(self, cell: int, t: int) -> int | None:
        slots = self.occasion_slots[cell]
        idx = bisect_left(slots, t)
        return slots[idx] if idx < len(slots) else None

    def _cg_next(self, uid: int, t: int) -> int | None:
        ue = self.by_id[uid]
        period = self.cfg.access.cg_period_slots
        start = self.enum.cg_start(uid)
        k = start + max(0, math.ceil((t - start) / period)) * period
        while k < self.horizon:
            if k >= t and self.hop.is_active(ue.cell, k):
                return k
            k += period
        return None

    def _backoff(self, uid: int, attempt: int) -> int:
        slots = self.cfg.access.backoff_slots
        if slots <= 0:
            return 0
        rng = ue_rng(self.outcome_seed, STREAM_BACKOFF, uid, attempt)
        return int(rng.integers(0, slots + 1))

    # -- PRACH attempt ---------------------------------------------------
    def _prach_attempt(self, uid: int, cell: int, slot: int, access: str,
                       attempt: int, preamble: int, dedicated: bool,
                       planned_response: int | None = None):
        """Run the message exchange. Returns (success, completion, reason, timeline)."""
        ue = self.by_id[uid]
        tl = build_timeline(access, slot, cell, ue.delay_ms, ue.tau_slots, self.hop)
        arrival = int(math.ceil(slot + ue.delay_ms - 1e-12))
        first = tl.response_slot if planned_response is None else planned_response
        if access in ("2step-cbra", "2step-cfra"):
            if not self._consume(uid, cell, slot, "pusch", 12):
                return False, None, "capacity", None
            window_end = arrival + MSGB_WINDOW_SLOTS
            response = None
            for s in range(first, window_end + 1):
                if not self.hop.is_active(cell, s):
                    continue
                if (self.ledger.remaining("cce", cell, s) >= 2
                        and self.ledger.remaining("pdsch", cell, s) >= 8):
                    response = s
                    break
            if response is None:
                return False, None, "msgb-window-full", None
            self._consume(uid, cell, response, "cce", 2)
            self._consume(uid, cell, response, "pdsch", 8)
            completion = response + ue.delay_ms
            return True, completion, "connected-target", (response, None, None)

        window_end = arrival + RAR_WINDOW_SLOTS
        response = None
        for s in range(first, window_end + 1):
            if not self.hop.is_active(cell, s):
                continue
            if (self.ledger.remaining("rar-grant", cell, s) >= 1
                    and self.ledger.remaining("cce", cell, s) >= 1
                    and self.ledger.remaining("pdsch", cell, s) >= 8):
                response = s
                break
        if response is None:
            return False, None, "rar-window-full", None
        self._consume(uid, cell, response, "rar-grant", 1)
        self._consume(uid, cell, response, "cce", 1)
        self._consume(uid, cell, response, "pdsch", 8)
        msg3, cr, completion = four_step_tail(
            response, cell, ue.delay_ms, ue.tau_slots, self.hop
        )
        if not self._consume(uid, cell, msg3, "pusch", 12):
            return False, None, "capacity", None
        if (self.ledger.remaining("cce", cell, cr) < 1
                or self.ledger.remaining("pdsch", cell, cr) < 8):
            return False, None, "cr-window-full", None
        self._consume(uid, cell, cr, "cce", 1)
        self._consume(uid, cell, cr, "pdsch", 8)
        return True, completion, "connected-target", (response, msg3, cr)

    # -- cell process ----------------------------------------------------
    def _cell_process(self, env: simpy.Environment, cell: int):
        for slot in self.occasion_slots[cell]:
            delay = slot - env.now
            if delay > 0:
                yield env.timeout(delay)
            entries = self.queue[cell].pop(slot, None)
            if entries:
                self._process_occasion(cell, slot, entries)

    def _process_occasion(self, cell: int, slot: int, entries: list[tuple]) -> None:
        entries.sort(key=lambda e: (e[0], e[1]))
        draws: dict[int, int] = {}
        survivors: list[tuple] = []
        for uid, access, attempt, dedicated_index in entries:
            ue = self.by_id[uid]
            dedicated = dedicated_index is not None and access.endswith("cfra")
            if dedicated:
                if not self._consume(uid, cell, slot, "preamble-dedicated", 1):
                    self._retry(uid, slot, access, attempt, "capacity")
                    continue
                self.ledger.mark_dedicated(cell, slot, dedicated_index)
                preamble = dedicated_index
                draws[uid] = preamble
                survivors.append((uid, access, attempt, preamble, dedicated))
            else:
                if not self._consume(uid, cell, slot, "preamble-contention", 1):
                    self._retry(uid, slot, access, attempt, "capacity")
                    continue
                rng = ue_rng(self.outcome_seed, STREAM_PREAMBLE, uid, attempt)
                preamble = int(rng.integers(0, self.ledger.capacities["preamble-contention"]))
                draws[uid] = preamble
                survivors.append((uid, access, attempt, preamble, False))

        counts: dict[int, int] = defaultdict(int)
        for uid, access, _attempt, preamble, dedicated in survivors:
            if not dedicated:
                counts[preamble] += 1

        for uid, access, attempt, preamble, dedicated in survivors:
            conflict = (not dedicated) and counts[preamble] > 1
            if conflict:
                self._record_attempt(uid, cell, access, attempt, slot, preamble,
                                     "failure", "collision")
                self._retry(uid, slot, access, attempt, "collision")
                continue
            rng = ue_rng(self.outcome_seed, STREAM_DETECTION, uid, attempt)
            if rng.random() >= self.cfg.access.detection_probability:
                self._record_attempt(uid, cell, access, attempt, slot, preamble,
                                     "failure", "detection-failure")
                self._retry(uid, slot, access, attempt, "detection-failure")
                continue
            if access.startswith("2step"):
                rng = ue_rng(self.outcome_seed, STREAM_MSGA, uid, attempt)
                if rng.random() >= self.cfg.access.msga_success_probability:
                    self._record_attempt(uid, cell, access, attempt, slot, preamble,
                                         "failure", "msga-failure")
                    self._retry(uid, slot, access, attempt, "msga-failure")
                    continue
            rec = self.allocation.records[uid]
            planned_response = None
            if (attempt == 0 and rec.option is not None
                    and rec.option.access == access
                    and rec.option.timeline.response_slot is not None):
                planned_response = rec.option.timeline.response_slot
            ok, completion, reason, timeline = self._prach_attempt(
                uid, cell, slot, access, attempt, preamble, dedicated, planned_response
            )
            if ok:
                self._record_attempt(uid, cell, access, attempt, slot, preamble,
                                     "success", reason)
                self._finish(uid, self.allocation.records[uid].option, "success", reason,
                             access, completion, attempt + 1, timeline)
            else:
                self._record_attempt(uid, cell, access, attempt, slot, preamble,
                                     "failure", reason)
                self._retry(uid, slot, access, attempt, reason)

    def _retry(self, uid: int, slot: int, access: str, attempt: int, reason: str) -> None:
        ue = self.by_id[uid]
        if attempt + 1 >= self.cfg.access.max_attempts:
            self._finish(uid, self.allocation.records[uid].option, "failure",
                         "max-attempts", access, None, attempt + 1)
            return
        t = slot + 1 + self._backoff(uid, attempt)
        nxt = self._next_occasion(ue.cell, t)
        if nxt is None or nxt + 19 > self.horizon:
            self._finish(uid, self.allocation.records[uid].option, "failure",
                         "horizon-censored", access, None, attempt + 1)
            return
        next_access = self._next_access(access)
        self.queue[ue.cell][nxt].append((uid, next_access, attempt + 1, None))

    def _next_access(self, access: str) -> str:
        if access.startswith("2step"):
            if self.cfg.access.enable_two_step_fallback:
                return "4step-cbra"
            return access
        if access.endswith("cfra"):
            return "4step-cbra"
        return access

    # -- CG / DG ---------------------------------------------------------
    def _cg_process(self, env: simpy.Environment, uid: int, attempt: int):
        ue = self.by_id[uid]
        rec = self.allocation.records[uid]
        option = rec.option
        slot = option.start_slot if attempt == 0 else self._cg_next(uid, env.now)
        if slot is None:
            self._finish(uid, option, "failure", "cg-timeout", "cg", None, attempt)
            return
        if slot > env.now:
            yield env.timeout(slot - env.now)
        deadline = min(ue.budget_ms, self.horizon)
        if slot + 1 + ue.delay_ms > deadline:
            self._finish(uid, option, "failure", "cg-timeout", "cg", None, attempt)
            return
        if not self._consume(uid, ue.cell, slot, "pusch", 12):
            self._finish(uid, option, "failure", "capacity", "cg", None, attempt)
            return
        if self.cfg.access.enable_sync_error:
            rng = ue_rng(self.outcome_seed, STREAM_SYNC, uid, attempt)
            if rng.random() < self.cfg.access.sync_error_probability:
                self.counters["cg_sync_error"] += 1
                nxt = self._cg_next(uid, slot + 1)
                if nxt is None:
                    self._finish(uid, option, "failure", "cg-sync-error", "cg", None, attempt + 1)
                else:
                    env.process(self._cg_process(env, uid, attempt + 1))
                return
        completion = slot + RESPONSE_SLOTS + ue.delay_ms
        self._finish(uid, option, "success", "connected-target", "cg", completion,
                     attempt + 1, timeline=(None, None, None))

    def _dg_process(self, env: simpy.Environment, uid: int):
        ue = self.by_id[uid]
        rec = self.allocation.records[uid]
        option = rec.option
        grant = option.start_slot
        if grant > env.now:
            yield env.timeout(grant - env.now)
        deadline = min(ue.budget_ms, self.horizon)
        if grant + 1 + ue.delay_ms > deadline:
            self._finish(uid, option, "failure", "dg-timeout", "dg", None, 0)
            return
        if not self._consume(uid, ue.cell, grant, "dg-grant", 1):
            self._finish(uid, option, "failure", "capacity", "dg", None, 0)
            return
        if not self._consume(uid, ue.cell, grant, "cce", 1):
            self._finish(uid, option, "failure", "capacity", "dg", None, 0)
            return
        if self.cfg.access.dg_decode_failure_probability > 0.0:
            rng = ue_rng(self.outcome_seed, STREAM_DG, uid, 0)
            if rng.random() < self.cfg.access.dg_decode_failure_probability:
                self.counters["dg_decode_failure"] += 1
                self._finish(uid, option, "failure", "dg-decode-failure", "dg", None, 0)
                return
        tx = option.timeline.tx_slot
        if tx is None:
            tx = self.hop.next_active(
                ue.cell, grant + max(DG_GRANT_OFFSET, ue.tau_slots + 1)
            )
        if tx + RESPONSE_SLOTS + ue.delay_ms > self.horizon:
            self._finish(uid, option, "failure", "horizon-censored", "dg", None, 0)
            return
        if not self._consume(uid, ue.cell, tx, "pusch", 12):
            self._finish(uid, option, "failure", "capacity", "dg", None, 0)
            return
        completion = tx + RESPONSE_SLOTS + ue.delay_ms
        self._finish(uid, option, "success", "connected-target", "dg", completion, 1,
                     timeline=(None, None, None))

    # -- run -------------------------------------------------------------
    def run(self) -> EpisodeResult:
        env = simpy.Environment()
        cells = self.active_cells if self.active_cells is not None else range(self.layout.n_cells)
        for cell in cells:
            env.process(self._cell_process(env, cell))
        for uid, rec in sorted(self.allocation.records.items()):
            if rec.option is None:
                continue
            if rec.option.access == "cg":
                env.process(self._cg_process(env, uid, 0))
            elif rec.option.access == "dg":
                env.process(self._dg_process(env, uid))
        env.run(until=self.horizon + 1)
        for uid in self.by_id:
            if uid not in self.outcomes:
                rec = self.allocation.records.get(uid)
                self._finish(uid, None if rec is None else rec.option, "failure",
                             "horizon-censored", "", None, 0)
        stats = {
            "attempts": len(self.attempts),
            "reasons": dict(sorted(self.reason_counts.items())),
            "counters": dict(sorted(self.counters.items())),
            "ledger_totals": self.ledger.totals(),
        }
        return EpisodeResult(self.outcomes, self.attempts, self.ledger,
                             self.consumed, stats)
