"""Static geometry: Earth model, footprint, hexagonal cells, per-UE delay.

The model is the one in DESIGN.md section 5.1 and 5.2: a spherical Earth with
radius 6371 km, one satellite at altitude ``h``, and hexagonal cells of
circumradius ``r_c`` on the ground. Cells are fixed on the ground (quasi
Earth-fixed), so a stationary UE belongs to exactly one cell.
"""

from __future__ import annotations

import math
from dataclasses import dataclass

RE_KM = 6371.0
C_KM_S = 299792.458

SQRT3 = math.sqrt(3.0)


def footprint_central_angle_rad(altitude_km: float, min_elevation_deg: float) -> float:
    """Earth central angle from nadir to the footprint edge, radians."""
    theta = math.radians(min_elevation_deg)
    return math.acos(RE_KM * math.cos(theta) / (RE_KM + altitude_km)) - theta


def footprint_radius_km(altitude_km: float, min_elevation_deg: float) -> float:
    """Ground radius of the footprint, km."""
    return RE_KM * footprint_central_angle_rad(altitude_km, min_elevation_deg)


def elevation_deg(ground_range_km: float, altitude_km: float) -> float:
    """Elevation angle of a UE at ground range ``r`` from nadir."""
    if ground_range_km <= 0.0:
        return 90.0
    psi = ground_range_km / RE_KM
    return math.degrees(
        math.atan((math.cos(psi) - RE_KM / (RE_KM + altitude_km)) / math.sin(psi))
    )


def slant_range_km(ground_range_km: float, altitude_km: float) -> float:
    """Slant range from the satellite to a UE at ground range ``r``."""
    psi = ground_range_km / RE_KM
    return math.sqrt(
        RE_KM**2
        + (RE_KM + altitude_km) ** 2
        - 2.0 * RE_KM * (RE_KM + altitude_km) * math.cos(psi)
    )


def one_way_delay_ms(ground_range_km: float, altitude_km: float) -> float:
    """One-way propagation delay, ms."""
    return slant_range_km(ground_range_km, altitude_km) / C_KM_S * 1000.0


def cell_area_km2(cell_radius_km: float) -> float:
    """Area of a regular hexagon with circumradius ``r_c``."""
    return (3.0 * SQRT3 / 2.0) * cell_radius_km**2


@dataclass(frozen=True)
class Cell:
    """One quasi-Earth-fixed ground cell."""

    cid: int
    i: int
    j: int
    x_km: float
    y_km: float
    range_km: float
    elevation_deg: float
    delay_ms: float


@dataclass(frozen=True)
class Layout:
    """Resolved cell layout of one geometry profile."""

    profile: str
    cells: tuple[Cell, ...]
    cell_radius_km: float
    footprint_radius_km: float
    cell_area_km2: float
    rings: int

    @property
    def n_cells(self) -> int:
        return len(self.cells)

    @property
    def outer_center_radius_km(self) -> float:
        return max(c.range_km for c in self.cells)

    @property
    def total_area_km2(self) -> float:
        return self.cell_area_km2 * self.n_cells


def _axial_points(max_ring: int):
    for i in range(-max_ring, max_ring + 1):
        for j in range(-max_ring, max_ring + 1):
            if (abs(i) + abs(j) + abs(i + j)) // 2 <= max_ring:
                yield i, j


def _lattice_xy(i: int, j: int, spacing_km: float) -> tuple[float, float]:
    return spacing_km * (i + j / 2.0), spacing_km * (SQRT3 / 2.0) * j


def _build_layout(
    profile: str,
    altitude_km: float,
    min_elevation_deg: float,
    cell_radius_km: float,
    rings: int,
) -> Layout:
    spacing = SQRT3 * cell_radius_km
    if profile == "cluster":
        points = list(_axial_points(rings))
    elif profile == "full":
        radius = footprint_radius_km(altitude_km, min_elevation_deg)
        max_index = int(math.ceil(radius / spacing)) + 1
        points = []
        for i in range(-max_index, max_index + 1):
            for j in range(-max_index, max_index + 1):
                x, y = _lattice_xy(i, j, spacing)
                if math.hypot(x, y) <= radius + 1e-9:
                    points.append((i, j))
    else:  # pragma: no cover - config validation rejects this first
        raise ValueError(f"unknown geometry profile {profile!r}")

    raw = []
    for i, j in points:
        x, y = _lattice_xy(i, j, spacing)
        r = math.hypot(x, y)
        raw.append((r, math.atan2(y, x), i, j, x, y))
    # Deterministic order: ring first, then angle, then lattice index.
    raw.sort(key=lambda t: (round(t[0], 9), round(t[1], 12), t[2], t[3]))
    cells = tuple(
        Cell(
            cid=k,
            i=i,
            j=j,
            x_km=x,
            y_km=y,
            range_km=r,
            elevation_deg=elevation_deg(r, altitude_km),
            delay_ms=one_way_delay_ms(r, altitude_km),
        )
        for k, (r, _a, i, j, x, y) in enumerate(raw)
    )
    return Layout(
        profile=profile,
        cells=cells,
        cell_radius_km=cell_radius_km,
        footprint_radius_km=footprint_radius_km(altitude_km, min_elevation_deg),
        cell_area_km2=cell_area_km2(cell_radius_km),
        rings=rings,
    )


def build_layout(
    profile: str,
    altitude_km: float,
    min_elevation_deg: float,
    cell_radius_km: float,
    rings: int = 3,
) -> Layout:
    """Build the cell layout of a declared geometry profile."""
    return _build_layout(profile, altitude_km, min_elevation_deg, cell_radius_km, rings)


def tau_slots(delay_ms: float) -> int:
    """One-way delay in whole slots (ceiling), at least one slot.

    The slot grid is 1 ms. A message transmitted at a slot boundary arrives at
    ``slot + delay_ms``; the receiving slot is the first whole slot at or after
    that instant, so the delay is rounded up. The analytic no-wait expressions
    of the design use the exact delay and are therefore lower bounds.
    """
    return max(1, int(math.ceil(delay_ms - 1e-12)))
