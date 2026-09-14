/*
 * Copyright (c) 2026 Michigan State University
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Ishaan Lagwankar <lagwanka@msu.edu>
 */

#include "ntp-header.h"

namespace ns3
{

NtpHeader::NtpHeader()
{
}

TypeId
NtpHeader::GetTypeId()
{
    static TypeId tid = TypeId("ns3::NtpHeader")
                            .SetParent<Header>()
                            .SetGroupName("Applications")
                            .AddConstructor<NtpHeader>();
    return tid;
}

TypeId
NtpHeader::GetInstanceTypeId() const
{
    return GetTypeId();
}

void
NtpHeader::Print(std::ostream& os) const
{
    os << "(t1=" << GetT1().As(Time::MS) << " t2=" << GetT2().As(Time::MS)
       << " t3=" << GetT3().As(Time::MS) << " reply=" << +m_isReply << ")";
}

uint32_t
NtpHeader::GetSerializedSize() const
{
    return 8 + 8 + 8 + 1;
}

void
NtpHeader::Serialize(Buffer::Iterator start) const
{
    Buffer::Iterator i = start;
    i.WriteHtonU64(m_t1);
    i.WriteHtonU64(m_t2);
    i.WriteHtonU64(m_t3);
    i.WriteU8(m_isReply);
}

uint32_t
NtpHeader::Deserialize(Buffer::Iterator start)
{
    Buffer::Iterator i = start;
    m_t1 = i.ReadNtohU64();
    m_t2 = i.ReadNtohU64();
    m_t3 = i.ReadNtohU64();
    m_isReply = i.ReadU8();
    return GetSerializedSize();
}

void
NtpHeader::SetReply(bool isReply)
{
    m_isReply = isReply ? 1 : 0;
}

bool
NtpHeader::IsReply() const
{
    return m_isReply != 0;
}

void
NtpHeader::SetT1(Time t1)
{
    m_t1 = static_cast<uint64_t>(t1.GetTimeStep());
}

Time
NtpHeader::GetT1() const
{
    return TimeStep(static_cast<int64_t>(m_t1));
}

void
NtpHeader::SetT2(Time t2)
{
    m_t2 = static_cast<uint64_t>(t2.GetTimeStep());
}

Time
NtpHeader::GetT2() const
{
    return TimeStep(static_cast<int64_t>(m_t2));
}

void
NtpHeader::SetT3(Time t3)
{
    m_t3 = static_cast<uint64_t>(t3.GetTimeStep());
}

Time
NtpHeader::GetT3() const
{
    return TimeStep(static_cast<int64_t>(m_t3));
}

} // namespace ns3
