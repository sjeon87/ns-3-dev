/*
 * Copyright (c) 2026 SRM Institute of Science and Technology, India
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Usham Roy <ushamroy80@gmail.com>
 */

#include "sixlowpan-trickle-suppression.h"

#include "ns3/log.h"
#include "ns3/simulator.h"
#include "ns3/uinteger.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("SixLowPanTrickleSuppression");
NS_OBJECT_ENSURE_REGISTERED(SixLowPanTrickleSuppression);

TypeId
SixLowPanTrickleSuppression::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::SixLowPanTrickleSuppression")
            .SetParent<SixLowPanMeshUnderRouting>()
            .SetGroupName("SixLowPan")
            .AddConstructor<SixLowPanTrickleSuppression>()
            .AddAttribute("MinInterval",
                          "RFC 6206 Imin: the minimum Trickle interval.",
                          TimeValue(MilliSeconds(10)),
                          MakeTimeAccessor(&SixLowPanTrickleSuppression::m_minInterval),
                          MakeTimeChecker(MilliSeconds(1)))
            .AddAttribute("Doublings",
                          "Number of interval doublings; Imax = MinInterval * 2^Doublings.",
                          UintegerValue(4),
                          MakeUintegerAccessor(&SixLowPanTrickleSuppression::m_doublings),
                          MakeUintegerChecker<uint8_t>(0, 16))
            .AddAttribute("RedundancyConstant",
                          "RFC 6206 k: forward only if fewer than k copies were heard. "
                          "Zero disables suppression.",
                          UintegerValue(1),
                          MakeUintegerAccessor(&SixLowPanTrickleSuppression::m_redundancy),
                          MakeUintegerChecker<uint16_t>())
            .AddAttribute("MaxForwardingDelay",
                          "Discard a pending packet if it is not forwarded within this time "
                          "(suppression won, or too many tries).",
                          TimeValue(MilliSeconds(500)),
                          MakeTimeAccessor(&SixLowPanTrickleSuppression::m_maxForwardingDelay),
                          MakeTimeChecker(MilliSeconds(1)));
    return tid;
}

SixLowPanTrickleSuppression::SixLowPanTrickleSuppression()
    : m_minInterval(MilliSeconds(10)),
      m_doublings(4),
      m_redundancy(1),
      m_maxForwardingDelay(MilliSeconds(500)),
      m_timerRunning(false)
{
    NS_LOG_FUNCTION(this);
    m_timer.SetFunction(&SixLowPanTrickleSuppression::Transmit, this);
}

SixLowPanTrickleSuppression::~SixLowPanTrickleSuppression()
{
    NS_LOG_FUNCTION(this);
}

void
SixLowPanTrickleSuppression::DoDispose()
{
    NS_LOG_FUNCTION(this);
    StopTimer();
    m_pending.clear();
    SixLowPanMeshUnderRouting::DoDispose();
}

void
SixLowPanTrickleSuppression::OnPacketForward(Ptr<Packet> packet,
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
SixLowPanTrickleSuppression::OnDuplicateReceived(const Address& originator, uint8_t seqNo)
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
SixLowPanTrickleSuppression::StartTimer()
{
    NS_LOG_FUNCTION(this);

    m_timer.SetParameters(m_minInterval, m_doublings, m_redundancy);
    m_timer.Enable();
    m_timerRunning = true;
    m_discardEvent = Simulator::Schedule(m_maxForwardingDelay,
                                         &SixLowPanTrickleSuppression::DiscardPending,
                                         this);
}

void
SixLowPanTrickleSuppression::StopTimer()
{
    NS_LOG_FUNCTION(this);

    m_timer.Stop();
    m_timerRunning = false;
    m_discardEvent.Cancel();
}

void
SixLowPanTrickleSuppression::Transmit()
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
SixLowPanTrickleSuppression::DiscardPending()
{
    NS_LOG_FUNCTION(this);

    // Suppression kept us silent for the whole MaxForwardingDelay window;
    // give up on the pending packets and reset.
    NS_LOG_LOGIC("Discarding " << m_pending.size() << " suppressed packet(s)");
    m_pending.clear();
    StopTimer();
}

int64_t
SixLowPanTrickleSuppression::AssignStreams(int64_t stream)
{
    NS_LOG_FUNCTION(this << stream);
    return m_timer.AssignStreams(stream);
}

} // namespace ns3
