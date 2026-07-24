/*
 * Copyright (c) 2026 Somaiya Vidyavihar University, India
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Manmita Das <das.manmita12@gmail.com>
 */

#include "ethernet-switch.h"

#include "ethernet-channel.h"
#include "ethernet-mac.h"
#include "ethernet-switch-scheduler.h"

#include "ns3/boolean.h"
#include "ns3/ethernet-trailer.h"
#include "ns3/log.h"
#include "ns3/nstime.h"
#include "ns3/simulator.h"
#include "ns3/trace-source-accessor.h"

namespace ns3
{
namespace ethernet
{

NS_LOG_COMPONENT_DEFINE("EthernetSwitch");

NS_OBJECT_ENSURE_REGISTERED(EthernetSwitch);

TypeId
EthernetSwitch::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::ethernet::EthernetSwitch")
            .SetParent<NetDevice>()
            .SetGroupName("Ethernet")
            .AddConstructor<EthernetSwitch>()
            .AddAttribute("Mtu",
                          "The link-layer maximum transmission unit.",
                          UintegerValue(1500),
                          MakeUintegerAccessor(&EthernetSwitch::SetMtu, &EthernetSwitch::GetMtu),
                          MakeUintegerChecker<uint16_t>())
            .AddAttribute("ExpirationTime",
                          "The time after which a learned address is forgotten.",
                          TimeValue(Seconds(300)),
                          MakeTimeAccessor(&EthernetSwitch::m_expirationTime),
                          MakeTimeChecker())
            .AddTraceSource("ForwardDrop",
                            "Trace source indicating a frame was dropped rather than "
                            "forwarded to the port it is destined to",
                            MakeTraceSourceAccessor(&EthernetSwitch::m_forwardDropTrace),
                            "ns3::Packet::TracedCallback");
    return tid;
}

EthernetSwitch::EthernetSwitch()
    : m_expirationTime(Seconds(300)),
      m_node(nullptr),
      m_ifIndex(0),
      m_mtu(1500),
      m_linkUp(false),
      m_sendEnable(true),
      m_address(Mac48Address::Allocate())
{
    NS_LOG_FUNCTION(this);
}

EthernetSwitch::~EthernetSwitch()
{
    NS_LOG_FUNCTION(this);
}

void
EthernetSwitch::DoDispose()
{
    NS_LOG_FUNCTION(this);

    for (const auto& port : m_ports)
    {
        if (port && port->GetMac())
        {
            port->GetMac()->SetRxIndicationCallback(Callback<void>());
        }
        if (port)
        {
            port->Dispose();
        }
    }

    m_ports.clear();
    m_scheduler = nullptr;
    m_buffer.clear();
    m_learnState.clear();
    m_node = nullptr;

    NetDevice::DoDispose();
}

void
EthernetSwitch::AddPort(Ptr<EthernetNetDevice> port)
{
    NS_LOG_FUNCTION(this << port);

    NS_ASSERT(port);
    NS_ASSERT(port->GetMac());

    port->GetMac()->SetRxIndicationCallback(
        MakeCallback(&EthernetSwitch::ReceiveFromPort, this).Bind(port));
    m_ports.push_back(port);
}

uint32_t
EthernetSwitch::GetNPorts() const
{
    NS_LOG_FUNCTION(this);

    return m_ports.size();
}

Ptr<EthernetNetDevice>
EthernetSwitch::GetPort(uint32_t n) const
{
    NS_LOG_FUNCTION(this << n);

    NS_ASSERT_MSG(n < m_ports.size(), "EthernetSwitch port index out of range");

    return m_ports[n];
}

void
EthernetSwitch::ReceiveFromPort(Ptr<EthernetNetDevice> incomingPort)
{
    NS_ASSERT(incomingPort);
    NS_ASSERT(incomingPort->GetMac());

    Ptr<Queue<Packet>> rxQueue = incomingPort->GetMac()->GetRxQueue();
    NS_ASSERT(rxQueue);

    if (rxQueue->IsEmpty())
    {
        return;
    }

    Ptr<const Packet> queuedPacket = rxQueue->Peek();

    //
    // Check whether the switch allows this frame
    // to leave the ingress RX queue.
    //
    uint32_t bytes = EthernetMac::GetFrameSize(queuedPacket->GetSize());

    if (!CanAccept(bytes))
    {
        NS_LOG_LOGIC("Switch memory unavailable, keeping packet in ingress RX queue on port "
                     << incomingPort << " (queue size = "
                     << incomingPort->GetMac()->GetRxQueue()->GetNPackets() << ")");
        return;
    }

    Ptr<Packet> packet = rxQueue->Dequeue();

    // Extract Ethernet header from packet.
    EthernetHeader header(false);
    EthernetTrailer trailer;

    packet->RemoveTrailer(trailer);
    packet->RemoveHeader(header);

    Mac48Address src = header.GetSource();
    Mac48Address dst = header.GetDestination();

    LearnMacAddress(src, incomingPort);

    Ptr<EthernetNetDevice> outPort = nullptr;

    if (!dst.IsBroadcast() && !dst.IsGroup())
    {
        outPort = LookupMacAddress(dst);
    }

    if (outPort && outPort != incomingPort)
    {
        SwitchBufferEntry entry;
        entry.packet = packet;
        entry.header = header;
        entry.outgoingPort = outPort;

        EnqueueToBuffer(entry);
    }
    else if (!outPort)
    {
        FloodFrame(incomingPort, packet, header);
    }
}

bool
EthernetSwitch::TransmitToPort(Ptr<EthernetNetDevice> outgoingPort,
                               Ptr<const Packet> packet,
                               uint16_t protocol,
                               const Address& source,
                               const Address& destination)
{
    NS_LOG_FUNCTION(this << outgoingPort << packet << protocol);

    //
    // The frame keeps the address of the station that sent it rather than
    // taking the address of the port it leaves the switch through.
    //
    return outgoingPort->SendFrom(packet->Copy(), source, destination, protocol);
}

bool
EthernetSwitch::CanForwardTo(Ptr<EthernetNetDevice> port, Ptr<const Packet> packet) const
{
    NS_LOG_FUNCTION(this << port << packet);

    NS_ASSERT(port);

    if (!IsPortSendEnabled(port) || !port->IsLinkUp())
    {
        NS_LOG_LOGIC("Port " << port << " is not able to transmit");
        return false;
    }

    if (port->GetMac()->IsTxPaused())
    {
        NS_LOG_LOGIC("Port " << port << " transmission is paused");

        return false;
    }

    Ptr<Queue<Packet>> txQueue = port->GetMac()->GetTxQueue();
    if (txQueue && txQueue->WouldOverflow(1, packet->GetSize()))
    {
        NS_LOG_LOGIC("Transmit queue of port " << port << " is full");
        return false;
    }

    if (!m_memoryCheck.IsNull() &&
        !m_memoryCheck(port, EthernetMac::GetFrameSize(packet->GetSize())))
    {
        NS_LOG_LOGIC("Memory limit kept by the switch reached for port " << port);
        return false;
    }

    return true;
}

bool
EthernetSwitch::CanAccept(uint32_t bytes) const
{
    NS_LOG_FUNCTION(this << bytes);

    if (!m_switchMemoryCheck.IsNull() && !m_switchMemoryCheck(bytes))
    {
        NS_LOG_LOGIC("Switch memory check rejected frame of size " << bytes << " bytes");
        return false;
    }

    return true;
}

void
EthernetSwitch::SetMemoryCheck(MemoryCheckCallback check)
{
    NS_LOG_FUNCTION(this);

    m_memoryCheck = check;
}

void
EthernetSwitch::SetSwitchMemoryCheck(SwitchMemoryCheckCallback check)
{
    NS_LOG_FUNCTION(this);

    m_switchMemoryCheck = check;
}

void
EthernetSwitch::NotifyForwardDrop(const SwitchBufferEntry& entry)
{
    NS_LOG_FUNCTION(this << entry.packet);

    m_forwardDropTrace(entry.packet);
}

void
EthernetSwitch::LearnMacAddress(Mac48Address source, Ptr<EthernetNetDevice> port)
{
    NS_LOG_FUNCTION(this << source << port);
    LearnedState& state = m_learnState[source];
    state.associatedPort = port;
    state.expirationTime = Simulator::Now() + m_expirationTime;
}

Ptr<EthernetNetDevice>
EthernetSwitch::LookupMacAddress(Mac48Address destination)
{
    NS_LOG_FUNCTION(this << destination);

    Time now = Simulator::Now();

    auto iter = m_learnState.find(destination);
    if (iter != m_learnState.end())
    {
        LearnedState& state = iter->second;

        if (state.expirationTime > now)
        {
            return state.associatedPort;
        }

        m_learnState.erase(iter);
    }

    return nullptr;
}

void
EthernetSwitch::FloodFrame(Ptr<EthernetNetDevice> incomingPort,
                           Ptr<const Packet> packet,
                           const EthernetHeader& header)
{
    for (const auto& port : m_ports)
    {
        // Do not send the frame back out the ingress port
        if (!port || port == incomingPort)
        {
            continue;
        }

        SwitchBufferEntry entry;
        entry.packet = packet->Copy();
        entry.header = header;
        entry.outgoingPort = port;

        EnqueueToBuffer(entry);
    }
}

void
EthernetSwitch::EnqueueToBuffer(const SwitchBufferEntry& entry)
{
    NS_LOG_FUNCTION(this << entry.packet);

    m_buffer.push_back(entry);
    if (m_scheduler)
    {
        m_scheduler->ScheduleTransmission();
    }
}

bool
EthernetSwitch::HasBufferedFrames() const
{
    NS_LOG_FUNCTION(this);

    return !m_buffer.empty();
}

void
EthernetSwitch::SetScheduler(Ptr<EthernetSwitchScheduler> scheduler)
{
    NS_ASSERT(scheduler);

    m_scheduler = scheduler;
    m_scheduler->SetSwitch(this);
}

void
EthernetSwitch::SetIfIndex(const uint32_t index)
{
    m_ifIndex = index;
}

uint32_t
EthernetSwitch::GetIfIndex() const
{
    return m_ifIndex;
}

Ptr<Channel>
EthernetSwitch::GetChannel() const
{
    return nullptr;
}

void
EthernetSwitch::SetAddress(Address address)
{
    m_address = address;
}

Address
EthernetSwitch::GetAddress() const
{
    return m_address;
}

bool
EthernetSwitch::SetMtu(const uint16_t mtu)
{
    m_mtu = mtu;
    return true;
}

uint16_t
EthernetSwitch::GetMtu() const
{
    return m_mtu;
}

bool
EthernetSwitch::IsLinkUp() const
{
    return m_linkUp;
}

void
EthernetSwitch::AddLinkChangeCallback(Callback<void> callback)
{
    m_linkChangeCallbacks.ConnectWithoutContext(callback);
}

bool
EthernetSwitch::IsBroadcast() const
{
    return true;
}

Address
EthernetSwitch::GetBroadcast() const
{
    return Mac48Address::GetBroadcast();
}

bool
EthernetSwitch::IsMulticast() const
{
    return true;
}

Address
EthernetSwitch::GetMulticast(Ipv4Address multicastGroup) const
{
    return Mac48Address::GetMulticast(multicastGroup);
}

Address
EthernetSwitch::GetMulticast(Ipv6Address addr) const
{
    return Mac48Address::GetMulticast(addr);
}

bool
EthernetSwitch::IsBridge() const
{
    return true;
}

bool
EthernetSwitch::IsPointToPoint() const
{
    return false;
}

bool
EthernetSwitch::Send(Ptr<Packet> packet, const Address& destination, uint16_t protocolNumber)
{
    return SendFrom(packet, m_address, destination, protocolNumber);
}

bool
EthernetSwitch::SendFrom(Ptr<Packet> packet,
                         const Address& source,
                         const Address& destination,
                         uint16_t protocolNumber)
{
    if (!m_sendEnable)
    {
        return false;
    }

    Mac48Address dst = Mac48Address::ConvertFrom(destination);
    Mac48Address src = Mac48Address::ConvertFrom(source);

    if (m_ports.empty())
    {
        return false;
    }

    Ptr<EthernetNetDevice> outPort = LookupMacAddress(dst);
    if (outPort == nullptr)
    {
        for (const auto& port : m_ports)
        {
            if (port == nullptr)
            {
                continue;
            }
            port->SendFrom(packet->Copy(), src, dst, protocolNumber);
        }
        return true;
    }

    if (IsPortSendEnabled(outPort) && outPort->IsLinkUp())
    {
        return outPort->SendFrom(packet->Copy(), src, dst, protocolNumber);
    }

    return false;
}

Ptr<Node>
EthernetSwitch::GetNode() const
{
    return m_node;
}

void
EthernetSwitch::SetNode(Ptr<Node> node)
{
    m_node = node;
}

bool
EthernetSwitch::NeedsArp() const
{
    return true;
}

void
EthernetSwitch::SetReceiveCallback(NetDevice::ReceiveCallback cb)
{
    m_rxCallback = cb;
}

void
EthernetSwitch::SetPromiscReceiveCallback(NetDevice::PromiscReceiveCallback cb)
{
    m_promiscRxCallback = cb;
}

bool
EthernetSwitch::SupportsSendFrom() const
{
    return true;
}

std::list<SwitchBufferEntry>&
EthernetSwitch::GetBuffer()
{
    return m_buffer;
}

bool
EthernetSwitch::IsPortSendEnabled(Ptr<EthernetNetDevice> port) const
{
    BooleanValue sendEnable;
    port->GetAttribute("SendEnable", sendEnable);
    return sendEnable.Get();
}

} // namespace ethernet
} // namespace ns3
