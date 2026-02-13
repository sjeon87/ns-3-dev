/*
 * Copyright (c) 2019-2026 NITK Surathkal
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 *
 *
 * Author: Shikha Bakshi <shikhabakshi912@gmail.com>
 *         Mohit P. Tahiliani <tahiliani@nitk.edu.in>
 *         Keerthana Polkampally <keerthana.keetu.p@gmail.com>
 *         Archana Priyadarshani Sahoo <archana98priya@gmail.com>
 *         Durvesh Shyam  Bhalekar <durvesh.5.db@gmail.com>
 *         Mohnish Hemanth Kumar <mohnishhemanthkumar@gmail.com>
 *         Nikhil Kottoli <nikhilkottoli2005@gmail.com>
 *         Manish Agarwal <manishagarwal428728@gmail.com>
 *         Patel Pal Bharat <ppal61679@gmail.com>
 */

#include "tcp-tlp.h"

#include "ns3/log.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("TcpTlp");
NS_OBJECT_ENSURE_REGISTERED(TcpTlp);

TypeId
TcpTlp::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::TcpTlp").SetParent<Object>().AddConstructor<TcpTlp>().SetGroupName("Internet");
    return tid;
}

TcpTlp::TcpTlp()
    : Object(),
      m_pto(MilliSeconds(2))
{
    NS_LOG_FUNCTION(this);
}

TcpTlp::TcpTlp(const TcpTlp& other)
    : Object(other),
      m_pto(other.m_pto)

{
    NS_LOG_FUNCTION(this);
}

TcpTlp::~TcpTlp()
{
    NS_LOG_FUNCTION(this);
}

// Calculate the value of PTO
Time
TcpTlp::CalculatePto(Time srtt, uint32_t inflight, double rto)
{
    NS_LOG_FUNCTION(this);

    Time curr_pto;

    if (srtt > Seconds(0))
    {
        curr_pto = 2 * srtt;

        if (inflight == 1)
        {
            curr_pto += m_maxAckDelay;
        }
    }
    else
    {
        curr_pto = Seconds(1);
    }

    curr_pto = Min(curr_pto, Time::FromDouble(rto, Time::S));
    return curr_pto;
}

void
TcpTlp::Reset(uint32_t currentSamples)
{
    NS_LOG_FUNCTION(this);
    m_tlpEndSeq = SequenceNumber32(0);
    m_tlpIsRetrans = false;
    m_lastRttSamples = currentSamples;
}

bool
TcpTlp::CanSendProbe(uint32_t currRttSamples) const
{
    NS_LOG_FUNCTION(this);
    if (m_tlpEndSeq != SequenceNumber32(0))
    {
        return false;
    }

    if (currRttSamples <= m_lastRttSamples)
    {
        return false;
    }

    return true;
}

void
TcpTlp::OnProbeSent(SequenceNumber32 endSeq, bool isRetrans, uint32_t rttSamples)
{
    NS_LOG_FUNCTION(this);
    m_tlpEndSeq = endSeq;
    m_tlpIsRetrans = isRetrans;
    m_lastRttSamples = rttSamples;
}

bool
TcpTlp::IsProbeOutstanding() const
{
    NS_LOG_FUNCTION(this);
    return m_tlpEndSeq != SequenceNumber32(0);
}

bool
TcpTlp::OnAckReceived(SequenceNumber32 ack, bool hasDsack, bool isDupAckNoSack)
{
    NS_LOG_FUNCTION(this);
    if (m_tlpEndSeq == SequenceNumber32(0) || ack < m_tlpEndSeq)
    {
        return false;
    }

    if (!m_tlpIsRetrans)
    {
        m_tlpEndSeq = SequenceNumber32(0);
        return false;
    }

    if (hasDsack)
    {
        m_tlpEndSeq = SequenceNumber32(0);
        return false;
    }

    if (ack > m_tlpEndSeq)
    {
        m_tlpEndSeq = SequenceNumber32(0);
        return true;
    }

    if (isDupAckNoSack)
    {
        m_tlpEndSeq = SequenceNumber32(0);
        return false;
    }
    return false;
}

} // namespace ns3
