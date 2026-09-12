/*
 * Copyright (c) 2026 Somaiya Vidyavihar University, India
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Manmita Das <das.manmita12@gmail.com>
 */

#include "ethernet-mac.h"

#include "ethernet-flow-control-header.h"
#include "ethernet-net-device.h"
#include "ethernet-phy.h"

#include "ns3/drop-tail-queue.h"
#include "ns3/ethernet-header.h"
#include "ns3/ethernet-trailer.h"
#include "ns3/log.h"
#include "ns3/node.h"
#include "ns3/pointer.h"
#include "ns3/simulator.h"
#include "ns3/trace-source-accessor.h"
#include "ns3/uinteger.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("EthernetMac");

namespace ethernet
{

NS_OBJECT_ENSURE_REGISTERED(EthernetMac);

std::ostream&
operator<<(std::ostream& os, EthernetMacState state)
{
    return os << static_cast<uint32_t>(state);
}

TypeId
EthernetMac::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::ethernet::EthernetMac")
            .SetParent<Object>()
            .SetGroupName("Ethernet")
            .AddConstructor<EthernetMac>()
            .AddAttribute("Address",
                          "The MAC address of this device.",
                          Mac48AddressValue(Mac48Address("ff:ff:ff:ff:ff:ff")),
                          MakeMac48AddressAccessor(&EthernetMac::m_address),
                          MakeMac48AddressChecker())
            .AddAttribute("Mtu",
                          "The MAC-level Maximum Transmission Unit",
                          UintegerValue(DEFAULT_MTU),
                          MakeUintegerAccessor(&EthernetMac::SetMtu, &EthernetMac::GetMtu),
                          MakeUintegerChecker<uint16_t>())
            .AddAttribute("TxQueue",
                          "The queue holding the frames waiting for transmission.",
                          PointerValue(),
                          MakePointerAccessor(&EthernetMac::m_txQueue),
                          MakePointerChecker<Queue<Packet>>())
            .AddAttribute("RxQueue",
                          "The queue holding the received frames waiting to be processed.",
                          PointerValue(),
                          MakePointerAccessor(&EthernetMac::m_rxQueue),
                          MakePointerChecker<Queue<Packet>>())
            .AddTraceSource("MacTx",
                            "Trace source indicating a packet has arrived for transmission "
                            "by this device",
                            MakeTraceSourceAccessor(&EthernetMac::m_macTxTrace),
                            "ns3::Packet::TracedCallback")
            .AddTraceSource("MacTxDrop",
                            "Trace source indicating a packet has been dropped by the "
                            "device before transmission",
                            MakeTraceSourceAccessor(&EthernetMac::m_macTxDropTrace),
                            "ns3::ethernet::EthernetMac::DroppedFrameCallback")
            .AddTraceSource("MacPromiscRx",
                            "A packet has been received by this device and is being "
                            "forwarded up the local protocol stack (promiscuous)",
                            MakeTraceSourceAccessor(&EthernetMac::m_macPromiscRxTrace),
                            "ns3::Packet::TracedCallback")
            .AddTraceSource("MacRx",
                            "A packet has been received by this device and is being "
                            "forwarded up the local protocol stack (non-promiscuous)",
                            MakeTraceSourceAccessor(&EthernetMac::m_macRxTrace),
                            "ns3::Packet::TracedCallback")
            .AddTraceSource("MacRxDrop",
                            "Trace source indicating a packet was received, "
                            "but dropped before being forwarded up the stack",
                            MakeTraceSourceAccessor(&EthernetMac::m_macRxDropTrace),
                            "ns3::ethernet::EthernetMac::DroppedFrameCallback")
            .AddTraceSource("Sniffer",
                            "Trace source simulating a non-promiscuous packet sniffer",
                            MakeTraceSourceAccessor(&EthernetMac::m_snifferTrace),
                            "ns3::Packet::TracedCallback")
            .AddTraceSource("PromiscSniffer",
                            "Trace source simulating a promiscuous packet sniffer",
                            MakeTraceSourceAccessor(&EthernetMac::m_promiscSnifferTrace),
                            "ns3::Packet::TracedCallback");
    return tid;
}

EthernetMac::EthernetMac()
    : m_macTxState(EthernetMacState::MAC_IDLE),
      m_txPaused(false),
      m_isPauseFrameSent(false)
{
    NS_LOG_FUNCTION(this);

    m_txQueue = CreateObject<DropTailQueue<Packet>>();
    m_rxQueue = CreateObject<DropTailQueue<Packet>>();
}

EthernetMac::~EthernetMac()
{
    NS_LOG_FUNCTION(this);
}

void
EthernetMac::DoDispose()
{
    NS_LOG_FUNCTION(this);
    Simulator::Cancel(m_resumeEvent);
    m_device = nullptr;
    m_phy = nullptr;
    m_txQueue = nullptr;
    m_rxQueue = nullptr;
    Object::DoDispose();
}

void
EthernetMac::DoInitialize()
{
    NS_LOG_FUNCTION(this);

    m_rxQueue->TraceConnectWithoutContext("Dequeue",
                                          MakeCallback(&EthernetMac::SendUnpauseFrame, this));

    Object::DoInitialize();
}

void
EthernetMac::SetPhy(Ptr<EthernetPhy> phy)
{
    m_phy = phy;
}

Ptr<EthernetPhy>
EthernetMac::GetPhy()
{
    return m_phy;
}

void
EthernetMac::SetDevice(Ptr<NetDevice> d)
{
    m_device = DynamicCast<EthernetNetDevice>(d);
}

Ptr<NetDevice>
EthernetMac::GetDevice() const
{
    return m_device;
}

bool
EthernetMac::Send(Ptr<Packet> packet,
                  const Mac48Address& source,
                  const Mac48Address& destination,
                  uint16_t protocolNumber)
{
    NS_LOG_FUNCTION(Simulator::Now()
                    << packet->GetUid() << source << destination << protocolNumber);

    Ptr<Packet> frame = packet->Copy();

    EthernetHeader header(false);
    header.SetSource(source);
    header.SetDestination(destination);
    header.SetLengthType(protocolNumber);
    EthernetTrailer trailer;

    if (frame->GetSize() < MIN_PAYLOAD_SIZE)
    {
        frame->AddPaddingAtEnd(MIN_PAYLOAD_SIZE - frame->GetSize());
    }

    frame->AddHeader(header);

    if (Node::ChecksumEnabled())
    {
        trailer.EnableFcs(true);
    }
    trailer.CalcFcs(frame);
    frame->AddTrailer(trailer);

    //
    // A frame can only go straight out if the transmitter is free and the peer
    // has not asked us to pause. Anything else waits in the queue.
    //
    if (m_macTxState == EthernetMacState::MAC_IDLE && !m_txPaused && m_txQueue->IsEmpty())
    {
        TxStart(frame);
        return true;
    }

    if (!m_txQueue->Enqueue(frame))
    {
        NS_LOG_INFO("Transmit queue full, dropping packet " << packet->GetUid());
        m_macTxDropTrace(ETHERNET_MAC_DROP_TX_QUEUE_FULL, packet);
        return false;
    }

    NS_LOG_LOGIC("TX queue size after enqueue: " << m_txQueue->GetNPackets());

    return true;
}

void
EthernetMac::SendPauseFrame(uint16_t pauseQuanta)
{
    NS_LOG_FUNCTION(this << pauseQuanta);
    NS_LOG_INFO("Sending PAUSE frame");

    EthernetFlowControlHeader header;
    header.SetPauseQuanta(pauseQuanta);

    Ptr<Packet> packet = Create<Packet>();
    packet->AddHeader(header);

    m_device->Send(packet, Mac48Address("01:80:C2:00:00:01"), PAUSE_LENGTH_TYPE);
}

void
EthernetMac::SendUnpauseFrame(Ptr<const Packet> packet)
{
    if (m_isPauseFrameSent && ShouldSendUnpauseFrame())
    {
        NS_LOG_LOGIC("Releasing peer transmission");
        SendPauseFrame(0);
        m_isPauseFrameSent = false;
    }
}

void
EthernetMac::Receive(Ptr<Packet> frame)
{
    NS_LOG_FUNCTION(Simulator::Now() << frame->GetUid());

    if (!m_device->IsReceiveEnabled())
    {
        NS_LOG_LOGIC("Receive disabled on the NetDevice, dropping frame");
        m_macRxDropTrace(ETHERNET_MAC_DROP_RX_DISABLED, frame);
        return;
    }

    Ptr<Packet> validatedFrame = frame->Copy();
    EthernetTrailer trailer;
    validatedFrame->RemoveTrailer(trailer);

    if (Node::ChecksumEnabled())
    {
        trailer.EnableFcs(true);
    }

    if (!trailer.CheckFcs(validatedFrame))
    {
        NS_LOG_LOGIC("Dropping frame " << frame->GetUid() << " that failed FCS validation");
        m_macRxDropTrace(ETHERNET_MAC_DROP_FCS_ERROR, frame);
        return;
    }

    EthernetHeader header(false);
    validatedFrame->RemoveHeader(header);

    if (header.GetLengthType() == PAUSE_LENGTH_TYPE)
    {
        NS_LOG_LOGIC("Received MAC control frame");
        m_promiscSnifferTrace(frame);
        ProcessPauseFrame(validatedFrame);
        return;
    }

    if (ShouldSendPauseFrame() && !m_isPauseFrameSent)
    {
        NS_LOG_LOGIC("Asking the peer to pause, pauseQuanta=65535");
        SendPauseFrame(0xFFFF);
        m_isPauseFrameSent = true;
    }

    if (!m_rxQueue->Enqueue(frame))
    {
        NS_LOG_LOGIC("Receive queue full, dropping frame " << frame->GetUid());
        m_macRxDropTrace(ETHERNET_MAC_DROP_RX_QUEUE_FULL, frame);
        return;
    }

    NS_LOG_LOGIC("RX queue size after enqueue: " << m_rxQueue->GetNPackets());

    if (!m_rxIndicationCallback.IsNull())
    {
        m_rxIndicationCallback();
    }
}

void
EthernetMac::SetInterframeGap(Time gap)
{
    m_interframeGapTime = gap;
}

void
EthernetMac::TxStart(Ptr<Packet> frame)
{
    NS_LOG_FUNCTION(Simulator::Now() << frame->GetUid());

    NS_ASSERT_MSG(m_macTxState == EthernetMacState::MAC_IDLE,
                  " Mac State must be IDLE " << m_macTxState);

    SetTxMacState(EthernetMacState::MAC_TRANSMITTING);

    if (!m_phy->TxStart(frame))
    {
        NS_LOG_LOGIC("PHY could not start transmission of frame " << frame->GetUid()
                                                                  << ", dropping it");
        m_macTxDropTrace(ETHERNET_MAC_DROP_LINK_DOWN, frame);
        SetTxMacState(EthernetMacState::MAC_IDLE);
        return;
    }

    m_snifferTrace(frame);
    m_promiscSnifferTrace(frame);

    m_macTxTrace(frame);

    NS_LOG_INFO("Transmitting frame " << frame->GetUid());
}

void
EthernetMac::TxNext()
{
    NS_LOG_FUNCTION(this);

    NS_ASSERT_MSG(m_macTxState == EthernetMacState::MAC_IDLE, "MAC state must be IDLE");

    if (m_txPaused)
    {
        NS_LOG_INFO("Transmission paused, holding " << m_txQueue->GetNPackets()
                                                    << " queued frame(s)");
        return;
    }

    Ptr<Packet> frame = m_txQueue->Dequeue();
    if (!frame)
    {
        return;
    }

    TxStart(frame);
}

void
EthernetMac::TxEnd()
{
    NS_LOG_FUNCTION(this);

    NS_ASSERT_MSG(m_macTxState == EthernetMacState::MAC_TRANSMITTING,
                  " Mac State must be TRANSMITTING " << m_macTxState);

    SetTxMacState(EthernetMacState::MAC_GAP);
    Simulator::Schedule(m_interframeGapTime, &EthernetMac::InterframeGapEnd, this);
}

void
EthernetMac::InterframeGapEnd()
{
    NS_ASSERT_MSG(m_macTxState == EthernetMacState::MAC_GAP,
                  " Mac State must be GAP " << m_macTxState);

    SetTxMacState(EthernetMacState::MAC_IDLE);
    TxNext();
}

void
EthernetMac::PauseTransmission(Time pauseDuration)
{
    NS_LOG_FUNCTION(this << pauseDuration);

    m_txPaused = true;
    NS_LOG_LOGIC("TX PAUSED at " << Simulator::Now());

    Simulator::Cancel(m_resumeEvent);

    m_resumeEvent = Simulator::Schedule(pauseDuration, &EthernetMac::ResumeTransmission, this);
}

void
EthernetMac::ResumeTransmission()
{
    NS_LOG_FUNCTION(this);

    m_txPaused = false;
    NS_LOG_INFO("TX RESUMED at " << Simulator::Now());

    TxNext();
}

bool
EthernetMac::IsTxPaused() const
{
    return m_txPaused;
}

Ptr<Queue<Packet>>
EthernetMac::GetTxQueue() const
{
    return m_txQueue;
}

void
EthernetMac::SetTxQueue(Ptr<Queue<Packet>> queue)
{
    NS_ASSERT_MSG(queue, "Cannot set a null transmit queue");
    NS_ABORT_MSG_IF(IsInitialized(), "Cannot replace the transmit queue after initialization");
    m_txQueue = queue;
}

Ptr<Queue<Packet>>
EthernetMac::GetRxQueue() const
{
    return m_rxQueue;
}

void
EthernetMac::SetRxQueue(Ptr<Queue<Packet>> queue)
{
    NS_ASSERT_MSG(queue, "Cannot set a null receive queue");
    NS_ABORT_MSG_IF(IsInitialized(), "Cannot replace the receive queue after initialization");
    m_rxQueue = queue;
}

void
EthernetMac::ProcessPauseFrame(Ptr<Packet> payload)
{
    NS_LOG_FUNCTION(this << payload->GetUid());

    EthernetFlowControlHeader controlHeader;
    if (payload->RemoveHeader(controlHeader) == 0)
    {
        NS_LOG_LOGIC("Dropping MAC control frame carrying an unsupported opcode");
        m_macRxDropTrace(ETHERNET_MAC_DROP_INVALID_PAUSE_FRAME, payload);
        return;
    }

    Time pauseTime = ConvertPauseQuantaToTime(controlHeader.GetPauseQuanta());
    NS_LOG_INFO("Valid PAUSE frame received, time=" << pauseTime);
    PauseTransmission(pauseTime);
    NS_LOG_LOGIC("PAUSE applied at MAC, time = " << Simulator::Now());
}

void
EthernetMac::SetTxMacState(EthernetMacState newState)
{
    NS_LOG_FUNCTION(this << newState);
    m_macTxState = newState;
}

EthernetMacState
EthernetMac::GetTxMacState() const
{
    return m_macTxState;
}

void
EthernetMac::SetAddress(Address address)
{
    m_address = Mac48Address::ConvertFrom(address);
}

Address
EthernetMac::GetAddress() const
{
    return m_address;
}

bool
EthernetMac::SetMtu(const uint16_t mtu)
{
    m_mtu = mtu;
    return true;
}

uint16_t
EthernetMac::GetMtu() const
{
    return m_mtu;
}

bool
EthernetMac::IsPauseFrameSent() const
{
    NS_LOG_FUNCTION(this);
    return m_isPauseFrameSent;
}

void
EthernetMac::SetPauseFrameSent(bool sent)
{
    NS_LOG_FUNCTION(this << sent);
    m_isPauseFrameSent = sent;
}

Time
EthernetMac::ConvertPauseQuantaToTime(uint16_t pauseQuanta)
{
    // 1 pause quanta = 512 bit-times.
    // Pause duration = (pauseQuanta * 512 bit-times) / link data rate (bits/s).
    return Seconds((512.0 * pauseQuanta) / m_device->GetPhy()->GetDataRate().GetBitRate());
}

bool
EthernetMac::ShouldSendPauseFrame() const
{
    NS_LOG_FUNCTION(this);
    uint32_t maxPackets = m_rxQueue->GetMaxSize().GetValue();

    if (maxPackets == 0)
    {
        return false;
    }

    double queuePercentage = (static_cast<double>(m_rxQueue->GetNPackets()) / maxPackets) * 100.0;
    // Send pause frame if queue is 80% full or more
    return queuePercentage >= 80.0;
}

bool
EthernetMac::ShouldSendUnpauseFrame() const
{
    uint32_t maxPackets = m_rxQueue->GetMaxSize().GetValue();

    if (maxPackets == 0)
    {
        return false;
    }

    double queuePercentage = (static_cast<double>(m_rxQueue->GetNPackets()) / maxPackets) * 100.0;

    return queuePercentage <= 50.0;
}

uint32_t
EthernetMac::GetFrameSize(uint32_t payloadSize)
{
    return std::max(payloadSize, static_cast<uint32_t>(MIN_PAYLOAD_SIZE)) +
           EthernetHeader().GetSerializedSize() + EthernetTrailer().GetSerializedSize();
}

void
EthernetMac::SetRxIndicationCallback(Callback<void> callback)
{
    m_rxIndicationCallback = callback;
}

void
EthernetMac::NotifyRx(Ptr<const Packet> packet) const
{
    m_macRxTrace(packet);
}

void
EthernetMac::NotifyPromiscRx(Ptr<const Packet> packet) const
{
    m_macPromiscRxTrace(packet);
}

void
EthernetMac::NotifyRxDrop(EthernetMacDropReason reason, Ptr<const Packet> packet) const
{
    m_macRxDropTrace(reason, packet);
}

void
EthernetMac::NotifySniffer(Ptr<const Packet> packet) const
{
    m_snifferTrace(packet);
}

void
EthernetMac::NotifyPromiscSniffer(Ptr<const Packet> packet) const
{
    m_promiscSnifferTrace(packet);
}

void
EthernetMac::NotifyLinkUp()
{
    NS_LOG_FUNCTION(this);

    if (m_macTxState == EthernetMacState::MAC_IDLE)
    {
        TxNext();
    }
}

} // namespace ethernet
} // namespace ns3
