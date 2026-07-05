/*
 * Copyright (c) 2026 Michigan State University
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Ishaan Lagwankar <lagwanka@msu.edu>
 */

#ifndef STATIC_SKEW_SCHEDULER_H
#define STATIC_SKEW_SCHEDULER_H

#include "epoch-table.h"
#include "event-id.h"
#include "map-scheduler.h"
#include "nstime.h"
#include "ptr.h"
#include "random-variable-stream.h"

namespace ns3
{

/**
 * @brief A scheduler that implements scheduling based on unchanging skews.
 */
class StaticSkewScheduler : public MapScheduler
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
    StaticSkewScheduler();

    /**
     * @brief Destructor.
     */
    ~StaticSkewScheduler() override;

    /**
     * @brief Assign a fixed random variable stream number to the random variables
     * used by this model.
     * @param stream first stream index to use
     * @return the number of stream indices assigned by this model
     */
    int64_t AssignStreams(int64_t stream);

    /**
     * @brief Insert an event into the schedule.
     * @param ev The event to schedule.
     */
    void Insert(const Event& ev) override;

    /**
     * @brief Get the underlying epoch table.
     * @return A pointer to the EpochTable.
     */
    Ptr<EpochTable> GetEpochTable() const;

    /**
     * @brief Static accessor to get the epoch table of the currently active scheduler.
     * @return A pointer to the active EpochTable, or nullptr if not active.
     */
    static Ptr<EpochTable> GetCurrentEpochTable();

  protected:
    /**
     * @brief Extends the epoch table for a specific node up to the target local time.
     * Virtual so derived schedulers can constrain how epochs are generated.
     * @param nodeId The context/node ID.
     * @param targetNodeTime The local time we need the table to cover.
     */
    virtual void ExtendEpochTable(uint32_t nodeId, Time targetNodeTime);

    /**
     * @brief Schedule the periodic cleanup task.
     * Virtual so derived schedulers can customize the cleanup cadence or side effects.
     */
    virtual void StartCleanupTask();

    /**
     * @brief Periodic cleanup event handler.
     * Prunes old epochs from the table to manage memory usage. Virtual so derived schedulers
     * can extend or replace the pruning behavior.
     */
    virtual void Cleanup();

    Ptr<EpochTable> m_epochTable;    //!< The epoch table instance
    double m_maxSkew;                //!< Maximum allowed clock skew
    double m_minSkew;                //!< Minimum allowed clock skew
    Time m_windowSize;               //!< Duration of the lookahead window
    Time m_updatePeriod;             //!< How often the skew changes (upsilon)
    EventId m_cleanupEvent;          //!< The ID of the next scheduled cleanup event
    Ptr<UniformRandomVariable> m_uv; //!< RNG for assigning node skew per epoch

  private:
    bool m_initialized; //!< Specific initialization flag
};

} // namespace ns3

#endif /* STATIC_SKEW_SCHEDULER_H */
