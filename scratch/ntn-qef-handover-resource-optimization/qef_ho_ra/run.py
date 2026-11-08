"""Episode runner: one seed from configuration to artifacts."""

from __future__ import annotations

import os

from .access import OptionEnumerator
from .allocator import Allocator
from .config import ScenarioConfig, resolve
from .engine import Episode
from .geometry import build_layout
from .hop import HopSchedule
from .metrics import analyze, write_artifacts
from .population import generate_population


class Runner:
    """Builds the shared context once and runs episodes on it."""

    def __init__(self, cfg: ScenarioConfig):
        self.cfg = cfg
        self.resolved = resolve(cfg)
        self.layout = build_layout(
            cfg.geometry.profile,
            cfg.geometry.altitude_km,
            cfg.geometry.min_elevation_deg,
            cfg.geometry.cell_radius_km,
            cfg.geometry.rings,
        )
        self.hop = HopSchedule(
            self.layout.n_cells,
            cfg.hop.active_cells,
            cfg.hop.dwell_ms,
            cfg.hop.phase_slots,
        )
        self._cache: dict[int, tuple] = {}

    def with_allocator(self, **overrides) -> "Runner":
        doc = self.cfg.model_dump(by_alias=True)
        doc["allocator"].update(overrides)
        return Runner(ScenarioConfig.model_validate(doc))

    def _population(self, seed: int):
        if seed not in self._cache:
            ues = generate_population(self.cfg, self.layout, seed)
            enumerator = OptionEnumerator(self.cfg, self.resolved, self.layout, self.hop)
            self._cache[seed] = (ues, enumerator)
        return self._cache[seed]

    def episode(self, seed: int, outcome_seed: int | None = None,
                weights: dict[int, float] | None = None):
        ues, enumerator = self._population(seed)
        allocator = Allocator(self.cfg, self.resolved, self.layout, self.hop, ues,
                              enumerator, weights=weights)
        allocation = allocator.run()
        engine = Episode(self.cfg, self.resolved, self.layout, self.hop, ues,
                         enumerator, allocation, seed, outcome_seed)
        result = engine.run()
        summary = analyze(self.cfg, self.resolved, self.layout, ues, allocation,
                          result, seed)
        return {
            "ues": ues,
            "enumerator": enumerator,
            "allocation": allocation,
            "episode": result,
            "summary": summary,
        }

    def run_series(self, seeds: list[int], outdir: str | None = None) -> list[dict]:
        summaries = []
        for i, seed in enumerate(seeds):
            bundle = self.episode(seed)
            summaries.append(bundle["summary"])
            if outdir:
                target = os.path.join(outdir, f"seed-{seed}")
                write_artifacts(target, self.cfg, self.resolved, self.layout,
                                bundle["ues"], bundle["allocation"], bundle["episode"],
                                seed, bundle["summary"])
        return summaries
