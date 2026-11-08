"""Exact reference: decomposition equals joint enumeration; tiny-instance gap."""

import itertools

import pytest

from qef_ho_ra.reference import Candidate, combine, lower_bound, reference_optimum
from qef_ho_ra.config import ScenarioConfig
from qef_ho_ra.run import Runner
from qef_ho_ra.cli import load_config


def vector(**counts):
    base = [0] * 11
    for key, value in counts.items():
        base[int(key)] = value
    return tuple(base)


def test_combination_equals_joint_enumeration():
    frontiers = [
        [Candidate(vector(**{"0": 1}), 10), Candidate(vector(**{"1": 0}), 7)],
        [Candidate(vector(**{"2": 1}), 5), Candidate(vector(**{"0": 0}), 9)],
        [Candidate(vector(**{"0": 1}), 3), Candidate(vector(**{"1": 1}), 2)],
    ]
    sizes = [4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4]
    supported = [True] * 11
    reference = combine(frontiers, sizes, 12.0, supported)["tuple"]

    best = None
    for combo in itertools.product(*frontiers):
        counts = [0] * 11
        cost = 0
        for cand in combo:
            counts = [a + b for a, b in zip(counts, cand.counts)]
            cost += cand.cost
        stage1 = sum(counts) / 12.0
        stage2 = max(counts[i] / sizes[i] for i in range(11))
        tup = (round(stage1, 12), round(stage2, 12), cost)
        if best is None or tup < best:
            best = tup
    assert tuple(reference) == best


def test_combination_keeps_stage2_distinct_states():
    """Two plans with one violation each differ in the protected stratum."""
    a = Candidate(vector(**{"0": 1}), 10)   # capability P1, size 1
    b = Candidate(vector(**{"7": 1}), 10)   # budget tight, size 2
    sizes = [1, 10, 10, 10, 10, 10, 10, 2, 10, 10, 10]
    # Both cells offer one violation; only the vector tells them apart.
    result = combine([[a, b]], sizes, 2.0, [True] * 11)
    assert result["tuple"][0] == pytest.approx(0.5)
    assert result["tuple"][1] == pytest.approx(0.5)


def test_tiny_instance_reference_bounds_the_allocator():
    cfg = load_config("scenarios/tiny-joint.json")
    runner = Runner(cfg)
    bundle = runner.episode(1)
    ref = reference_optimum(cfg, runner.resolved, runner.layout, runner.hop,
                            bundle["ues"], bundle["enumerator"], seed=1)
    optimum = ref["optimum"]["tuple"]
    allocator = bundle["summary"]["objective"]
    allocator_tuple = (allocator["stage1_weighted_violation"],
                       allocator["stage2_worst_supported_stratum"],
                       allocator["stage3_resource_cost"])
    assert tuple(optimum) <= allocator_tuple
    assert optimum[0] == pytest.approx(bundle["summary"]["kpis"]["budget_violation_ratio"])
    assert all(size >= 1 for size in ref["frontier_sizes"])


def test_lower_bound_marks_infeasible_budgets():
    cfg = ScenarioConfig.model_validate({
        **load_config("scenarios/tiny-joint.json").model_dump(by_alias=True),
    })
    runner = Runner(load_config("scenarios/tiny-joint.json"))
    bundle = runner.episode(1)
    lb = lower_bound(cfg, bundle["ues"], runner.hop)
    assert lb["forced_violations"] <= bundle["summary"]["kpis"]["n_ues"]
    assert lb["forced_rate"] <= bundle["summary"]["kpis"]["budget_violation_ratio"] + 1e-9
