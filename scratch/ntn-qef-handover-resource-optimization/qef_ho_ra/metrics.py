"""KPIs, strata, conservation and artifact writers (DESIGN.md section 10)."""

from __future__ import annotations

import csv
import json
import math
import os

from . import BUDGET_CLASSES, CAPABILITY_PROFILES
from .resources import POOLS


def _percentile(values: list[float], q: float) -> float:
    if not values:
        return 0.0
    ordered = sorted(values)
    if len(ordered) == 1:
        return ordered[0]
    pos = (len(ordered) - 1) * q
    lo = int(math.floor(pos))
    hi = int(math.ceil(pos))
    if lo == hi:
        return ordered[lo]
    return ordered[lo] + (ordered[hi] - ordered[lo]) * (pos - lo)


def _stats(values: list[float]) -> dict:
    if not values:
        return {"n": 0, "mean": 0.0, "median": 0.0, "p95": 0.0, "p99": 0.0, "max": 0.0}
    return {
        "n": len(values),
        "mean": sum(values) / len(values),
        "median": _percentile(values, 0.50),
        "p95": _percentile(values, 0.95),
        "p99": _percentile(values, 0.99),
        "max": max(values),
    }


def _stratum_rows(groups, key_name: str):
    rows = []
    for key, uids in groups.items():
        n = len(uids)
        success = sum(1 for u in uids if u.outcome == "success")
        violation = sum(1 for u in uids if u.budget_violation)
        censored = sum(1 for u in uids if u.censored)
        attempts = sum(u.attempts for u in uids)
        rows.append({
            key_name: key,
            "n_ues": n,
            "success": success,
            "violation": violation,
            "censored": censored,
            "attempts": attempts,
            "success_rate": success / n if n else 0.0,
            "violation_rate": violation / n if n else 0.0,
            "low_support": n < 20 or (attempts < 100 and attempts > 0),
        })
    return rows


def analyze(cfg, resolved: dict, layout, ues, allocation, episode, seed: int) -> dict:
    """Compute the KPI document of one episode."""
    outcomes = episode.outcomes
    n_in = len(ues)
    n_success = sum(1 for o in outcomes.values() if o.outcome == "success")
    n_violation = sum(1 for o in outcomes.values() if o.budget_violation)
    n_censored = sum(1 for o in outcomes.values() if o.censored)
    completions = [o.completion_ms for o in outcomes.values() if o.completion_ms is not None]
    attempts = [o.attempts for o in outcomes.values()]
    total_attempts = sum(attempts)
    reattempts = sum(max(0, a - 1) for a in attempts if a > 0)
    with_attempt = sum(1 for a in attempts if a > 0)

    planned_mix: dict[str, int] = {a: 0 for a in (
        "4step-cbra", "4step-cfra", "2step-cbra", "2step-cfra", "cg", "dg", "unassigned")}
    realised_mix: dict[str, int] = {a: 0 for a in (
        "4step-cbra", "4step-cfra", "2step-cbra", "2step-cfra", "cg", "dg", "none")}
    for uid, rec in allocation.records.items():
        key = rec.option.access if rec.option is not None else "unassigned"
        planned_mix[key] = planned_mix.get(key, 0) + 1
    for o in outcomes.values():
        key = o.realised_access if o.realised_access else "none"
        realised_mix[key] = realised_mix.get(key, 0) + 1

    consumed_totals = {p: 0 for p in POOLS}
    for demands in episode.consumed.values():
        for d in demands:
            consumed_totals[d.pool] += d.amount
    ledger_totals = episode.ledger.totals()
    conservation_ok = all(consumed_totals[p] == ledger_totals.get(p, 0) for p in POOLS)

    plan_reasons: dict[str, int] = {}
    for rec in allocation.records.values():
        if rec.option is None:
            plan_reasons[rec.reason] = plan_reasons.get(rec.reason, 0) + 1

    by_cap: dict[str, list] = {k: [] for k in CAPABILITY_PROFILES}
    by_budget: dict[str, list] = {k: [] for k in BUDGET_CLASSES}
    by_cell: dict[str, list] = {str(c.cid): [] for c in layout.cells}
    by_access: dict[str, list] = {a: [] for a in (
        "4step-cbra", "4step-cfra", "2step-cbra", "2step-cfra", "cg", "dg")}
    for ue in ues:
        o = outcomes[ue.uid]
        by_cap[ue.profile_name].append(o)
        by_budget[ue.budget_name].append(o)
        by_cell[str(ue.cell)].append(o)
        if o.access:
            by_access[o.access].append(o)

    matched = sum(1 for o in outcomes.values() if o.matched_prediction)
    first_attempt_success = [o for o in outcomes.values()
                             if o.outcome == "success" and o.attempts == 1]
    delayed = 0
    for o in first_attempt_success:
        rec = allocation.records[o.ue]
        if rec.option is not None and o.response_slot is not None:
            if rec.option.timeline.response_slot is not None and \
                    o.response_slot > rec.option.timeline.response_slot:
                delayed += 1
    clean = len(first_attempt_success) - delayed
    mismatched_clean = sum(
        1 for o in first_attempt_success
        if o.response_slot is not None
        and allocation.records[o.ue].option is not None
        and o.response_slot == allocation.records[o.ue].option.timeline.response_slot
        and not o.matched_prediction
    )

    counters = getattr(episode, "stats", {}).get("counters", {})
    cg_waste = 12 * counters.get("cg_sync_error", 0)

    summary = {
        "run": {
            "seed": seed,
            "policy": cfg.allocator.access_policy,
            "allowed_access": list(cfg.allocator.allowed_access),
            "capability_fallback": cfg.allocator.capability_fallback,
            "horizon_slots": cfg.evaluation.horizon_slots,
        },
        "kpis": {
            "n_ues": n_in,
            "n_success": n_success,
            "n_violation": n_violation,
            "n_censored": n_censored,
            "n_assigned": sum(1 for r in allocation.records.values() if r.option is not None),
            "handover_success_ratio": n_success / n_in if n_in else 0.0,
            "handover_failure_ratio": (n_in - n_success) / n_in if n_in else 0.0,
            "budget_violation_ratio": n_violation / n_in if n_in else 0.0,
            "censored_ratio": n_censored / n_in if n_in else 0.0,
            "censored_horizon_limited": (n_censored / n_in if n_in else 0.0) > 0.01,
            "completion_ms": _stats(completions),
            "total_attempts": total_attempts,
            "mean_attempts_per_ue": total_attempts / n_in if n_in else 0.0,
            "mean_reattempts_per_attempting_ue": reattempts / with_attempt if with_attempt else 0.0,
            "ues_with_attempt": with_attempt,
        },
        "objective": {
            "stage1_weighted_violation": allocation.stats["objective"][0],
            "stage2_worst_supported_stratum": allocation.stats["objective"][1],
            "stage3_resource_cost": allocation.stats["objective"][2],
        },
        "access_mix_planned": planned_mix,
        "access_mix_realised": realised_mix,
        "plan": {
            "assigned": allocation.stats["assigned"],
            "reasons": plan_reasons,
            "fallback_used": sum(1 for r in allocation.records.values() if r.fallback_used),
            "swap": allocation.stats.get("swap", {}),
        },
        "resources": {
            "ledger_totals": ledger_totals,
            "consumed_totals": consumed_totals,
            "conservation_ok": conservation_ok,
            "cg_waste_prb_symbols": cg_waste,
            "cg_sync_errors": counters.get("cg_sync_error", 0),
            "dg_decode_failures": counters.get("dg_decode_failure", 0),
        },
        "validation": {
            "success_matched_prediction": matched,
            "first_attempt_success": len(first_attempt_success),
            "first_attempt_success_clean": clean,
            "first_attempt_success_delayed": delayed,
            "clean_success_mismatch": mismatched_clean,
        },
        "strata": {
            "capability": _stratum_rows(by_cap, "capability"),
            "budget": _stratum_rows(by_budget, "budget"),
            "cell": _stratum_rows(by_cell, "cell"),
            "access": _stratum_rows(by_access, "access"),
        },
        "event_count": len(episode.attempts),
    }
    return summary


def _rounded(obj):
    if isinstance(obj, float):
        if math.isnan(obj) or math.isinf(obj):
            return None
        return round(obj, 9)
    if isinstance(obj, dict):
        return {k: _rounded(v) for k, v in obj.items()}
    if isinstance(obj, (list, tuple)):
        return [_rounded(v) for v in obj]
    return obj


def aggregate_summaries(summaries: list[dict]) -> dict:
    """Mean, sd and 95% CI of the headline KPIs across episodes."""
    from scipy import stats

    keys = (
        "handover_success_ratio",
        "budget_violation_ratio",
        "censored_ratio",
        "mean_attempts_per_ue",
        "mean_reattempts_per_attempting_ue",
        "total_attempts",
        "n_ues",
        "n_success",
        "n_violation",
    )
    out = {"episodes": len(summaries), "kpis": {}, "completion": {}}
    if not summaries:
        return out
    for key in keys:
        values = [float(s["kpis"][key]) for s in summaries]
        n = len(values)
        mean = sum(values) / n
        sd = (sum((v - mean) ** 2 for v in values) / (n - 1)) ** 0.5 if n > 1 else 0.0
        half = float(stats.t.ppf(0.975, n - 1)) * sd / (n ** 0.5) if n > 1 else 0.0
        out["kpis"][key] = {"mean": mean, "sd": sd, "ci95": [mean - half, mean + half],
                            "values": values}
    for key in ("median", "p95", "p99", "max", "mean"):
        values = [float(s["kpis"]["completion_ms"][key]) for s in summaries]
        n = len(values)
        mean = sum(values) / n
        sd = (sum((v - mean) ** 2 for v in values) / (n - 1)) ** 0.5 if n > 1 else 0.0
        half = float(stats.t.ppf(0.975, n - 1)) * sd / (n ** 0.5) if n > 1 else 0.0
        out["completion"][key] = {"mean": mean, "sd": sd, "ci95": [mean - half, mean + half],
                                  "values": values}
    cost = [float(s["objective"]["stage3_resource_cost"]) for s in summaries]
    out["stage3_resource_cost"] = {"mean": sum(cost) / len(cost),
                                   "values": cost}
    out["censored_horizon_limited"] = any(
        s["kpis"]["censored_horizon_limited"] for s in summaries)
    out["conservation_ok"] = all(s["resources"]["conservation_ok"] for s in summaries)
    return out


def paired_difference(a: list[float], b: list[float]) -> dict:
    """Mean paired difference a - b with a 95% CI (paired seeds)."""
    from scipy import stats

    diffs = [x - y for x, y in zip(a, b)]
    n = len(diffs)
    if n == 0:
        return {"mean": 0.0, "ci95": [0.0, 0.0], "n": 0}
    mean = sum(diffs) / n
    sd = (sum((d - mean) ** 2 for d in diffs) / (n - 1)) ** 0.5 if n > 1 else 0.0
    half = float(stats.t.ppf(0.975, n - 1)) * sd / (n ** 0.5) if n > 1 else 0.0
    return {"mean": mean, "ci95": [mean - half, mean + half], "n": n,
            "excludes_zero": (mean - half) * (mean + half) > 0}


def _write_csv(path: str, header: list[str], rows) -> None:
    with open(path, "w", newline="", encoding="utf-8") as handle:
        writer = csv.writer(handle, lineterminator="\n")
        writer.writerow(header)
        for row in rows:
            writer.writerow(row)


def _fmt(value) -> str:
    if isinstance(value, float):
        return f"{value:.6f}"
    return str(value)


def write_artifacts(outdir: str, cfg, resolved: dict, layout, ues, allocation,
                    episode, seed: int, summary: dict) -> None:
    """Write every artifact of one episode. Byte-identical for a fixed seed."""
    os.makedirs(outdir, exist_ok=True)
    horizon = cfg.evaluation.horizon_slots

    _write_csv(
        os.path.join(outdir, "ues.csv"),
        ["ue", "cell", "capability", "budget_class", "budget_ms", "range_km",
         "elevation_deg", "delay_ms", "tau_slots"],
        ((u.uid, u.cell, u.profile_name, u.budget_name, u.budget_ms,
          _fmt(u.range_km), _fmt(u.elevation_deg), _fmt(u.delay_ms), u.tau_slots)
         for u in ues),
    )

    plan_rows = []
    for uid in sorted(allocation.records):
        rec = allocation.records[uid]
        opt = rec.option
        if opt is None:
            continue
        tl = opt.timeline
        is_prach = opt.access in ("4step-cbra", "4step-cfra", "2step-cbra", "2step-cfra")
        pool = ""
        if is_prach:
            pool = "dedicated" if opt.dedicated else "contention"
        cce = sum(m.amount for m in opt.demands if m.pool == "cce")
        plan_rows.append((
            uid, rec.cell, opt.access, opt.start_slot,
            opt.start_slot if is_prach else "",
            pool,
            rec.dedicated_index if (opt.dedicated and rec.dedicated_index is not None) else "",
            rec.prb_start, 1, 12, cce,
            opt.start_slot if opt.access == "cg" else "",
            opt.start_slot if opt.access == "dg" else "",
            int(math.ceil(opt.predicted_completion - 1e-12)),
            opt.prediction_basis,
            int(opt.fallback_used),
        ))
    _write_csv(
        os.path.join(outdir, "plan.csv"),
        ["ue", "cell", "access", "start_slot", "prach_occasion", "preamble_pool",
         "preamble_index", "prb_start", "prb_count", "symbol_count", "cce_count",
         "cg_occasion", "dg_slot", "predicted_completion_slot", "prediction_basis",
         "fallback_used"],
        plan_rows,
    )

    _write_csv(
        os.path.join(outdir, "access-events.csv"),
        ["ue", "cell", "access", "attempt", "slot", "preamble", "outcome", "reason"],
        ((a.ue, a.cell, a.access, a.attempt, a.slot,
          "" if a.preamble is None else a.preamble, a.outcome, a.reason)
         for a in episode.attempts),
    )

    _write_csv(
        os.path.join(outdir, "ue-outcomes.csv"),
        ["ue", "cell", "access", "realised_access", "outcome", "reason", "attempts",
         "completion_ms", "planned_completion_ms", "budget_violation", "censored",
         "matched_prediction"],
        ((o.ue, o.cell, o.access, o.realised_access, o.outcome, o.reason, o.attempts,
          "" if o.completion_ms is None else _fmt(o.completion_ms),
          "" if o.planned_completion is None else _fmt(o.planned_completion),
          int(o.budget_violation), int(o.censored), int(o.matched_prediction))
         for o in (episode.outcomes[uid] for uid in sorted(episode.outcomes))),
    )

    _write_csv(
        os.path.join(outdir, "ledger.csv"),
        ["cell", "slot", "pool", "used", "capacity"],
        ((cell, slot, pool, used, cap)
         for cell, slot, pool, used, cap in episode.ledger.rows()),
    )

    rows = []
    for group in ("capability", "budget", "access", "cell"):
        for row in summary["strata"][group]:
            key = row[group]
            rows.append((
                group, key, row["n_ues"], row["success"], row["success_rate"],
                row["violation"], row["violation_rate"], row["censored"],
                round(row["censored"] / row["n_ues"], 9) if row["n_ues"] else 0.0,
                row["attempts"], int(row["low_support"]),
            ))
    rows.append(("all", "all", summary["kpis"]["n_ues"],
                 summary["kpis"]["n_success"],
                 summary["kpis"]["handover_success_ratio"],
                 summary["kpis"]["n_violation"],
                 summary["kpis"]["budget_violation_ratio"],
                 summary["kpis"]["n_censored"],
                 summary["kpis"]["censored_ratio"],
                 summary["kpis"]["total_attempts"], 0))
    _write_csv(
        os.path.join(outdir, "summary.csv"),
        ["group", "stratum", "n_ues", "success", "success_rate", "violation",
         "violation_rate", "censored", "censored_rate", "attempts", "low_support"],
        rows,
    )

    with open(os.path.join(outdir, "summary.json"), "w", encoding="utf-8") as handle:
        json.dump(_rounded(summary), handle, indent=2, sort_keys=True)
        handle.write("\n")

    with open(os.path.join(outdir, "run.json"), "w", encoding="utf-8") as handle:
        json.dump(_rounded({
            "document_version": "5.1",
            "schema_version": resolved["schema_version"],
            "csv_header_version": 1,
            "resolved": resolved,
            "seed": seed,
            "seed_list": list(range(cfg.evaluation.first_seed,
                                    cfg.evaluation.first_seed + cfg.evaluation.episodes)),
            "outcome_seed": seed,
            "horizon_slots": horizon,
        }), handle, indent=2, sort_keys=True)
        handle.write("\n")
