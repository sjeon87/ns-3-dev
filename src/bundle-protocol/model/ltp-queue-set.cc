/*
 * Copyright (c) 2014 Universitat Autònoma de Barcelona
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 *
 *
 * Author: Rubén Martínez <rmartinez@deic.uab.cat>
 */

#include "ltp-queue-set.h"

#include "ns3/log.h"

NS_LOG_COMPONENT_DEFINE("LtpQueueSet");

namespace ns3
{
namespace ltp
{

TypeId
LtpQueueSet::GetTypeId(void)
{
    static TypeId tid =
        TypeId("ns3::LtpQueueSet").SetParent<DropTailQueue<Packet>>().AddConstructor<LtpQueueSet>();

    return tid;
}

LtpQueueSet::LtpQueueSet()
    : DropTailQueue<Packet>(),
      m_internalOps(),
      m_appData()
{
    NS_LOG_FUNCTION(this);
}

LtpQueueSet::~LtpQueueSet()
{
    NS_LOG_FUNCTION(this);
}

Ptr<Packet>
LtpQueueSet::DoDequeue(void)
{
    NS_LOG_FUNCTION(this);
    Ptr<Packet> p;

    if (!m_internalOps.empty())
    {
        p = m_internalOps.front();
        m_internalOps.pop();
    }
    else if (!m_appData.empty())
    {
        p = m_appData.front();
        m_appData.pop();
    }
    else
    {
        NS_LOG_LOGIC("QueueSet is empty");
        return 0;
    }
    return p;
}

bool
LtpQueueSet::DoEnqueue(Ptr<Packet> p)
{
    NS_LOG_FUNCTION(this);
    LtpHeader header;
    p->PeekHeader(header);
    SegmentType type = header.GetSegmentType();

    switch (type)
    {
    case LTPTYPE_RD:
    case LTPTYPE_RD_CP:
    case LTPTYPE_RD_CP_EORP:
    case LTPTYPE_RD_CP_EORP_EOB:
    case LTPTYPE_GD:
    case LTPTYPE_GD_EOB:
        m_appData.push(p);
        break;
    case LTPTYPE_RS:
    case LTPTYPE_RAS:
    case LTPTYPE_CS:
    case LTPTYPE_CAS:
    case LTPTYPE_CR:
    case LTPTYPE_CAR:
        m_internalOps.push(p);
        break;
    default:
        NS_LOG_LOGIC("Unexpected Segment type");
        return false;
        break;
    }

    return true;
}

Ptr<const Packet>
LtpQueueSet::DoPeek(void) const
{
    NS_LOG_FUNCTION(this);
    Ptr<Packet> p;

    if (!m_internalOps.empty())
    {
        p = m_internalOps.front();
    }
    else if (!m_appData.empty())
    {
        p = m_appData.front();
    }
    else
    {
        NS_LOG_LOGIC("QueueSet is empty");
        return 0;
    }
    return p;

    NS_LOG_LOGIC("Number packets " << m_internalOps.size() + m_appData.size());

    return p;
}

} // namespace ltp
} // namespace ns3
