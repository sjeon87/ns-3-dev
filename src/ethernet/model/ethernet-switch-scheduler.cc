/*
 * Copyright (c) 2026 Somaiya Vidyavihar University, India
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Manmita Das <das.manmita12@gmail.com>
 */

#include "ethernet-switch-scheduler.h"

#include "ethernet-channel.h"
#include "ethernet-mac.h"
#include "ethernet-phy.h"

#include "ns3/log.h"
#include "ns3/simulator.h"

namespace ns3
{
namespace ethernet
{
NS_LOG_COMPONENT_DEFINE("EthernetSwitchScheduler");

NS_OBJECT_ENSURE_REGISTERED(EthernetSwitchScheduler);

TypeId
EthernetSwitchScheduler::GetTypeId()
{
    static TypeId tid = TypeId("ns3::ethernet::EthernetSwitchScheduler")
                            .SetParent<Object>()
                            .SetGroupName("Ethernet");
    return tid;
}

EthernetSwitchScheduler::EthernetSwitchScheduler()
{
    NS_LOG_FUNCTION(this);
}

EthernetSwitchScheduler::~EthernetSwitchScheduler()
{
    NS_LOG_FUNCTION(this);
}

void
EthernetSwitchScheduler::SetSwitch(Ptr<EthernetSwitch> sw)
{
    NS_LOG_FUNCTION(this << sw);

    NS_ASSERT(sw);

    m_switch = sw;
}

void
EthernetSwitchScheduler::DoDispose()
{
    NS_LOG_FUNCTION(this);

    m_switch = nullptr;
    m_portSchedulingState.clear();

    Object::DoDispose();
}

bool
EthernetSwitchScheduler::CanTransmit(const SwitchBufferEntry& entry) const
{
    NS_ASSERT(m_switch);

    return m_switch->CanForwardTo(entry.outgoingPort, entry.packet);
}

bool
EthernetSwitchScheduler::IsPortIdle(const SwitchBufferEntry& entry) const
{
    auto state = m_portSchedulingState.find(entry.outgoingPort);

    return state == m_portSchedulingState.end() || state->second == PortSchedulingState::PORT_IDLE;
}

void
EthernetSwitchScheduler::DropEntry(const SwitchBufferEntry& entry)
{
    NS_LOG_FUNCTION(this << entry.packet);

    NS_ASSERT(m_switch);

    m_switch->NotifyForwardDrop(entry);
}

void
EthernetSwitchScheduler::SchedulePortTransmission(const SwitchBufferEntry& entry)
{
    NS_ASSERT(entry.packet);
    NS_ASSERT(entry.outgoingPort);
    Ptr<EthernetNetDevice> port = entry.outgoingPort;

    if (m_portSchedulingState[port] == PortSchedulingState::PORT_TRANSMITTING)
    {
        NS_LOG_INFO("Port is busy, cannot schedule transmission." << port);
        return;
    }
    m_portSchedulingState[port] = PortSchedulingState::PORT_TRANSMITTING;

    Time txTime =
        entry.outgoingPort->GetPhy()->GetDataRate().CalculateBytesTxTime(entry.packet->GetSize());

    NS_LOG_INFO("Scheduling transmission for packet on port "
                << port << " with transmission time: " << txTime.GetSeconds() << " seconds.");

    Simulator::Schedule(txTime, &EthernetSwitchScheduler::TransmissionComplete, this, entry);
}

void
EthernetSwitchScheduler::Transmit(const SwitchBufferEntry& entry)
{
    NS_LOG_FUNCTION(this << entry.packet);

    NS_ASSERT(m_switch);
    NS_ASSERT(entry.outgoingPort);

    uint16_t protocol = entry.header.GetLengthType();

    Mac48Address source = entry.header.GetSource();
    Mac48Address destination = entry.header.GetDestination();

    if (!m_switch->TransmitToPort(entry.outgoingPort, entry.packet, protocol, source, destination))
    {
        //
        // The output port may no longer be able to accept the frame when the
        // scheduled transmission is performed. If transmission fails, drop the
        // frame from the switch buffer.
        //
        NS_LOG_WARN("Port " << entry.outgoingPort << " refused frame " << entry.packet->GetUid());
        DropEntry(entry);
    }
}

void
EthernetSwitchScheduler::TransmissionComplete(SwitchBufferEntry entry)
{
    NS_LOG_FUNCTION(this << entry.packet);

    Transmit(entry);
    m_portSchedulingState[entry.outgoingPort] = PortSchedulingState::PORT_IDLE;
    ScheduleTransmission();
}

} // namespace ethernet
} // namespace ns3
