/*
 * Copyright (c) 2025 Michigan State University
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */

#ifndef MULTI_RATE_CLOCK_H
#define MULTI_RATE_CLOCK_H

#include "local-clock.h"
#include "node-level-scheduler.h"

namespace ns3
{

/**
 * @brief A clock that reports time based on the NodeTimingGraph singleton.
 *
 * This clock uses the singleton graph to convert the global Simulator::Now()
 * into the specific skewed local time for a given Node ID.
 */
class MultiRateClock : public LocalClock
{
  public:
    static TypeId GetTypeId();
    MultiRateClock();
    ~MultiRateClock() override;

    /**
     * @brief Set the Node ID associated with this clock.
     * This is required for the clock to look up the correct intervals in the graph.
     */
    void SetNodeId(uint32_t nodeId);

    /**
     * @brief Returns the skewed local time.
     */
    Time Now() override;

  private:
    uint32_t m_nodeId;
};

} // namespace ns3

#endif /* MULTI_RATE_CLOCK_H */
