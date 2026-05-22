/*
 * Copyright (c) 2026 Michigan State University
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Ishaan Lagwankar <lagwanka@msu.edu>
 */

#ifndef NODE_TIMING_GRAPH_H
#define NODE_TIMING_GRAPH_H

#include "ns3/nstime.h"
#include "ns3/object.h"

#include <map>
#include <vector>

namespace ns3
{

/**
 * @brief Maintain the mapping between Simulator Time and Node Time.
 */
class NodeTimingGraph : public Object
{
  public:
    /**
     * @brief Structure representing a period of constant clock skew.
     */
    struct Interval
    {
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
     * Removes intervals where the SimulatorEndTime is older than the cutoff.
     *
     * @param cutoff The simulator time threshold for pruning.
     */
    void PruneIntervals(Time cutoff);

  private:
    std::map<uint32_t, std::vector<Interval>>
        m_nodeIntervals; //!< Map of Node IDs to their list of timing intervals.
    std::map<uint32_t, Time> m_lastSimEndTime;  //!< Last known simulator end time per node
    std::map<uint32_t, Time> m_lastNodeEndTime; //!< Last known node end time per node
    std::map<uint32_t, double> m_lastSkew;      //!< Last known skew per node
};

} // namespace ns3

#endif /* NODE_TIMING_GRAPH_H */
