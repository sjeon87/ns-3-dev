"""V2: beam-hopping schedule duty, cycle, and pause algebra."""

import pytest

from qef_ho_ra.hop import HopSchedule, prach_occasions


def test_group_sizes_match_design_table():
    cases = [
        (37, 7, 20, 6, 120, (7, 6, 6, 6, 6, 6), 1 / 6),
        (37, 19, 20, 2, 40, (19, 18), 0.5),
        (37, 37, 20, 1, 20, (37,), 1.0),
        (517, 64, 20, 9, 180, (58, 58, 58, 58, 57, 57, 57, 57, 57), 1 / 9),
    ]
    for n, c, dwell, g, cycle, sizes, duty in cases:
        hop = HopSchedule(n, c, dwell, 0)
        assert hop.groups == g
        assert hop.cycle_slots == cycle
        assert hop.group_sizes() == sizes
        assert sum(hop.group_sizes()) == n
        assert hop.per_cell_duty == pytest.approx(duty)


def test_active_mask_duty_and_period():
    hop = HopSchedule(37, 7, 20, 0)
    for cell in (0, 5, 36):
        window = 120 * 10
        active = sum(1 for k in range(window) if hop.is_active(cell, k))
        assert active == pytest.approx(window * hop.per_cell_duty, abs=1e-9)
        for k in range(0, 400):
            assert hop.is_active(cell, k) == hop.is_active(cell, k + hop.cycle_slots)


def test_phase_shifts_the_mask():
    a = HopSchedule(37, 19, 20, 0)
    b = HopSchedule(37, 19, 20, 20)
    active_a = [k for k in range(40) if a.is_active(0, k)]
    active_b = [k for k in range(40) if b.is_active(0, k)]
    assert active_a == list(range(0, 20))
    assert active_b == list(range(20, 40))
    assert len(active_a) == len(active_b)


def test_next_active_and_wait():
    hop = HopSchedule(37, 7, 20, 0)  # group 0 active in [0,20), [120,140), ...
    assert hop.next_active(0, 0) == 0
    assert hop.next_active(0, 20) == 120
    assert hop.next_active(0, 25) == 120
    assert hop.wait(0, 100) == 20
    assert hop.next_active(1, 0) == 20  # group 1
    assert hop.next_active(1, 20) == 20


def test_advance_counts_active_slots_only():
    hop = HopSchedule(37, 7, 20, 0)
    assert hop.advance(0, 0, 2) == 2
    assert hop.advance(0, 15, 2) == 17
    # 5 active slots remain in [15, 20); 5 more are consumed in [120, 140)
    assert hop.advance(0, 15, 10) == 125
    # A timer that starts in a gap waits for the window first.
    assert hop.advance(0, 25, 1) == 121
    # A whole window exactly lands on the next window start.
    assert hop.advance(0, 0, 20) == 120
    assert hop.advance(0, 0, 21) == 121
    for n in range(0, 25):
        assert hop.is_active(0, hop.advance(0, 3, n))


def test_prach_occasions_only_in_active_slots():
    hop = HopSchedule(37, 7, 20, 0)
    occ = prach_occasions(hop, 0, 10, 1, 200)
    assert occ == [10, 120, 130]
    assert all(hop.is_active(0, k) for k in occ)
