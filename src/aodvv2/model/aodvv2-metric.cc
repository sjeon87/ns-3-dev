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
bool
Metric<T>::DefaultLoopFree(const std::vector<Ptr<Node>>& r1, const std::vector<Ptr<Node>>& r2)
{
    // Check if r2 is a sub-section of r1
    if (r1.size() <= r2.size())
    {
        return false;
    }

    for (size_t i = 0; i <= r1.size() - r2.size(); ++i)
    {
        bool isSubSection = true;
        for (size_t j = 0; j < r2.size(); ++j)
        {
            if (r1[i + j] != r2[j])
            {
                isSubSection = false;
                break;
            }
        }
        if (isSubSection)
        {
            return false;
        }
    }

    return true;
}

} // namespace aodvv2
} // namespace ns3
