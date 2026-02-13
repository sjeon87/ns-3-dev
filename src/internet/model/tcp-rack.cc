/*
 * Copyright (c) 2018-2026 NITK Surathkal
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Authors: Shikha Bakshi <shikhabakshi912@gmail.com>
 *          Mohit P. Tahiliani <tahiliani@nitk.edu.in>
 *          Mohnish Hemanth Kumar <mohnishhemanthkumar@gmail.com>
 *          Nikhil Kottoli <nikhilkottoli2005@gmail.com>
 *          Manish Agarwal <manishagarwal428728@gmail.com>
 *          Patel Pal Bharat <ppal61679@gmail.com>
 */

#include "tcp-rack.h"

#include "ns3/log.h"

#include <algorithm>

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("TcpRack");
NS_OBJECT_ENSURE_REGISTERED(TcpRack);

TypeId
TcpRack::GetTypeId()
{
    static TypeId tid = TypeId("ns3::TcpRack")
                            .SetParent<Object>()
                            .AddConstructor<TcpRack>()
                            .SetGroupName("Internet")
                            .AddAttribute("RackMinRttWindow",
                                          "Window over which RACK.min_RTT is filtered.",
                                          TimeValue(Seconds(300)),
                                          MakeTimeAccessor(&TcpRack::m_minRttWindow),
                                          MakeTimeChecker());
    return tid;
}

TcpRack::TcpRack()
    : Object(),
      m_rackXmitTs(0),
      m_rackEndSeq(0),
      m_rackRtt(0),
      m_reoWnd(0),
      m_minRtt(0),
      m_dsackRound(0),
      m_reoWndMult(1),
      m_reoWndPersist(16)
{
    NS_LOG_FUNCTION(this);
}

TcpRack::TcpRack(const TcpRack& other)
    : Object(other),
      m_rackXmitTs(other.m_rackXmitTs),
      m_rackEndSeq(other.m_rackEndSeq),
      m_rackRtt(other.m_rackRtt),
      m_reoWnd(other.m_reoWnd),
      m_minRtt(other.m_minRtt),
      m_minRttWindow(other.m_minRttWindow),
      m_rttSamples(other.m_rttSamples),
      m_dsackRound(other.m_dsackRound),
      m_reoWndMult(other.m_reoWndMult),
      m_reoWndPersist(other.m_reoWndPersist)
{
    NS_LOG_FUNCTION(this);
}

TcpRack::~TcpRack()
{
    NS_LOG_FUNCTION(this);
}

bool
TcpRack::SentAfter(Time t1, Time t2, uint32_t seq1, uint32_t seq2)
{
    NS_LOG_FUNCTION(this);
    return (t1 > t2 || (t1 == t2 && seq1 > seq2));
}

void
TcpRack::UpdateStats(uint32_t tser,
                     bool retrans,
                     Time xmitTs,
                     SequenceNumber32 endSeq,
                     SequenceNumber32 sndNxt,
                     Time rtt)
{
    NS_LOG_FUNCTION(this);

    // Check if the ACK was for a retransmitted packet. Also if it was a spurious retransmission
    if (retrans)
    {
        if (tser != 0 && tser < xmitTs.GetInteger())
        {
            return;
        }

        if (rtt < m_minRtt)
        {
            return;
        }
    }

    if (rtt > Seconds(0))
    {
        m_rackRtt = rtt;

        const Time now = Simulator::Now();
        m_rttSamples.emplace_back(now, m_rackRtt);
        const bool removedMin = PruneRttWindow(now);

        m_minRtt = m_minRtt.IsZero() ? m_rackRtt : std::min(m_minRtt, m_rackRtt);
        if (removedMin)
        {
            RecomputeMinRtt();
        }
    }

    if (SentAfter(xmitTs, m_rackXmitTs, endSeq.GetValue(), m_rackEndSeq.GetValue()))
    {
        m_rackXmitTs = xmitTs;
        m_rackEndSeq = endSeq;
    }
}

void
TcpRack::UpdateReoWnd(bool reorderSeen,
                      bool dsackSeen,
                      SequenceNumber32 sndNxt,
                      SequenceNumber32 sndUna,
                      Ptr<TcpSocketState> tcb,
                      uint32_t sacked,
                      uint32_t dupAckThresh,
                      bool exiting)
{
    NS_LOG_FUNCTION(this);

    if (m_dsackRound != SequenceNumber32(0) && sndUna >= m_dsackRound)
    {
        m_dsackRound = 0;
    }

    if (m_dsackRound == SequenceNumber32(0) && dsackSeen)
    {
        m_reoWndMult++;
        m_dsackRound = sndNxt;
        m_reoWndPersist = 16; // Keep window for 16 recoveries
    }
    else if (exiting) // Exiting Loss Recovery
    {
        m_reoWndPersist--;
        if (m_reoWndPersist <= 0)
        {
            m_reoWndMult = 1;
        }
    }

    if (!reorderSeen)
    {
        if ((tcb->m_congState >= TcpSocketState::CA_RECOVERY) || (sacked >= dupAckThresh))
        {
            m_reoWnd = 0;
            return;
        }
    }

    double srtt = tcb->m_srtt.Get().GetSeconds();
    m_reoWnd = (m_minRtt / 4).GetSeconds() * m_reoWndMult;
    m_reoWnd = std::min(m_reoWnd, srtt);
}

bool
TcpRack::PruneRttWindow(Time now)
{
    bool removedMin = false;
    while (!m_rttSamples.empty() && (now - m_rttSamples.front().first) > m_minRttWindow)
    {
        if (m_rttSamples.front().second == m_minRtt)
        {
            removedMin = true;
        }
        m_rttSamples.pop_front();
    }

    return removedMin;
}

void
TcpRack::RecomputeMinRtt()
{
    m_minRtt = Seconds(0);
    for (const auto& sample : m_rttSamples)
    {
        m_minRtt = m_minRtt.IsZero() ? sample.second : std::min(m_minRtt, sample.second);
    }
}

} // namespace ns3
