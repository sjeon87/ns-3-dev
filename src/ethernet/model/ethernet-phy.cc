/*
 * Copyright (c) 2026 Somaiya Vidyavihar University, India
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Manmita Das <das.manmita12@gmail.com>
 */

#include "ethernet-phy.h"

#include "ethernet-channel.h"
#include "ethernet-mac.h"
#include "ethernet-net-device.h"

#include "ns3/error-model.h"
#include "ns3/log.h"
#include "ns3/pointer.h"
#include "ns3/simulator.h"
#include "ns3/trace-source-accessor.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("EthernetPhy");

namespace ethernet
{

NS_OBJECT_ENSURE_REGISTERED(EthernetPhy);

std::ostream&
operator<<(std::ostream& os, EthernetPhyState state)
{
    os << static_cast<uint32_t>(state);
    return os;
}

TypeId
EthernetPhy::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::ethernet::EthernetPhy")
            .SetParent<Object>()
            .SetGroupName("Ethernet")
            .AddConstructor<EthernetPhy>()
            .AddAttribute("ReceiveErrorModel",
                          "The receive error model used to simulate packet loss",
                          PointerValue(),
                          MakePointerAccessor(&EthernetPhy::m_receiveErrorModel),
                          MakePointerChecker<ErrorModel>())
            .AddTraceSource("PhyTxBegin",
                            "Trace source indicating a packet has begun transmitting",
                            MakeTraceSourceAccessor(&EthernetPhy::m_phyTxBeginTrace),
                            "ns3::Packet::TracedCallback")
            .AddTraceSource("PhyTxEnd",
                            "Trace source indicating a packet has completed transmission",
                            MakeTraceSourceAccessor(&EthernetPhy::m_phyTxEndTrace),
                            "ns3::Packet::TracedCallback")
            .AddTraceSource("PhyTxDrop",
                            "Trace source indicating a packet has been dropped during "
                            "transmission",
                            MakeTraceSourceAccessor(&EthernetPhy::m_phyTxDropTrace),
                            "ns3::Packet::TracedCallback")
            .AddTraceSource("PhyRxBegin",
                            "Trace source indicating a packet has begun reception",
                            MakeTraceSourceAccessor(&EthernetPhy::m_phyRxBeginTrace),
                            "ns3::Packet::TracedCallback")
            .AddTraceSource("PhyRxEnd",
                            "Trace source indicating a packet has completed reception",
                            MakeTraceSourceAccessor(&EthernetPhy::m_phyRxEndTrace),
                            "ns3::Packet::TracedCallback")
            .AddTraceSource("PhyRxDrop",
                            "Trace source indicating a packet has been dropped during "
                            "reception",
                            MakeTraceSourceAccessor(&EthernetPhy::m_phyRxDropTrace),
                            "ns3::Packet::TracedCallback");
    return tid;
}

EthernetPhy::EthernetPhy()
    : m_rxState(EthernetPhyState::PHY_IDLE),
      m_txState(EthernetPhyState::PHY_IDLE),
      m_linkUp(false)
{
    NS_LOG_FUNCTION(this);
}

EthernetPhy::~EthernetPhy()
{
    NS_LOG_FUNCTION(this);
}

void
EthernetPhy::DoDispose()
{
    NS_LOG_FUNCTION(this);
    m_device = nullptr;
    m_channel = nullptr;
    m_mac = nullptr;
    m_receiveErrorModel = nullptr;
    Object::DoDispose();
}

void
EthernetPhy::SetChannel(Ptr<EthernetChannel> c)
{
    m_channel = c;
}

Ptr<EthernetChannel>
EthernetPhy::GetChannel()
{
    return m_channel;
}

Ptr<EthernetMac>
EthernetPhy::GetMac() const
{
    return m_mac;
}

void
EthernetPhy::SetMac(Ptr<EthernetMac> mac)
{
    m_mac = mac;
}

void
EthernetPhy::SetDevice(Ptr<EthernetNetDevice> d)
{
    m_device = d;
}

Ptr<EthernetNetDevice>
EthernetPhy::GetDevice() const
{
    return m_device;
}

DataRate
EthernetPhy::GetDataRate() const
{
    //
    // The PHY itself is agnostic to the rate: it carries whatever rate the
    // device settled on with its peer.
    //
    NS_ASSERT_MSG(m_device, "Device not set, cannot determine the data rate");
    return m_device->GetDataRate();
}

bool
EthernetPhy::TxStart(Ptr<Packet> packet)
{
    NS_LOG_FUNCTION(Simulator::Now() << packet->GetUid());

    NS_ASSERT_MSG(packet, "Cannot transmit null packet");
    NS_ASSERT_MSG(m_channel, "Channel not set");
    NS_ASSERT_MSG(m_device, "Device not set");

    if (!IsLinkUp())
    {
        m_phyTxDropTrace(packet);
        return false;
    }

    ChangeTxState(EthernetPhyState::PHY_TRANSMITTING);

    m_phyTxBeginTrace(packet);

    Time txTime = CalculateTxTime(packet);
    NS_LOG_DEBUG("Transmission time = " << txTime.GetNanoSeconds() << " ns");

    NS_LOG_INFO("Starting PHY transmission for packet " << packet->GetUid());
    m_channel->PropagationStart(packet, m_device);

    Simulator::Schedule(txTime, &EthernetPhy::TxEnd, this, packet);

    return true;
}

bool
EthernetPhy::TxEnd(Ptr<Packet> packet)
{
    NS_LOG_FUNCTION(Simulator::Now() << packet->GetUid());

    ChangeTxState(EthernetPhyState::PHY_IDLE);
    m_phyTxEndTrace(packet);
    m_mac->TxEnd();
    m_channel->TxEnd(packet, m_device);
    return true;
}

void
EthernetPhy::RxStart(Ptr<const Packet> packet, Ptr<EthernetNetDevice> sender)
{
    NS_LOG_FUNCTION(Simulator::Now() << packet->GetUid() << sender);

    ChangeRxState(EthernetPhyState::PHY_RECEIVING);
    m_phyRxBeginTrace(packet);
}

void
EthernetPhy::Receive(Ptr<const Packet> packet, Ptr<EthernetNetDevice> sender)
{
    NS_LOG_FUNCTION(Simulator::Now() << packet->GetUid() << sender);

    ChangeRxState(EthernetPhyState::PHY_IDLE);

    if (!IsLinkUp())
    {
        m_phyRxDropTrace(packet);
        return;
    }

    m_phyRxEndTrace(packet);

    Ptr<Packet> pktCopy = packet->Copy();

    if (m_receiveErrorModel && m_receiveErrorModel->IsCorrupt(pktCopy))
    {
        NS_LOG_INFO("Dropping corrupted packet " << pktCopy->GetUid());
        m_phyRxDropTrace(pktCopy);
        return;
    }

    m_mac->Receive(pktCopy);
}

void
EthernetPhy::SetErrorModel(Ptr<ErrorModel> e)
{
    m_receiveErrorModel = e;
}

Ptr<ErrorModel>
EthernetPhy::GetErrorModel() const
{
    return m_receiveErrorModel;
}

void
EthernetPhy::ChangeRxState(EthernetPhyState newState)
{
    switch (newState)
    {
    case EthernetPhyState::PHY_RECEIVING:
        NS_ASSERT_MSG(m_rxState == EthernetPhyState::PHY_IDLE,
                      "PHY must be IDLE before starting reception");
        break;

    case EthernetPhyState::PHY_IDLE:
        NS_ASSERT_MSG(m_rxState == EthernetPhyState::PHY_RECEIVING,
                      "PHY must be RECEIVING before becoming IDLE");
        break;

    default:
        break;
    }
    m_rxState = newState;
}

void
EthernetPhy::ChangeTxState(EthernetPhyState newState)
{
    switch (newState)
    {
    case EthernetPhyState::PHY_TRANSMITTING:
        NS_ASSERT_MSG(m_txState == EthernetPhyState::PHY_IDLE,
                      "PHY state is not IDLE. Cannot start transmission.");
        break;

    case EthernetPhyState::PHY_IDLE:
        NS_ASSERT_MSG(m_txState == EthernetPhyState::PHY_TRANSMITTING,
                      "PHY must be TRANSMITTING before becoming IDLE");
        break;

    default:
        break;
    }
    m_txState = newState;
}

Time
EthernetPhy::CalculateTxTime(Ptr<const Packet> packet) const
{
    // Adding 8 Bytes for Preamble(7 Bytes) and SFD(1 Byte)
    return GetDataRate().CalculateBytesTxTime(packet->GetSize() + 8);
}

void
EthernetPhy::LinkUp()
{
    NS_LOG_FUNCTION(this);
    m_linkUp = true;

    if (m_device)
    {
        m_device->LinkUp();
    }
}

void
EthernetPhy::LinkDown()
{
    NS_LOG_FUNCTION(this);
    m_linkUp = false;

    if (m_device)
    {
        m_device->LinkDown();
    }
}

bool
EthernetPhy::IsLinkUp() const
{
    return m_linkUp;
}

void
EthernetPhy::UpdateInterframeGap()
{
    if (m_mac && m_device)
    {
        m_mac->SetInterframeGap(GetDataRate().CalculateBytesTxTime(96 / 8));
    }
}

} // namespace ethernet
} // namespace ns3
