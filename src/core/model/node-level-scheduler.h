/*
 * Copyright (c) 2025 Michigan State University
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Ishaan Lagwankar <lagwanka@msu.edu>
 */

#ifndef NODE_LEVEL_SCHEDULER_H
#define NODE_LEVEL_SCHEDULER_H

#include "nstime.h"
#include "object.h"
#include "priority-queue-scheduler.h"
#include "ptr.h"
#include "simulator.h"

#include <deque>
#include <string>
#include <unordered_map>
#include <vector>

/**
 * @file
 * @ingroup scheduler
 * Declaration of ns3::NodeLevelScheduler class.
 */

namespace ns3
{

struct Interval
{
    Time simulatorStartTime;
    Time simulatorEndTime;
    Time nodeStartTime;
    Time nodeEndTime;
    double skew;
};

class NodeTimingGraph : public Object
{
  public:
    static TypeId GetTypeId();
    NodeTimingGraph() = default;
    ~NodeTimingGraph() = default;

    /**
     * @brief Add a timing interval for a specific node.
     */
    void AddInterval(uint32_t nodeId, const Interval& interval)
    {
        m_nodeIntervals[nodeId].push_back(interval);
    }

    /**
     * @brief Remove intervals that ended before the cutoff time.
     * @param cutoff The simulation time before which intervals should be removed.
     */
    void PruneIntervals(Time cutoff);

    /**
     * @brief Get the global simulator time corresponding to a given node time.
     */
    Time GetSimulatorTimeFromNodeTime(uint32_t nodeId, Time nodeTime) const;

  private:
    std::unordered_map<uint32_t, std::vector<Interval>> m_nodeIntervals;
};

class NodeLevelScheduler : public PriorityQueueScheduler
{
  public:
    static TypeId GetTypeId();

    NodeLevelScheduler();
    ~NodeLevelScheduler() override;

    void Insert(const Scheduler::Event& ev) override;

    Ptr<NodeTimingGraph> GetTimingGraph() const
    {
        return m_nodeTimings;
    }

    void SetIntervalsFromString(const std::string& intervalsStr);

  private:
    /**
     * @brief Updates the active interval table.
     * Loads new intervals within the window and removes old ones.
     */
    void UpdateIntervalWindow();

    Ptr<NodeTimingGraph> m_nodeTimings;

    struct PendingInterval
    {
        uint32_t nodeId;
        Interval data;
    };

    std::deque<PendingInterval> m_pendingIntervals; //!< Sorted list of all future intervals
    Time m_windowSize;   //!< How far ahead to load intervals (e.g., 100s)
    Time m_updatePeriod; //!< How often to run the cleanup/load event
    bool m_initialized;  //!< Ensures the window update loop starts once
};

} // namespace ns3

#endif /* NODE_LEVEL_SCHEDULER_H */
