/*
 * Copyright (c) 2026 Sergio Andreozzi
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Sergio Andreozzi <digitalities@gmail.com>
 */

#include "tcp-retransmit-tag.h"

namespace ns3
{

NS_OBJECT_ENSURE_REGISTERED(TcpRetransmitTag);

TypeId
TcpRetransmitTag::GetTypeId()
{
    static TypeId tid = TypeId("ns3::TcpRetransmitTag")
                            .SetParent<Tag>()
                            .SetGroupName("Internet")
                            .AddConstructor<TcpRetransmitTag>();
    return tid;
}

TypeId
TcpRetransmitTag::GetInstanceTypeId() const
{
    return GetTypeId();
}

TcpRetransmitTag::TcpRetransmitTag()
    : m_retxCount(1)
{
}

TcpRetransmitTag::TcpRetransmitTag(uint8_t retxCount)
    : m_retxCount(retxCount)
{
}

uint32_t
TcpRetransmitTag::GetSerializedSize() const
{
    return 1;
}

void
TcpRetransmitTag::Serialize(TagBuffer i) const
{
    i.WriteU8(m_retxCount);
}

void
TcpRetransmitTag::Deserialize(TagBuffer i)
{
    m_retxCount = i.ReadU8();
}

void
TcpRetransmitTag::Print(std::ostream& os) const
{
    os << "retx=" << static_cast<uint32_t>(m_retxCount);
}

} // namespace ns3
