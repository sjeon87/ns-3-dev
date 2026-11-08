"""Command line interface: one configuration, one output directory."""

from __future__ import annotations

import argparse
import json
import os

from .config import ScenarioConfig, resolve
from .run import Runner


def load_config(path: str) -> ScenarioConfig:
    with open(path, encoding="utf-8") as handle:
        doc = json.load(handle)
    return ScenarioConfig.model_validate(doc)


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(prog="qef_ho_ra", description=(
        "Quasi-Earth-fixed NTN handover resource allocation study"))
    parser.add_argument("--config", required=True, help="scenario JSON file")
    parser.add_argument("--output", required=True, help="output directory")
    parser.add_argument("--episodes", type=int, default=None,
                        help="override evaluation.episodes")
    parser.add_argument("--first-seed", type=int, default=None,
                        help="override evaluation.first_seed")
    parser.add_argument("--outcome-seed", type=int, default=None,
                        help="use a separate stream seed for outcome draws (V6a)")
    parser.add_argument("--quiet", action="store_true")
    args = parser.parse_args(argv)

    cfg = load_config(args.config)
    if args.episodes is not None:
        cfg = cfg.model_copy(update={"evaluation": cfg.evaluation.model_copy(
            update={"episodes": args.episodes})})
    if args.first_seed is not None:
        cfg = cfg.model_copy(update={"evaluation": cfg.evaluation.model_copy(
            update={"first_seed": args.first_seed})})

    resolved = resolve(cfg)
    runner = Runner(cfg)
    os.makedirs(args.output, exist_ok=True)
    with open(os.path.join(args.output, "resolved.json"), "w", encoding="utf-8") as handle:
        json.dump(resolved, handle, indent=2, sort_keys=True)
        handle.write("\n")

    summaries = []
    for i in range(cfg.evaluation.episodes):
        seed = cfg.evaluation.first_seed + i
        outcome_seed = args.outcome_seed
        bundle = runner.episode(seed, outcome_seed=outcome_seed)
        summaries.append(bundle["summary"])
        if not args.quiet:
            k = bundle["summary"]["kpis"]
            print(f"seed {seed}: success {k['handover_success_ratio']:.4f} "
                  f"violation {k['budget_violation_ratio']:.4f} "
                  f"censored {k['censored_ratio']:.4f} "
                  f"completion_median {k['completion_ms']['median']:.3f} ms")
        from .metrics import write_artifacts

        write_artifacts(os.path.join(args.output, f"seed-{seed}"), cfg, resolved,
                        runner.layout, bundle["ues"], bundle["allocation"],
                        bundle["episode"], seed, bundle["summary"])

    mean = {
        key: sum(s["kpis"][key] for s in summaries) / len(summaries)
        for key in ("handover_success_ratio", "budget_violation_ratio", "censored_ratio")
    }
    if not args.quiet:
        print(f"episodes {len(summaries)}: mean success {mean['handover_success_ratio']:.4f} "
              f"mean violation {mean['budget_violation_ratio']:.4f} "
              f"mean censored {mean['censored_ratio']:.4f}")
    with open(os.path.join(args.output, "episodes.json"), "w", encoding="utf-8") as handle:
        json.dump({"mean": mean, "episodes": summaries}, handle, indent=2, sort_keys=True)
        handle.write("\n")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
