"""Access timelines, option pruning and the budget precheck."""

import math

import pytest

from qef_ho_ra.access import (
    OptionEnumerator,
    build_timeline,
    min_completion_ms,
    no_wait_completion_ms,
    option_cost,
)
from qef_ho_ra.config import ScenarioConfig, resolve
from qef_ho_ra.geometry import build_layout
from qef_ho_ra.hop import HopSchedule
from qef_ho_ra.population import UE
from scenarios.generate import scenario


def always_active():
    return HopSchedule(1, 1, 5000, 0)


def paused(dwell=20, active_cells=7, cells=37):
    return HopSchedule(cells, active_cells, dwell, 0)


def ue(uid=0, cell=0, tau=2.0, budget=400, capability=6):
    return UE(uid=uid, cell=cell, x_km=0.0, y_km=0.0, range_km=0.0,
              elevation_deg=90.0, delay_ms=tau,
              tau_slots=max(1, math.ceil(tau)), capability=capability,
              budget_class=2, budget_ms=budget)


def test_no_wait_timeline_matches_the_design_expressions():
    hop = always_active()
    tl = build_timeline("4step-cbra", 0, 0, 2.0, 2, hop)
    assert (tl.response_slot, tl.msg3_slot, tl.cr_slot) == (4, 12, 23)
    assert tl.completion_ms == pytest.approx(19.0 + 3 * 2.0)
    tl = build_timeline("2step-cbra", 0, 0, 2.0, 2, hop)
    assert tl.response_slot == 5
    assert tl.completion_ms == pytest.approx(3.0 + 2 * 2.0)
    tl = build_timeline("cg", 7, 0, 2.0, 2, hop)
    assert tl.completion_ms == pytest.approx(7 + 1 + 2.0)
    tl = build_timeline("dg", 0, 0, 2.0, 2, hop)
    assert tl.tx_slot == 8
    assert tl.completion_ms == pytest.approx(9.0 + 2.0)


@pytest.mark.parametrize("tau", [2.0014, 2.5, 3.586])
def test_slot_rounding_keeps_the_timeline_above_the_continuous_bound(tau):
    hop = always_active()
    for access in ("4step-cbra", "2step-cbra", "cg", "dg"):
        tl = build_timeline(access, 0, 0, tau, math.ceil(tau), hop)
        bound = no_wait_completion_ms(access, tau)
        assert tl.completion_ms >= bound - 1e-9
        assert tl.completion_ms <= bound + 4.0


def test_pause_rule_moves_messages_to_the_next_window():
    hop = paused()  # group 0 active in [0,20), [120,140), ...
    tl = build_timeline("4step-cbra", 10, 0, 2.0, 2, hop)
    assert hop.is_active(0, tl.response_slot)
    assert hop.is_active(0, tl.msg3_slot)
    assert hop.is_active(0, tl.cr_slot)
    # The 8-slot CR timer cannot finish inside [10,20); the CR waits for 120.
    assert tl.cr_slot >= 120


def test_all_messages_are_in_active_slots():
    hop = paused()
    for access in ("4step-cbra", "2step-cbra", "cg", "dg"):
        for start in (0, 10, 15, 120):
            tl = build_timeline(access, start, 0, 2.135, 3, hop)
            for slot in (tl.response_slot, tl.msg3_slot, tl.cr_slot, tl.tx_slot):
                if slot is not None:
                    assert hop.is_active(0, slot)


def test_option_enumeration_prunes_to_three_starts_per_type():
    cfg = ScenarioConfig.model_validate(scenario({
        "geometry": {"rings": 1},
        "hop": {"active_cells": 7},
        "population": {"density_persons_per_km2": 0.7},
        "evaluation": {"horizon_slots": 200},
    }))
    resolved = resolve(cfg)
    layout = build_layout("cluster", 600, 30, 41, 1)
    hop = HopSchedule(layout.n_cells, 7, 20, 0)
    enum = OptionEnumerator(cfg, resolved, layout, hop)
    u = ue(uid=3, cell=0, budget=400)
    options = enum.enumerate_for(u, ("4step-cbra", "4step-cfra", "2step-cbra",
                                     "2step-cfra", "cg", "dg"), False)
    per_type = {}
    for o in options:
        per_type.setdefault(o.access, []).append(o.start_slot)
    assert len(per_type) == 6
    assert all(len(v) <= cfg.allocator.options_per_access for v in per_type.values())
    assert all(v == sorted(v) for v in per_type.values())
    assert len(options) <= 18
    for o in options:
        assert o.predicted_completion <= cfg.evaluation.horizon_slots


def test_option_cost_weights_control_units():
    hop = always_active()
    tl = build_timeline("4step-cbra", 0, 0, 2.0, 2, hop)
    from qef_ho_ra.access import access_demands

    demands = access_demands("4step-cbra", tl, False, 56, 0)
    # 12 Msg3 + 16 PDSCH + 12 x (2 CCE + 1 RAR + 1 preamble)
    assert option_cost(demands) == 12 + 16 + 12 * 4


def test_min_completion_and_budget_boundary():
    hop = paused()
    u = ue(uid=0, cell=0, tau=2.0, budget=100)
    allowed = ("4step-cbra",)
    mc = min_completion_ms(u, allowed, hop, 1)
    assert mc == pytest.approx(1 + hop.wait(0, 1) + 19 + 3 * 2.0)
    assert not mc > 100  # 63 ms lower bound fits a 100 ms budget
    # Exactly at the budget is feasible (strict inequality).
    exact = ue(uid=0, cell=0, tau=2.0, budget=int(math.ceil(mc)))
    assert not min_completion_ms(exact, allowed, hop, 1) > exact.budget_ms
