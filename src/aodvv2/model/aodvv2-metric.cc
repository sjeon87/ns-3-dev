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

} // namespace aodvv2
} // namespace ns3
