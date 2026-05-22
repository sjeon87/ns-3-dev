/*
 * Copyright (c) 2026 Michigan State University
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Ishaan Lagwankar <lagwanka@msu.edu>
 */

#ifndef NODE_LEVEL_SCHEDULER_H
#define NODE_LEVEL_SCHEDULER_H

#include "ns3/event-id.h"
#include "ns3/map-scheduler.h"
#include "ns3/nstime.h"
#include "ns3/object.h"

#include <map>
#include <vector>

namespace ns3
{

/**
 * @ingroup core
 * @defgroup node-level-scheduler Node Level Scheduler
 * * \brief Classes to support per-node clock skew simulation.
 */

/**
 * @ingroup node-level-scheduler
 * @brief Maintain the mapping between Simulator Time and Node Time.
 *
 * This class stores a history of time intervals for each node. Each interval
 * represents a period where the node's clock runs at a constant drift (skew)
 * relative to the global Simulator time.
 *
 * It allows converting a local Node Time (what the node thinks the time is)
 * to the absolute Simulator Time,
 * and vice versa.
 */
class NodeTimingGraph : public Object
{
  public:
    /**
     * @brief Structure representing a continuous period of constant clock skew.
     */
    struct Interval
    {
        Time simulatorStartTime; /**< Start time in global simulator reference. */
        Time simulatorEndTime;   /**< End time in global simulator reference. */
        Time nodeStartTime;      /**< Start time in local node reference. */
        Time nodeEndTime;        /**< End time in local node reference. */
        double skew;             /**< The clock skew factor (1.0 = perfect sync). */
    };

    /**
     * @brief Get the type ID.
     * @return the object TypeId
     */
    static TypeId GetTypeId();

    /**
     * @brief Constructor.
     */
    NodeTimingGraph();

    /**
     * @brief Destructor.
     */
    ~NodeTimingGraph() override;

    /**
     * @brief Add a new timing interval for a specific node.
     * @param nodeId The ID of the node.
     * @param interval The timing interval details.
     */
    void AddInterval(uint32_t nodeId, const Interval& interval);

    /**
     * @brief Check if timing information exists for a node.
     * @param nodeId The ID of the node.
     * @return True if the node is tracked, false otherwise.
     */
    bool HasNode(uint32_t nodeId) const;

    /**
     * @brief Convert local Node Time to global Simulator Time.
     *
     * @param nodeId The ID of the node context.
     * @param nodeTime The local time on the node.
     * @return The corresponding global simulator time.
     */
    Time GetSimulatorTimeFromNodeTime(uint32_t nodeId, Time nodeTime) const;

    /**
     * @brief Convert global Simulator Time to local Node Time.
     * * This is used by local clocks to report the "current time" based on
     * the global simulator state.
     *
     * @param nodeId The ID of the node context.
     * @param simulatorTime The global simulator time.
     * @return The corresponding local time on the node.
     */
    Time GetNodeTimeFromSimulatorTime(uint32_t nodeId, Time simulatorTime) const;

    /**
     * @brief Get the latest local time covered by the graph for a node.
     * @param nodeId The ID of the node.
     * @return The max node time currently tracked.
     */
    Time GetMaxNodeTime(uint32_t nodeId) const;

    /**
     * @brief Get the latest simulator time covered by the graph for a node.
     * @param nodeId The ID of the node.
     * @return The max simulator time currently tracked.
     */
    Time GetMaxSimulatorTime(uint32_t nodeId) const;

    /**
     * @brief Remove old intervals that are no longer needed.
     * * Removes intervals where the SimulatorEndTime is older than the cutoff.
     *
     * @param cutoff The simulator time threshold for pruning.
     */
    void PruneIntervals(Time cutoff);

  private:
    /**
     * @brief Map of Node IDs to their list of timing intervals.
     */
    std::map<uint32_t, std::vector<Interval>> m_nodeIntervals;
    std::map<uint32_t, Time> m_lastSimEndTime;  /**< Last known simulator end time per node. */
    std::map<uint32_t, Time> m_lastNodeEndTime; /**< Last known node end time per node. */
    std::map<uint32_t, double> m_lastSkew;      /**< Last known skew per node. */
};

/**
 * @ingroup node-level-scheduler
 * @brief A custom scheduler that implements per-node clock skew.
 *
 * This scheduler wraps the standard MapScheduler. When an event is inserted
 * with a specific Node Context, the scheduler translates the requested
 * execution time (assumed to be relative to the Node's local clock) into
 * the correct global Simulator Time based on the NodeTimingGraph.
 */
class NodeLevelScheduler : public MapScheduler
{
  public:
    /**
     * @brief Get the type ID.
     * @return the object TypeId
     */
    static TypeId GetTypeId();

    /**
     * @brief Constructor.
     */
    NodeLevelScheduler();

    /**
     * @brief Destructor.
     */
    ~NodeLevelScheduler() override;

    /**
     * @brief Insert an event into the schedule.
     * * This overrides the base Scheduler::Insert. It checks the event's context
     * (Node ID). If valid, it calculates the delay based on the node's
     * clock skew, adjusts the event's timestamp to the correct global time,
     * and then delegates to MapScheduler::Insert.
     *
     * @param ev The event to schedule.
     */
    void Insert(const Event& ev) override;

    /**
     * @brief Get the underlying timing graph.
     * @return A pointer to the NodeTimingGraph.
     */
    Ptr<NodeTimingGraph> GetTimingGraph() const;

    /**
     * @brief Static accessor to get the graph of the currently active scheduler.
     * @return A pointer to the active NodeTimingGraph, or nullptr if not active.
     */
    static Ptr<NodeTimingGraph> GetCurrentGraph();

  private:
    /**
     * @brief Schedule the periodic cleanup task.
     */
    void StartCleanupTask();

    /**
     * @brief Ensure the timing graph covers the target node time.
     * * If the target time is beyond the current graph history, new intervals
     * are generated and appended.
     *
     * @param nodeId The node context.
     * @param targetNodeTime The local time that needs to be reached.
     */
    void ExtendTimingGraph(uint32_t nodeId, Time targetNodeTime);

    /**
     * @brief Generate a new window of random skew intervals for a node.
     * @param nodeId The node to generate intervals for.
     */
    void AppendWindow(uint32_t nodeId);

    /**
     * @brief Periodic cleanup event handler.
     * * Prunes old intervals from the graph to manage memory usage.
     */
    void Cleanup();

    Ptr<NodeTimingGraph> m_nodeTimings; /**< The timing graph instance. */
    bool m_initialized;                 /**< specific initialization flag. */

    double m_maxSkew;       /**< Maximum allowed clock skew. */
    double m_minSkew;       /**< Minimum allowed clock skew */
    Time m_windowSize;      /**< Duration of the lookahead window. */
    Time m_updatePeriod;    /**< How often the skew changes. */
    EventId m_cleanupEvent; /**< The ID of the next scheduled cleanup event. */
};

} // namespace ns3

#endif /* NODE_LEVEL_SCHEDULER_H */
