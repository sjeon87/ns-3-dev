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

#include "aodvv2-packet.h"

#include "ns3/internet-module.h"

#include <functional>
#include <limits>

namespace ns3
{

namespace aodvv2
{

/**
 * \ingroup aodvv2
 * \brief Metric node description
 */
template <typename T>
struct MetricNode
{
    /// Metric node address
    T m_address;

    // Constructor
    MetricNode(const T& address)
        : m_address(address)
    {
    }
};

/**
 * \ingroup aodvv2
 * \brief define how a metric is represented in the AODVv2 protocol
 */
template <typename T>
class Metric
    : public std::enable_if_t<std::is_same_v<Ipv4Address, T> || std::is_same_v<Ipv6Address, T>, T>
{
  public:
    /**
     * constructor
     * \brief Default constructor sets the metric type to hop count
     */
    Metric()
    {
        m_metricType = AODVV2_METRIC_HOP;
        m_maxMetric = std::numeric_limits<uint8_t>::max();
        m_linkCost = [](const MetricNode<T>&) { return 1; };
        m_routeCost = [](const uint8_t& routeCost, const MetricNode<T>& currentNode) {
            return routeCost + 1;
        };
        m_loopFree = [](const uint8_t& r1, const uint8_t& r2) { return r1 <= r2; };
    }

    /**
     * constructor
     * @param metricType The type of metric to use.
     * @param maxMetric The maximum value for the metric type.
     * @param linkCost Function to determine the cost of an incoming link.
     * @param routeCost Function to determine the cost of a route.
     */
    Metric(uint8_t metricType,
           uint8_t maxMetric,
           std::function<uint8_t(const MetricNode<T>&)> linkCost,
           std::function<uint8_t(const uint8_t&, const MetricNode<T>&)> routeCost,
           std::function<bool(const uint8_t&, const uint8_t&)> loopFree)
        : m_metricType(metricType),
          m_maxMetric(maxMetric),
          m_linkCost(linkCost),
          m_routeCost(routeCost),
          m_loopFree(loopFree)
    {
    }

    /**
     * Set the metric type.
     * @param metricType Metric type.
     */
    void SetMetricType(uint8_t metricType)
    {
        m_metricType = metricType;
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
     * @param currentNode The current node.
     * @return The cost of the incoming link.
     */
    uint8_t linkCost(const MetricNode<T> currentNode) const
    {
        return m_linkCost(currentNode);
    }

    /**
     * Calculate the cost of a route.
     * @param routeCost The cost of the previous part of the route.
     * @param currentNode The current node.
     * @return The cost of the route.
     */
    uint8_t routeCost(const uint8_t& routeCost, const MetricNode<T>& currentNode) const
    {
        return m_routeCost(routeCost, currentNode);
    }

    /**
     * Check if the routes are loop-free.
     * @param r1 The first route cost.
     * @param r2 The second route cost.
     * @return True if the routes are loop-free, false otherwise.
     */
    bool LoopFree(const uint8_t& r1, const uint8_t& r2) const
    {
        return m_loopFree(r1, r2);
    }

  private:
    uint8_t m_metricType;
    uint8_t m_maxMetric;
    std::function<uint8_t(const MetricNode<T>&)> m_linkCost;
    std::function<uint8_t(const uint8_t&, const MetricNode<T>&)> m_routeCost;
    std::function<bool(const uint8_t&, const uint8_t&)> m_loopFree;
};

} // namespace aodvv2
} // namespace ns3

#endif /* AODVV2_METRIC_H */
