/*
 * Copyright (c) 2026 Somaiya Vidyavihar University, India
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Manmita Das <das.manmita12@gmail.com>
 */

#include "ethernet-channel.h"

#include "ethernet-net-device.h"
#include "ethernet-phy.h"

#include "ns3/log.h"
#include "ns3/net-device.h"
#include "ns3/node.h"
#include "ns3/packet.h"
#include "ns3/simulator.h"

namespace ns3
{
namespace ethernet
{

NS_LOG_COMPONENT_DEFINE("EthernetChannel");
NS_OBJECT_ENSURE_REGISTERED(EthernetChannel);

TypeId
EthernetChannel::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::EthernetChannel")
            .SetParent<Channel>()
            .SetGroupName("Ethernet")
            .AddConstructor<EthernetChannel>()
            .AddAttribute(
                "Length",
                "The length of the channel (m), which cannot exceed the 100 m "
                "maximum length of an Ethernet cable. Changing this attribute while"
                "packets are being transmitted is not supported.",
                DoubleValue(20),
                MakeDoubleAccessor(&EthernetChannel::SetLength, &EthernetChannel::GetLength),
                MakeDoubleChecker<double>())
            .AddAttribute(
                "Speed",
                "The propagation speed (m/s) in the propagation medium"
                "being considered. Changing this attribute while packets"
                "are being transmitted is not supported.",
                DoubleValue(200000000),
                MakeDoubleAccessor(&EthernetChannel::SetSpeed, &EthernetChannel::GetSpeed),
                MakeDoubleChecker<double>());
    return tid;
}

EthernetChannel::EthernetChannel()
    : Channel()
{
    NS_LOG_FUNCTION(this);
}

EthernetChannel::~EthernetChannel()
{
    NS_LOG_FUNCTION(this);
}

void
EthernetChannel::DoDispose()
{
    NS_LOG_FUNCTION(this);
    m_deviceList.fill(nullptr);
    m_deviceCount = 0;
    Channel::DoDispose();
}

uint32_t
EthernetChannel::Attach(Ptr<EthernetNetDevice> device)
{
    NS_LOG_FUNCTION(this << device);

    NS_ASSERT_MSG(device, "Cannot attach a null EthernetNetDevice");

    NS_ASSERT_MSG(m_deviceCount < 2, "Only two devices permitted");

    for (std::size_t i = 0; i < m_deviceList.size(); ++i)
    {
        if (!m_deviceList[i])
        {
            m_deviceList[i] = device;
            m_deviceCount++;

            NS_LOG_INFO("EthernetChannel: Attached device " << device << " at index " << i
                                                            << " (total devices = " << m_deviceCount
                                                            << ")");

            NegotiateLinkType();

            if (m_deviceCount == 2)
            {
                m_deviceList[0]->GetPhy()->LinkUp();
                m_deviceList[1]->GetPhy()->LinkUp();
            }

            return i;
        }
    }
    NS_FATAL_ERROR("Failed to attach device");
    return m_deviceList.size();
}

bool
EthernetChannel::Detach(Ptr<EthernetNetDevice> device)
{
    NS_LOG_FUNCTION(this << device);
    for (auto it = m_deviceList.begin(); it != m_deviceList.end(); ++it)
    {
        if (*it == device)
        {
            *it = nullptr;
            m_deviceCount--;

            for (auto dev : m_deviceList)
            {
                if (dev)
                {
                    dev->GetPhy()->LinkDown();
                }
            }

            NegotiateLinkType();
            return true;
        }
    }

    NS_LOG_WARN("EthernetChannel: Attempted to detach device " << device
                                                               << " but it was not found");

    return false;
}

std::size_t
EthernetChannel::GetNDevices() const
{
    return m_deviceCount;
}

Ptr<NetDevice>
EthernetChannel::GetDevice(std::size_t i) const
{
    NS_LOG_FUNCTION(this << i);

    if (i >= m_deviceList.size())
    {
        return nullptr;
    }
    return m_deviceList[i];
}

Time
EthernetChannel::GetDelay() const
{
    double delay = m_length / m_speed;
    return Seconds(delay);
}

void
EthernetChannel::NegotiateLinkType()
{
    NS_LOG_FUNCTION(this);

    Ptr<EthernetNetDevice> devA = m_deviceList[0];
    Ptr<EthernetNetDevice> devB = m_deviceList[1];

    //
    // A device with no peer yet runs at its own maximum supported link type.
    //
    if (!devA || !devB)
    {
        Ptr<EthernetNetDevice> device = devA ? devA : devB;

        if (device)
        {
            NS_LOG_LOGIC("Single device attached, running at "
                         << device->GetMaxSupportedLinkType());
            device->SetActualLinkType(device->GetMaxSupportedLinkType());
        }
        return;
    }

    //
    // Without IEEE 802.3 autonegotiation there is no protocol to run and no
    // delay to account for: both ends simply settle on the slower of the two
    // link types at once. The comparison is made on the resolved data rates
    // rather than on the enumeration values, so that it stays correct if link
    // types are ever added out of order.
    //
    EthernetLinkType maxA = devA->GetMaxSupportedLinkType();
    EthernetLinkType maxB = devB->GetMaxSupportedLinkType();

    EthernetLinkType linkType =
        EthernetLinkTypeToDataRate(maxA) <= EthernetLinkTypeToDataRate(maxB) ? maxA : maxB;

    NS_LOG_LOGIC("Settled on link type " << linkType << " between " << maxA << " and " << maxB);

    devA->SetActualLinkType(linkType);
    devB->SetActualLinkType(linkType);
}

void
EthernetChannel::PropagationStart(Ptr<const Packet> packet, Ptr<EthernetNetDevice> srcDevice)
{
    NS_LOG_FUNCTION(Simulator::Now() << packet->GetUid() << srcDevice);

    for (std::size_t i = 0; i < m_deviceList.size(); ++i)
    {
        if (m_deviceList[i] && m_deviceList[i] != srcDevice)
        {
            Simulator::ScheduleWithContext(m_deviceList[i]->GetNode()->GetId(),
                                           GetDelay(),
                                           &EthernetPhy::RxStart,
                                           m_deviceList[i]->GetPhy(),
                                           packet,
                                           srcDevice);
        }
    }
}

void
EthernetChannel::TxEnd(Ptr<Packet> packet, Ptr<EthernetNetDevice> srcDevice)
{
    NS_LOG_FUNCTION(Simulator::Now() << packet->GetUid() << srcDevice);

    NS_LOG_LOGIC("Schedule Receive Event in " << GetDelay().As(Time::S));

    for (std::size_t i = 0; i < m_deviceList.size(); ++i)
    {
        if (m_deviceList[i] && m_deviceList[i] != srcDevice)
        {
            Simulator::ScheduleWithContext(m_deviceList[i]->GetNode()->GetId(),
                                           GetDelay(),
                                           &EthernetPhy::Receive,
                                           m_deviceList[i]->GetPhy(),
                                           packet->Copy(),
                                           srcDevice);
        }
    }
}

void
EthernetChannel::SetLength(double length)
{
    NS_ABORT_MSG_IF(Simulator::GetContext() != Simulator::NO_CONTEXT,
                    "Cannot change channel length while simulation is running");

    m_length = length;
}

double
EthernetChannel::GetLength() const
{
    return m_length;
}

void
EthernetChannel::SetSpeed(double speed)
{
    NS_ABORT_MSG_IF(Simulator::GetContext() != Simulator::NO_CONTEXT,
                    "Cannot change channel speed while simulation is running");

    m_speed = speed;
}

double
EthernetChannel::GetSpeed() const
{
    return m_speed;
}

} // namespace ethernet
} // namespace ns3
