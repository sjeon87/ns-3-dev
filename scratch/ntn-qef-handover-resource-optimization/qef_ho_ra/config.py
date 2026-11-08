"""Typed configuration schema (schema_version 6) and the resolved document.

The loader rejects a missing key, an out-of-range value and a cross-field
inconsistency. It never edits a key silently. The resolved document adds every
derived quantity and a config hash; it is written to ``run.json``.
"""

from __future__ import annotations

import hashlib
import json
import math
from typing import Literal

from pydantic import BaseModel, ConfigDict, Field, model_validator

from . import ACCESS_TYPES, CAPABILITY_PROFILES, SCHEMA_VERSION
from .geometry import build_layout, cell_area_km2, one_way_delay_ms
from .hop import HopSchedule


class _Strict(BaseModel):
    model_config = ConfigDict(extra="forbid", allow_inf_nan=False)


class GeometryCfg(_Strict):
    altitude_km: float = Field(gt=100.0, le=2000.0)
    min_elevation_deg: float = Field(ge=10.0, lt=90.0)
    cell_radius_km: float = Field(gt=0.0, le=2000.0)
    profile: Literal["cluster", "full"]
    rings: int = Field(ge=1, le=8)


class HopCfg(_Strict):
    active_cells: int = Field(ge=1)
    dwell_ms: int = Field(ge=1)
    phase_slots: int = Field(ge=0)


class RadioCfg(_Strict):
    bandwidth_mhz: float = Field(gt=0.0, le=400.0)
    subcarrier_spacing_khz: Literal[15, 30]
    spectral_efficiency_bits_per_re: float = Field(gt=0.0, le=12.0)
    prach_period_slots: int = Field(ge=1)
    preamble_total: int = Field(ge=1, le=64)
    preamble_dedicated: int = Field(ge=0, le=64)
    pdcch_cce_per_slot: int = Field(ge=1)
    max_rar_per_slot: int = Field(ge=1)
    max_dg_grants_per_slot: int = Field(ge=1)

    @model_validator(mode="after")
    def _preamble_split(self):
        if self.preamble_dedicated > self.preamble_total:
            raise ValueError("preamble_dedicated must not exceed preamble_total")
        return self


class HotspotCfg(_Strict):
    center_cell: int = Field(ge=0)
    radius_km: float = Field(gt=0.0)
    weight: float = Field(ge=0.0, le=1.0)


class PopulationCfg(_Strict):
    density_persons_per_km2: float = Field(gt=0.0)
    terminal_penetration: float = Field(gt=0.0, le=1.0)
    active_fraction: float = Field(gt=0.0, le=1.0)
    capability_shares: list[float]
    budget_shares: list[float]
    budget_ms: list[int]
    capability_budget_correlation: Literal["independent", "advanced-on-tight"]
    spatial_distribution: Literal["uniform", "hotspot"]
    hotspot: HotspotCfg

    @model_validator(mode="after")
    def _shares(self):
        if len(self.capability_shares) != 7:
            raise ValueError("capability_shares must list seven profiles")
        if any(s < 0.0 for s in self.capability_shares):
            raise ValueError("capability_shares must be non-negative")
        if abs(sum(self.capability_shares) - 1.0) > 1e-9:
            raise ValueError("capability_shares must sum to one")
        if len(self.budget_shares) != 4 or len(self.budget_ms) != 4:
            raise ValueError("budget shares and budget_ms must list four classes")
        if any(s < 0.0 for s in self.budget_shares):
            raise ValueError("budget_shares must be non-negative")
        if abs(sum(self.budget_shares) - 1.0) > 1e-9:
            raise ValueError("budget_shares must sum to one")
        if any(b < 1 for b in self.budget_ms):
            raise ValueError("every delay budget must be at least the 1 ms shortest procedure")
        if self.budget_ms != sorted(self.budget_ms):
            raise ValueError("budget_ms must be ascending")
        return self


class AccessCfg(_Strict):
    max_attempts: int = Field(ge=1, le=100)
    backoff_slots: int = Field(ge=0)
    detection_probability: float = Field(ge=0.0, le=1.0)
    msga_success_probability: float = Field(ge=0.0, le=1.0)
    dg_decode_failure_probability: float = Field(ge=0.0, le=1.0)
    sync_error_probability: float = Field(ge=0.0, le=1.0)
    enable_two_step_fallback: bool
    cg_period_slots: int = Field(ge=1)
    enable_sync_error: bool


class AllocatorCfg(_Strict):
    decision_latency_ms: int = Field(ge=0)
    options_per_access: int = Field(ge=1, le=8)
    lambda_: float = Field(alias="lambda", ge=0.0)
    mu: float = Field(ge=0.0)
    access_policy: Literal["joint", "fixed"]
    allowed_access: list[str]
    capability_fallback: Literal["strict", "4step-cbra", "best-legal"]
    swap_cap: int = Field(ge=0)
    max_swap_passes: int = Field(ge=0)

    @model_validator(mode="after")
    def _policy(self):
        unknown = [a for a in self.allowed_access if a not in ACCESS_TYPES]
        if unknown:
            raise ValueError(f"unknown access type(s): {unknown}")
        if len(set(self.allowed_access)) != len(self.allowed_access):
            raise ValueError("allowed_access must not repeat a type")
        if self.access_policy == "fixed" and not self.allowed_access:
            raise ValueError("a fixed access policy needs a non-empty allowed_access")
        if self.access_policy == "joint" and not self.allowed_access:
            raise ValueError("the joint policy needs a non-empty allowed_access")
        return self


class EvaluationCfg(_Strict):
    horizon_slots: int = Field(ge=1)
    episodes: int = Field(ge=1)
    first_seed: int = Field(ge=0)


class ScenarioConfig(_Strict):
    schema_version: int
    geometry: GeometryCfg
    hop: HopCfg
    radio: RadioCfg
    population: PopulationCfg
    access: AccessCfg
    allocator: AllocatorCfg
    evaluation: EvaluationCfg

    @model_validator(mode="after")
    def _schema(self):
        if self.schema_version != SCHEMA_VERSION:
            raise ValueError(
                f"schema_version must be {SCHEMA_VERSION}, got {self.schema_version}"
            )
        return self


def config_hash(doc: dict) -> str:
    """Stable hash of a configuration document."""
    blob = json.dumps(doc, sort_keys=True, separators=(",", ":")).encode()
    return hashlib.sha256(blob).hexdigest()


def _largest_remainder(total: int, weights: list[float]) -> list[int]:
    denom = sum(weights)
    raw = [total * w / denom for w in weights]
    floors = [int(math.floor(x)) for x in raw]
    remaining = total - sum(floors)
    order = sorted(range(len(weights)), key=lambda i: (-(raw[i] - floors[i]), i))
    for i in order[:remaining]:
        floors[i] += 1
    return floors


def per_cell_counts(total: int, n_cells: int, weights: list[float] | None = None) -> list[int]:
    """Largest-remainder split of ``total`` UEs over cells, ties to lower index."""
    if weights is None:
        weights = [1.0] * n_cells
    return _largest_remainder(total, list(weights))


def spatial_weights(cfg: ScenarioConfig, layout) -> list[float]:
    """Per-cell population weight: uniform, or the declared hotspot block."""
    if cfg.population.spatial_distribution == "hotspot":
        hot = cfg.population.hotspot
        if hot.center_cell >= layout.n_cells:
            raise ValueError("hotspot center_cell is outside the layout")
        center = layout.cells[hot.center_cell]
        inside = [
            math.hypot(c.x_km - center.x_km, c.y_km - center.y_km) <= hot.radius_km
            for c in layout.cells
        ]
        n_inside = sum(1 for flag in inside if flag)
        # Declared rule: the hotspot cells share w*N_cells of weight equally,
        # the other cells share (1-w)*N_cells equally. w=0 is uniform.
        return [
            (hot.weight * layout.n_cells / n_inside if flag else
             (1.0 - hot.weight) * layout.n_cells / (layout.n_cells - n_inside))
            for flag in inside
        ]
    return [1.0] * layout.n_cells


def resolve(cfg: ScenarioConfig) -> dict:
    """Build the resolved configuration document (config plus derived values)."""
    g = cfg.geometry
    layout = build_layout(
        g.profile, g.altitude_km, g.min_elevation_deg, g.cell_radius_km, g.rings
    )
    if cfg.hop.active_cells > layout.n_cells:
        raise ValueError(
            f"active_cells={cfg.hop.active_cells} exceeds n_cells={layout.n_cells}"
        )
    hop = HopSchedule(
        n_cells=layout.n_cells,
        active_cells=cfg.hop.active_cells,
        dwell_slots=cfg.hop.dwell_ms,
        phase_slots=cfg.hop.phase_slots,
    )
    if cfg.hop.phase_slots >= hop.cycle_slots:
        raise ValueError("phase_slots must be smaller than the hop cycle")

    prb = int(math.floor(cfg.radio.bandwidth_mhz * 1e6 / (12.0 * cfg.radio.subcarrier_spacing_khz * 1e3)))
    ul_symbols = prb * 12
    dl_symbols = prb * 11
    rate_ul_mbps = (
        ul_symbols * 12 * cfg.radio.spectral_efficiency_bits_per_re / 1e-3 / 1e6
    )

    total_ues = int(round(
        cfg.population.density_persons_per_km2
        * cfg.population.terminal_penetration
        * cfg.population.active_fraction
        * layout.total_area_km2
    ))
    weights = spatial_weights(cfg, layout)
    counts = per_cell_counts(total_ues, layout.n_cells, weights)

    max_budget = max(cfg.population.budget_ms)
    worst_wait = hop.cycle_slots - hop.dwell_slots
    k_min = max_budget + cfg.access.max_attempts * (worst_wait + 19)
    resolved_hop = {
        "active_cells": cfg.hop.active_cells,
        "dwell_ms": cfg.hop.dwell_ms,
        "phase_slots": cfg.hop.phase_slots,
        "groups": hop.groups,
        "cycle_slots": hop.cycle_slots,
        "group_sizes": list(hop.group_sizes()),
        "per_cell_duty": hop.per_cell_duty,
    }
    resolved = {
        "schema_version": cfg.schema_version,
        "config_sha256": config_hash(cfg.model_dump(by_alias=True)),
        "geometry": {
            **g.model_dump(),
            "footprint_radius_km": layout.footprint_radius_km,
            "cell_area_km2": cell_area_km2(g.cell_radius_km),
            "n_cells": layout.n_cells,
            "outer_center_radius_km": layout.outer_center_radius_km,
            "nadir_delay_ms": one_way_delay_ms(0.0, g.altitude_km),
            "edge_delay_ms": one_way_delay_ms(layout.footprint_radius_km, g.altitude_km),
            "total_area_km2": layout.total_area_km2,
        },
        "hop": resolved_hop,
        "radio": {
            **cfg.radio.model_dump(),
            "prb": prb,
            "ul_prb_symbols": ul_symbols,
            "dl_prb_symbols": dl_symbols,
            "instantaneous_ul_rate_mbps": rate_ul_mbps,
            "contention_preambles": cfg.radio.preamble_total - cfg.radio.preamble_dedicated,
        },
        "population": {
            **cfg.population.model_dump(),
            "total_ues": total_ues,
            "per_cell_counts": counts,
            "mean_ues_per_cell": total_ues / layout.n_cells,
        },
        "access": cfg.access.model_dump(),
        "allocator": cfg.allocator.model_dump(by_alias=True),
        "evaluation": {
            **cfg.evaluation.model_dump(),
            "k_min_slots": k_min,
            "horizon_covers_retry_path": cfg.evaluation.horizon_slots >= k_min,
            "censored_risk": cfg.evaluation.horizon_slots < k_min,
        },
    }
    return resolved


def capability_types(profile_index: int) -> tuple[str, ...]:
    return CAPABILITY_PROFILES[f"P{profile_index + 1}"]
