/*
 * Copyright (c) 2026 Michigan State University
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Ishaan Lagwankar <lagwanka@msu.edu>
 */

#ifndef BOUNDED_SKEW_SCHEDULER_H
#define BOUNDED_SKEW_SCHEDULER_H

#include "epoch-table.h"
#include "nstime.h"
#include "ptr.h"
#include "static-skew-scheduler.h"

namespace ns3
{

/**
 * @brief A StaticSkewScheduler variant that bounds how far a node's local clock may drift
 * from Simulator::Now().
 */
class BoundedSkewScheduler : public StaticSkewScheduler
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
    BoundedSkewScheduler();

    /**
     * @brief Destructor.
     */
    ~BoundedSkewScheduler() override;

    /**
     * @brief Static accessor to get the epoch table of the currently active scheduler.
     * @return A pointer to the active EpochTable, or nullptr if not active.
     */
    static Ptr<EpochTable> GetCurrentEpochTable();

  protected:
    /**
     * @brief Extends the epoch table for a node, capping each epoch's duration so the node's
     * local time never drifts more than Epsilon away from Simulator::Now(), and restricting
     * the skew once an offset rests on a bound.
     * @param nodeId The node ID.
     * @param targetNodeTime The local time we need the table to cover.
     */
    void ExtendEpochTable(uint32_t nodeId, Time targetNodeTime) override;

  private:
    Time m_epsilon; //!< Maximum allowed drift
};

} // namespace ns3

#endif /* BOUNDED_SKEW_SCHEDULER_H */
