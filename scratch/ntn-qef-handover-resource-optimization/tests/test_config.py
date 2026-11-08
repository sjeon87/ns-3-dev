"""Config schema: acceptance and every rejection rule of DESIGN.md 12.3."""

import json

import pytest
from pydantic import ValidationError

from qef_ho_ra.config import ScenarioConfig, per_cell_counts, resolve
from scenarios.generate import scenario


def load(doc):
    return ScenarioConfig.model_validate(doc)


def test_default_cluster_resolves_to_design_values():
    cfg = load(scenario())
    resolved = resolve(cfg)
    assert resolved["geometry"]["n_cells"] == 37
    assert resolved["geometry"]["footprint_radius_km"] == pytest.approx(853.47, abs=0.01)
    assert resolved["geometry"]["edge_delay_ms"] == pytest.approx(3.586, abs=1e-3)
    assert resolved["population"]["total_ues"] == 4848
    counts = resolved["population"]["per_cell_counts"]
    assert sum(counts) == 4848
    assert sorted(set(counts)) == [131, 132]
    assert counts.count(132) == 1 and counts[0] == 132
    assert resolved["radio"]["prb"] == 83
    assert resolved["radio"]["ul_prb_symbols"] == 996
    assert resolved["hop"]["groups"] == 2
    assert resolved["hop"]["group_sizes"] == [19, 18]
    assert resolved["evaluation"]["horizon_covers_retry_path"] is True
    assert resolved["evaluation"]["k_min_slots"] == 1190
    assert resolve(load(scenario({"hop": {"active_cells": 7, "dwell_ms": 100}})))["evaluation"]["k_min_slots"] == 5990


def test_full_profile_resolves_to_design_values():
    doc = scenario({
        "geometry": {"profile": "full"},
        "hop": {"active_cells": 64},
        "population": {"density_persons_per_km2": 5},
    })
    resolved = resolve(load(doc))
    assert resolved["geometry"]["n_cells"] == 517
    assert resolved["population"]["total_ues"] == 11290
    counts = resolved["population"]["per_cell_counts"]
    assert counts.count(21) == 84 and counts.count(22) == 433
    assert resolved["hop"]["group_sizes"] == [58, 58, 58, 58, 57, 57, 57, 57, 57]


def test_largest_remainder_ties_go_to_lower_index():
    counts = per_cell_counts(10, 4)
    assert counts == [3, 3, 2, 2]
    assert sum(counts) == 10


@pytest.mark.parametrize("mutate,match", [
    (lambda d: d["geometry"].pop("altitude_km"), "altitude_km"),
    (lambda d: d["population"].update(capability_shares=[0.5, 0.5]), "seven profiles"),
    (lambda d: d["population"].update(capability_shares=[0.5] * 7), "sum to one"),
    (lambda d: d["population"].update(budget_shares=[0.5] * 4), "sum to one"),
    (lambda d: d["allocator"].update(access_policy="fixed", allowed_access=[]), "non-empty"),
    (lambda d: d["allocator"].update(allowed_access=["4step-cbra", "nope"]), "unknown access"),
    (lambda d: d["allocator"].update(capability_fallback="fancy"), "capability_fallback"),
    (lambda d: d["access"].update(detection_probability=1.5), "detection_probability"),
    (lambda d: d["radio"].update(preamble_dedicated=65), "preamble_dedicated"),
    (lambda d: d["radio"].update(preamble_total=32, preamble_dedicated=40), "must not exceed"),
    (lambda d: d["population"].update(budget_ms=[0, 200, 400, 800]), "shortest procedure"),
    (lambda d: d["population"].update(spatial_distribution="clustered"), "spatial_distribution"),
    (lambda d: d["population"]["hotspot"].update(weight=1.5), "hotspot"),
    (lambda d: d.update(schema_version=5), "schema_version"),
    (lambda d: d["geometry"].update(profile="rings"), "profile"),
    (lambda d: d["hop"].update(active_cells=38), "exceeds n_cells"),
    (lambda d: d["evaluation"].update(horizon_slots=0), "horizon_slots"),
    (lambda d: d["geometry"].update(altitude_km=10), "altitude_km"),
])
def test_loader_rejects_invalid_documents(mutate, match):
    doc = scenario()
    mutate(doc)
    with pytest.raises((ValidationError, ValueError), match=match):
        cfg = load(doc)
        resolve(cfg)


def test_unknown_key_is_rejected():
    doc = scenario()
    doc["radio"]["max_rar_per_slot_typo"] = 8
    with pytest.raises(ValidationError, match="extra_forbidden"):
        load(doc)


def test_config_hash_is_stable_and_sensitive():
    cfg = load(scenario())
    a = resolve(cfg)["config_sha256"]
    b = resolve(load(scenario()))["config_sha256"]
    assert a == b
    doc = scenario()
    doc["hop"]["dwell_ms"] = 10
    assert resolve(load(doc))["config_sha256"] != a


def test_resolved_document_is_json_serialisable():
    resolved = resolve(load(scenario()))
    text = json.dumps(resolved, sort_keys=True)
    assert "config_sha256" in text
