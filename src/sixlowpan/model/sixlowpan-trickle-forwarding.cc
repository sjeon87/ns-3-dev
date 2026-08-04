/*
 * Copyright (c) 2026 SRM Institute of Science and Technology, India
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Usham Roy <ushamroy80@gmail.com>
 */

#include "sixlowpan-trickle-forwarding.h"

#include "ns3/log.h"
#include "ns3/simulator.h"
#include "ns3/uinteger.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("SixLowPanTrickleForwarding");
NS_OBJECT_ENSURE_REGISTERED(SixLowPanTrickleForwarding);

TypeId
SixLowPanTrickleForwarding::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::SixLowPanTrickleForwarding")
            .SetParent<SixLowPanMeshUnderRouting>()
            .SetGroupName("SixLowPan")
            .AddConstructor<SixLowPanTrickleForwarding>()
            .AddAttribute("MinInterval",
                          "RFC 6206 Imin: the minimum Trickle interval.",
                          TimeValue(MilliSeconds(10)),
                          MakeTimeAccessor(&SixLowPanTrickleForwarding::m_minInterval),
                          MakeTimeChecker(MilliSeconds(1)))
            .AddAttribute("Doublings",
                          "Number of interval doublings; Imax = MinInterval * 2^Doublings.",
                          UintegerValue(4),
                          MakeUintegerAccessor(&SixLowPanTrickleForwarding::m_doublings),
                          MakeUintegerChecker<uint8_t>(0, 16))
            .AddAttribute("RedundancyConstant",
                          "RFC 6206 k: forward only if fewer than k copies were heard. "
                          "Zero disables suppression.",
                          UintegerValue(1),
                          MakeUintegerAccessor(&SixLowPanTrickleForwarding::m_redundancy),
                          MakeUintegerChecker<uint16_t>())
            .AddAttribute("MaxForwardingDelay",
                          "Discard a pending packet if it is not forwarded within this time "
                          "(suppression won, or too many tries).",
                          TimeValue(MilliSeconds(500)),
                          MakeTimeAccessor(&SixLowPanTrickleForwarding::m_maxForwardingDelay),
                          MakeTimeChecker(MilliSeconds(1)));
    return tid;
}

SixLowPanTrickleForwarding::SixLowPanTrickleForwarding()
    : m_timerRunning(false)
{
    NS_LOG_FUNCTION(this);
    m_timer.SetFunction(&SixLowPanTrickleForwarding::Transmit, this);
}

SixLowPanTrickleForwarding::~SixLowPanTrickleForwarding()
{
    NS_LOG_FUNCTION(this);
}

void
SixLowPanTrickleForwarding::DoDispose()
{
    NS_LOG_FUNCTION(this);
    StopTimer();
    m_pending.clear();
    SixLowPanMeshUnderRouting::DoDispose();
}

void
SixLowPanTrickleForwarding::OnPacketForward(Ptr<Packet> packet,
                                            const Address& originator,
                                            uint8_t seqNo,
                                            uint8_t hopsLeft,
                                            ForwardCallback forwardCb)
{
    NS_LOG_FUNCTION(this << packet << originator << +seqNo << +hopsLeft);

    // A new packet joins the pending set; it does not restart a running
    // timer, so an earlier pending packet is never starved.
    m_pending.push_back({packet, forwardCb});

    if (!m_timerRunning)
    {
        StartTimer();
    }
}

void
SixLowPanTrickleForwarding::OnDuplicateReceived(const Address& originator [[maybe_unused]],
                                                uint8_t seqNo [[maybe_unused]])
{
    NS_LOG_FUNCTION(this << originator << +seqNo);

    // Consistent event (Rule 3): a neighbour is spreading information we
    // have also seen. Count it; sustained consistency grows the interval.
    if (m_timerRunning)
    {
        m_timer.ConsistentEvent();
    }
}

void
SixLowPanTrickleForwarding::StartTimer()
{
    NS_LOG_FUNCTION(this);

    m_timer.SetParameters(m_minInterval, m_doublings, m_redundancy);
    m_timer.Enable();
    m_timerRunning = true;
    m_discardEvent = Simulator::Schedule(m_maxForwardingDelay,
                                         &SixLowPanTrickleForwarding::DiscardPending,
                                         this);
}

void
SixLowPanTrickleForwarding::StopTimer()
{
    NS_LOG_FUNCTION(this);

    m_timer.Stop();
    m_timerRunning = false;
    m_discardEvent.Cancel();
}

void
SixLowPanTrickleForwarding::Transmit()
{
    NS_LOG_FUNCTION(this);

    // Reached only when c < k (the TrickleTimer enforces this). Forward the
    // whole pending set, then reset by stopping until the next arrival.
    NS_LOG_LOGIC("Forwarding " << m_pending.size() << " pending packet(s)");
    for (auto& entry : m_pending)
    {
        entry.forwardCb(entry.packet);
    }
    m_pending.clear();
    StopTimer();
}

void
SixLowPanTrickleForwarding::DiscardPending()
{
    NS_LOG_FUNCTION(this);

    // Suppression kept us silent for the whole MaxForwardingDelay window;
    // give up on the pending packets and reset.
    NS_LOG_LOGIC("Discarding " << m_pending.size() << " suppressed packet(s)");
    m_pending.clear();
    StopTimer();
}

int64_t
SixLowPanTrickleForwarding::AssignStreams(int64_t stream)
{
    NS_LOG_FUNCTION(this << stream);
    return m_timer.AssignStreams(stream);
}

} // namespace ns3
