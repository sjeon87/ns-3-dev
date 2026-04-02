/*
 * Copyright (c) 2014 Universitat Autònoma de Barcelona
 *                2026 Michigan State University
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 *
 *
 * Author: Rubén Martínez <rmartinez@deic.uab.cat>
          Ishaan Lagwankar <lagwanka@msu.edu>
 */

#include "ltp-queue-set.h"

#include "ns3/log.h"

NS_LOG_COMPONENT_DEFINE("LtpQueueSet");

namespace ns3
{
namespace ltp
{

NS_OBJECT_ENSURE_REGISTERED(LtpQueueSet);

TypeId
LtpQueueSet::GetTypeId(void)
{
    static TypeId tid = TypeId("ns3::ltp::LtpQueueSet").SetParent<Object>();
    return tid;
}

LtpQueueSet::LtpQueueSet()
    : Object(),
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
LtpQueueSet::Dequeue(void)
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
        return nullptr;
    }
    return p;
}

bool
LtpQueueSet::Enqueue(Ptr<Packet> p)
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
    }
    return true;
}

Ptr<const Packet>
LtpQueueSet::Peek(void) const
{
    NS_LOG_FUNCTION(this);
    if (!m_internalOps.empty())
    {
        return m_internalOps.front();
    }
    else if (!m_appData.empty())
    {
        return m_appData.front();
    }
    NS_LOG_LOGIC("QueueSet is empty");
    return nullptr;
}

Ptr<Packet>
LtpQueueSet::Remove(void)
{
    NS_LOG_FUNCTION(this);
    return Dequeue();
}

uint32_t
LtpQueueSet::GetNPackets(void) const
{
    return m_internalOps.size() + m_appData.size();
}

} // namespace ltp
} // namespace ns3
