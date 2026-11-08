"""Engine gates: V5 boundaries, V6a information boundary, V6b replay, V8 determinism."""

import dataclasses
import hashlib
import math
import os

import pytest

from qef_ho_ra.allocator import AllocationResult, PlanRecord
from qef_ho_ra.engine import Episode
from qef_ho_ra.metrics import write_artifacts
from qef_ho_ra.run import Runner
from qef_ho_ra.config import ScenarioConfig
from qef_ho_ra.resources import Ledger, build_capacities
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


def digest_dir(path):
    out = {}
    for name in sorted(os.listdir(path)):
        with open(os.path.join(path, name), "rb") as handle:
            out[name] = hashlib.sha256(handle.read()).hexdigest()
    return out


def test_v8_determinism_is_byte_identical(tmp_path):
    cfg = tiny()
    runner = Runner(cfg)
    for target in ("a", "b"):
        bundle = runner.episode(3)
        write_artifacts(str(tmp_path / target), cfg, runner.resolved, runner.layout,
                        bundle["ues"], bundle["allocation"], bundle["episode"], 3,
                        bundle["summary"])
    assert digest_dir(tmp_path / "a") == digest_dir(tmp_path / "b")


def test_v6a_plan_does_not_read_outcome_draws(tmp_path):
    cfg = tiny(access={"detection_probability": 0.9})
    runner = Runner(cfg)
    a = runner.episode(5, outcome_seed=5)
    b = runner.episode(5, outcome_seed=999)
    write_artifacts(str(tmp_path / "a"), cfg, runner.resolved, runner.layout,
                    a["ues"], a["allocation"], a["episode"], 5, a["summary"])
    write_artifacts(str(tmp_path / "b"), cfg, runner.resolved, runner.layout,
                    b["ues"], b["allocation"], b["episode"], 5, b["summary"])
    with open(tmp_path / "a" / "plan.csv") as fa, open(tmp_path / "b" / "plan.csv") as fb:
        assert fa.read() == fb.read()
    with open(tmp_path / "a" / "ue-outcomes.csv") as fa, open(tmp_path / "b" / "ue-outcomes.csv") as fb:
        assert fa.read() != fb.read()


def test_v6b_success_path_replay_matches_the_prediction():
    """Clean first-attempt successes complete exactly at the predicted slot."""
    cfg = ScenarioConfig.model_validate(scenario())
    cfg = cfg.model_copy(update={"evaluation": cfg.evaluation.model_copy(
        update={"episodes": 1})})
    runner = Runner(cfg)
    bundle = runner.episode(1)
    v = bundle["summary"]["validation"]
    assert v["first_attempt_success_clean"] > 1000
    assert v["clean_success_mismatch"] == 0
    assert v["success_matched_prediction"] >= v["first_attempt_success_clean"]


def test_v5_attempt_limit_is_a_failure():
    cfg = tiny(
        population={"density_persons_per_km2": 3.0},
        allocator={"access_policy": "fixed", "allowed_access": ["4step-cbra"]},
        access={"max_attempts": 1},
    )
    bundle = Runner(cfg).episode(1)
    reasons = [o.reason for o in bundle["episode"].outcomes.values()]
    assert "max-attempts" in reasons


def test_v5_dg_decode_failure():
    cfg = tiny(
        population={"capability_shares": [0, 0, 0, 0, 0, 0, 1.0]},
        allocator={"access_policy": "fixed", "allowed_access": ["dg"]},
        access={"dg_decode_failure_probability": 1.0},
    )
    bundle = Runner(cfg).episode(1)
    assert all(o.reason == "dg-decode-failure"
               for o in bundle["episode"].outcomes.values())
    assert all(o.attempts == 0 for o in bundle["episode"].outcomes.values())


def test_v5_two_step_fallback_consumes_a_second_occasion():
    cfg = tiny(
        population={"capability_shares": [0, 1.0, 0, 0, 0, 0, 0],
                    "density_persons_per_km2": 0.7},
        allocator={"access_policy": "fixed", "allowed_access": ["2step-cbra"]},
        access={"msga_success_probability": 0.0, "max_attempts": 3},
    )
    bundle = Runner(cfg).episode(1)
    attempts = bundle["episode"].attempts
    assert any(a.access == "4step-cbra" for a in attempts)
    assert any(o.attempts >= 2 for o in bundle["episode"].outcomes.values())


def test_v5_access_policy_gap_and_capability_fallback():
    cfg = tiny(
        population={"capability_shares": [1.0, 0, 0, 0, 0, 0, 0]},
        allocator={"access_policy": "fixed", "allowed_access": ["cg"],
                   "capability_fallback": "strict"},
    )
    bundle = Runner(cfg).episode(1)
    assert all(o.reason == "no-legal-option" for o in bundle["episode"].outcomes.values())

    cfg = tiny(
        population={"capability_shares": [1.0, 0, 0, 0, 0, 0, 0]},
        allocator={"access_policy": "fixed", "allowed_access": ["cg"],
                   "capability_fallback": "4step-cbra"},
    )
    bundle = Runner(cfg).episode(1)
    assert all(r.fallback_used for r in bundle["allocation"].records.values())
    assert all(o.outcome == "success" for o in bundle["episode"].outcomes.values())
    assert bundle["plan" if "plan" in bundle else "summary"]["plan"]["fallback_used"] > 0


def test_v5_preamble_exhaustion():
    cfg = tiny(
        population={"density_persons_per_km2": 30.0},
        radio={"preamble_total": 64, "preamble_dedicated": 2},
        allocator={"access_policy": "fixed", "allowed_access": ["4step-cfra"],
                   "options_per_access": 1, "swap_cap": 0},
    )
    bundle = Runner(cfg).episode(1)
    reasons = [r.reason for r in bundle["allocation"].records.values()]
    assert "preamble-exhausted" in reasons


def _single_option_episode(access, start, budget=100):
    cfg = tiny()
    runner = Runner(cfg)
    ues, enum = runner._population(1)
    base = dataclasses.replace(ues[0], budget_ms=budget)
    option = enum._make_option(base, access, start, False)
    record = PlanRecord(base.uid, base.cell, option=option, reason="assigned")
    allocation = AllocationResult({base.uid: record},
                                  Ledger(build_capacities(runner.resolved),
                                         cfg.evaluation.horizon_slots), [], {})
    episode = Episode(cfg, runner.resolved, runner.layout, runner.hop, [base], enum,
                      allocation, 1, active_cells=[base.cell])
    return episode.run().outcomes[base.uid]


def test_v5_cg_and_dg_deadline_expire():
    cg = _single_option_episode("cg", 50, budget=10)
    assert cg.reason == "cg-timeout"
    dg = _single_option_episode("dg", 50, budget=10)
    assert dg.reason == "dg-timeout"


def test_v5_budget_equal_to_completion_is_not_a_violation():
    from qef_ho_ra.access import build_timeline

    runner = Runner(tiny())
    hop = runner.hop
    ues, _enum = runner._population(1)
    tl = build_timeline("cg", 10, 0, ues[0].delay_ms, ues[0].tau_slots, hop)
    u = dataclasses.replace(ues[0], budget_ms=int(math.ceil(tl.completion_ms)))
    assert not (tl.completion_ms > u.budget_ms)
    exact = dataclasses.replace(u, budget_ms=int(math.floor(tl.completion_ms)))
    assert tl.completion_ms > exact.budget_ms
