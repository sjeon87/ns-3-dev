"""Integer resource ledger (DESIGN.md sections 7.3 to 7.5).

Every pool is an integer counter per ``(cell, slot, pool)``. One PRB-symbol is
the unit; the ledger never stores a float. A reservation holds its capacity
until it is released or consumed, and an unused reservation still blocks.
"""

from __future__ import annotations

from collections import defaultdict
from dataclasses import dataclass

POOLS = (
    "pusch",
    "pdsch",
    "cce",
    "rar-grant",
    "dg-grant",
    "preamble-contention",
    "preamble-dedicated",
)

# Pools whose unit is not a PRB-symbol; used for the stage-3 cost weights.
CONTROL_POOLS = ("cce", "rar-grant", "dg-grant",
                 "preamble-contention", "preamble-dedicated")


@dataclass(frozen=True)
class Demand:
    """One resource demand of a planned or realised message."""

    pool: str
    cell: int
    slot: int
    amount: int

    @property
    def key(self) -> tuple[int, int, str]:
        return (self.cell, self.slot, self.pool)


class Ledger:
    """Sparse integer ledger with per-pool capacities."""

    def __init__(self, capacities: dict[str, int], horizon: int):
        self.capacities = dict(capacities)
        self.horizon = horizon
        self.used: dict[tuple[int, int, str], int] = defaultdict(int)
        self.dedicated: dict[tuple[int, int], set[int]] = defaultdict(set)

    def capacity(self, pool: str) -> int:
        return self.capacities[pool]

    def remaining(self, pool: str, cell: int, slot: int) -> int:
        return self.capacities[pool] - self.used.get((cell, slot, pool), 0)

    def aggregate(self, demands) -> dict[tuple[int, int, str], int]:
        agg: dict[tuple[int, int, str], int] = defaultdict(int)
        for d in demands:
            agg[d.key] += d.amount
        return agg

    def blocking_pools(self, demands) -> set[str]:
        """Pools that cannot hold the whole demand set (preamble index aside)."""
        blocking = set()
        for key, amount in self.aggregate(demands).items():
            cell, slot, pool = key
            if self.remaining(pool, cell, slot) < amount:
                blocking.add(pool)
        return blocking

    def fits(self, demands) -> bool:
        return not self.blocking_pools(demands)

    def reserve(self, demands) -> None:
        agg = self.aggregate(demands)
        for key, amount in agg.items():
            cell, slot, pool = key
            if self.remaining(pool, cell, slot) < amount:
                raise RuntimeError(f"reservation exceeds capacity on {key}")
        for key, amount in agg.items():
            self.used[key] += amount

    def release(self, demands) -> None:
        for key, amount in self.aggregate(demands).items():
            self.used[key] -= amount
            if self.used[key] < 0:
                raise RuntimeError(f"release below zero on {key}")

    def reserve_dedicated(self, cell: int, occasion: int) -> int:
        """Reserve the lowest free dedicated preamble index of an occasion."""
        taken = self.dedicated[(cell, occasion)]
        capacity = self.capacities["preamble-dedicated"]
        for index in range(capacity):
            if index not in taken:
                taken.add(index)
                self.used[(cell, occasion, "preamble-dedicated")] += 1
                return index
        raise RuntimeError("no free dedicated preamble")

    def free_dedicated_index(self, cell: int, occasion: int) -> int | None:
        taken = self.dedicated[(cell, occasion)]
        for index in range(self.capacities["preamble-dedicated"]):
            if index not in taken:
                return index
        return None

    def mark_dedicated(self, cell: int, occasion: int, index: int) -> None:
        """Record a dedicated index; the pool count comes from the demand."""
        self.dedicated[(cell, occasion)].add(index)

    def unmark_dedicated(self, cell: int, occasion: int, index: int) -> None:
        self.dedicated[(cell, occasion)].discard(index)

    def release_dedicated(self, cell: int, occasion: int, index: int) -> None:
        taken = self.dedicated[(cell, occasion)]
        if index in taken:
            taken.remove(index)
            self.used[(cell, occasion, "preamble-dedicated")] -= 1

    def reserve_preamble(self, pool: str, cell: int, slot: int) -> None:
        if self.remaining(pool, cell, slot) < 1:
            raise RuntimeError(f"preamble pool {pool} is full on ({cell}, {slot})")
        self.used[(cell, slot, pool)] += 1

    def utilisation(self, pool: str, cell: int, slot: int) -> float:
        return self.used.get((cell, slot, pool), 0) / self.capacities[pool]

    def totals(self) -> dict[str, int]:
        out = {p: 0 for p in self.capacities}
        for (_cell, _slot, pool), amount in self.used.items():
            out[pool] = out.get(pool, 0) + amount
        return out

    def rows(self):
        for (cell, slot, pool), amount in sorted(self.used.items()):
            if amount:
                yield cell, slot, pool, amount, self.capacities[pool]


def build_capacities(resolved: dict) -> dict[str, int]:
    """Pool capacities of one cell and slot from the resolved configuration."""
    radio = resolved["radio"]
    return {
        "pusch": radio["ul_prb_symbols"],
        "pdsch": radio["dl_prb_symbols"],
        "cce": radio["pdcch_cce_per_slot"],
        "rar-grant": radio["max_rar_per_slot"],
        "dg-grant": radio["max_dg_grants_per_slot"],
        "preamble-contention": radio["contention_preambles"],
        "preamble-dedicated": radio["preamble_dedicated"],
    }
