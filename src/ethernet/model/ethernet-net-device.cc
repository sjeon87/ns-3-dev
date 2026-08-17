/*
 * Copyright (c) 2026 Somaiya Vidyavihar University, India
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Manmita Das <das.manmita12@gmail.com>
 */

#include "ethernet-net-device.h"

#include "ethernet-channel.h"
#include "ethernet-mac.h"
#include "ethernet-phy.h"

#include "ns3/boolean.h"
#include "ns3/drop-tail-queue.h"
#include "ns3/enum.h"
#include "ns3/ethernet-header.h"
#include "ns3/ethernet-trailer.h"
#include "ns3/fatal-error.h"
#include "ns3/log.h"
#include "ns3/mac48-address.h"
#include "ns3/net-device-queue-interface.h"
#include "ns3/node.h"
#include "ns3/pointer.h"
#include "ns3/queue.h"
#include "ns3/simulator.h"
#include "ns3/trace-source-accessor.h"
#include "ns3/uinteger.h"

namespace ns3
{
namespace ethernet
{

NS_LOG_COMPONENT_DEFINE("EthernetNetDevice");

NS_OBJECT_ENSURE_REGISTERED(EthernetNetDevice);

DataRate
EthernetLinkTypeToDataRate(EthernetLinkType type)
{
    switch (type)
    {
    case EthernetLinkType::BASE10_T:
        return DataRate("10Mbps");

    case EthernetLinkType::BASE100_TX:
        return DataRate("100Mbps");

    case EthernetLinkType::BASE1000_T:
        return DataRate("1Gbps");

    case EthernetLinkType::G10_T:
        return DataRate("10Gbps");
    }

    NS_FATAL_ERROR("Unknown EthernetLinkType");
    return DataRate();
}

std::ostream&
operator<<(std::ostream& os, EthernetLinkType type)
{
    switch (type)
    {
    case EthernetLinkType::BASE10_T:
        return os << "10Base-T";

    case EthernetLinkType::BASE100_TX:
        return os << "100Base-TX";

    case EthernetLinkType::BASE1000_T:
        return os << "1000Base-T";

    case EthernetLinkType::G10_T:
        return os << "10GBase-T";
    }

    return os << "unknown";
}

TypeId
EthernetNetDevice::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::EthernetNetDevice")
            .SetParent<NetDevice>()
            .SetGroupName("Ethernet")
            .AddConstructor<EthernetNetDevice>()
            .AddAttribute(
                "Mac",
                "The Ethernet MAC associated with this device.",
                PointerValue(),
                MakePointerAccessor(&EthernetNetDevice::SetMac, &EthernetNetDevice::GetMac),
                MakePointerChecker<EthernetMac>())
            .AddAttribute(
                "Phy",
                "The Ethernet PHY associated with this device.",
                PointerValue(),
                MakePointerAccessor(&EthernetNetDevice::SetPhy, &EthernetNetDevice::GetPhy),
                MakePointerChecker<EthernetPhy>())
            .AddAttribute("Channel",
                          "The channel attached to this device.",
                          PointerValue(),
                          MakePointerAccessor(&EthernetNetDevice::m_channel),
                          MakePointerChecker<EthernetChannel>())
            .AddAttribute(
                "MaxSupportedEthernetLinkType",
                "The fastest Ethernet link type this device is able to run at. The "
                "link type actually used is the slowest of the ones supported by the "
                "two devices sharing the channel.",
                EnumValue<EthernetLinkType>(EthernetLinkType::BASE100_TX),
                MakeEnumAccessor<EthernetLinkType>(&EthernetNetDevice::SetMaxSupportedLinkType,
                                                   &EthernetNetDevice::GetMaxSupportedLinkType),
                MakeEnumChecker<EthernetLinkType>(EthernetLinkType::BASE10_T,
                                                  "10Base-T",
                                                  EthernetLinkType::BASE100_TX,
                                                  "100Base-TX",
                                                  EthernetLinkType::BASE1000_T,
                                                  "1000Base-T",
                                                  EthernetLinkType::G10_T,
                                                  "10GBase-T"))
            .AddAttribute("SendEnable",
                          "Enable or disable the transmitter section of the device.",
                          BooleanValue(true),
                          MakeBooleanAccessor(&EthernetNetDevice::m_sendEnable),
                          MakeBooleanChecker())
            .AddAttribute("ReceiveEnable",
                          "Enable or disable the receiver section of the device.",
                          BooleanValue(true),
                          MakeBooleanAccessor(&EthernetNetDevice::m_receiveEnable),
                          MakeBooleanChecker());
    return tid;
}

EthernetNetDevice::EthernetNetDevice()
    : m_linkUp(false),
      m_ifIndex(0),
      m_deviceId(0)
{
    NS_LOG_FUNCTION(this);

    m_phy = CreateObject<EthernetPhy>();
    m_mac = CreateObject<EthernetMac>();

    m_phy->SetDevice(this);
    m_phy->SetMac(m_mac);
    m_mac->SetPhy(m_phy);
    m_mac->SetDevice(this);

    m_mac->SetRxIndicationCallback(MakeCallback(&EthernetNetDevice::RxIndication, this));

    Ptr<NetDeviceQueueInterface> ndqi = CreateObject<NetDeviceQueueInterface>();
    ndqi->GetTxQueue(0)->ConnectQueueTraces(m_mac->GetTxQueue());
    AggregateObject(ndqi);
}

EthernetNetDevice::~EthernetNetDevice()
{
    NS_LOG_FUNCTION(this);
}

void
EthernetNetDevice::DoDispose()
{
    NS_LOG_FUNCTION(this);

    //
    // The MAC and the PHY hold a pointer back to this device, so they have to be
    // disposed of explicitly to break the reference cycles they form with it.
    //
    if (m_mac)
    {
        m_mac->Dispose();
        m_mac = nullptr;
    }
    if (m_phy)
    {
        m_phy->Dispose();
        m_phy = nullptr;
    }

    m_node = nullptr;
    m_channel = nullptr;

    m_rxCallback.Nullify();
    m_promiscRxCallback.Nullify();

    NetDevice::DoDispose();
}

void
EthernetNetDevice::DoInitialize()
{
    if (!m_channel)
    {
        m_actualLinkType = m_maxLinkType;
    }

    NetDevice::DoInitialize();
}

void
EthernetNetDevice::SetMac(Ptr<EthernetMac> mac)
{
    m_mac = mac;
    m_mac->SetDevice(this);
    m_mac->SetPhy(m_phy);
    if (m_phy)
    {
        m_phy->SetMac(m_mac);
    }
}

void
EthernetNetDevice::SetPhy(Ptr<EthernetPhy> phy)
{
    m_phy = phy;
    m_phy->SetDevice(this);
    m_phy->SetMac(m_mac);
    if (m_mac)
    {
        m_mac->SetPhy(m_phy);
    }
}

void
EthernetNetDevice::Attach(Ptr<EthernetChannel> channel)
{
    NS_LOG_FUNCTION(this << channel);
    m_channel = channel;
    m_deviceId = m_channel->Attach(this);
    m_phy->SetChannel(channel);
}

void
EthernetNetDevice::SetMaxSupportedLinkType(EthernetLinkType type)
{
    NS_LOG_FUNCTION(this << type);

    m_maxLinkType = type;

    if (m_channel)
    {
        m_channel->NegotiateLinkType();
    }
}

EthernetLinkType
EthernetNetDevice::GetMaxSupportedLinkType() const
{
    return m_maxLinkType;
}

EthernetLinkType
EthernetNetDevice::GetActualLinkType() const
{
    return m_actualLinkType;
}

void
EthernetNetDevice::SetActualLinkType(EthernetLinkType type)
{
    NS_LOG_FUNCTION(this << type);

    m_actualLinkType = type;

    if (m_phy)
    {
        m_phy->UpdateInterframeGap();
    }
}

DataRate
EthernetNetDevice::GetDataRate() const
{
    return EthernetLinkTypeToDataRate(m_actualLinkType);
}

Ptr<EthernetMac>
EthernetNetDevice::GetMac() const
{
    return m_mac;
}

Ptr<EthernetPhy>
EthernetNetDevice::GetPhy() const
{
    return m_phy;
}

Ptr<Channel>
EthernetNetDevice::GetChannel() const
{
    return m_channel;
}

void
EthernetNetDevice::SetIfIndex(const uint32_t index)
{
    m_ifIndex = index;
}

uint32_t
EthernetNetDevice::GetIfIndex() const
{
    return m_ifIndex;
}

void
EthernetNetDevice::SetAddress(Address address)
{
    m_mac->SetAddress(address);
}

Address
EthernetNetDevice::GetAddress() const
{
    return m_mac->GetAddress();
}

bool
EthernetNetDevice::SetMtu(const uint16_t mtu)
{
    return m_mac->SetMtu(mtu);
}

uint16_t
EthernetNetDevice::GetMtu() const
{
    return m_mac->GetMtu();
}

uint16_t
EthernetNetDevice::GetPaddingThreshold() const
{
    return 46;
}

bool
EthernetNetDevice::IsLinkUp() const
{
    return m_linkUp;
}

void
EthernetNetDevice::AddLinkChangeCallback(Callback<void> callback)
{
    m_linkChangeCallbacks.ConnectWithoutContext(callback);
}

bool
EthernetNetDevice::IsBroadcast() const
{
    return true;
}

Address
EthernetNetDevice::GetBroadcast() const
{
    return Mac48Address::GetBroadcast();
}

bool
EthernetNetDevice::IsMulticast() const
{
    return true;
}

Address
EthernetNetDevice::GetMulticast(Ipv4Address multicast) const
{
    Mac48Address ad = Mac48Address::GetMulticast(multicast);
    return ad;
}

Address
EthernetNetDevice::GetMulticast(Ipv6Address addr) const
{
    Mac48Address ad = Mac48Address::GetMulticast(addr);
    return ad;
}

bool
EthernetNetDevice::IsBridge() const
{
    return false;
}

bool
EthernetNetDevice::IsPointToPoint() const
{
    return false;
}

bool
EthernetNetDevice::Send(Ptr<Packet> packet, const Address& destination, uint16_t protocolNumber)
{
    return SendFrom(packet, m_mac->GetAddress(), destination, protocolNumber);
}

bool
EthernetNetDevice::SendFrom(Ptr<Packet> packet,
                            const Address& source,
                            const Address& destination,
                            uint16_t protocolNumber)
{
    NS_LOG_FUNCTION(Simulator::Now()
                    << packet->GetUid() << source << destination << protocolNumber);

    if (!m_sendEnable)
    {
        NS_LOG_WARN("Send Disabled on The NetDevice");
        return false;
    }

    Mac48Address dest = Mac48Address::ConvertFrom(destination);
    Mac48Address src = Mac48Address::ConvertFrom(source);

    return m_mac->Send(packet, src, dest, protocolNumber);
}

Ptr<Node>
EthernetNetDevice::GetNode() const
{
    return m_node;
}

void
EthernetNetDevice::SetNode(Ptr<Node> node)
{
    m_node = node;
}

bool
EthernetNetDevice::NeedsArp() const
{
    return true;
}

bool
EthernetNetDevice::IsReceiveEnabled() const
{
    return m_receiveEnable;
}

void
EthernetNetDevice::SetReceiveCallback(NetDevice::ReceiveCallback callBack)
{
    m_rxCallback = callBack;
}

void
EthernetNetDevice::SetPromiscReceiveCallback(PromiscReceiveCallback callBack)
{
    m_promiscRxCallback = callBack;
}

bool
EthernetNetDevice::SupportsSendFrom() const
{
    return true;
}

void
EthernetNetDevice::LinkUp()
{
    m_linkUp = true;
    m_linkChangeCallbacks();
}

void
EthernetNetDevice::LinkDown()
{
    m_linkUp = false;
    m_linkChangeCallbacks();
}

void
EthernetNetDevice::RxIndication()
{
    NS_LOG_FUNCTION(Simulator::Now());

    auto rxQueue = m_mac->GetRxQueue();

    while (!rxQueue->IsEmpty())
    {
        Ptr<Packet> frame = m_mac->GetRxQueue()->Dequeue();

        EthernetHeader header(false);

        Ptr<Packet> payload = frame->Copy();

        EthernetTrailer trailer;
        payload->RemoveTrailer(trailer);
        payload->RemoveHeader(header);

        NS_LOG_LOGIC("Received frame with length/type: " << header.GetLengthType());

        NetDevice::PacketType packetType;

        if (header.GetDestination().IsBroadcast())
        {
            packetType = NetDevice::PACKET_BROADCAST;
        }
        else if (header.GetDestination().IsGroup())
        {
            packetType = NetDevice::PACKET_MULTICAST;
        }
        else if (header.GetDestination() == m_mac->GetAddress())
        {
            packetType = NetDevice::PACKET_HOST;
        }
        else
        {
            packetType = NetDevice::PACKET_OTHERHOST;
        }

        m_mac->NotifyPromiscSniffer(frame);

        if (!m_promiscRxCallback.IsNull())
        {
            m_mac->NotifyPromiscRx(payload);

            m_promiscRxCallback(this,
                                payload,
                                header.GetLengthType(),
                                header.GetSource(),
                                header.GetDestination(),
                                packetType);
        }

        if (packetType == NetDevice::PACKET_OTHERHOST)
        {
            NS_LOG_LOGIC("Frame is addressed to " << header.GetDestination()
                                                  << ", not forwarding it up");
            continue;
        }

        m_mac->NotifySniffer(frame);
        m_mac->NotifyRx(payload);

        if (m_rxCallback.IsNull())
        {
            NS_LOG_LOGIC("No receive callback set on the device, dropping frame");
            m_mac->NotifyRxDrop(ETHERNET_MAC_DROP_NO_RX_CALLBACK, payload);
            continue;
        }

        m_rxCallback(this, payload, header.GetLengthType(), header.GetSource());
    }
}

} // namespace ethernet
} // namespace ns3
