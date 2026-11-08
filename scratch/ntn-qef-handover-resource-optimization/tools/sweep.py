"""Experiment harness: E1..E9 and the control sweep (DESIGN.md section 14).

Each point rebuilds the configuration from the base file, resolves it, runs the
declared seed block, and writes ``experiment.json`` with the per-episode values
and the aggregate. A point is skipped when its file exists and ``--resume`` is
given, so a long matrix can be restarted.

Usage:
  python tools/sweep.py --experiment E1 --out results --resume
  python tools/sweep.py --experiment E3 --seeds 3 --only cluster
"""

from __future__ import annotations

import argparse
import json
import os
import sys
import time

sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

from qef_ho_ra.config import ScenarioConfig, resolve  # noqa: E402
from qef_ho_ra.metrics import aggregate_summaries, paired_difference  # noqa: E402
from qef_ho_ra.run import Runner  # noqa: E402
from scenarios.generate import scenario  # noqa: E402

ARMS = ("4step-cbra", "4step-cfra", "2step-cbra", "2step-cfra", "cg", "dg")


def _alloc(access=None, **extra):
    block = {
        "access_policy": "fixed" if access else "joint",
        "allowed_access": [access] if access else list(ARMS),
        "capability_fallback": "strict",
    }
    block.update(extra)
    return {"allocator": block}


def points(experiment: str, seeds_main: int, seeds_sweep: int):
    base = scenario()
    out = []

    if experiment == "E1":
        out.append(("joint", {}, seeds_main, "joint access menu on the cluster"))
        for arm in ARMS:
            out.append((arm, _alloc(arm), seeds_main, f"strict arm: {arm} only"))

    elif experiment == "E2":
        for rho in (2, 10, 30, 100):
            out.append((f"density-{rho}", {"population": {"density_persons_per_km2": rho}},
                        seeds_sweep, f"density {rho} persons/km2"))

    elif experiment == "E3":
        for cmax in (7, 19, 37):
            for dwell in (5, 10, 20, 50, 100):
                out.append((f"cmax{cmax}-dwell{dwell}",
                            {"hop": {"active_cells": cmax, "dwell_ms": dwell}},
                            seeds_sweep, f"C_max {cmax}, dwell {dwell} ms, horizon K_min"))

    elif experiment == "E4":
        variants = {
            "tight-heavy": [0.35, 0.35, 0.20, 0.10],
            "default": [0.20, 0.30, 0.30, 0.20],
            "loose-heavy": [0.10, 0.20, 0.35, 0.35],
        }
        for name, shares in variants.items():
            out.append((name, {"population": {"budget_shares": shares}},
                        seeds_sweep, f"budget shares {shares}"))

    elif experiment == "E5":
        for dedicated in (0, 8, 16, 32):
            out.append((f"dedicated-{dedicated}",
                        {"radio": {"preamble_dedicated": dedicated}},
                        seeds_sweep, f"{dedicated} dedicated preambles"))

    elif experiment == "E6":
        for bw in (5, 15, 30):
            out.append((f"bandwidth-{bw}", {"radio": {"bandwidth_mhz": bw}},
                        seeds_sweep, f"{bw} MHz per cell"))

    elif experiment == "E7":
        for name in ("independent", "advanced-on-tight"):
            out.append((name, {"population": {"capability_budget_correlation": name}},
                        seeds_sweep, f"capability-budget correlation: {name}"))

    elif experiment == "E8":
        out.append(("full-profile", {
            "geometry": {"profile": "full"},
            "hop": {"active_cells": 64},
            "population": {"density_persons_per_km2": 5},
        }, seeds_sweep, "full footprint, density 5, C_max 64"))

    elif experiment == "E9":
        sensitivity = {
            "detection-1.00": {"access": {"detection_probability": 1.0}},
            "detection-0.99": {"access": {"detection_probability": 0.99}},
            "detection-0.90": {"access": {"detection_probability": 0.90}},
            "msga-0.99": {"access": {"msga_success_probability": 0.99}},
            "msga-0.95": {"access": {"msga_success_probability": 0.95}},
            "dg-0.00": {"access": {"dg_decode_failure_probability": 0.0}},
            "dg-0.01": {"access": {"dg_decode_failure_probability": 0.01}},
            "sync-off": {"access": {"enable_sync_error": False}},
            "sync-on-0.005": {"access": {"enable_sync_error": True,
                                         "sync_error_probability": 0.005}},
            "sync-on-0.01": {"access": {"enable_sync_error": True,
                                        "sync_error_probability": 0.01}},
            "fallback-off": {"access": {"enable_two_step_fallback": False}},
        }
        for name, override in sensitivity.items():
            out.append((name, override, seeds_sweep, "sensitivity, not a headline"))

    elif experiment == "C1":
        controls = {
            "mu-0": {"mu": 0.0},
            "mu-2": {"mu": 2.0},
            "lambda-0.1": {"lambda": 0.1},
            "lambda-10": {"lambda": 10.0},
            "latency-0": {"decision_latency_ms": 0},
            "latency-5": {"decision_latency_ms": 5},
            "options-1": {"options_per_access": 1},
            "options-5": {"options_per_access": 5},
            "swap-0": {"swap_cap": 0, "max_swap_passes": 0},
            "swap-500": {"swap_cap": 500},
        }
        for name, override in controls.items():
            out.append((name, {"allocator": {**scenario()["allocator"], **override}},
                        seeds_sweep, "control sensitivity"))
    else:
        raise SystemExit(f"unknown experiment {experiment!r}")

    del base
    return out


def run_point(experiment: str, name: str, overrides: dict, seeds: int, out_root: str,
             resume: bool, description: str = "") -> dict:
    path = os.path.join(out_root, experiment, name)
    target = os.path.join(path, "experiment.json")
    if resume and os.path.exists(target):
        with open(target, encoding="utf-8") as handle:
            return json.load(handle)

    cfg = ScenarioConfig.model_validate(scenario(overrides))
    resolved = resolve(cfg)
    if experiment == "E3":
        horizon = int(resolved["evaluation"]["k_min_slots"]) + 20
        horizon = ((horizon + 9) // 10) * 10
        cfg = cfg.model_copy(update={"evaluation": cfg.evaluation.model_copy(
            update={"horizon_slots": horizon, "episodes": seeds})})
        resolved = resolve(cfg)
    first = cfg.evaluation.first_seed
    seed_list = list(range(first, first + seeds))

    runner = Runner(cfg)
    started = time.time()
    summaries = []
    for seed in seed_list:
        summaries.append(runner.episode(seed)["summary"])
    aggregate = aggregate_summaries(summaries)
    doc = {
        "experiment": experiment,
        "point": name,
        "description": description,
        "seeds": seed_list,
        "horizon_slots": cfg.evaluation.horizon_slots,
        "resolved": {
            "config_sha256": resolved["config_sha256"],
            "geometry": {k: resolved["geometry"][k] for k in
                         ("profile", "n_cells", "footprint_radius_km", "edge_delay_ms")},
            "hop": resolved["hop"],
            "population": {k: resolved["population"][k] for k in
                           ("density_persons_per_km2", "total_ues", "mean_ues_per_cell")},
            "radio": {k: resolved["radio"][k] for k in
                      ("bandwidth_mhz", "prb", "contention_preambles",
                       "preamble_dedicated")},
            "allocator": resolved["allocator"],
            "evaluation": resolved["evaluation"],
        },
        "aggregate": aggregate,
        "episodes": [
            {
                "seed": s["run"]["seed"],
                "kpis": s["kpis"],
                "objective": s["objective"],
                "access_mix_planned": s["access_mix_planned"],
                "validation": s["validation"],
                "resources": s["resources"],
            }
            for s in summaries
        ],
        "wall_seconds": round(time.time() - started, 3),
    }
    os.makedirs(path, exist_ok=True)
    with open(target, "w", encoding="utf-8") as handle:
        json.dump(doc, handle, indent=2, sort_keys=True)
        handle.write("\n")
    return doc


def main(argv=None) -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--experiment", required=True,
                        help="E1..E9 or C1")
    parser.add_argument("--out", default="results")
    parser.add_argument("--seeds", type=int, default=None,
                        help="override the seed count of every point")
    parser.add_argument("--main-seeds", type=int, default=30,
                        help="seed count of the E1 headline arms")
    parser.add_argument("--sweep-seeds", type=int, default=3,
                        help="seed count of a sweep point")
    parser.add_argument("--only", default=None, help="run points matching this substring")
    parser.add_argument("--resume", action="store_true")
    parser.add_argument("--list", action="store_true")
    args = parser.parse_args(argv)

    pts = points(args.experiment, args.main_seeds, args.sweep_seeds)
    if args.only:
        pts = [p for p in pts if args.only in p[0]]
    if args.list:
        for name, _o, seeds, desc in pts:
            print(f"{args.experiment}/{name}: {seeds} seeds - {desc}")
        return 0

    for name, overrides, seeds, desc in pts:
        if args.seeds is not None:
            seeds = args.seeds
        started = time.time()
        doc = run_point(args.experiment, name, overrides, seeds, args.out,
                        args.resume, description=desc)
        k = doc["aggregate"]["kpis"]
        print(f"{args.experiment}/{name}: seeds {len(doc['seeds'])} "
              f"success {k['handover_success_ratio']['mean']:.4f} "
              f"violation {k['budget_violation_ratio']['mean']:.4f} "
              f"censored {k['censored_ratio']['mean']:.4f} "
              f"({time.time() - started:.1f}s)", flush=True)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
