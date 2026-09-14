/*
 * Copyright (c) 2026 Michigan State University
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Ishaan Lagwankar <lagwanka@msu.edu>
 */

#ifndef DYNAMIC_SKEW_SCHEDULER_H
#define DYNAMIC_SKEW_SCHEDULER_H

#include "epoch-table.h"
#include "event-id.h"
#include "event-impl.h"
#include "nstime.h"
#include "ptr.h"
#include "random-variable-stream.h"
#include "scheduler.h"

#include <queue>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace ns3
{

/**
 * @brief Comparator to prioritize earliest local node time (for Q_LTS).
 */
struct EventLocalTimeCmp
{
    /**
     * @brief Compare two events by local node time.
     * @param a The first event.
     * @param b The second event.
     * @return true if a should sort after b.
     */
    bool operator()(const Scheduler::Event& a, const Scheduler::Event& b) const
    {
        if (a.key.m_ts != b.key.m_ts)
        {
            return a.key.m_ts > b.key.m_ts;
        }
        return a.key.m_uid > b.key.m_uid;
    }
};

/**
 * @brief Comparator to prioritize earliest simulator time (for Q_sim).
 */
struct EventSimTimeCmp
{
    /**
     * @brief Compare two events by simulator time.
     * @param a The first event.
     * @param b The second event.
     * @return true if a should sort after b.
     */
    bool operator()(const Scheduler::Event& a, const Scheduler::Event& b) const
    {
        if (a.key.m_ts != b.key.m_ts)
        {
            return a.key.m_ts > b.key.m_ts;
        }
        return a.key.m_uid > b.key.m_uid;
    }
};

/**
 * @brief Custom indexed min-heap for O(log N) updates of projected events (Q_proj).
 */
class ProjectedQueue
{
  public:
    ProjectedQueue() = default;
    ~ProjectedQueue() = default;

    /**
     * @brief Updates the projected simulator time for a node's top local event in O(log N).
     * @param nodeId The context ID of the node.
     * @param ev The event with the newly calculated projected simulator time.
     */
    void Update(uint32_t nodeId, const Scheduler::Event& ev);

    /**
     * @brief Retrieves the event with the lowest projected simulator time.
     * @return The top event.
     */
    Scheduler::Event Top() const;

    /**
     * @brief Checks if the projected queue is empty.
     * @return true if empty.
     */
    bool IsEmpty() const;

  private:
    /**
     * @brief Restore heap order by moving the element at idx up.
     * @param idx Index of the element to bubble up.
     */
    void BubbleUp(size_t idx);

    /**
     * @brief Restore heap order by moving the element at idx down.
     * @param idx Index of the element to bubble down.
     */
    void BubbleDown(size_t idx);

    /**
     * @brief Swap two heap entries and update their index map entries.
     * @param i Index of the first entry.
     * @param j Index of the second entry.
     */
    void Swap(size_t i, size_t j);

    std::vector<Scheduler::Event> m_heapArray; //!< The 0-indexed underlying heap array
    std::unordered_map<uint32_t, size_t>
        m_indexMap; //!< Reverse-lookup map for O(1) index discovery
};

/**
 * @brief A scheduler that implements O(log N) scheduling for continuous dynamic clock skews.
 */
class DynamicSkewScheduler : public Scheduler
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
     * @brief Insert an event into the schedule. Routes to Q_sim or Q_LTS based on context.
     * @param ev The event to schedule.
     */
    void Insert(const Event& ev) override;

    /**
     * @brief Checks if the scheduler has any pending events.
     * @return true if there are no events left.
     */
    bool IsEmpty() const override;

    /**
     * @brief Peek at the next event without removing it.
     * @return The next event.
     */
    Event PeekNext() const override;

    /**
     * @brief Remove a specific event from the schedule.
     * @param ev The event to remove.
     */
    void Remove(const Event& ev) override;

    /**
     * @brief Remove the next event from the schedule and return it. Compares Q_sim and Q_proj.
     * @return The next event.
     */
    Event RemoveNext() override;

    /**
     * @brief Truncates the active epoch and changes a node's instantaneous skew.
     * @param nodeId The node ID who's skew is changing.
     * @param skew The updated skew.
     */
    void ChangeSkew(uint32_t nodeId, double skew);

    /**
     * @brief Get the underlying epoch table.
     * @return A pointer to the EpochTable.
     */
    Ptr<EpochTable> GetEpochTable() const;

    /**
     * @brief Static accessor to get the table of the currently active scheduler.
     * @return A pointer to the active EpochTable, or nullptr if not active.
     */
    static Ptr<EpochTable> GetCurrentEpochTable();

    /**
     * @brief Static accessor to change the skew of a node on the currently active scheduler.
     *
     * @param nodeId The node whose skew is being changed.
     * @param skew The new skew value to apply.
     */
    static void ChangeCurrentSkew(uint32_t nodeId, double skew);

  protected:
    /**
     * @brief Give a node a clock anchor if it does not have one yet, drawing its initial skew.
     * @param nodeId The node context.
     */
    void EnsureAnchor(uint32_t nodeId);

    /**
     * @brief Re-anchor a node's clock at the current time with a new skew.
     *
     * @param nodeId The node context.
     * @param skew The new clock skew.
     */
    void ApplySkew(uint32_t nodeId, double skew);

    /**
     * @brief Re-project a node's earliest pending event into simulator time.
     * @param nodeId The node context.
     */
    void ReprojectTop(uint32_t nodeId);

    /**
     * @brief Schedule the next free-running skew redraw for a node.
     * @param nodeId The node context.
     */
    void ScheduleSkewRedraw(uint32_t nodeId);

    /**
     * @brief Redraw a node's free-running skew, then queue the following redraw.
     * @param nodeId The node context.
     * @param generation The redraw generation this event was scheduled for
     */
    void RedrawSkew(uint32_t nodeId, uint64_t generation);

    Ptr<EpochTable> m_epochTable;    //!< The epoch table instance
    double m_maxSkew;                //!< Maximum allowed clock skew
    double m_minSkew;                //!< Minimum allowed clock skew
    Time m_updatePeriod;             //!< How long a skew holds before it is redrawn (upsilon)
    Ptr<UniformRandomVariable> m_uv; //!< RNG for assigning node skew

  private:
    /**
     * @brief Get how long a skew holds for, substituting a default if none was configured.
     * @return The effective update period.
     */
    Time GetEffectiveUpdatePeriod() const;

    std::unordered_map<uint32_t, uint64_t>
        m_redrawGen; //!< Generation of the newest redraw scheduled, per node

    std::unordered_set<uint32_t> m_redrawPending; //!< Nodes with a redraw already outstanding

    std::unordered_set<uint32_t>
        m_selfAnchored; //!< Nodes whose clock this scheduler created, and so may redraw

    std::priority_queue<Scheduler::Event, std::vector<Scheduler::Event>, EventSimTimeCmp>
        m_globalQueue; //!< Q_sim: Holds global/physical simulator events

    std::unordered_map<
        uint32_t,
        std::priority_queue<Scheduler::Event, std::vector<Scheduler::Event>, EventLocalTimeCmp>>
        m_nodeQueues; //!< Q_LTS: Node ID to Local Event Heap

    ProjectedQueue
        m_projectedQueue; //!< Q_proj: Constant-size custom indexed min-heap for projected events

    std::unordered_set<uint32_t> m_cancelled; //!< Set of cancelled events
};

} // namespace ns3

#endif /* DYNAMIC_SKEW_SCHEDULER_H */
