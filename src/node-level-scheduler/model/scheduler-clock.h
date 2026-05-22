/*
 * Copyright (c) 2026 Ishaan Lagwankar
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Ishaan Lagwankar <lagwanka@msu.edu>
 */
#ifndef SCHEDULER_CLOCK_H
#define SCHEDULER_CLOCK_H

#include "local-clock.h"
#include "node-level-scheduler.h"

namespace ns3
{

/**
 * @brief A clock that reports time based on the NodeTimingGraph.
 */
class SchedulerClock : public LocalClock
{
  public:
    /**
    * @brief Get the type ID.
    * @return The object TypeId.
    */
    static TypeId GetTypeId();

    SchedulerClock();
    ~SchedulerClock() override;

    /**
     * @brief Set the Node ID associated with this clock.
     * @param nodeId the Node ID to be set.
     */
    void SetNodeId(uint32_t nodeId);

    /**
     * @brief Returns the skewed local time.
     * @return the current time for that node
     */
    Time Now() override;

  private:
    uint32_t m_nodeId;
};

} // namespace ns3

#endif /* SCHEDULER_CLOCK_H */
