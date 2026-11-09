"""Allocator: policy filter, precheck, greedy, swap, stage tuple."""

import math

import pytest

from qef_ho_ra.allocator import policy_types
from qef_ho_ra.config import ScenarioConfig, resolve
from qef_ho_ra.run import Runner
from qef_ho_ra import CAPABILITY_PROFILES
from scenarios.generate import scenario


def tiny(**overrides):
    doc = scenario({
        "geometry": {"rings": 1},
        "hop": {"active_cells": 7},
        "population": {"density_persons_per_km2": 0.7},
        "evaluation": {"horizon_slots": 200, "episodes": 1},
    })
    for key, value in overrides.items():
        doc[key].update(value)
    return ScenarioConfig.model_validate(doc)


@pytest.mark.parametrize("profile", list(CAPABILITY_PROFILES))
def test_policy_filter_uses_the_capability_profile(profile):
    cfg = tiny()
    from qef_ho_ra.population import UE

    u = UE(0, 0, 0, 0, 0, 90, 2.0, 2, int(profile[1]) - 1, 2, 400)
    allowed, fallback = policy_types(cfg, u)
    assert allowed == CAPABILITY_PROFILES[profile]
    assert fallback is False


def test_fixed_policy_and_capability_fallback():
    from qef_ho_ra.population import UE

    cfg = tiny(allocator={"access_policy": "fixed", "allowed_access": ["cg"],
                          "capability_fallback": "strict"})
    p1 = UE(0, 0, 0, 0, 0, 90, 2.0, 2, 0, 2, 400)
    assert policy_types(cfg, p1) == ((), False)

    cfg = tiny(allocator={"access_policy": "fixed", "allowed_access": ["cg"],
                          "capability_fallback": "4step-cbra"})
    assert policy_types(cfg, p1) == (("4step-cbra",), True)

    cfg = tiny(allocator={"access_policy": "fixed", "allowed_access": ["cg"],
                          "capability_fallback": "best-legal"})
    assert policy_types(cfg, p1) == (CAPABILITY_PROFILES["P1"], True)


def test_budget_precheck_marks_infeasible_ues():
    cfg = tiny(population={"budget_ms": [4, 200, 400, 800]})
    runner = Runner(cfg)
    bundle = runner.episode(1)
    reasons = [r.reason for r in bundle["allocation"].records.values()]
    assert "budget-infeasible" in reasons
    tight = [u for u in bundle["ues"] if u.budget_ms == 4]
    assert tight and all(bundle["allocation"].records[u.uid].reason == "budget-infeasible"
                         for u in tight)


def test_assignment_respects_capacity_and_conserves():
    cfg = tiny()
    runner = Runner(cfg)
    bundle = runner.episode(1)
    led = bundle["allocation"].ledger
    for (cell, slot, pool), used in led.used.items():
        assert used <= led.capacities[pool]
    consumed = {}
    for demands in bundle["episode"].consumed.values():
        for d in demands:
            consumed[d.pool] = consumed.get(d.pool, 0) + d.amount
    total = led.totals()
    for pool in led.capacities:
        assert consumed.get(pool, 0) == total.get(pool, 0)
    assert bundle["summary"]["resources"]["conservation_ok"] is True


def test_objective_is_lexicographic_and_swap_never_worsens():
    cfg = tiny()
    runner = Runner(cfg)
    plain = runner.with_allocator(swap_cap=0, max_swap_passes=0).episode(1)
    swapped = runner.episode(1)
    a = plain["allocation"].stats["objective"]
    b = swapped["allocation"].stats["objective"]
    assert tuple(b) <= tuple(a)
    assert swapped["allocation"].stats["assigned"] >= plain["allocation"].stats["assigned"]


def test_equal_score_tie_prefers_the_cheaper_access():
    """G2: on an exact score tie the cheaper option wins, not the first one.

    On the tiny instance every score is zero for the first UE, so the old
    strict comparison took the first enumerated option (4-step, cost 76). The
    tie-break reaches the exact reference cost of 520.
    """
    from qef_ho_ra.cli import load_config

    cfg = load_config("scenarios/tiny-joint.json")
    bundle = Runner(cfg).episode(1)
    summary = bundle["summary"]
    assert summary["objective"]["stage3_resource_cost"] == 520
    assert summary["access_mix_planned"]["cg"] >= 1
    assert summary["access_mix_planned"]["2step-cbra"] >= 1


def test_mu_is_inert_under_option_wise_scoring():
    """Characterisation of a design gap: mu multiplies a UE-level constant.

    The score compares options of one UE, so ``mu * (1 - pressure[u])`` is the
    same for every candidate and cannot change the choice. Reported in the
    validation record as G1.
    """
    cfg = tiny()
    runner = Runner(cfg)
    a = runner.with_allocator(mu=0.0).episode(1)["allocation"]
    b = runner.with_allocator(mu=2.0).episode(1)["allocation"]
    picks_a = {uid: (r.option.access, r.option.start_slot)
               for uid, r in a.records.items() if r.option is not None}
    picks_b = {uid: (r.option.access, r.option.start_slot)
               for uid, r in b.records.items() if r.option is not None}
    assert picks_a == picks_b


def test_stage_tuple_components_are_consistent():
    cfg = tiny()
    summary = Runner(cfg).episode(1)["summary"]
    objective = summary["objective"]
    assert 0.0 <= objective["stage1_weighted_violation"] <= 1.0
    assert 0.0 <= objective["stage2_worst_supported_stratum"] <= 1.0
    assert objective["stage3_resource_cost"] > 0
    assert objective["stage3_resource_cost"] == sum(
        r.option.direct_cost for r in Runner(cfg).episode(1)["allocation"].records.values()
        if r.option is not None)
