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
#include "node-timing-graph.h"

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
     * @brief Set the NodeTimingGraph associated with this clock.
     * @param graph the NodeTimingGraph to be set.
     */
    void SetNodeTimingGraph(Ptr<NodeTimingGraph> graph);

    /**
     * @brief Returns the skewed local time.
     * @return the current time for that node
     */
    Time Now() override;

  private:
    uint32_t m_nodeId;            //!< Node ID of the current clock
    Ptr<NodeTimingGraph> m_graph; //!< Timing graph of all nodes
};

} // namespace ns3

#endif /* SCHEDULER_CLOCK_H */
