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
#include <fstream>  // Required for std::ifstream
#include <optional> // Required for std::optional (C++17)
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

/**
 * @brief Represents a time dilation interval for a node.
 */
struct Interval
{
    Time simulatorStartTime;
    Time simulatorEndTime;
    Time nodeStartTime;
    Time nodeEndTime;
    double skew;
};

/**
 * @brief A helper class to manage the *active* lookup table for node times.
 * This graph only holds intervals relevant to the current sliding window.
 */
class NodeTimingGraph : public Object
{
  public:
    static TypeId GetTypeId();
    NodeTimingGraph() = default;
    ~NodeTimingGraph() = default;

    /**
     * @brief Add a timing interval to the active memory cache.
     * This is called by the scheduler when an interval enters the sliding window.
     * @param nodeId The ID of the node.
     * @param interval The timing interval data.
     */
    void AddInterval(uint32_t nodeId, const Interval& interval)
    {
        m_nodeIntervals[nodeId].push_back(interval);
    }

    /**
     * @brief Remove intervals that have finished (EndTime < cutoff).
     * This keeps memory usage low by discarding past history.
     * @param cutoff The current simulation time (or slightly before it).
     */
    void PruneIntervals(Time cutoff);

    /**
     * @brief Translate Node Local Time -> Global Simulator Time.
     * @param nodeId The node context.
     * @param nodeTime The local time requested by the event.
     * @return The calculated global simulation time.
     */
    Time GetSimulatorTimeFromNodeTime(uint32_t nodeId, Time nodeTime) const;

  private:
    std::unordered_map<uint32_t, std::vector<Interval>> m_nodeIntervals;
};

/**
 * @ingroup scheduler
 * @brief A scheduler that streams interval data from a file to manage node-level time dilation.
 *
 * This scheduler intercepts event insertions, interprets the timestamp as Node Local Time,
 * and maps it to Global Simulator Time based on intervals streamed from a CSV file.
 * It uses a sliding window approach to keep memory usage constant regardless of simulation length.
 */
class NodeLevelScheduler : public PriorityQueueScheduler
{
  public:
    static TypeId GetTypeId();

    NodeLevelScheduler();
    ~NodeLevelScheduler() override;

    // Inherited from Scheduler
    void Insert(const Scheduler::Event& ev) override;

    /**
     * @brief Sets the path to the CSV file containing intervals.
     * The file is opened lazily when the simulation starts.
     * @param filepath Path to the CSV file.
     */
    void SetIntervalFile(const std::string& filepath);

  private:
    /**
     * @brief Periodic maintenance task.
     * 1. Prunes old intervals from RAM.
     * 2. Reads new intervals from Disk.
     * 3. Adds them to NodeTimingGraph (RAM) if they are within the window.
     * 4. Reschedules itself.
     */
    void UpdateIntervalWindow();

    /**
     * @brief Helper to parse the next line from the CSV stream.
     * Populates m_nextBufferedInterval if successful.
     * @return true if a line was parsed successfully, false on EOF.
     */
    bool ParseNextLine();

    Ptr<NodeTimingGraph> m_nodeTimings; //!< The active lookup table (RAM)

    // --- File Streaming Members ---
    std::string m_intervalsFilePath;
    std::ifstream m_intervalStream; //!< The open file handle

    // Temporary storage for parsed CSV data
    struct PendingInterval
    {
        uint32_t nodeId;
        Interval data;
    };

    /**
     * @brief Buffer for a single interval read from disk.
     * Since we read sequentially, we might read one line that is "too far in the future"
     * (beyond the current window). We store it here until the window catches up.
     */
    std::optional<PendingInterval> m_nextBufferedInterval;

    Time m_windowSize;   //!< Lookahead horizon (e.g., 60s)
    Time m_updatePeriod; //!< Maintenance frequency (e.g., 10s)
    bool m_initialized;  //!< Flag to start the maintenance loop
};

} // namespace ns3

#endif /* NODE_LEVEL_SCHEDULER_H */