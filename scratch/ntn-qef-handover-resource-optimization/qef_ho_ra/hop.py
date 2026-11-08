"""Beam-hopping schedule (DESIGN.md section 5.3).

Cells are split into ``G = ceil(N_cells / C_max)`` groups. Group ``g(c) = c mod G``
is active during hop period ``floor(k / D_slots) mod G``. ``C_max`` is a ceiling:
the realised active count is the group size, which the schedule reports.

The schedule is a hard constraint. A procedure that reaches an inactive slot
pauses; processing timers count active slots of the cell. Propagation delay does
not pause.
"""

from __future__ import annotations

import math
from dataclasses import dataclass


@dataclass(frozen=True)
class HopSchedule:
    """Round-robin beam-hopping schedule over a fixed slot grid."""

    n_cells: int
    active_cells: int
    dwell_slots: int
    phase_slots: int

    @property
    def groups(self) -> int:
        return int(math.ceil(self.n_cells / self.active_cells))

    @property
    def cycle_slots(self) -> int:
        return self.groups * self.dwell_slots

    def group_of(self, cell: int) -> int:
        return cell % self.groups

    def group_sizes(self) -> tuple[int, ...]:
        """Realised active count per group; ``N mod G`` groups get the extra cell."""
        g = self.groups
        base = self.n_cells // g
        extra = self.n_cells % g
        return tuple(base + (1 if k < extra else 0) for k in range(g))

    def group_size(self, cell: int) -> int:
        return self.group_sizes()[self.group_of(cell)]

    @property
    def realised_active_count(self) -> int:
        """Largest realised group size (the design reports the group size)."""
        return max(self.group_sizes())

    @property
    def realised_active_count_min(self) -> int:
        return min(self.group_sizes())

    @property
    def per_cell_duty(self) -> float:
        return self.dwell_slots / self.cycle_slots

    def window_start(self, cell: int, slot: int) -> int:
        """Start slot of the hop period that contains ``slot`` for ``cell``."""
        period = self.cycle_slots
        offset = self.phase_slots + self.group_of(cell) * self.dwell_slots
        return offset + ((slot - offset) // period) * period

    def is_active(self, cell: int, slot: int) -> bool:
        if slot < 0:
            return False
        return slot - self.window_start(cell, slot) < self.dwell_slots

    def next_active(self, cell: int, slot: int) -> int:
        """First slot at or after ``slot`` in which ``cell`` is active."""
        if slot < 0:
            slot = 0
        start = self.window_start(cell, slot)
        if slot < start + self.dwell_slots:
            return slot
        return start + self.cycle_slots

    def wait(self, cell: int, slot: int) -> int:
        return self.next_active(cell, slot) - slot

    def advance(self, cell: int, slot: int, n_active: int) -> int:
        """Slot reached after ``n_active`` active slots of ``cell`` from ``slot``.

        The timer starts in the first active slot at or after ``slot``. The
        return value is active by construction, so the following message needs
        no extra wait.
        """
        if n_active < 0:
            raise ValueError("n_active must be non-negative")
        t0 = self.next_active(cell, slot)
        if n_active == 0:
            return t0
        dwell = self.dwell_slots
        period = self.cycle_slots
        start = self.window_start(cell, t0)
        offset = t0 - start
        remaining = dwell - offset
        if n_active <= remaining:
            return self.next_active(cell, t0 + n_active)
        leftover = n_active - remaining
        cycles, rem = divmod(leftover, dwell)
        return self.next_active(cell, start + (cycles + 1) * period + rem)

    def active_slots(self, cell: int, start: int, stop: int):
        """Yield the active slots of ``cell`` in ``[start, stop)``."""
        t = self.next_active(cell, start)
        while t < stop:
            yield t
            t = self.next_active(cell, t + 1)

    def windows(self, cell: int, start: int, stop: int):
        """Yield ``(window_start, window_end)`` pairs clipped to ``[start, stop)``."""
        t = self.next_active(cell, start)
        while t < stop:
            w_start = t
            w_end = min(self.window_start(cell, t) + self.dwell_slots, stop)
            yield w_start, w_end
            t = self.next_active(cell, w_end)


def prach_occasions(
    hop: HopSchedule,
    cell: int,
    period_slots: int,
    start: int,
    stop: int,
) -> list[int]:
    """PRACH occasion slots of ``cell``: every ``period_slots`` while active."""
    first = ((start + period_slots - 1) // period_slots) * period_slots
    out = []
    k = first
    while k < stop:
        if hop.is_active(cell, k):
            out.append(k)
        k += period_slots
    return out
