/*
 * Copyright (c) 2026 NITK Surathkal
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Authors: Ayush Nigam <ash12521198@gmail.com>
 *          S B L Prateek <sblprateek@gmail.com>
 *          A R Sharan Kumar <arsharankumar99@gmail.com>
 *          Yashwanth R <ryashwanth990@gmail.com>
 *          Mohit P. Tahiliani <tahiliani@nitk.edu.in>
 */

#include "tcp-ledbat-pp.h"

#include "tcp-socket-state.h"

#include "ns3/log.h"
#include "ns3/simulator.h"

#include <cmath>

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("TcpLedbatPp");
NS_OBJECT_ENSURE_REGISTERED(TcpLedbatPp);

TypeId
TcpLedbatPp::GetTypeId()
{
    static TypeId tid = TypeId("ns3::TcpLedbatPp")
                            .SetParent<TcpCongestionOps>()
                            .AddConstructor<TcpLedbatPp>()
                            .SetGroupName("Internet")
                            .AddAttribute("TargetDelay",
                                          "Target queue delay",
                                          TimeValue(MilliSeconds(60)),
                                          MakeTimeAccessor(&TcpLedbatPp::m_target),
                                          MakeTimeChecker(Time(1)))
                            .AddAttribute("BaseHistoryLen",
                                          "Number of base delay samples",
                                          UintegerValue(10),
                                          MakeUintegerAccessor(&TcpLedbatPp::m_baseHistoLen),
                                          MakeUintegerChecker<uint32_t>(1))
                            .AddAttribute("NoiseFilterLen",
                                          "Number of current delay samples",
                                          UintegerValue(4),
                                          MakeUintegerAccessor(&TcpLedbatPp::m_noiseFilterLen),
                                          MakeUintegerChecker<uint32_t>(1))
                            .AddAttribute("MinCwnd",
                                          "Minimum cWnd for LEDBAT++, in segments",
                                          UintegerValue(2),
                                          MakeUintegerAccessor(&TcpLedbatPp::m_minCwnd),
                                          MakeUintegerChecker<uint32_t>(1))
                            .AddAttribute("Constant",
                                          "LEDBAT++ constant scaling the multiplicative decrease",
                                          DoubleValue(1.0),
                                          MakeDoubleAccessor(&TcpLedbatPp::m_constant),
                                          MakeDoubleChecker<double>())
                            .AddTraceSource("QueueDelay",
                                            "Estimated queuing delay (current delay - base delay)",
                                            MakeTraceSourceAccessor(&TcpLedbatPp::m_queueDelay),
                                            "ns3::TracedValueCallback::Time")
                            .AddTraceSource("CurrentDelay",
                                            "Filtered current RTT (minimum of the noise filter)",
                                            MakeTraceSourceAccessor(&TcpLedbatPp::m_currentDelay),
                                            "ns3::TracedValueCallback::Time")
                            .AddTraceSource("BaseDelay",
                                            "Estimated base delay (minimum RTT over the history)",
                                            MakeTraceSourceAccessor(&TcpLedbatPp::m_baseDelay),
                                            "ns3::TracedValueCallback::Time")
                            .AddTraceSource("Gain",
                                            "Dynamic GAIN applied to the congestion window",
                                            MakeTraceSourceAccessor(&TcpLedbatPp::m_gain),
                                            "ns3::TracedValueCallback::Double")
                            .AddTraceSource("Phase",
                                            "LEDBAT++ phase (initial slow start, congestion "
                                            "avoidance or slowdown)",
                                            MakeTraceSourceAccessor(&TcpLedbatPp::m_state),
                                            "ns3::TcpLedbatPp::PhaseTracedCallback");

    return tid;
}

TcpLedbatPp::TcpLedbatPp()
    : TcpCongestionOps()
{
    NS_LOG_FUNCTION(this);
    InitCircBuf(m_baseHistory);
    InitCircBuf(m_noiseFilter);
    m_lastRollover = Seconds(0);
    m_currentDelay = Seconds(0);
    m_baseDelay = Seconds(0);
    m_queueDelay = Seconds(0);
}

void
TcpLedbatPp::InitCircBuf(RttCircBuf& buffer)
{
    NS_LOG_FUNCTION(this);
    buffer.buffer.clear();
    buffer.min = 0;
}

TcpLedbatPp::TcpLedbatPp(const TcpLedbatPp& sock)
    : TcpCongestionOps(sock)
{
    NS_LOG_FUNCTION(this);
    m_target = sock.m_target;
    m_gain = sock.m_gain;
    m_baseHistoLen = sock.m_baseHistoLen;
    m_noiseFilterLen = sock.m_noiseFilterLen;
    m_baseHistory = sock.m_baseHistory;
    m_noiseFilter = sock.m_noiseFilter;
    m_lastRollover = sock.m_lastRollover;
    m_slowdownEntry = sock.m_slowdownEntry;
    m_freeze = sock.m_freeze;
    m_state = sock.m_state;
    m_minCwnd = sock.m_minCwnd;
    m_duration = sock.m_duration;
    m_nextSdStart = sock.m_nextSdStart;
    m_currentDelay = sock.m_currentDelay;
    m_baseDelay = sock.m_baseDelay;
    m_queueDelay = sock.m_queueDelay;
    m_constant = sock.m_constant;
}

TcpLedbatPp::~TcpLedbatPp()
{
    NS_LOG_FUNCTION(this);
}

Ptr<TcpCongestionOps>
TcpLedbatPp::Fork()
{
    return CopyObject<TcpLedbatPp>(this);
}

std::string
TcpLedbatPp::GetName() const
{
    return "TcpLedbatPp";
}

uint32_t
TcpLedbatPp::GetSsThresh(Ptr<const TcpSocketState> tcb, uint32_t bytesInFlight)
{
    NS_LOG_FUNCTION(this << tcb << bytesInFlight);
    return std::max(m_minCwnd * tcb->m_segmentSize, tcb->m_cWnd.Get() / 2);
}

Time
TcpLedbatPp::MinCircBuf(RttCircBuf& b)
{
    NS_LOG_FUNCTION_NOARGS();
    if (b.buffer.empty())
    {
        return Time::Max();
    }
    else
    {
        return b.buffer[b.min];
    }
}

Time
TcpLedbatPp::CurrentDelay()
{
    NS_LOG_FUNCTION(this);
    return MinCircBuf(m_noiseFilter);
}

Time
TcpLedbatPp::BaseDelay()
{
    NS_LOG_FUNCTION(this);
    return MinCircBuf(m_baseHistory);
}

void
TcpLedbatPp::IncreaseWindow(Ptr<TcpSocketState> tcb, uint32_t segmentsAcked)
{
    NS_LOG_FUNCTION(this << tcb << segmentsAcked);
    if (tcb->m_cWnd.Get() <= tcb->m_segmentSize)
    {
        // A timeout has collapsed the window, so begin again from the initial slow start.
        NS_LOG_INFO("cWnd collapsed to <= 1 MSS; restarting initial slow start");
        m_state = INITIAL_SLOW_START;
    }

    Time currTime = Simulator::Now();
    Time rtt = tcb->m_lastRtt.Get();

    if (m_baseHistory.buffer.empty())
    {
        // Nothing has been sampled yet. Assume the queue is empty and pick the
        // smallest gain the formula can produce, which is the cautious choice.
        m_queueDelay = Seconds(0);
        m_gain = 1.0 / 16.0;
    }
    else
    {
        m_currentDelay = CurrentDelay();
        m_baseDelay = BaseDelay();

        if (m_currentDelay.Get() > m_baseDelay.Get())
        {
            m_queueDelay = m_currentDelay.Get() - m_baseDelay.Get();
        }
        else
        {
            m_queueDelay = Seconds(0);
        }

        if (m_baseDelay.Get().IsStrictlyPositive())
        {
            double delayRatio = std::ceil((2 * m_target / m_baseDelay.Get()).GetDouble());
            m_gain = 1.0 / std::min(16.0, delayRatio);
        }
        else
        {
            m_gain = 1.0 / 16.0;
        }
    }

    NS_LOG_DEBUG("currentDelay=" << m_currentDelay.Get().As(Time::MS)
                                 << " baseDelay=" << m_baseDelay.Get().As(Time::MS)
                                 << " queueDelay=" << m_queueDelay.Get().As(Time::MS)
                                 << " gain=" << m_gain.Get());

    if (m_state == INITIAL_SLOW_START)
    {
        if (tcb->m_cWnd < tcb->m_ssThresh && m_queueDelay.Get() <= 0.75 * m_target)
        {
            segmentsAcked = SlowStart(tcb, segmentsAcked);
            return;
        }
        m_state = CONGESTION_AVOIDANCE;
        m_nextSdStart = currTime + 2 * rtt;
        NS_LOG_INFO("Exiting initial slow start -> congestion avoidance; first slowdown at "
                    << m_nextSdStart.As(Time::S));
    }

    if (m_state == CONGESTION_AVOIDANCE && currTime >= m_nextSdStart)
    {
        m_slowdownEntry = currTime;
        m_freeze = m_slowdownEntry + 2 * rtt;
        tcb->m_ssThresh = tcb->m_cWnd;
        m_state = SLOWDOWN;
        NS_LOG_INFO("Entering slowdown: freezing cWnd to 2 MSS until "
                    << m_freeze.As(Time::S) << ", ssThresh=" << tcb->m_ssThresh);
    }

    if (m_state == SLOWDOWN)
    {
        SlowDown(tcb, segmentsAcked, currTime);
    }
    else
    {
        CongestionAvoidance(tcb, segmentsAcked);
    }
}

uint32_t
TcpLedbatPp::SlowStart(Ptr<TcpSocketState> tcb, uint32_t segmentsAcked)
{
    NS_LOG_FUNCTION(this << tcb << segmentsAcked);

    if (segmentsAcked >= 1)
    {
        uint32_t sndCwnd = tcb->m_cWnd;
        tcb->m_cWnd = std::min(
            (sndCwnd + static_cast<uint32_t>(segmentsAcked * tcb->m_segmentSize * m_gain.Get())),
            (uint32_t)tcb->m_ssThresh);
        NS_LOG_INFO("In SlowStart, updated to cwnd " << tcb->m_cWnd << " ssthresh "
                                                     << tcb->m_ssThresh);
        return segmentsAcked -
               ((tcb->m_cWnd - sndCwnd) / static_cast<uint32_t>(tcb->m_segmentSize * m_gain.Get()));
    }
    return 0;
}

void
TcpLedbatPp::CongestionAvoidance(Ptr<TcpSocketState> tcb, uint32_t segmentsAcked)
{
    NS_LOG_FUNCTION(this << tcb << segmentsAcked);
    uint32_t cwnd = (tcb->m_cWnd.Get());

    if (m_queueDelay.Get() < m_target)
    {
        auto increase = static_cast<uint32_t>(
            (m_gain.Get() * tcb->m_segmentSize * tcb->m_segmentSize * segmentsAcked) /
            static_cast<double>(cwnd));
        NS_LOG_DEBUG("CA additive increase (queueDelay < target): +" << increase << " bytes");
        cwnd += increase;
    }
    else
    {
        double excessDelayRatio = (m_queueDelay.Get() / m_target).GetDouble() - 1.0;
        double cwndDelta =
            ((m_gain.Get() * tcb->m_segmentSize) - (m_constant * cwnd * excessDelayRatio)) *
            segmentsAcked * tcb->m_segmentSize / static_cast<double>(cwnd);
        // The draft caps the decrease at W/2 per RTT. Spread over the ACKs that
        // arrive in one RTT, that works out to half a segment per acked segment.
        cwndDelta = std::max(cwndDelta, -0.5 * segmentsAcked * tcb->m_segmentSize);
        NS_LOG_DEBUG("CA multiplicative decrease (queueDelay >= target): cwndDelta=" << cwndDelta
                                                                                     << " bytes");

        cwnd = static_cast<uint32_t>(static_cast<double>(cwnd) + cwndDelta);
    }

    cwnd = std::max(cwnd, m_minCwnd * tcb->m_segmentSize);
    tcb->m_cWnd = cwnd;

    if (tcb->m_cWnd < tcb->m_ssThresh)
    {
        tcb->m_ssThresh = tcb->m_cWnd;
    }
}

void
TcpLedbatPp::SlowDown(Ptr<TcpSocketState> tcb, uint32_t segmentsAcked, Time currTime)
{
    NS_LOG_FUNCTION(this << tcb << segmentsAcked << currTime);
    if (currTime < m_freeze)
    {
        NS_LOG_DEBUG("Slowdown freeze active until " << m_freeze.As(Time::S)
                                                     << "; holding cWnd at 2 MSS");
        tcb->m_cWnd = 2 * tcb->m_segmentSize;
    }
    else
    {
        if (tcb->m_cWnd < tcb->m_ssThresh)
        {
            segmentsAcked = SlowStart(tcb, segmentsAcked);
        }
        if (tcb->m_cWnd >= tcb->m_ssThresh)
        {
            m_duration = currTime - m_slowdownEntry;
            m_nextSdStart = currTime + 9 * m_duration;
            m_state = CONGESTION_AVOIDANCE;
            NS_LOG_INFO("Slowdown complete: duration="
                        << m_duration.As(Time::MS) << ", next slowdown at "
                        << m_nextSdStart.As(Time::S) << " -> congestion avoidance");
        }
    }
}

void
TcpLedbatPp::AddDelay(RttCircBuf& cb, Time rttVal, uint32_t maxlen)
{
    NS_LOG_FUNCTION(this << rttVal << maxlen << cb.buffer.size());
    if (cb.buffer.empty())
    {
        NS_LOG_LOGIC("First Value for queue");
        cb.buffer.push_back(rttVal);
        cb.min = 0;
        return;
    }
    cb.buffer.push_back(rttVal);
    if (cb.buffer[cb.min] > rttVal)
    {
        cb.min = static_cast<uint32_t>(cb.buffer.size() - 1);
    }
    // Evict only once the buffer is over maxlen, so that it settles at exactly
    // maxlen samples.
    if (cb.buffer.size() > maxlen)
    {
        NS_LOG_LOGIC("Queue full" << maxlen);
        cb.buffer.erase(cb.buffer.begin());
        cb.min = 0;
        NS_LOG_LOGIC("Current min element" << cb.buffer[cb.min]);
        for (uint32_t i = 1; i < maxlen; i++)
        {
            if (cb.buffer[i] < cb.buffer[cb.min])
            {
                cb.min = i;
            }
        }
    }
}

void
TcpLedbatPp::UpdateBaseDelay(Time rttVal)
{
    NS_LOG_FUNCTION(this << rttVal);
    if (m_baseHistory.buffer.empty())
    {
        m_lastRollover = Simulator::Now();
        AddDelay(m_baseHistory, rttVal, m_baseHistoLen);
        return;
    }
    Time timestamp = Simulator::Now();

    if (timestamp - m_lastRollover > Seconds(60))
    {
        m_lastRollover = timestamp;
        AddDelay(m_baseHistory, rttVal, m_baseHistoLen);
    }
    else
    {
        auto last = static_cast<uint32_t>(m_baseHistory.buffer.size() - 1);
        if (rttVal < m_baseHistory.buffer[last])
        {
            m_baseHistory.buffer[last] = rttVal;
            if (rttVal < m_baseHistory.buffer[m_baseHistory.min])
            {
                m_baseHistory.min = last;
            }
        }
    }
}

void
TcpLedbatPp::PktsAcked(Ptr<TcpSocketState> tcb, uint32_t segmentsAcked, const Time& rtt)
{
    NS_LOG_FUNCTION(this << tcb << segmentsAcked << rtt);
    if (tcb->m_lastRtt.Get().IsPositive())
    {
        AddDelay(m_noiseFilter, tcb->m_lastRtt.Get(), m_noiseFilterLen);
        UpdateBaseDelay(tcb->m_lastRtt.Get());
    }
}

} // namespace ns3
