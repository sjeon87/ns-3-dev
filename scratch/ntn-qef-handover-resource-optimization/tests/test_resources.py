"""Resource ledger: capacity, blocking, conservation, dedicated indices."""

import pytest

from qef_ho_ra.resources import Demand, Ledger


def ledger():
    return Ledger({
        "pusch": 996,
        "pdsch": 913,
        "cce": 24,
        "rar-grant": 8,
        "dg-grant": 8,
        "preamble-contention": 56,
        "preamble-dedicated": 8,
    }, horizon=100)


def test_reserve_and_release_conserve():
    led = ledger()
    demands = [Demand("pusch", 0, 10, 12), Demand("cce", 0, 10, 1)]
    led.reserve(demands)
    assert led.used[(0, 10, "pusch")] == 12
    assert led.totals()["pusch"] == 12
    led.release(demands)
    assert led.totals()["pusch"] == 0


def test_capacity_binds_and_blocking_pool_is_reported():
    led = ledger()
    led.reserve([Demand("rar-grant", 0, 5, 8)])
    assert led.blocking_pools([Demand("rar-grant", 0, 5, 1)]) == {"rar-grant"}
    assert led.fits([Demand("rar-grant", 0, 6, 1)])


def test_atomic_reservation_never_partially_applies():
    led = ledger()
    led.reserve([Demand("cce", 0, 1, 23)])
    with pytest.raises(RuntimeError):
        led.reserve([Demand("pusch", 0, 1, 12), Demand("cce", 0, 1, 2)])
    assert led.totals()["pusch"] == 0
    assert led.totals()["cce"] == 23


def test_preamble_pools_are_separate():
    led = ledger()
    led.reserve([Demand("preamble-contention", 3, 20, 56)])
    assert not led.fits([Demand("preamble-contention", 3, 20, 1)])
    assert led.fits([Demand("preamble-dedicated", 3, 20, 1)])


def test_dedicated_indices_are_unique_and_released():
    led = ledger()
    seen = []
    for _ in range(8):
        index = led.free_dedicated_index(0, 10)
        assert index is not None
        led.mark_dedicated(0, 10, index)
        seen.append(index)
    assert sorted(seen) == list(range(8))
    assert led.free_dedicated_index(0, 10) is None
    led.unmark_dedicated(0, 10, 3)
    assert led.free_dedicated_index(0, 10) == 3


def test_aggregate_sums_repeated_pool_slot():
    led = ledger()
    demands = [Demand("pdsch", 1, 4, 8), Demand("pdsch", 1, 4, 8)]
    assert led.blocking_pools(demands) == set()
    led.reserve(demands)
    assert led.used[(1, 4, "pdsch")] == 16


def test_utilisation_and_rows():
    led = ledger()
    led.reserve([Demand("cce", 2, 7, 6)])
    assert led.utilisation("cce", 2, 7) == pytest.approx(0.25)
    rows = list(led.rows())
    assert rows == [(2, 7, "cce", 6, 24)]
