"""Population generation: density, placement, capability profiles, budgets.

Every UE is placed uniformly at random inside its cell (area-uniform draw on the
hexagon). Capability and budget class derive from independent streams seeded by
the episode seed, so a configuration change never shifts an exogenous stream.
"""

from __future__ import annotations

import math
from dataclasses import dataclass

import numpy as np

from .config import ScenarioConfig
from .geometry import Layout, elevation_deg, one_way_delay_ms, tau_slots

# Stream identifiers of the episode seed sequence. Recorded in run.json.
STREAM_POSITION = 0
STREAM_CAPABILITY = 1
STREAM_BUDGET = 2
STREAM_PREAMBLE = 3
STREAM_DETECTION = 4
STREAM_MSGA = 5
STREAM_DG = 6
STREAM_SYNC = 7
STREAM_BACKOFF = 8
STREAM_NAMES = {
    STREAM_POSITION: "position",
    STREAM_CAPABILITY: "capability",
    STREAM_BUDGET: "budget",
    STREAM_PREAMBLE: "preamble",
    STREAM_DETECTION: "detection",
    STREAM_MSGA: "msga",
    STREAM_DG: "dg",
    STREAM_SYNC: "sync",
    STREAM_BACKOFF: "backoff",
}


@dataclass(frozen=True)
class UE:
    uid: int
    cell: int
    x_km: float
    y_km: float
    range_km: float
    elevation_deg: float
    delay_ms: float
    tau_slots: int
    capability: int
    budget_class: int
    budget_ms: int

    @property
    def profile_name(self) -> str:
        return f"P{self.capability + 1}"

    @property
    def budget_name(self) -> str:
        from . import BUDGET_CLASSES

        return BUDGET_CLASSES[self.budget_class]


def ue_rng(seed: int, stream: int, uid: int, attempt: int = 0) -> np.random.Generator:
    """Order-independent random stream for one (UE, stream, attempt)."""
    seq = np.random.SeedSequence([int(seed), int(stream), int(uid), int(attempt)])
    return np.random.Generator(np.random.PCG64(seq))


def _hex_point(center_x: float, center_y: float, radius: float, rng: np.random.Generator):
    tri = int(rng.integers(0, 6))
    u = rng.random()
    v = rng.random()
    s = math.sqrt(u)
    angle = math.radians(60.0 * tri)
    a = (radius * math.cos(angle), radius * math.sin(angle))
    angle_next = math.radians(60.0 * (tri + 1))
    b = (radius * math.cos(angle_next), radius * math.sin(angle_next))
    px = s * ((1.0 - v) * a[0] + v * b[0])
    py = s * ((1.0 - v) * a[1] + v * b[1])
    return center_x + px, center_y + py


def _draw_index(rng: np.random.Generator, weights: list[float]) -> int:
    x = rng.random()
    acc = 0.0
    for i, w in enumerate(weights):
        acc += w
        if x < acc:
            return i
    return len(weights) - 1


def _correlated_capability_shares(base: list[float], budget_class: int) -> list[float]:
    """Advanced-on-tight: P2..P7 weight x3 for the tight class, renormalised."""
    if budget_class != 0:
        return list(base)
    boosted = [base[0]] + [3.0 * s for s in base[1:]]
    total = sum(boosted)
    return [s / total for s in boosted]


def generate_population(cfg: ScenarioConfig, layout: Layout, seed: int,
                        per_cell: list[int] | None = None) -> list[UE]:
    """Generate the incoming UE batch of one episode."""
    from .config import per_cell_counts, spatial_weights

    if per_cell is None:
        total = int(round(
            cfg.population.density_persons_per_km2
            * cfg.population.terminal_penetration
            * cfg.population.active_fraction
            * layout.total_area_km2
        ))
        per_cell = per_cell_counts(total, layout.n_cells, spatial_weights(cfg, layout))

    ues: list[UE] = []
    uid = 0
    for cell in layout.cells:
        for _ in range(per_cell[cell.cid]):
            rng = ue_rng(seed, STREAM_POSITION, uid)
            x, y = _hex_point(cell.x_km, cell.y_km, layout.cell_radius_km, rng)
            r = math.hypot(x, y)
            delay = one_way_delay_ms(r, cfg.geometry.altitude_km)

            budget_rng = ue_rng(seed, STREAM_BUDGET, uid)
            budget_class = _draw_index(budget_rng, cfg.population.budget_shares)

            cap_rng = ue_rng(seed, STREAM_CAPABILITY, uid)
            shares = cfg.population.capability_shares
            if cfg.population.capability_budget_correlation == "advanced-on-tight":
                shares = _correlated_capability_shares(shares, budget_class)
            capability = _draw_index(cap_rng, shares)

            ues.append(
                UE(
                    uid=uid,
                    cell=cell.cid,
                    x_km=x,
                    y_km=y,
                    range_km=r,
                    elevation_deg=elevation_deg(r, cfg.geometry.altitude_km),
                    delay_ms=delay,
                    tau_slots=tau_slots(delay),
                    capability=capability,
                    budget_class=budget_class,
                    budget_ms=cfg.population.budget_ms[budget_class],
                )
            )
            uid += 1
    return ues
