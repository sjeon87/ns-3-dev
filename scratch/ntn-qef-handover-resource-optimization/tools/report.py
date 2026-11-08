"""Report generator: tables and figures from results/<experiment>/*/experiment.json.

Usage: python tools/report.py --results results
"""

from __future__ import annotations

import argparse
import glob
import json
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

from qef_ho_ra.metrics import paired_difference  # noqa: E402

EXPERIMENTS = ("E1", "E2", "E3", "E4", "E5", "E6", "E7", "E8", "E9", "C1")

ACCESS_ORDER = ("4step-cbra", "4step-cfra", "2step-cbra", "2step-cfra", "cg", "dg")


def load(results: str, experiment: str) -> list[dict]:
    docs = []
    for path in sorted(glob.glob(os.path.join(results, experiment, "*", "experiment.json"))):
        with open(path, encoding="utf-8") as handle:
            docs.append(json.load(handle))
    return docs


def _fmt_ci(block: dict, digits: int = 4) -> str:
    mean = block["mean"]
    lo, hi = block["ci95"]
    return f"{mean:.{digits}f} [{lo:.{digits}f}, {hi:.{digits}f}]"


def table(docs: list[dict], baseline: dict | None = None) -> str:
    header = ("| point | episodes | success (95% CI) | violation (95% CI) | "
              "censored | median ms | p95 ms | cost | mix (top) |")
    lines = [header, "|" + "---|" * 10]
    for doc in docs:
        agg = doc["aggregate"]
        k = agg["kpis"]
        mix = doc["episodes"][0]["access_mix_planned"]
        top = sorted(((v, key) for key, v in mix.items() if key != "unassigned"),
                     reverse=True)[:2]
        mix_text = " ".join(f"{key}:{value / max(1, k['n_ues']['mean']):.2f}"
                            for value, key in top)
        line = (f"| {doc['point']} | {agg['episodes']} | "
                f"{_fmt_ci(k['handover_success_ratio'])} | "
                f"{_fmt_ci(k['budget_violation_ratio'])} | "
                f"{k['censored_ratio']['mean']:.4f} | "
                f"{agg['completion']['median']['mean']:.1f} | "
                f"{agg['completion']['p95']['mean']:.1f} | "
                f"{agg['stage3_resource_cost']['mean']:.0f} | {mix_text} |")
        lines.append(line)
    if baseline is not None:
        base_values = {e["seed"]: e["kpis"]["budget_violation_ratio"]
                       for e in baseline["episodes"]}
        lines.append("")
        lines.append("| point | paired violation difference vs joint (95% CI) | excludes 0 |")
        lines.append("|---|---|---|")
        for doc in docs:
            if doc["point"] == baseline["point"]:
                continue
            arm = {e["seed"]: e["kpis"]["budget_violation_ratio"] for e in doc["episodes"]}
            seeds = sorted(set(arm) & set(base_values))
            diff = paired_difference([arm[s] for s in seeds], [base_values[s] for s in seeds])
            lines.append(f"| {doc['point']} | {diff['mean']:+.4f} "
                         f"[{diff['ci95'][0]:+.4f}, {diff['ci95'][1]:+.4f}] | "
                         f"{diff['excludes_zero']} |")
    return "\n".join(lines)


def figures(results: str, outdir: str) -> list[str]:
    import matplotlib
    matplotlib.use("Agg")
    import matplotlib.pyplot as plt

    os.makedirs(outdir, exist_ok=True)
    written = []

    e1 = load(results, "E1")
    if e1:
        labels = [d["point"] for d in e1]
        values = [d["aggregate"]["kpis"]["budget_violation_ratio"]["mean"] for d in e1]
        errors = [d["aggregate"]["kpis"]["budget_violation_ratio"]["sd"] for d in e1]
        fig, ax = plt.subplots(figsize=(7, 3.2))
        ax.bar(range(len(labels)), values, yerr=errors, capsize=3)
        ax.set_xticks(range(len(labels)), labels, rotation=20, ha="right")
        ax.set_ylabel("delay-budget violation rate")
        ax.set_title("E1: fixed access arms vs joint (cluster, C_max=19, 30 seeds)")
        fig.tight_layout()
        path = os.path.join(outdir, "e1-violation.png")
        fig.savefig(path, dpi=150)
        plt.close(fig)
        written.append(path)

    e2 = load(results, "E2")
    if e2:
        xs = [d["resolved"]["population"]["density_persons_per_km2"] for d in e2]
        fig, ax = plt.subplots(figsize=(5, 3))
        ax.errorbar(xs, [d["aggregate"]["kpis"]["budget_violation_ratio"]["mean"] for d in e2],
                    yerr=[d["aggregate"]["kpis"]["budget_violation_ratio"]["sd"] for d in e2],
                    marker="o", label="violation")
        ax.errorbar(xs, [d["aggregate"]["kpis"]["handover_success_ratio"]["mean"] for d in e2],
                    yerr=[d["aggregate"]["kpis"]["handover_success_ratio"]["sd"] for d in e2],
                    marker="s", label="success")
        ax.set_xscale("log")
        ax.set_xlabel("density (persons/km2)")
        ax.set_ylim(0, 1.05)
        ax.legend()
        ax.set_title("E2: load sweep (cluster)")
        fig.tight_layout()
        path = os.path.join(outdir, "e2-load.png")
        fig.savefig(path, dpi=150)
        plt.close(fig)
        written.append(path)

    e3 = load(results, "E3")
    if e3:
        fig, ax = plt.subplots(figsize=(5, 3.2))
        for cmax in sorted({d["resolved"]["hop"]["active_cells"] for d in e3}):
            rows = [d for d in e3 if d["resolved"]["hop"]["active_cells"] == cmax]
            xs = [d["resolved"]["hop"]["dwell_ms"] for d in rows]
            ys = [d["aggregate"]["kpis"]["budget_violation_ratio"]["mean"] for d in rows]
            ax.plot(xs, ys, marker="o", label=f"C_max={cmax}")
        ax.set_xlabel("dwell (ms)")
        ax.set_ylabel("violation rate")
        ax.legend()
        ax.set_title("E3: hop sweep")
        fig.tight_layout()
        path = os.path.join(outdir, "e3-hop.png")
        fig.savefig(path, dpi=150)
        plt.close(fig)
        written.append(path)

    if e1:
        fig, ax = plt.subplots(figsize=(7, 3.2))
        bottom = [0.0] * len(e1)
        for access in ACCESS_ORDER:
            shares = [d["episodes"][0]["access_mix_planned"].get(access, 0)
                      / max(1, d["episodes"][0]["kpis"]["n_ues"]) for d in e1]
            ax.bar([d["point"] for d in e1], shares, bottom=bottom, label=access)
            bottom = [a + b for a, b in zip(bottom, shares)]
        ax.set_ylabel("planned access share")
        ax.set_xticklabels([d["point"] for d in e1], rotation=20, ha="right")
        ax.legend(fontsize=7, ncol=3)
        ax.set_title("E1: planned access mix")
        fig.tight_layout()
        path = os.path.join(outdir, "e1-mix.png")
        fig.savefig(path, dpi=150)
        plt.close(fig)
        written.append(path)

    return written


def main(argv=None) -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--results", default="results")
    parser.add_argument("--out", default=None)
    parser.add_argument("--no-figures", action="store_true")
    args = parser.parse_args(argv)
    out = args.out or os.path.join(args.results, "report.md")
    chunks = ["# Experiment report", ""]
    for experiment in EXPERIMENTS:
        docs = load(args.results, experiment)
        if not docs:
            continue
        baseline = None
        if experiment in ("E1", "C1"):
            baseline = next((d for d in docs if d["point"] in ("joint", "mu-0")), None)
        chunks.append(f"## {experiment}")
        chunks.append("")
        chunks.append(table(docs, baseline))
        chunks.append("")
    if not args.no_figures:
        written = figures(args.results, os.path.join(args.results, "figures"))
        if written:
            chunks.append("## Figures")
            chunks.append("")
            for path in written:
                chunks.append(f"- `{os.path.relpath(path)}`")
            chunks.append("")
    text = "\n".join(chunks)
    with open(out, "w", encoding="utf-8") as handle:
        handle.write(text)
        handle.write("\n")
    print(text)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
