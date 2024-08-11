/*
 * Copyright (c) 2009 IITP RAS
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Based on
 *      NS-2 AODV model developed by the CMU/MONARCH group and optimized and
 *      tuned by Samir Das and Mahesh Marina, University of Cincinnati;
 *
 *      AODV-UU implementation by Erik Nordström of Uppsala University
 *      https://web.archive.org/web/20100527072022/http://core.it.uu.se/core/index.php/AODV-UU
 *
 * Authors: Elena Buchatskaia <borovkovaes@iitp.ru>
 *          Pavel Boyko <boyko@iitp.ru>
 */
#include "aodvv2-packet.h"

#include "ns3/address-utils.h"
#include "ns3/packet.h"
#include "ns3/packetbb.h"

namespace ns3
{
namespace aodvv2
{

//-----------------------------------------------------------------------------
// RREQ
//-----------------------------------------------------------------------------
RreqHeader::RreqHeader(Ipv4Address origIp,
                       uint16_t origMask,
                       Ipv4Address targIp,
                       uint16_t targMask,
                       uint32_t seqNo,
                       uint8_t hopCount)
    : m_origIp(origIp),
      m_origMask(origMask),
      m_targIp(targIp),
      m_targMask(targMask),
      m_seqNo(seqNo),
      m_hopCount(hopCount)
{
}

NS_OBJECT_ENSURE_REGISTERED(RreqHeader);

TypeId
RreqHeader::GetTypeId()
{
    static TypeId tid = TypeId("ns3::aodvv2::RreqHeader")
                            .SetParent<Header>()
                            .SetGroupName("Aodvv2")
                            .AddConstructor<RreqHeader>();
    return tid;
}

TypeId
RreqHeader::GetInstanceTypeId() const
{
    return GetTypeId();
}

uint32_t
RreqHeader::GetSerializedSize() const
{
    return m_tlvHeader->GetSerializedSize();
}

void
RreqHeader::Serialize(Buffer::Iterator i) const
{
    m_tlvHeader->Serialize(i);
}

void
RreqHeader::CreateTlvHeader() const
{
    m_tlvHeader = Create<PbbPacket>();
    m_tlvHeader->SetSequenceNumber(this->m_seqNo);

    Ptr<PbbMessageIpv4> msg1 = Create<PbbMessageIpv4>();
    msg1->SetType(AODVV2_TYPE_RREQ);
    msg1->SetHopLimit(AODVV2_MAX_HOP_COUNT);

    // ****************************** OrigPrefix Address Block ******************************
    Ptr<PbbAddressBlockIpv4> msg1a1 = Create<PbbAddressBlockIpv4>();
    msg1a1->AddressPushBack(this->m_origIp);
    msg1a1->PrefixPushBack(this->m_origMask);

    // Add ADDRESS_TYPE TLV
    Ptr<PbbAddressTlv> msg1a1tlv1 = Create<PbbAddressTlv>();
    msg1a1tlv1->SetType(AODVV2_ADDRESS_TYPE);
    uint8_t msg1a1tlv1val[] = {AODVV2_ORIGPREFIX};
    msg1a1tlv1->SetValue(msg1a1tlv1val, sizeof(msg1a1tlv1val));
    msg1a1->TlvPushBack(msg1a1tlv1);

    // Add SEQ_NUM TLV
    Ptr<PbbAddressTlv> msg1a1tlv2 = Create<PbbAddressTlv>();
    msg1a1tlv2->SetType(AODVV2_SEQ_NUM);
    uint8_t msg1a1tlv2val[] = {static_cast<uint8_t>(this->m_seqNo + 1)};
    msg1a1tlv2->SetValue(msg1a1tlv2val, sizeof(msg1a1tlv2val));
    msg1a1->TlvPushBack(msg1a1tlv2);

    // Add PATH_METRIC TLV
    Ptr<PbbAddressTlv> msg1a1tlv3 = Create<PbbAddressTlv>();
    msg1a1tlv3->SetType(AODVV2_PATH_METRIC);
    uint8_t msg1a1tlv3val[] = {1};
    msg1a1tlv3->SetValue(msg1a1tlv3val, sizeof(msg1a1tlv3val));
    msg1a1->TlvPushBack(msg1a1tlv3);

    msg1->AddressBlockPushBack(msg1a1);

    // ****************************** TargPrefix Address Block ******************************
    Ptr<PbbAddressBlockIpv4> msg1a2 = Create<PbbAddressBlockIpv4>();
    msg1a2->AddressPushBack(this->m_targIp);
    msg1a2->PrefixPushBack(this->m_targMask);

    // Add ADDRESS_TYPE TLV
    Ptr<PbbAddressTlv> msg1a2tlv1 = Create<PbbAddressTlv>();
    msg1a2tlv1->SetType(AODVV2_ADDRESS_TYPE);
    uint8_t msg1a2tlv1val[] = {AODVV2_TARGPREFIX};
    msg1a2tlv1->SetValue(msg1a2tlv1val, sizeof(msg1a2tlv1val));
    msg1a2->TlvPushBack(msg1a2tlv1);

    // Add SEQ_NUM TLV
    Ptr<PbbAddressTlv> msg1a2tlv2 = Create<PbbAddressTlv>();
    msg1a2tlv2->SetType(AODVV2_SEQ_NUM);
    uint8_t msg1a2tlv2val[] = {0}; // TODO what value?
    msg1a2tlv2->SetValue(msg1a2tlv2val, sizeof(msg1a2tlv2val));
    msg1a2->TlvPushBack(msg1a2tlv2);

    msg1->AddressBlockPushBack(msg1a2);
    m_tlvHeader->MessagePushBack(msg1);
}

void
RreqHeader::SetTlvHeader(PbbPacket tlvHeader)
{
    this->SetSeqNo(tlvHeader.GetSequenceNumber());

    Ptr<PbbMessage> msg1 = tlvHeader.MessageFront();
    this->SetHopCount(msg1->GetHopLimit());

    for (auto i = msg1->AddressBlockBegin(); i != msg1->AddressBlockEnd(); i++)
    {
        Ptr<PbbAddressBlock> addressBlock = *i;
        for (auto j = addressBlock->TlvBegin(); j != addressBlock->TlvEnd(); j++)
        {
            Ptr<PbbAddressTlv> tlv = *j;

            if (tlv->GetType() == AODVV2_ADDRESS_TYPE)
            {
                if (tlv->GetValue().Begin().ReadU8() == AODVV2_ORIGPREFIX)
                {
                    this->SetOrigIp(Ipv4Address::ConvertFrom(addressBlock->AddressFront()));
                    this->SetOrigMask(addressBlock->PrefixFront());
                }
                else if (tlv->GetValue().Begin().ReadU8() == AODVV2_TARGPREFIX)
                {
                    this->SetTargIp(Ipv4Address::ConvertFrom(addressBlock->AddressFront()));
                    this->SetTargMask(addressBlock->PrefixFront());
                }
            }

            // TODO finire lettura del TLV
        }
    }

    CreateTlvHeader();
}

uint32_t
RreqHeader::Deserialize(Buffer::Iterator start)
{
    Buffer::Iterator i = start;

    m_tlvHeader = Create<PbbPacket>();
    m_tlvHeader->Deserialize(i);

    uint32_t dist = i.GetDistanceFrom(start);
    std::cout << dist << " " << GetSerializedSize() << std::endl;
    // NS_ASSERT(dist == GetSerializedSize());
    return dist;
}

void
RreqHeader::Print(std::ostream& os) const
{
    os << "sequence number " << m_seqNo << " hop count " << m_hopCount << " originator ipv4 "
       << m_origIp << " originator mask " << m_origMask << " target ipv4 " << m_targIp
       << " target mask " << m_targMask;
}

std::ostream&
operator<<(std::ostream& os, const RreqHeader& h)
{
    h.Print(os);
    return os;
}

bool
RreqHeader::operator==(const RreqHeader& o) const
{
    return (m_seqNo == o.m_seqNo && m_hopCount == o.m_hopCount && m_origIp == o.m_origIp &&
            m_origMask == o.m_origMask && m_targIp == o.m_targIp && m_targMask == o.m_targMask);
}

//-----------------------------------------------------------------------------
// RREP
//-----------------------------------------------------------------------------

RrepHeader::RrepHeader(Ipv4Address origIp,
                       uint16_t origMask,
                       Ipv4Address targIp,
                       uint16_t targMask,
                       uint32_t seqNo,
                       uint8_t hopCount)
    : m_origIp(origIp),
      m_origMask(origMask),
      m_targIp(targIp),
      m_targMask(targMask),
      m_seqNo(seqNo),
      m_hopCount(hopCount)
{
}

NS_OBJECT_ENSURE_REGISTERED(RrepHeader);

TypeId
RrepHeader::GetTypeId()
{
    static TypeId tid = TypeId("ns3::aodvv2::RrepHeader")
                            .SetParent<Header>()
                            .SetGroupName("Aodvv2")
                            .AddConstructor<RrepHeader>();
    return tid;
}

TypeId
RrepHeader::GetInstanceTypeId() const
{
    return GetTypeId();
}

uint32_t
RrepHeader::GetSerializedSize() const
{
    return m_tlvHeader->GetSerializedSize();
}

void
RrepHeader::Serialize(Buffer::Iterator i) const
{
    m_tlvHeader->Serialize(i);
}

void
RrepHeader::CreateTlvHeader() const
{
    m_tlvHeader = Create<PbbPacket>();
    m_tlvHeader->SetSequenceNumber(0); // TODO

    Ptr<PbbMessageIpv4> msg1 = Create<PbbMessageIpv4>();
    msg1->SetType(AODVV2_TYPE_RREP);
    msg1->SetHopLimit(AODVV2_MAX_HOP_COUNT - this->m_hopCount);

    // ****************************** OrigPrefix Address Block ******************************
    Ptr<PbbAddressBlockIpv4> msg1a1 = Create<PbbAddressBlockIpv4>();
    msg1a1->AddressPushBack(this->m_origIp);
    msg1a1->PrefixPushBack(this->m_origMask);

    // Add ADDRESS_TYPE TLV
    Ptr<PbbAddressTlv> msg1a1tlv1 = Create<PbbAddressTlv>();
    msg1a1tlv1->SetType(AODVV2_ADDRESS_TYPE);
    uint8_t msg1a1tlv1val[] = {AODVV2_ORIGPREFIX};
    msg1a1tlv1->SetValue(msg1a1tlv1val, sizeof(msg1a1tlv1val));
    msg1a1->TlvPushBack(msg1a1tlv1);

    msg1->AddressBlockPushBack(msg1a1);

    // ****************************** TargPrefix Address Block ******************************
    Ptr<PbbAddressBlockIpv4> msg1a2 = Create<PbbAddressBlockIpv4>();
    msg1a2->AddressPushBack(this->m_targIp);
    msg1a2->PrefixPushBack(this->m_targMask);

    // Add ADDRESS_TYPE TLV
    Ptr<PbbAddressTlv> msg1a2tlv1 = Create<PbbAddressTlv>();
    msg1a2tlv1->SetType(AODVV2_ADDRESS_TYPE);
    uint8_t msg1a2tlv1val[] = {AODVV2_TARGPREFIX};
    msg1a2tlv1->SetValue(msg1a2tlv1val, sizeof(msg1a2tlv1val));
    msg1a2->TlvPushBack(msg1a2tlv1);

    // Add SEQ_NUM TLV
    Ptr<PbbAddressTlv> msg1a2tlv2 = Create<PbbAddressTlv>();
    msg1a2tlv2->SetType(AODVV2_SEQ_NUM);
    uint8_t msg1a2tlv2val[] = {0}; // TODO seq number of router generating RREP
    msg1a2tlv2->SetValue(msg1a2tlv2val, sizeof(msg1a2tlv2val));
    msg1a1->TlvPushBack(msg1a2tlv2);

    // Add PATH_METRIC TLV
    Ptr<PbbAddressTlv> msg1a2tlv3 = Create<PbbAddressTlv>();
    msg1a2tlv3->SetType(AODVV2_PATH_METRIC);
    uint8_t msg1a2tlv3val[] = {1};
    msg1a2tlv3->SetValue(msg1a2tlv3val, sizeof(msg1a2tlv3val));
    msg1a1->TlvPushBack(msg1a2tlv3);

    msg1->AddressBlockPushBack(msg1a2);
    m_tlvHeader->MessagePushBack(msg1);
}

void
RrepHeader::SetTlvHeader(PbbPacket tlvHeader)
{
    this->SetSeqNo(tlvHeader.GetSequenceNumber());

    Ptr<PbbMessage> msg1 = tlvHeader.MessageFront();
    this->SetHopCount(msg1->GetHopLimit());

    for (auto i = msg1->AddressBlockBegin(); i != msg1->AddressBlockEnd(); i++)
    {
        Ptr<PbbAddressBlock> addressBlock = *i;
        for (auto j = addressBlock->TlvBegin(); j != addressBlock->TlvEnd(); j++)
        {
            Ptr<PbbAddressTlv> tlv = *j;

            if (tlv->GetType() == AODVV2_ADDRESS_TYPE)
            {
                if (tlv->GetValue().Begin().ReadU8() == AODVV2_ORIGPREFIX)
                {
                    this->SetOrigIp(Ipv4Address::ConvertFrom(addressBlock->AddressFront()));
                    this->SetOrigMask(addressBlock->PrefixFront());
                }
                else if (tlv->GetValue().Begin().ReadU8() == AODVV2_TARGPREFIX)
                {
                    this->SetTargIp(Ipv4Address::ConvertFrom(addressBlock->AddressFront()));
                    this->SetTargMask(addressBlock->PrefixFront());
                }
            }

            // TODO finire lettura del TLV
        }
    }

    CreateTlvHeader();
}

uint32_t
RrepHeader::Deserialize(Buffer::Iterator start)
{
    Buffer::Iterator i = start;

    m_tlvHeader = Create<PbbPacket>();
    m_tlvHeader->Deserialize(i);

    uint32_t dist = i.GetDistanceFrom(start);
    std::cout << dist << " " << GetSerializedSize() << std::endl;
    // NS_ASSERT(dist == GetSerializedSize());
    return dist;
}

void
RrepHeader::Print(std::ostream& os) const
{
    os << " originator ipv4 " << m_origIp << " originator mask " << m_origMask << " target ipv4 "
       << m_targIp << " target mask " << m_targMask;
}

bool
RrepHeader::operator==(const RrepHeader& o) const
{
    return (m_origIp == o.m_origIp && m_origMask == o.m_origMask && m_targIp == o.m_targIp &&
            m_targMask == o.m_targMask);
}

std::ostream&
operator<<(std::ostream& os, const RrepHeader& h)
{
    h.Print(os);
    return os;
}

//-----------------------------------------------------------------------------
// RREP-ACK
//-----------------------------------------------------------------------------

RrepAckHeader::RrepAckHeader()
    : m_reserved(0)
{
}

NS_OBJECT_ENSURE_REGISTERED(RrepAckHeader);

TypeId
RrepAckHeader::GetTypeId()
{
    static TypeId tid = TypeId("ns3::aodvv2::RrepAckHeader")
                            .SetParent<Header>()
                            .SetGroupName("Aodvv2")
                            .AddConstructor<RrepAckHeader>();
    return tid;
}

TypeId
RrepAckHeader::GetInstanceTypeId() const
{
    return GetTypeId();
}

uint32_t
RrepAckHeader::GetSerializedSize() const
{
    return 1;
}

void
RrepAckHeader::Serialize(Buffer::Iterator i) const
{
    i.WriteU8(m_reserved);
}

uint32_t
RrepAckHeader::Deserialize(Buffer::Iterator start)
{
    Buffer::Iterator i = start;
    m_reserved = i.ReadU8();
    uint32_t dist = i.GetDistanceFrom(start);
    NS_ASSERT(dist == GetSerializedSize());
    return dist;
}

void
RrepAckHeader::Print(std::ostream& os) const
{
}

bool
RrepAckHeader::operator==(const RrepAckHeader& o) const
{
    return m_reserved == o.m_reserved;
}

std::ostream&
operator<<(std::ostream& os, const RrepAckHeader& h)
{
    h.Print(os);
    return os;
}

//-----------------------------------------------------------------------------
// RERR
//-----------------------------------------------------------------------------
RerrHeader::RerrHeader()
    : m_flag(0),
      m_reserved(0)
{
}

NS_OBJECT_ENSURE_REGISTERED(RerrHeader);

TypeId
RerrHeader::GetTypeId()
{
    static TypeId tid = TypeId("ns3::aodvv2::RerrHeader")
                            .SetParent<Header>()
                            .SetGroupName("Aodvv2")
                            .AddConstructor<RerrHeader>();
    return tid;
}

TypeId
RerrHeader::GetInstanceTypeId() const
{
    return GetTypeId();
}

uint32_t
RerrHeader::GetSerializedSize() const
{
    return (3 + 8 * GetDestCount());
}

void
RerrHeader::Serialize(Buffer::Iterator i) const
{
    i.WriteU8(m_flag);
    i.WriteU8(m_reserved);
    i.WriteU8(GetDestCount());
    for (auto j = m_unreachableDstSeqNo.begin(); j != m_unreachableDstSeqNo.end(); ++j)
    {
        WriteTo(i, (*j).first);
        i.WriteHtonU32((*j).second);
    }
}

uint32_t
RerrHeader::Deserialize(Buffer::Iterator start)
{
    Buffer::Iterator i = start;
    m_flag = i.ReadU8();
    m_reserved = i.ReadU8();
    uint8_t dest = i.ReadU8();
    m_unreachableDstSeqNo.clear();
    Ipv4Address address;
    uint32_t seqNo;
    for (uint8_t k = 0; k < dest; ++k)
    {
        ReadFrom(i, address);
        seqNo = i.ReadNtohU32();
        m_unreachableDstSeqNo.insert(std::make_pair(address, seqNo));
    }

    uint32_t dist = i.GetDistanceFrom(start);
    NS_ASSERT(dist == GetSerializedSize());
    return dist;
}

void
RerrHeader::Print(std::ostream& os) const
{
    os << "Unreachable destination (ipv4 address, seq. number):";
    for (auto j = m_unreachableDstSeqNo.begin(); j != m_unreachableDstSeqNo.end(); ++j)
    {
        os << (*j).first << ", " << (*j).second;
    }
    os << "No delete flag " << (*this).GetNoDelete();
}

void
RerrHeader::SetNoDelete(bool f)
{
    if (f)
    {
        m_flag |= (1 << 0);
    }
    else
    {
        m_flag &= ~(1 << 0);
    }
}

bool
RerrHeader::GetNoDelete() const
{
    return (m_flag & (1 << 0));
}

bool
RerrHeader::AddUnDestination(Ipv4Address dst, uint32_t seqNo)
{
    if (m_unreachableDstSeqNo.find(dst) != m_unreachableDstSeqNo.end())
    {
        return true;
    }

    NS_ASSERT(GetDestCount() < 255); // can't support more than 255 destinations in single RERR
    m_unreachableDstSeqNo.insert(std::make_pair(dst, seqNo));
    return true;
}

bool
RerrHeader::RemoveUnDestination(std::pair<Ipv4Address, uint32_t>& un)
{
    if (m_unreachableDstSeqNo.empty())
    {
        return false;
    }
    auto i = m_unreachableDstSeqNo.begin();
    un = *i;
    m_unreachableDstSeqNo.erase(i);
    return true;
}

void
RerrHeader::Clear()
{
    m_unreachableDstSeqNo.clear();
    m_flag = 0;
    m_reserved = 0;
}

bool
RerrHeader::operator==(const RerrHeader& o) const
{
    if (m_flag != o.m_flag || m_reserved != o.m_reserved || GetDestCount() != o.GetDestCount())
    {
        return false;
    }

    auto j = m_unreachableDstSeqNo.begin();
    auto k = o.m_unreachableDstSeqNo.begin();
    for (uint8_t i = 0; i < GetDestCount(); ++i)
    {
        if ((j->first != k->first) || (j->second != k->second))
        {
            return false;
        }

        j++;
        k++;
    }
    return true;
}

std::ostream&
operator<<(std::ostream& os, const RerrHeader& h)
{
    h.Print(os);
    return os;
}
} // namespace aodvv2
} // namespace ns3
