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
 * @ingroup aodvv2
 * @brief Metric node description
 */
struct MetricNode
{
    /// Metric node address
    Ptr<Node> m_node;

    // Constructor
    MetricNode(Ptr<Node> node)
        : m_node(node)
    {
    }
};

/**
 * @ingroup aodvv2
 * @brief define how a metric is represented in the AODVv2 protocol
 */
template <typename T>
class Metric
    : public std::enable_if_t<std::is_same_v<Ipv4Address, T> || std::is_same_v<Ipv6Address, T>, T>
{
  public:
    /**
     * constructor
     * @brief Default constructor sets the metric type to hop count
     */
    Metric()
    {
        m_metricType = AODVV2_METRIC_HOP;
        m_metricSize = 1;
        m_maxMetric = 255;
        m_linkCost = [](const MetricNode&) { return new uint8_t[1]{1}; };
        m_routeCost = [](const uint8_t* routeCost, const MetricNode& currentNode) {
            uint8_t* newCost = new uint8_t[1];
            newCost[0] = static_cast<uint8_t>(routeCost[0] + 1);
            return newCost;
        };
        m_loopFree = [](const uint8_t* r1, const uint8_t* r2) { return r1[0] <= r2[0]; };
    }

    /**
     * constructor
     * @param metricType The type of metric to use.
     * @param metricSize The size of the metric type.
     * @param maxMetric The maximum value for the metric type.
     * @param LinkCost Function to determine the cost of an incoming link.
     * @param RouteCost Function to determine the cost of a route.
     * @param LoopFree Function to determine the loop free.
     */
    Metric(uint8_t metricType,
           uint8_t metricSize,
           uint32_t maxMetric,
           std::function<uint8_t*(const MetricNode&)> LinkCost,
           std::function<uint8_t*(const uint8_t*, const MetricNode&)> RouteCost,
           std::function<bool(const uint8_t*, const uint8_t*)> LoopFree)
        : m_metricType(metricType),
          m_metricSize(metricSize),
          m_maxMetric(maxMetric),
          m_linkCost(LinkCost),
          m_routeCost(RouteCost),
          m_loopFree(LoopFree)
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
     * Get the metric size.
     * @return Metric size.
     */
    uint8_t GetMetricSize() const
    {
        return m_metricSize;
    }

    /**
     * Get the maximum metric value allowed.
     * @return Maximum metric value.
     */
    uint32_t GetMaxMetric() const
    {
        return m_maxMetric;
    }

    /**
     * Calculate the cost of an incoming link.
     * @param currentNode The current node.
     * @return The cost of the incoming link.
     */
    uint8_t* LinkCost(const MetricNode currentNode) const
    {
        return m_linkCost(currentNode);
    }

    /**
     * Calculate the cost of a route.
     * @param routeCost The cost of the previous part of the route.
     * @param currentNode The current node.
     * @return The cost of the route.
     */
    uint8_t* RouteCost(const uint8_t* routeCost, const MetricNode& currentNode) const
    {
        return m_routeCost(routeCost, currentNode);
    }

    /**
     * Check if the routes are loop-free.
     * @param r1 The first route cost.
     * @param r2 The second route cost.
     * @return True if the routes are loop-free, false otherwise.
     */
    bool LoopFree(const uint8_t* r1, const uint8_t* r2) const
    {
        return m_loopFree(r1, r2);
    }

  private:
    uint8_t m_metricType;
    uint8_t m_metricSize;
    uint32_t m_maxMetric;
    std::function<uint8_t*(const MetricNode&)> m_linkCost;
    std::function<uint8_t*(const uint8_t*, const MetricNode&)> m_routeCost;
    std::function<bool(const uint8_t*, const uint8_t*)> m_loopFree;
};

} // namespace aodvv2
} // namespace ns3

#endif /* AODVV2_METRIC_H */
