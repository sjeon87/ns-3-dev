"""V4: the contention model follows n*(1-1/M)^(n-1).

The contention pool holds 56 preambles (64 total minus 8 dedicated), so the
design's worked example in section 7.6, which uses M=64, is 16.9 singletons;
with the declared split the same expression gives 12.6. See the validation
record.
"""

import math

import numpy as np
import pytest

from qef_ho_ra.population import STREAM_PREAMBLE, ue_rng


def singletons(n: int, pool: int, trials: int = 400) -> float:
    total = 0
    for seed in range(trials):
        draws = [int(ue_rng(seed, STREAM_PREAMBLE, uid).integers(0, pool))
                 for uid in range(n)]
        counts = {}
        for d in draws:
            counts[d] = counts.get(d, 0) + 1
        total += sum(1 for c in counts.values() if c == 1)
    return total / trials


def test_singleton_curve_follows_the_closed_form():
    n, pool = 131, 56
    expected = n * (1 - 1 / pool) ** (n - 1)
    observed = singletons(n, pool)
    # 95% band from the per-trial variance of a Poisson-like count.
    assert abs(observed - expected) / max(expected, 1e-9) < 0.05
    assert expected == pytest.approx(12.6, abs=0.2)


def test_singleton_curve_uses_the_contention_pool_not_the_total():
    from qef_ho_ra.config import ScenarioConfig
    from scenarios.generate import scenario

    cfg = ScenarioConfig.model_validate(scenario())
    total = cfg.radio.preamble_total
    dedicated = cfg.radio.preamble_dedicated
    pool = total - dedicated
    assert pool == 56  # declared split of DESIGN.md 7.3
    expected_pool = 131 * (1 - 1 / pool) ** 130
    expected_total = 131 * (1 - 1 / total) ** 130
    assert expected_pool == pytest.approx(12.6, abs=0.1)
    observed = singletons(131, pool)
    assert abs(observed - expected_pool) < abs(observed - expected_total)


def test_singletons_scale_with_load():
    pool = 56
    values = [singletons(n, pool, trials=120) for n in (20, 60, 200)]
    expected = [n * (1 - 1 / pool) ** (n - 1) for n in (20, 60, 200)]
    for observed, want in zip(values, expected):
        assert abs(observed - want) / want < 0.10
