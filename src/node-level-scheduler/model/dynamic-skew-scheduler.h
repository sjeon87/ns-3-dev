/*
 * Copyright (c) 2026 Michigan State University
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Ishaan Lagwankar <lagwanka@msu.edu>
 */

#ifndef DYNAMIC_SKEW_SCHEDULER_H
#define DYNAMIC_SKEW_SCHEDULER_H

#include "node-timing-graph.h"

#include "ns3/event-id.h"
#include "ns3/event-impl.h"
#include "ns3/map-scheduler.h"
#include "ns3/nstime.h"
#include "ns3/ptr.h"
#include "ns3/random-variable-stream.h"

#include <map>
#include <queue>
#include <unordered_set>
#include <vector>

namespace ns3
{

/**
 * @brief Comparator to prioritize earliest local node time.
 */
struct EventLocalTimeCmp
{
    bool operator()(const Scheduler::Event& a, const Scheduler::Event& b) const
    {
        return a.key.m_ts > b.key.m_ts;
    }
};

/**
 * @brief A scheduler that implements scheduling based on
 * changing skews.
 */
class DynamicSkewScheduler : public MapScheduler
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
    DynamicSkewScheduler();

    /**
     * @brief Destructor.
     */
    ~DynamicSkewScheduler() override;

    /**
     * @brief Assign a fixed random variable stream number to the random variables used by this
     * model.
     * @param stream first stream index to use
     * @return the number of stream indices assigned by this model
     */
    int64_t AssignStreams(int64_t stream);

    /**
     * @brief Insert an event into the schedule.
     *
     * @param ev The event to schedule.
     */
    void Insert(const Event& ev) override;

    /**
     * @brief Remove a specific event from the schedule.
     *
     * @param ev The event to remove.
     */
    void Remove(const Event& ev) override;

    /**
     * @brief Remove the next event from the schedule and return it.
     *
     * @return The next event.
     */
    Event RemoveNext() override;

    /**
     * @brief Changes a node's instantaneous skew.
     *
     * @param nodeId The node ID who's skew is changing.
     * @param skew The updated skew
     */
    void ChangeSkew(uint32_t nodeId, double skew);

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

    /**
     * @brief Promotes the top local event of a node into the global MapScheduler.
     * @param context The node context ID.
     */
    void RebalanceNode(uint32_t context);

    Ptr<NodeTimingGraph> m_nodeTimings; //!< The timing graph instance
    bool m_initialized;                 //!< specific initialization flag
    double m_maxSkew;                   //!< Maximum allowed clock skew
    double m_minSkew;                   //!< Minimum allowed clock skew
    Time m_windowSize;                  //!< Duration of the lookahead window
    Time m_updatePeriod;                //!< How often the skew changes
    EventId m_cleanupEvent;             //!< The ID of the next scheduled cleanup event
    Ptr<UniformRandomVariable> m_uv;    //!< RNG for assigning node skew per interval

    typedef std::priority_queue<Scheduler::Event, std::vector<Scheduler::Event>, EventLocalTimeCmp>
        EventQueue;

    std::map<uint32_t, EventQueue> m_nodeQueues;         //!< Node ID to Local Event Heap
    std::map<uint32_t, Scheduler::Event> m_activeEvents; //!< Node ID to Promoted global event
    std::unordered_set<uint32_t> m_cancelled;            //!< Set of cancelled events
};

} // namespace ns3

#endif /* DYNAMIC_SKEW_SCHEDULER_H */
