"""Access procedures and option enumeration (DESIGN.md sections 7.4 and 8).

The message timeline is computed once per option and reused by the allocator
(prediction) and by the engine (execution). Transmission instants are whole
slots; a message transmitted in slot ``s`` arrives at ``s + tau``. A processing
timer of ``n`` slots starts in the first active slot at or after the arrival and
counts ``n`` active slots of the cell, so it pauses while the cell is inactive.
Propagation delay never pauses. The completion instant is the reception instant
of the last message, in ms.
"""

from __future__ import annotations

import math
from dataclasses import dataclass

from .geometry import tau_slots
from .hop import HopSchedule
from .resources import Demand

DETECTION_SLOTS = 2
TWO_STEP_RESPONSE_SLOTS = 3
RAR_WINDOW_SLOTS = 10
MSGB_WINDOW_SLOTS = 10
RAR_GRANT_OFFSET = 8
DG_GRANT_OFFSET = 8
MSG3_SLOTS = 1
CR_TIMER_SLOTS = 8
RESPONSE_SLOTS = 1

PRACH_TYPES = ("4step-cbra", "4step-cfra", "2step-cbra", "2step-cfra")
FOUR_STEP_TYPES = ("4step-cbra", "4step-cfra")
RACH_LESS_TYPES = ("cg", "dg")


@dataclass(frozen=True)
class Message:
    """One resource demand at a concrete slot."""

    pool: str
    cell: int
    slot: int
    amount: int

    @property
    def key(self) -> tuple[int, int, str]:
        return (self.cell, self.slot, self.pool)


@dataclass(frozen=True)
class Timeline:
    access: str
    start_slot: int
    response_slot: int | None
    msg3_slot: int | None
    cr_slot: int | None
    tx_slot: int | None
    completion_ms: float


@dataclass(frozen=True)
class Option:
    ue: int
    cell: int
    access: str
    start_slot: int
    timeline: Timeline
    demands: tuple[Message, ...]
    predicted_completion: float
    direct_cost: int
    dedicated: bool
    fallback_used: bool
    prediction_basis: str = "no-failure"

    @property
    def needs_preamble(self) -> bool:
        return self.dedicated or self.access in ("4step-cbra", "2step-cbra")


def _place(hop: HopSchedule, cell: int, instant: float, n_active: int) -> int:
    """Slot of a response whose processing timer starts at ``instant``."""
    return hop.advance(cell, int(math.ceil(instant - 1e-12)), n_active)


def four_step_tail(response_slot: int, cell: int, tau_ms: float, tau_slot: int,
                   hop: HopSchedule):
    """(Msg3 slot, CR slot, completion ms) after a RAR transmit slot."""
    msg3_slot = hop.next_active(cell, response_slot + max(RAR_GRANT_OFFSET, tau_slot + 1))
    msg3_rx = msg3_slot + tau_ms + MSG3_SLOTS
    cr_slot = _place(hop, cell, msg3_rx, CR_TIMER_SLOTS)
    return msg3_slot, cr_slot, cr_slot + tau_ms


def build_timeline(
    access: str,
    start_slot: int,
    cell: int,
    tau_ms: float,
    tau_slot: int,
    hop: HopSchedule,
    cg_period: int = 8,
) -> Timeline:
    """Message schedule of one access type that starts in ``start_slot``."""
    if access in FOUR_STEP_TYPES:
        rar_slot = _place(hop, cell, start_slot + tau_ms, DETECTION_SLOTS)
        msg3_slot, cr_slot, completion = four_step_tail(
            rar_slot, cell, tau_ms, tau_slot, hop
        )
        return Timeline(
            access=access,
            start_slot=start_slot,
            response_slot=rar_slot,
            msg3_slot=msg3_slot,
            cr_slot=cr_slot,
            tx_slot=None,
            completion_ms=completion,
        )
    if access in ("2step-cbra", "2step-cfra"):
        msgb_slot = _place(hop, cell, start_slot + tau_ms, TWO_STEP_RESPONSE_SLOTS)
        return Timeline(
            access=access,
            start_slot=start_slot,
            response_slot=msgb_slot,
            msg3_slot=None,
            cr_slot=None,
            tx_slot=None,
            completion_ms=msgb_slot + tau_ms,
        )
    if access == "cg":
        return Timeline(
            access=access,
            start_slot=start_slot,
            response_slot=None,
            msg3_slot=None,
            cr_slot=None,
            tx_slot=start_slot,
            completion_ms=start_slot + RESPONSE_SLOTS + tau_ms,
        )
    if access == "dg":
        tx_slot = hop.next_active(cell, start_slot + max(DG_GRANT_OFFSET, tau_slot + 1))
        return Timeline(
            access=access,
            start_slot=start_slot,
            response_slot=None,
            msg3_slot=None,
            cr_slot=None,
            tx_slot=tx_slot,
            completion_ms=tx_slot + RESPONSE_SLOTS + tau_ms,
        )
    raise ValueError(f"unknown access type {access!r}")


def no_wait_completion_ms(access: str, tau_ms: float) -> float:
    """Continuous no-wait completion of DESIGN.md section 6.4."""
    if access in FOUR_STEP_TYPES:
        return 19.0 + 3.0 * tau_ms
    if access in ("2step-cbra", "2step-cfra"):
        return 3.0 + 2.0 * tau_ms
    if access == "cg":
        return 1.0 + tau_ms
    if access == "dg":
        return 9.0 + tau_ms
    raise ValueError(f"unknown access type {access!r}")


def access_demands(access: str, tl: Timeline, dedicated: bool,
                   contention_preambles: int, cell: int) -> tuple[Message, ...]:
    """Resource vector of DESIGN.md Table 7.4 on the planned timeline."""
    msgs: list[Message] = []
    if access in PRACH_TYPES:
        pool = "preamble-dedicated" if dedicated else "preamble-contention"
        msgs.append(Message(pool, cell, tl.start_slot, 1))
    if access in FOUR_STEP_TYPES:
        assert tl.response_slot is not None and tl.msg3_slot is not None
        assert tl.cr_slot is not None
        msgs.append(Message("rar-grant", cell, tl.response_slot, 1))
        msgs.append(Message("cce", cell, tl.response_slot, 1))
        msgs.append(Message("pdsch", cell, tl.response_slot, 8))
        msgs.append(Message("pusch", cell, tl.msg3_slot, 12))
        msgs.append(Message("cce", cell, tl.cr_slot, 1))
        msgs.append(Message("pdsch", cell, tl.cr_slot, 8))
    elif access in ("2step-cbra", "2step-cfra"):
        assert tl.response_slot is not None
        msgs.append(Message("cce", cell, tl.response_slot, 2))
        msgs.append(Message("pdsch", cell, tl.response_slot, 8))
        msgs.append(Message("pusch", cell, tl.start_slot, 12))
    elif access == "cg":
        assert tl.tx_slot is not None
        msgs.append(Message("pusch", cell, tl.tx_slot, 12))
    elif access == "dg":
        assert tl.tx_slot is not None
        msgs.append(Message("cce", cell, tl.start_slot, 1))
        msgs.append(Message("dg-grant", cell, tl.start_slot, 1))
        msgs.append(Message("pusch", cell, tl.tx_slot, 12))
    return tuple(msgs)


def option_cost(demands: tuple[Message, ...]) -> int:
    """Stage-3 direct cost: PRB-symbols plus 12 per control unit."""
    cost = 0
    for m in demands:
        if m.pool in ("pusch", "pdsch"):
            cost += m.amount
        else:
            cost += 12 * m.amount
    return cost


def with_response(option: Option, response_slot: int, tau_ms: float, tau_slot: int,
                  hop: HopSchedule, contention: int) -> Option:
    """Rebuild an option when its response is placed in a later window slot."""
    cell = option.cell
    if option.access in FOUR_STEP_TYPES:
        msg3, cr, completion = four_step_tail(
            response_slot, cell, tau_ms, tau_slot, hop
        )
        tl = Timeline(option.access, option.start_slot, response_slot, msg3, cr,
                      None, completion)
    elif option.access in ("2step-cbra", "2step-cfra"):
        tl = Timeline(option.access, option.start_slot, response_slot, None, None,
                      None, response_slot + tau_ms)
    else:  # pragma: no cover - only PRACH types are rebuilt
        return option
    demands = access_demands(option.access, tl, option.dedicated, contention, cell)
    return Option(
        ue=option.ue,
        cell=cell,
        access=option.access,
        start_slot=option.start_slot,
        timeline=tl,
        demands=demands,
        predicted_completion=tl.completion_ms,
        direct_cost=option_cost(demands),
        dedicated=option.dedicated,
        fallback_used=option.fallback_used,
        prediction_basis=option.prediction_basis,
    )


class OptionEnumerator:
    """Per-cell occasion lists and option construction for one episode."""

    def __init__(self, cfg, resolved: dict, layout, hop: HopSchedule):
        self.cfg = cfg
        self.resolved = resolved
        self.layout = layout
        self.hop = hop
        self.horizon = cfg.evaluation.horizon_slots
        self.decision_end = cfg.allocator.decision_latency_ms
        self.options_per_access = cfg.allocator.options_per_access
        self.cg_period = cfg.access.cg_period_slots
        self.contention = resolved["radio"]["contention_preambles"]
        self.occasions: list[list[int]] = []
        self._occasion_index: dict[int, dict[int, int]] = {}
        self._build_occasions()

    def _build_occasions(self) -> None:
        period = self.cfg.radio.prach_period_slots
        first = ((self.decision_end + period - 1) // period) * period
        for cell in self.layout.cells:
            slots = []
            k = first
            while k < self.horizon:
                if self.hop.is_active(cell.cid, k):
                    slots.append(k)
                k += period
            self.occasions.append(slots)

    def cg_start(self, uid: int) -> int:
        return (uid * 5 + 3) % self.cg_period

    def cg_occasions(self, uid: int, cell: int) -> list[int]:
        out = []
        k = self.cg_start(uid)
        while k < self.decision_end:
            k += self.cg_period
        while k < self.horizon:
            if self.hop.is_active(cell, k):
                out.append(k)
            k += self.cg_period
        return out

    def _make_option(self, ue, access: str, start: int, fallback: bool) -> Option:
        tl = build_timeline(
            access, start, ue.cell, ue.delay_ms, ue.tau_slots, self.hop, self.cg_period
        )
        dedicated = access.endswith("cfra")
        demands = access_demands(access, tl, dedicated, self.contention, ue.cell)
        return Option(
            ue=ue.uid,
            cell=ue.cell,
            access=access,
            start_slot=start,
            timeline=tl,
            demands=demands,
            predicted_completion=tl.completion_ms,
            direct_cost=option_cost(demands),
            dedicated=dedicated,
            fallback_used=fallback,
        )

    def enumerate_for(self, ue, allowed: tuple[str, ...], fallback: bool) -> list[Option]:
        """Options of one UE for the allowed access types, pruned by design 9.4."""
        out: list[Option] = []
        n_keep = self.options_per_access
        if any(a in PRACH_TYPES for a in allowed):
            slots = self.occasions[ue.cell]
            for access in PRACH_TYPES:
                if access not in allowed:
                    continue
                kept = 0
                for start in slots:
                    opt = self._make_option(ue, access, start, fallback)
                    if opt.predicted_completion > self.horizon:
                        break
                    out.append(opt)
                    kept += 1
                    if kept >= n_keep:
                        break
        if "cg" in allowed:
            kept = 0
            for start in self.cg_occasions(ue.uid, ue.cell):
                opt = self._make_option(ue, "cg", start, fallback)
                if opt.predicted_completion > self.horizon:
                    break
                out.append(opt)
                kept += 1
                if kept >= n_keep:
                    break
        if "dg" in allowed:
            kept = 0
            k = self.decision_end
            while k < self.horizon and kept < n_keep:
                if self.hop.is_active(ue.cell, k):
                    # The grant slot and the granted transmission slot must both
                    # be active; the offset can cross a hop boundary.
                    tx = self.hop.next_active(
                        ue.cell, k + max(DG_GRANT_OFFSET, ue.tau_slots + 1)
                    )
                    if self.hop.is_active(ue.cell, tx):
                        opt = self._make_option(ue, "dg", k, fallback)
                        if opt.predicted_completion <= self.horizon:
                            out.append(opt)
                            kept += 1
                k += 1
        return out


def min_completion_ms(ue, allowed: tuple[str, ...], hop: HopSchedule,
                      decision_latency_ms: int) -> float:
    """Necessary-condition lower bound of DESIGN.md section 6.4."""
    if not allowed:
        return math.inf
    wait = hop.wait(ue.cell, decision_latency_ms)
    best = min(no_wait_completion_ms(a, ue.delay_ms) for a in allowed)
    return decision_latency_ms + wait + best


def budget_infeasible(ue, allowed: tuple[str, ...], hop: HopSchedule,
                      decision_latency_ms: int) -> bool:
    return min_completion_ms(ue, allowed, hop, decision_latency_ms) > ue.budget_ms


def tau_slot_of(delay_ms: float) -> int:
    return tau_slots(delay_ms)
