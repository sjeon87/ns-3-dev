/*
 * Copyright (c) 2026 Somaiya Vidyavihar University, India
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Manmita Das <das.manmita12@gmail.com>
 */

#ifndef ETHERNET_SWITCH_FCFS_SCHEDULER_H
#define ETHERNET_SWITCH_FCFS_SCHEDULER_H

#include "ethernet-switch-scheduler.h"

#include "ns3/ethernet-header.h"
#include "ns3/mac48-address.h"
#include "ns3/net-device.h"

#include <map>
#include <queue>

namespace ns3
{
namespace ethernet
{
/**
 * @ingroup ethernet
 * @brief FCFS Ethernet switch scheduler.
 */
class EthernetSwitchFcfsScheduler : public EthernetSwitchScheduler
{
  public:
    /**
     * @brief Get the type ID.
     * @return the object TypeId
     */
    static TypeId GetTypeId();

    EthernetSwitchFcfsScheduler();
    ~EthernetSwitchFcfsScheduler() override;

    // Delete copy constructor and assignment operator to avoid misuse
    EthernetSwitchFcfsScheduler(const EthernetSwitchFcfsScheduler&) = delete;
    EthernetSwitchFcfsScheduler& operator=(const EthernetSwitchFcfsScheduler&) = delete;

    /**
     * @brief Schedule transmissions in first-come-first-served order.
     */
    void ScheduleTransmission() override;
};

} // namespace ethernet
} // namespace ns3

#endif /* ETHERNET_SWITCH_FCFS_SCHEDULER_H */
