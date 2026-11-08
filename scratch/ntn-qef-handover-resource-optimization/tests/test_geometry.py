"""V1: geometry against an independent calculation and the design table."""

import math

import pytest

from qef_ho_ra.geometry import (
    C_KM_S,
    RE_KM,
    build_layout,
    cell_area_km2,
    elevation_deg,
    footprint_radius_km,
    one_way_delay_ms,
    slant_range_km,
    tau_slots,
)


def independent_footprint(h_km, theta_min_deg):
    """Law-of-sines form of the footprint radius; independent of the module form."""
    theta = math.radians(theta_min_deg)
    # Earth central angle from the sat-UE geometry: e = arccos(Re*cos(theta)/(Re+h))
    e = math.acos(RE_KM * math.cos(theta) / (RE_KM + h_km))
    return RE_KM * (e - theta)


def independent_slant(r_km, h_km):
    """Chord form: d^2 = Re^2 + (Re+h)^2 - 2 Re (Re+h) cos(r/Re)."""
    return math.sqrt(RE_KM**2 + (RE_KM + h_km) ** 2
                     - 2 * RE_KM * (RE_KM + h_km) * math.cos(r_km / RE_KM))


def test_footprint_and_edge_delay_match_design_table():
    assert footprint_radius_km(600, 30) == pytest.approx(853.47, abs=0.01)
    assert one_way_delay_ms(0.0, 600) == pytest.approx(2.001, abs=1e-3)
    assert one_way_delay_ms(853.47, 600) == pytest.approx(3.586, abs=1e-3)
    assert footprint_radius_km(1200, 30) == pytest.approx(1469.71, abs=0.01)
    assert one_way_delay_ms(1469.71, 1200) == pytest.approx(6.668, abs=1e-3)


def test_footprint_matches_independent_form():
    for h, theta in [(400, 25), (600, 30), (1200, 40)]:
        assert footprint_radius_km(h, theta) == pytest.approx(
            independent_footprint(h, theta), rel=1e-12
        )
        r = footprint_radius_km(h, theta)
        assert slant_range_km(r, h) == pytest.approx(independent_slant(r, h), rel=1e-12)


def test_elevation_at_edge_is_minimum_elevation():
    assert elevation_deg(footprint_radius_km(600, 30), 600) == pytest.approx(30.0, abs=1e-9)
    assert elevation_deg(0.0, 600) == 90.0


def test_delay_increases_with_range():
    delays = [one_way_delay_ms(r, 600) for r in (0, 100, 300, 600, 852)]
    assert delays == sorted(delays)
    assert delays[0] == pytest.approx(2.001, abs=1e-3)


def test_cell_area_and_counts():
    assert cell_area_km2(41.0) == pytest.approx(4367.366, abs=1e-3)
    cluster = build_layout("cluster", 600, 30, 41, rings=3)
    assert cluster.n_cells == 37
    assert cluster.outer_center_radius_km == pytest.approx(213.0, abs=0.1)
    assert cluster.cells[0].range_km == 0.0
    assert cluster.total_area_km2 == pytest.approx(37 * 4367.366, abs=1e-1)
    full = build_layout("full", 600, 30, 41, rings=3)
    assert full.n_cells == 517
    assert full.outer_center_radius_km <= full.footprint_radius_km


def test_cluster_is_symmetric():
    cluster = build_layout("cluster", 600, 30, 41, rings=3)
    # A triangular lattice ring is not a circle: 6 distinct centre radii.
    radii = sorted({round(c.range_km, 6) for c in cluster.cells})
    assert len(radii) == 6
    assert radii[-1] == pytest.approx(3 * math.sqrt(3) * 41.0, abs=1e-6)
    assert sum(1 for c in cluster.cells if abs(c.range_km - 213.042) < 0.01) == 6


def test_tau_slots_rounds_up_and_is_at_least_one():
    assert tau_slots(2.0014) == 3
    assert tau_slots(3.586) == 4
    assert tau_slots(0.2) == 1
    assert tau_slots(2.0) == 2
