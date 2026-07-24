/*
 * Copyright (c) 2026 Somaiya Vidyavihar University, India
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Manmita Das <das.manmita12@gmail.com>
 */

#include "ethernet-switch-helper.h"

#include "ns3/ethernet-channel.h"
#include "ns3/ethernet-net-device.h"
#include "ns3/ethernet-switch-fcfs-scheduler.h"
#include "ns3/ethernet-switch-scheduler.h"
#include "ns3/log.h"
#include "ns3/mac48-address.h"
#include "ns3/net-device.h"
#include "ns3/node.h"
#include "ns3/nstime.h"
#include "ns3/simulator.h"

namespace ns3
{
namespace ethernet
{

NS_LOG_COMPONENT_DEFINE("EthernetSwitchHelper");

EthernetSwitchHelper::EthernetSwitchHelper()
{
    m_switchFactory.SetTypeId(EthernetSwitch::GetTypeId());
    m_portFactory.SetTypeId(EthernetNetDevice::GetTypeId());
    m_deviceFactory.SetTypeId(EthernetNetDevice::GetTypeId());
    // Default scheduler
    m_schedulerFactory.SetTypeId(EthernetSwitchFcfsScheduler::GetTypeId());
}

void
EthernetSwitchHelper::SetSwitchAttribute(std::string name, const AttributeValue& value)
{
    m_switchFactory.Set(name, value);
}

void
EthernetSwitchHelper::SetPortAttribute(std::string name, const AttributeValue& value)
{
    m_portFactory.Set(name, value);
}

NetDeviceContainer
EthernetSwitchHelper::Install(NodeContainer nodes)
{
    NetDeviceContainer devices;

    for (uint32_t i = 0; i < nodes.GetN(); ++i)
    {
        Ptr<EthernetNetDevice> device = m_deviceFactory.Create<EthernetNetDevice>();

        device->SetAddress(Mac48Address::Allocate());

        nodes.Get(i)->AddDevice(device);

        devices.Add(device);
    }

    return devices;
}

void
EthernetSwitchHelper::Install(Ptr<Node> node, NetDeviceContainer devices)
{
    NS_ASSERT(node);

    Ptr<EthernetSwitch> sw = m_switchFactory.Create<EthernetSwitch>();
    sw->SetNode(node);
    node->AggregateObject(sw);

    Ptr<EthernetSwitchScheduler> scheduler = m_schedulerFactory.Create<EthernetSwitchScheduler>();

    sw->SetScheduler(scheduler);

    for (uint32_t i = 0; i < devices.GetN(); ++i)
    {
        Ptr<EthernetNetDevice> device = DynamicCast<EthernetNetDevice>(devices.Get(i));

        NS_ASSERT(device);

        Ptr<EthernetNetDevice> port = m_portFactory.Create<EthernetNetDevice>();
        port->SetAddress(Mac48Address::Allocate());
        port->SetNode(node);
        port->SetIfIndex(i);
        Simulator::ScheduleWithContext(node->GetId(), Seconds(0), &NetDevice::Initialize, port);
        sw->AddPort(port);

        Ptr<EthernetChannel> channel = CreateObject<EthernetChannel>();

        device->Attach(channel);
        port->Attach(channel);
    }
}

} // namespace ethernet
} // namespace ns3
