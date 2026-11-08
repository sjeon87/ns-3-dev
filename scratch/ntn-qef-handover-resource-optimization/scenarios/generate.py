"""Scenarios of the study: cluster, full footprint, tiny exact and tiny joint."""

import json
import math
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

BASE = {
    "schema_version": 6,
    "geometry": {
        "altitude_km": 600,
        "min_elevation_deg": 30,
        "cell_radius_km": 41,
        "profile": "cluster",
        "rings": 3,
    },
    "hop": {"active_cells": 19, "dwell_ms": 20, "phase_slots": 0},
    "radio": {
        "bandwidth_mhz": 15,
        "subcarrier_spacing_khz": 15,
        "spectral_efficiency_bits_per_re": 4.0,
        "prach_period_slots": 10,
        "preamble_total": 64,
        "preamble_dedicated": 8,
        "pdcch_cce_per_slot": 24,
        "max_rar_per_slot": 8,
        "max_dg_grants_per_slot": 8,
    },
    "population": {
        "density_persons_per_km2": 30,
        "terminal_penetration": 0.001,
        "active_fraction": 1.0,
        "capability_shares": [0.30, 0.25, 0.10, 0.10, 0.10, 0.10, 0.05],
        "budget_shares": [0.20, 0.30, 0.30, 0.20],
        "budget_ms": [100, 200, 400, 800],
        "capability_budget_correlation": "independent",
        "spatial_distribution": "uniform",
        "hotspot": {"center_cell": 0, "radius_km": 10, "weight": 0.8},
    },
    "access": {
        "max_attempts": 10,
        "backoff_slots": 8,
        "detection_probability": 1.0,
        "msga_success_probability": 0.99,
        "dg_decode_failure_probability": 0.0,
        "sync_error_probability": 0.01,
        "enable_two_step_fallback": True,
        "cg_period_slots": 8,
        "enable_sync_error": False,
    },
    "allocator": {
        "decision_latency_ms": 1,
        "options_per_access": 3,
        "lambda": 1.0,
        "mu": 1.0,
        "access_policy": "joint",
        "allowed_access": [
            "4step-cbra",
            "4step-cfra",
            "2step-cbra",
            "2step-cfra",
            "cg",
            "dg",
        ],
        "capability_fallback": "strict",
        "swap_cap": 100,
        "max_swap_passes": 10,
    },
    "evaluation": {"horizon_slots": 2000, "episodes": 30, "first_seed": 1},
}


def _deep_merge(base: dict, override: dict) -> dict:
    out = json.loads(json.dumps(base))
    for key, value in override.items():
        if isinstance(value, dict) and isinstance(out.get(key), dict):
            out[key] = _deep_merge(out[key], value)
        else:
            out[key] = value
    return out


def scenario(overrides: dict | None = None) -> dict:
    return _deep_merge(BASE, overrides or {})


def _tiny_density(n_cells: int, per_cell: int) -> float:
    from qef_ho_ra.geometry import cell_area_km2

    area = n_cells * cell_area_km2(41.0)
    return per_cell * n_cells / (0.001 * area)


def _write(path: str, doc: dict) -> None:
    with open(path, "w", encoding="utf-8") as handle:
        json.dump(doc, handle, indent=2, sort_keys=False)
        handle.write("\n")


def main() -> None:
    here = os.path.dirname(os.path.abspath(__file__))
    os.makedirs(here, exist_ok=True)

    cluster = scenario()
    footprint = scenario({
        "geometry": {"profile": "full", "rings": 3},
        "hop": {"active_cells": 64},
        "population": {"density_persons_per_km2": 5},
    })
    tiny = scenario({
        "geometry": {"rings": 1},
        "hop": {"active_cells": 7, "dwell_ms": 20},
        "population": {"density_persons_per_km2": _tiny_density(7, 3)},
        "evaluation": {"horizon_slots": 60, "episodes": 1, "first_seed": 1},
        "allocator": {"swap_cap": 0, "max_swap_passes": 0},
    })
    joint = scenario({
        "geometry": {"rings": 1},
        "hop": {"active_cells": 7, "dwell_ms": 20},
        "population": {"density_persons_per_km2": _tiny_density(7, 2)},
        "evaluation": {"horizon_slots": 60, "episodes": 1, "first_seed": 1},
        "allocator": {"swap_cap": 0, "max_swap_passes": 0},
    })

    _write(os.path.join(here, "cluster-37.json"), cluster)
    _write(os.path.join(here, "footprint-517.json"), footprint)
    _write(os.path.join(here, "tiny-exact.json"), tiny)
    _write(os.path.join(here, "tiny-joint.json"), joint)
    print("wrote scenarios to", here)


if __name__ == "__main__":
    main()
