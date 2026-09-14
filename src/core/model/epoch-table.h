/*
 * Copyright (c) 2026 Michigan State University
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Ishaan Lagwankar <lagwanka@msu.edu>
 */

#ifndef EPOCH_TABLE_H
#define EPOCH_TABLE_H

#include "nstime.h"
#include "object.h"

#include <map>
#include <vector>

namespace ns3
{

/**
 * @brief Maintain the mapping between Simulator Time and Node Time.
 */
class EpochTable : public Object
{
  public:
    /**
     * @brief Structure representing a period of constant clock skew.
     */
    struct Epoch
    {
        uint32_t nodeId;         //!< Node ID
        uint32_t index;          //!< Index of epoch in epoch table by nodeID
        Time simulatorStartTime; //!< Start time in global simulator reference.
        Time simulatorEndTime;   //!< End time in global simulator reference.
        Time nodeStartTime;      //!< Start time in local node reference.
        Time nodeEndTime;        //!< End time in local node reference.
        double skew;             //!< The clock skew
    };

    /**
     * @brief Get the type ID.
     * @return the object TypeId
     */
    static TypeId GetTypeId();

    /**
     * @brief Constructor.
     */
    EpochTable();

    /**
     * @brief Destructor.
     */
    ~EpochTable() override;

    /**
     * @brief Add a new epoch for a specific node.
     * @param nodeId The ID of the node.
     * @param epoch The epoch details.
     */
    void AddEpoch(uint32_t nodeId, const Epoch& epoch);

    /**
     * @brief Get the latest epoch for a given local time.
     * @param nodeId The ID of the node.
     * @param tLocal The local node time to search for.
     * @return The latest epoch for the local time.
     */
    const EpochTable::Epoch& LocalTimeBinarySearch(uint32_t nodeId, Time tLocal) const;

    /**
     * @brief Get the latest epoch for a given simulator time.
     * @param nodeId The ID of the node.
     * @param tSimulator The simulator time to search for.
     * @return The latest epoch for the simulator time.
     */
    const EpochTable::Epoch& GlobalTimeBinarySearch(uint32_t nodeId, Time tSimulator) const;

    /**
     * @brief Check if a node has any epochs in the table.
     * @param nodeId The ID of the node.
     * @return True if the node exists in the table, false otherwise.
     */
    bool HasNode(uint32_t nodeId) const;

    /**
     * @brief Truncates the table at a specific time and inserts a new epoch.
     * * @param nodeId The node context.
     * @param simStart The simulator time to start the new epoch (and end the previous).
     * @param simEnd The simulator time the new epoch ends.
     * @param nodeStart The local time to start the new epoch.
     * @param nodeEnd The local time the new epoch ends.
     * @param skew The new clock skew.
     */
    void InsertEpoch(uint32_t nodeId,
                     Time simStart,
                     Time simEnd,
                     Time nodeStart,
                     Time nodeEnd,
                     double skew);

    /**
     * @brief Replace all of a node's epochs with a single epoch.
     *
     * @param nodeId The ID of the node.
     * @param epoch The epoch to store.
     */
    void SetSingleEpoch(uint32_t nodeId, const Epoch& epoch);

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
     * @brief Remove old epochs that are no longer needed.
     * Removes epochs where the SimulatorEndTime is older than the cutoff.
     *
     * @param cutoff The simulator time threshold for pruning.
     */
    void PruneEpochTable(Time cutoff);

  private:
    std::map<uint32_t, std::vector<Epoch>>
        m_epochTable; //!< Map of Node IDs to their list of timing intervals.
};

} // namespace ns3

#endif /* EPOCH_TABLE_H */
