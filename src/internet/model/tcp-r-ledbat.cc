/*
 * Copyright (c) 2026 NITK Surathkal
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author:  Jayesh Akot <akotjayesh@gmail.com>
 *          S B L Prateek <sblprateek@gmail.com>
 *          A R Sharan Kumar <arsharankumar99@gmail.com>
 *          Mohit P. Tahiliani <tahiliani@nitk.edu.in>
 */

#include "tcp-r-ledbat.h"

#include "tcp-header.h"
#include "tcp-option-ts.h"
#include "tcp-socket-state.h"

#include "ns3/log.h"
#include "ns3/simulator.h"

#include <algorithm>

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("TcpRLedbat");
NS_OBJECT_ENSURE_REGISTERED(TcpRLedbat);

TypeId
TcpRLedbat::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::TcpRLedbat")
            .SetParent<TcpSocketBase>()
            .AddConstructor<TcpRLedbat>()
            .SetGroupName("Internet")
            .AddAttribute("TargetDelay",
                          "Targeted Queue Delay",
                          TimeValue(MilliSeconds(100)),
                          MakeTimeAccessor(&TcpRLedbat::m_target),
                          MakeTimeChecker())
            .AddAttribute("BaseHistoryLen",
                          "Number of Base delay samples",
                          UintegerValue(10),
                          MakeUintegerAccessor(&TcpRLedbat::m_baseHistoLen),
                          MakeUintegerChecker<uint32_t>(1))
            .AddAttribute("NoiseFilterLen",
                          "Number of Current delay samples",
                          UintegerValue(4),
                          MakeUintegerAccessor(&TcpRLedbat::m_noiseFilterLen),
                          MakeUintegerChecker<uint32_t>(1))
            .AddAttribute("Gain",
                          "Offset Gain",
                          DoubleValue(1.0),
                          MakeDoubleAccessor(&TcpRLedbat::m_gain),
                          MakeDoubleChecker<double>(1e-6))
            .AddAttribute("SSParam",
                          "Possibility of Slow Start",
                          EnumValue(DO_SLOWSTART),
                          MakeEnumAccessor<SlowStartType>(&TcpRLedbat::SetDoSs),
                          MakeEnumChecker(DO_SLOWSTART, "yes", DO_NOT_SLOWSTART, "no"))
            .AddAttribute("MinRlwnd",
                          "Minimum RLWND for Ledbat",
                          UintegerValue(2),
                          MakeUintegerAccessor(&TcpRLedbat::m_minRlwnd),
                          MakeUintegerChecker<uint32_t>(1))
            .AddAttribute("InitRlwnd",
                          "Initial RLWND in segments",
                          UintegerValue(1),
                          MakeUintegerAccessor(&TcpRLedbat::m_initRlwnd),
                          MakeUintegerChecker<uint32_t>(1));
    return tid;
}

TcpRLedbat::TcpRLedbat()
    : TcpSocketBase(),
      m_target(MilliSeconds(100)),
      m_gain(1.0),
      m_doSs(DO_SLOWSTART),
      m_baseHistoLen(10),
      m_noiseFilterLen(4),
      m_lastRollover(Seconds(0)),
      m_RLWND(0),
      m_minRlwnd(2),
      m_initRlwnd(1),
      m_rcvHgh(0),
      m_tsvHgh(0),
      m_pendingReduction(0),
      m_ssthresh(UINT32_MAX)
{
    NS_LOG_FUNCTION(this);
    InitCircBuf(m_baseHistory);
    InitCircBuf(m_noiseFilter);
}

TcpRLedbat::TcpRLedbat(const TcpRLedbat& sock)
    : TcpSocketBase(sock),
      m_target(sock.m_target),
      m_gain(sock.m_gain),
      m_doSs(sock.m_doSs),
      m_baseHistoLen(sock.m_baseHistoLen),
      m_noiseFilterLen(sock.m_noiseFilterLen),
      m_lastRollover(sock.m_lastRollover),
      m_baseHistory(sock.m_baseHistory),
      m_noiseFilter(sock.m_noiseFilter),
      m_RLWND(sock.m_RLWND),
      m_minRlwnd(sock.m_minRlwnd),
      m_initRlwnd(sock.m_initRlwnd),
      m_rcvHgh(sock.m_rcvHgh),
      m_tsvHgh(sock.m_tsvHgh),
      m_pendingReduction(sock.m_pendingReduction),
      m_ssthresh(sock.m_ssthresh)
{
    NS_LOG_FUNCTION(this);
}

TcpRLedbat::~TcpRLedbat()
{
    NS_LOG_FUNCTION(this);
}

void
TcpRLedbat::SetDoSs(SlowStartType doSS)
{
    NS_LOG_FUNCTION(this << doSS);
    m_doSs = doSS;
    NS_LOG_DEBUG("Slow start " << (m_doSs == DO_SLOWSTART ? "enabled" : "disabled"));
}

void
TcpRLedbat::InitCircBuf(OwdCircBuf& buffer)
{
    NS_LOG_FUNCTION(this);
    buffer.buffer.clear();
    buffer.min = 0;
}

uint32_t
TcpRLedbat::MinCircBuf(OwdCircBuf& b)
{
    NS_LOG_FUNCTION(b.buffer.size());
    if (b.buffer.empty())
    {
        return ~0U;
    }
    return b.buffer[b.min];
}

void
TcpRLedbat::AddDelay(OwdCircBuf& cb, uint32_t owd, uint32_t maxlen)
{
    NS_LOG_FUNCTION(this << owd << maxlen << cb.buffer.size());
    if (cb.buffer.empty())
    {
        cb.buffer.push_back(owd);
        cb.min = 0;
        return;
    }

    cb.buffer.push_back(owd);
    if (cb.buffer[cb.min] > owd)
    {
        cb.min = cb.buffer.size() - 1;
    }

    if (cb.buffer.size() >= maxlen)
    {
        cb.buffer.erase(cb.buffer.begin());
        auto bufferStart = cb.buffer.begin();
        cb.min = std::distance(bufferStart, std::min_element(bufferStart, cb.buffer.end()));
    }
}

void
TcpRLedbat::UpdateBaseDelay(uint32_t owd)
{
    NS_LOG_FUNCTION(this << owd);

    if (m_baseHistory.buffer.empty())
    {
        AddDelay(m_baseHistory, owd, m_baseHistoLen);
        return;
    }

    Time timestamp = Simulator::Now();
    if ((timestamp - m_lastRollover) > Seconds(60))
    {
        m_lastRollover = timestamp;
        AddDelay(m_baseHistory, owd, m_baseHistoLen);
        NS_LOG_DEBUG("Base delay epoch rollover, new sample: " << owd);
    }
    else
    {
        size_t last = m_baseHistory.buffer.size() - 1;
        if (owd < m_baseHistory.buffer[last])
        {
            m_baseHistory.buffer[last] = owd;
            if (owd < m_baseHistory.buffer[m_baseHistory.min])
            {
                m_baseHistory.min = last;
            }
            NS_LOG_DEBUG("Base delay updated: " << owd);
        }
    }
}

bool
TcpRLedbat::DetectRetransmission(SequenceNumber32 segSeq, uint32_t tsvSeq)
{
    NS_LOG_FUNCTION(this << segSeq << tsvSeq);
    return (segSeq < m_rcvHgh && tsvSeq > m_tsvHgh);
}

void
TcpRLedbat::IncreaseWindow(uint32_t amount)
{
    NS_LOG_FUNCTION(this << amount);
    m_RLWND += amount;
    NS_LOG_DEBUG("RLWND increased by " << amount << ", new value: " << m_RLWND);
}

void
TcpRLedbat::DecreaseWindow(uint32_t ackedBytes)
{
    NS_LOG_FUNCTION(this << ackedBytes);
    uint32_t minWindow = m_minRlwnd * m_tcb->m_segmentSize;

    if (m_pendingReduction <= 0)
    {
        return;
    }

    if (ackedBytes >= static_cast<uint32_t>(m_pendingReduction))
    {
        if (m_RLWND > static_cast<uint32_t>(m_pendingReduction))
        {
            m_RLWND -= static_cast<uint32_t>(m_pendingReduction);
        }
        else
        {
            m_RLWND = minWindow;
        }
        m_pendingReduction = 0;
        if (m_RLWND < m_ssthresh)
        {
            m_ssthresh = m_RLWND;
        }
        NS_LOG_DEBUG("Reduction complete, new RLWND: " << m_RLWND << ", ssthresh: " << m_ssthresh);
    }
    else
    {
        if (m_RLWND > ackedBytes && (m_RLWND - ackedBytes) >= minWindow)
        {
            m_RLWND -= ackedBytes;
            m_pendingReduction -= static_cast<int32_t>(ackedBytes);
        }
        else
        {
            m_RLWND = minWindow;
            m_pendingReduction = 0;
        }

        if (m_pendingReduction <= 0 || m_RLWND == minWindow)
        {
            m_pendingReduction = 0;
            if (m_RLWND < m_ssthresh)
            {
                m_ssthresh = m_RLWND;
            }
            NS_LOG_DEBUG("Reduction complete (floor reached), new RLWND: "
                         << m_RLWND << ", ssthresh: " << m_ssthresh);
        }
        else
        {
            NS_LOG_DEBUG("Partial reduction, new RLWND: " << m_RLWND << ", pendingReduction: "
                                                          << m_pendingReduction);
        }
    }
}

void
TcpRLedbat::ReceivedData(Ptr<Packet> p, const TcpHeader& tcpHeader)
{
    NS_LOG_FUNCTION(this << p << tcpHeader);

    if (!tcpHeader.HasOption(TcpOption::TS))
    {
        NS_LOG_DEBUG("No timestamp option, skipping");
        TcpSocketBase::ReceivedData(p, tcpHeader);
        return;
    }

    Ptr<const TcpOptionTS> tsOption =
        DynamicCast<const TcpOptionTS>(tcpHeader.GetOption(TcpOption::TS));
    if (!tsOption || tsOption->GetTimestamp() == 0)
    {
        NS_LOG_DEBUG("Invalid timestamp value, skipping");
        TcpSocketBase::ReceivedData(p, tcpHeader);
        return;
    }

    if (m_RLWND == 0 && m_tcb->m_segmentSize > 0)
    {
        m_RLWND = m_initRlwnd * m_tcb->m_segmentSize;
        NS_LOG_DEBUG("RLWND initialised to " << m_RLWND);
    }

    uint32_t currentPacketSize = p->GetSize();
    uint32_t ackedBytes = currentPacketSize - tcpHeader.GetLength() * 4;

    SequenceNumber32 segSeq = tcpHeader.GetSequenceNumber();
    uint32_t tsValue = tsOption->GetTimestamp();

    if (segSeq >= m_rcvHgh)
    {
        m_rcvHgh = segSeq + ackedBytes;
        m_tsvHgh = tsValue;
    }

    uint32_t currentTime = static_cast<uint32_t>(Simulator::Now().GetMilliSeconds());
    uint32_t owd = currentTime - tsValue;
    AddDelay(m_noiseFilter, owd, m_noiseFilterLen);
    UpdateBaseDelay(owd);
    NS_LOG_DEBUG("OWD sample recorded: " << owd << " ms");

    uint32_t currentDelay = MinCircBuf(m_noiseFilter);
    uint32_t baseDelay = MinCircBuf(m_baseHistory);

    if (currentDelay == ~0U || baseDelay == ~0U)
    {
        NS_LOG_DEBUG("Delay history not ready, skipping window update");
        TcpSocketBase::ReceivedData(p, tcpHeader);
        return;
    }

    uint32_t queuingDelay = (currentDelay > baseDelay) ? (currentDelay - baseDelay) : 0;
    uint32_t minWindow = m_minRlwnd * m_tcb->m_segmentSize;

    NS_LOG_DEBUG("currentDelay: " << currentDelay << " ms, baseDelay: " << baseDelay
                                  << " ms, queuingDelay: " << queuingDelay << " ms");

    bool doSlowStart = (m_doSs == DO_SLOWSTART) && (m_RLWND < m_ssthresh);

    if (DetectRetransmission(segSeq, tsValue) && m_pendingReduction <= 0)
    {
        uint32_t newSsthresh = m_RLWND / 2;
        m_ssthresh = (newSsthresh >= minWindow) ? newSsthresh : minWindow;
        m_pendingReduction = static_cast<int32_t>(m_RLWND - m_ssthresh);
        if (m_pendingReduction < 0)
        {
            m_pendingReduction = 0;
        }
        InitCircBuf(m_noiseFilter);
        NS_LOG_DEBUG("Retransmission detected, RLWND: "
                     << m_RLWND << ", new ssthresh: " << m_ssthresh
                     << ", pendingReduction: " << m_pendingReduction);
    }

    if (m_pendingReduction > 0)
    {
        NS_LOG_DEBUG("Draining window, pendingReduction: " << m_pendingReduction);
        DecreaseWindow(ackedBytes);
    }
    else if (doSlowStart)
    {
        NS_LOG_DEBUG("Slow start, RLWND: " << m_RLWND << ", ssthresh: " << m_ssthresh);
        IncreaseWindow(ackedBytes);
    }
    else
    {
        auto target = static_cast<double>(m_target.GetMilliSeconds());
        double offTarget = (target - static_cast<double>(queuingDelay)) / target;
        double delta = m_gain * offTarget * static_cast<double>(ackedBytes) *
                       static_cast<double>(m_tcb->m_segmentSize) / static_cast<double>(m_RLWND);

        if (queuingDelay < static_cast<uint32_t>(m_target.GetMilliSeconds()))
        {
            NS_LOG_DEBUG("CA increase, offTarget: " << offTarget << ", delta: " << delta);
            IncreaseWindow(static_cast<uint32_t>(delta));
        }
        else
        {
            NS_LOG_DEBUG("CA decrease, offTarget: " << offTarget << ", delta: " << delta);
            m_pendingReduction += static_cast<int32_t>(static_cast<uint32_t>(-delta));
            DecreaseWindow(ackedBytes);
        }

        if (m_RLWND < minWindow)
        {
            m_RLWND = minWindow;
        }
    }

    TcpSocketBase::ReceivedData(p, tcpHeader);
}

uint16_t
TcpRLedbat::AdvertisedWindowSize(bool scale) const
{
    NS_LOG_FUNCTION(this << scale);

    uint32_t fcwnd;
    if (m_tcb->m_rxBuffer->GotFin())
    {
        fcwnd = m_advWnd;
    }
    else
    {
        NS_ASSERT_MSG(m_tcb->m_rxBuffer->MaxRxSequence() - m_tcb->m_rxBuffer->NextRxSequence() >= 0,
                      "Unexpected sequence number values");
        fcwnd = static_cast<uint32_t>(m_tcb->m_rxBuffer->MaxRxSequence() -
                                      m_tcb->m_rxBuffer->NextRxSequence());
    }

    uint32_t w = std::min(m_RLWND, fcwnd);
    uint32_t minWindow = m_minRlwnd * m_tcb->m_segmentSize;

    if (w < minWindow)
    {
        w = minWindow;
    }

    NS_LOG_DEBUG("Advertised window: " << w << " (RLWND: " << m_RLWND << ", fcwnd: " << fcwnd
                                       << ")");

    if (scale)
    {
        w >>= m_rcvWindShift;
    }

    if (w > m_maxWinSize)
    {
        w = m_maxWinSize;
    }

    return static_cast<uint16_t>(w);
}

Ptr<TcpSocketBase>
TcpRLedbat::Fork()
{
    return CopyObject<TcpRLedbat>(this);
}

} // namespace ns3
