"""Population: counts, placement, delay range, determinism, shares."""

import math

import pytest

from qef_ho_ra.config import ScenarioConfig, resolve
from qef_ho_ra.geometry import build_layout
from qef_ho_ra.population import generate_population
from scenarios.generate import scenario


def make(overrides=None):
    cfg = ScenarioConfig.model_validate(scenario(overrides or {}))
    resolved = resolve(cfg)
    layout = build_layout(
        cfg.geometry.profile, cfg.geometry.altitude_km, cfg.geometry.min_elevation_deg,
        cfg.geometry.cell_radius_km, cfg.geometry.rings,
    )
    return cfg, resolved, layout


def inside_hexagon(dx, dy, radius):
    """Regular hexagon with vertices at multiples of 60 degrees."""
    apothem = radius * math.sqrt(3) / 2
    for k in range(6):
        angle = math.radians(30 + 60 * k)
        if dx * math.cos(angle) + dy * math.sin(angle) > apothem + 1e-9:
            return False
    return True


def test_counts_and_placement():
    cfg, resolved, layout = make()
    ues = generate_population(cfg, layout, seed=1)
    assert len(ues) == 4848
    per_cell = [0] * layout.n_cells
    for ue in ues:
        per_cell[ue.cell] += 1
        cell = layout.cells[ue.cell]
        assert inside_hexagon(ue.x_km - cell.x_km, ue.y_km - cell.y_km,
                              layout.cell_radius_km)
        assert ue.range_km == pytest.approx(math.hypot(ue.x_km, ue.y_km))
    assert per_cell == resolved["population"]["per_cell_counts"]
    delays = [u.range_km for u in ues]
    assert 0 <= min(delays) and max(delays) <= 3 * 71.0141 + 41.0 + 1e-6


def test_delay_range_matches_geometry():
    cfg, _resolved, layout = make()
    ues = generate_population(cfg, layout, seed=3)
    assert min(u.delay_ms for u in ues) == pytest.approx(2.001, abs=2e-3)
    assert max(u.delay_ms for u in ues) < 2.20
    assert all(u.tau_slots == 3 for u in ues)


def test_same_seed_is_identical():
    cfg, _r, layout = make()
    a = generate_population(cfg, layout, seed=7)
    b = generate_population(cfg, layout, seed=7)
    assert a == b


def test_different_seed_moves_positions_and_draws():
    cfg, _r, layout = make()
    a = generate_population(cfg, layout, seed=7)
    b = generate_population(cfg, layout, seed=8)
    assert any(x.range_km != y.range_km for x, y in zip(a, b))
    assert any(x.capability != y.capability for x, y in zip(a, b))


def test_capability_and_budget_shares():
    cfg, _r, layout = make()
    ues = generate_population(cfg, layout, seed=11)
    caps = [0] * 7
    budgets = [0] * 4
    for ue in ues:
        caps[ue.capability] += 1
        budgets[ue.budget_class] += 1
    n = len(ues)
    for share, count in zip(cfg.population.capability_shares, caps):
        assert count / n == pytest.approx(share, abs=0.02)
    for share, count in zip(cfg.population.budget_shares, budgets):
        assert count / n == pytest.approx(share, abs=0.02)
    # 4-step is in every profile
    assert all(True for _ in ues)


def test_hotspot_concentrates_population():
    cfg, resolved, layout = make({
        "population": {"spatial_distribution": "hotspot",
                       "hotspot": {"center_cell": 0, "radius_km": 10, "weight": 0.8}},
    })
    counts = resolved["population"]["per_cell_counts"]
    assert sum(counts) == resolved["population"]["total_ues"]
    assert counts[0] > 10 * max(counts[1:])


def test_advanced_on_tight_correlation():
    cfg, _r, layout = make({
        "population": {"capability_budget_correlation": "advanced-on-tight"},
    })
    ues = generate_population(cfg, layout, seed=5)
    tight_advanced = sum(1 for u in ues if u.budget_class == 0 and u.capability > 0)
    tight_total = sum(1 for u in ues if u.budget_class == 0)
    other_advanced = sum(1 for u in ues if u.budget_class > 0 and u.capability > 0)
    other_total = sum(1 for u in ues if u.budget_class > 0)
    assert tight_advanced / tight_total > other_advanced / other_total + 0.10
