/*
 * Copyright (c) 2024 University of Florence
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Based on
 *      NS-3 AODV model developed by Elena Buchatskaya and Pavel Boyko of IITP RAS
 *
 * Authors: Francesco Todino <todinofrancesco97@gmail.com>
 *          Tommaso Pecorella <tommaso.pecorella@unifi.it>
 */

#ifndef AODVV2_METRIC_H
#define AODVV2_METRIC_H

#include "ns3/internet-module.h"

#include <functional>
#include <limits>

namespace ns3
{

namespace aodvv2
{

/**
 * \ingroup aodvv2
 * \brief define how a metric is represented in the AODVv2 protocol
 */
template <typename T>
class Metric
    : public std::enable_if_t<std::is_same_v<Ipv4Address, T> || std::is_same_v<Ipv6Address, T>, T>
{
    /// Alias for determining whether the parent is Ipv4Address or Ipv6Address
    static constexpr bool IsIpv4 = std::is_same_v<Ipv4Address, T>;
    /// Alias for Ipv4InterfaceAddress and Ipv6InterfaceAddress classes
    using IpInterfaceAddress =
        typename std::conditional_t<IsIpv4, Ipv4InterfaceAddress, Ipv6InterfaceAddress>;

  public:
    /**
     * constructor
     * @param metricType The type of metric to use.
     * @param maxMetric The maximum value for the metric type.
     * @param linkCost Function to determine the cost of an incoming link.
     * @param routeCost Function to determine the cost of a route.
     */
    Metric(uint8_t metricType,
           uint8_t maxMetric,
           std::function<double(const Ptr<Node>&, const Ptr<Node>&)> linkCost,
           std::function<double(const std::vector<Ptr<Node>>&)> routeCost)
        : m_metricType(metricType),
          m_maxMetric(maxMetric),
          m_linkCost(linkCost),
          m_routeCost(routeCost),
          m_loopFree(DefaultLoopFree)
    {
    }

    /**
     * Get the metric type.
     * @return Metric type.
     */
    uint8_t GetMetricType() const
    {
        return m_metricType;
    }

    /**
     * Get the maximum metric value allowed.
     * @return Maximum metric value.
     */
    uint8_t GetMaxMetric() const
    {
        return m_maxMetric;
    }

    /**
     * Calculate the cost of an incoming link.
     * @param node1 The first node of the link.
     * @param node2 The second node of the link.
     * @return The cost of the incoming link.
     */
    double Cost(const Ptr<Node>& node1, const Ptr<Node>& node2) const
    {
        return m_linkCost(node1, node2);
    }

    /**
     * Calculate the cost of a route.
     * @param route The vector of nodes representing the route.
     * @return The cost of the route.
     */
    double Cost(const std::vector<Ptr<Node>>& route) const
    {
        return m_routeCost(route);
    }

    /**
     * Check if the routes are loop-free.
     * @param r1 First route as a vector of nodes.
     * @param r2 Second route as a vector of nodes.
     * @return True if the routes are loop-free, false otherwise.
     */
    bool LoopFree(const std::vector<Ptr<Node>>& r1, const std::vector<Ptr<Node>>& r2) const
    {
        return m_loopFree(r1, r2);
    }

  private:
    uint8_t m_metricType;
    uint8_t m_maxMetric;
    std::function<double(const Ptr<Node>&, const Ptr<Node>&)> m_linkCost;
    std::function<double(const std::vector<Ptr<Node>>&)> m_routeCost;
    std::function<bool(const std::vector<Ptr<Node>>&, const std::vector<Ptr<Node>>&)> m_loopFree;

    /**
     * Default function to check if routes are loop-free.
     * @param r1 First route as a vector of nodes.
     * @param r2 Second route as a vector of nodes.
     * @return True if the routes are loop-free, false otherwise.
     */
    static bool DefaultLoopFree(const std::vector<Ptr<Node>>& r1, const std::vector<Ptr<Node>>& r2);
};

} // namespace aodvv2
} // namespace ns3

#endif /* AODVV2_METRIC_H */
