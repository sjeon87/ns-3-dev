/*
 * Copyright (c) 2026 Somaiya Vidyavihar University, India
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Manmita Das <das.manmita12@gmail.com>
 */

#include "ethernet-flow-control-header.h"

#include "ns3/log.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("EthernetFlowControlHeader");

namespace ethernet
{

NS_OBJECT_ENSURE_REGISTERED(EthernetFlowControlHeader);

TypeId
EthernetFlowControlHeader::GetTypeId()
{
    static TypeId tid = TypeId("ns3::ethernet::EthernetFlowControlHeader")
                            .SetParent<Header>()
                            .SetGroupName("Network")
                            .AddConstructor<EthernetFlowControlHeader>();

    return tid;
}

TypeId
EthernetFlowControlHeader::GetInstanceTypeId() const
{
    return GetTypeId();
}

EthernetFlowControlHeader::EthernetFlowControlHeader()
{
    NS_LOG_FUNCTION(this);
}

EthernetFlowControlHeader::~EthernetFlowControlHeader()
{
}

void
EthernetFlowControlHeader::SetPauseQuanta(uint16_t pauseQuanta)
{
    m_pauseQuanta = pauseQuanta;
}

uint16_t
EthernetFlowControlHeader::GetPauseQuanta() const
{
    return m_pauseQuanta;
}

void
EthernetFlowControlHeader::Print(std::ostream& os) const
{
    os << "Pause Quanta=" << m_pauseQuanta;
}

uint32_t
EthernetFlowControlHeader::GetSerializedSize() const
{
    // Opcode (2 bytes) + Pause Time (2 bytes) + Reserved (42 bytes)
    return 46;
}

void
EthernetFlowControlHeader::Serialize(Buffer::Iterator start) const
{
    NS_LOG_FUNCTION(this << &start);

    Buffer::Iterator i = start;

    i.WriteHtonU16(0x0001); // IEEE 802.3x PAUSE opcode
    i.WriteHtonU16(m_pauseQuanta);
    i.WriteU8(0, 42);
}

uint32_t
EthernetFlowControlHeader::Deserialize(Buffer::Iterator start)
{
    uint16_t opcode = start.ReadNtohU16();

    if (opcode != 0x0001)
    {
        NS_LOG_WARN("Unsupported Flow Control Opcode: " << opcode);
        return 0;
    }

    m_pauseQuanta = start.ReadNtohU16();

    // Skip reserved bytes
    start.Next(42);

    return GetSerializedSize();
}

} // namespace ethernet
} // namespace ns3
