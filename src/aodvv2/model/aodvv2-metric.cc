/*
 * Copyright (c) 2024 University of Florence
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Representation of a Metric in the AODVv2 protocol.
 *
 * Authors: Francesco Todino <todinofrancesco97@gmail.com>
 *          Tommaso Pecorella <tommaso.pecorella@unifi.it>
 */

#include "aodvv2-metric.h"

#include "ns3/log.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("Aodvv2Metric");

namespace aodvv2
{

template class Metric<Ipv4Address>;
template class Metric<Ipv6Address>;

template <typename T>
uint8_t
Metric<T>::DefaultRouteCost(const uint8_t& routeCost, const MetricNode<T>& currentNode)
{
    NS_LOG_FUNCTION(&routeCost << &currentNode);
    return routeCost + m_linkCost(currentNode);
}

template <typename T>
bool
Metric<T>::DefaultLoopFree(const uint8_t& r1, const uint8_t& r2)
{
    NS_LOG_FUNCTION(&r1 << &r2);
    return r1 <= r2;
}

} // namespace aodvv2
} // namespace ns3
