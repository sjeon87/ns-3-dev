/*
 * Copyright (c) 2016 IITP
 * Copyright (c) 2025 Michigan State University
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Ishaan Lagwankar <lagwanka@msu.edu>
 */

#ifndef NODE_LEVEL_SCHEDULER_H
#define NODE_LEVEL_SCHEDULER_H

#include "event-impl.h"
#include "map-scheduler.h"
#include "nstime.h"
#include "object.h"
#include "simulator.h"

#include <fstream>
#include <map>
#include <optional>
#include <string>
#include <vector>

namespace ns3
{

struct IntervalData
{
    Time simulatorStartTime;
    Time simulatorEndTime;
    Time nodeStartTime;
    Time nodeEndTime;
    double skew;
};

struct Interval : public IntervalData
{
};

struct PendingInterval
{
    uint32_t nodeId;
    IntervalData data;
};

/**
 * @brief A singleton graph that manages time translation for all nodes.
 */
class NodeTimingGraph : public Object
{
  public:
    static TypeId GetTypeId();
    static Ptr<NodeTimingGraph> GetInstance();
    NodeTimingGraph() = default;

    Time GetSimulatorTimeFromNodeTime(uint32_t nodeId, Time nodeTime) const;
    Time GetNodeTimeFromSimulatorTime(uint32_t nodeId, Time simulatorTime) const;
    void AddInterval(uint32_t nodeId, const IntervalData& interval);
    void PruneIntervals(Time cutoff);

  private:
    std::map<uint32_t, std::vector<Interval>> m_nodeIntervals;
};

/**
 * @brief Scheduler that intercepts events and adjusts execution time.
 * Inherits from MapScheduler to handle the actual event storage.
 */
class NodeLevelScheduler : public MapScheduler
{
  public:
    static TypeId GetTypeId();

    NodeLevelScheduler();
    virtual ~NodeLevelScheduler();

    // We only override Insert. MapScheduler handles Peek, Remove, etc.
    virtual void Insert(const Event& ev) override;

    void SetIntervalFile(const std::string& filepath);

  private:
    bool ParseNextLine();
    void UpdateIntervalWindow();

    std::string m_intervalsFilePath;
    std::ifstream m_intervalStream;
    Time m_windowSize;
    Time m_updatePeriod;

    std::optional<PendingInterval> m_nextBufferedInterval;
    Ptr<NodeTimingGraph> m_nodeTimings;
    bool m_initialized;
};

} // namespace ns3

#endif /* NODE_LEVEL_SCHEDULER_H */
