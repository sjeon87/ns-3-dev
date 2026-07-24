/*
 * Copyright (c) 2026 Somaiya Vidyavihar University, India
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Manmita Das <das.manmita12@gmail.com>
 */

#include "ethernet-switch-fcfs-scheduler.h"

#include "ethernet-channel.h"
#include "ethernet-mac.h"
#include "ethernet-phy.h"

#include "ns3/log.h"
#include "ns3/simulator.h"

namespace ns3
{
namespace ethernet
{
NS_LOG_COMPONENT_DEFINE("EthernetSwitchFcfsScheduler");

NS_OBJECT_ENSURE_REGISTERED(EthernetSwitchFcfsScheduler);

TypeId
EthernetSwitchFcfsScheduler::GetTypeId()
{
    static TypeId tid = TypeId("ns3::ethernet::EthernetSwitchFcfsScheduler")
                            .SetParent<EthernetSwitchScheduler>()
                            .SetGroupName("Ethernet")
                            .AddConstructor<EthernetSwitchFcfsScheduler>();
    return tid;
}

EthernetSwitchFcfsScheduler::EthernetSwitchFcfsScheduler()
{
    NS_LOG_FUNCTION(this);
}

EthernetSwitchFcfsScheduler::~EthernetSwitchFcfsScheduler()
{
    NS_LOG_FUNCTION(this);
}

void
EthernetSwitchFcfsScheduler::ScheduleTransmission()
{
    NS_LOG_FUNCTION(this);

    NS_ASSERT(m_switch);

    auto& buffer = m_switch->GetBuffer();

    if (buffer.empty())
    {
        return;
    }

    for (auto it = buffer.begin(); it != buffer.end();)
    {
        Ptr<EthernetNetDevice> port = it->outgoingPort;

        //
        // A frame waits its turn while the port it is destined to is busy
        // sending an earlier one.
        //
        if (!IsPortIdle(*it))
        {
            NS_LOG_INFO("Port is busy, cannot schedule transmission." << port);
            ++it;
            continue;
        }

        SwitchBufferEntry entry = *it;
        it = buffer.erase(it);

        //
        // The FCFS scheduler removes the frame from the switch buffer before
        // checking whether the output port can accept it. If the port cannot
        // accept the frame, the frame is dropped. Other scheduler policies may
        // choose to retain the frame and serve another buffered frame instead.
        //
        if (!CanTransmit(entry))
        {
            NS_LOG_INFO("Port " << port << " has no room left, dropping frame "
                                << entry.packet->GetUid());
            DropEntry(entry);
            continue;
        }

        NS_LOG_INFO("Scheduling transmission for packet on port " << port);
        SchedulePortTransmission(entry);
    }
}

} // namespace ethernet
} // namespace ns3
