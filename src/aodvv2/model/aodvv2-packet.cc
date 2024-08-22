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
template <typename T>
RreqHeader<T>::RreqHeader(T origIp,
                          uint16_t origMask,
                          T targIp,
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

// NS_OBJECT_ENSURE_REGISTERED(RreqHeader);

template <typename T>
TypeId
RreqHeader<T>::GetTypeId()
{
    static TypeId tid = TypeId("ns3::aodvv2::RreqHeader")
                            .SetParent<Header>()
                            .SetGroupName("Aodvv2")
                            .AddConstructor<RreqHeader<T>>();
    return tid;
}

template <typename T>
TypeId
RreqHeader<T>::GetInstanceTypeId() const
{
    return GetTypeId();
}

template <typename T>
uint32_t
RreqHeader<T>::GetSerializedSize() const
{
    return m_tlvHeader->GetSerializedSize();
}

template <typename T>
void
RreqHeader<T>::Serialize(Buffer::Iterator i) const
{
    m_tlvHeader->Serialize(i);
}

template <typename T>
void
RreqHeader<T>::CreateTlvHeader() const
{
    m_tlvHeader = Create<PbbPacket>();
    m_tlvHeader->SetSequenceNumber(this->m_seqNo);

    Ptr<PbbMessageIp> msg1 = Create<PbbMessageIp>();
    msg1->SetType(AODVV2_TYPE_RREQ);
    msg1->SetHopLimit(AODVV2_MAX_HOP_COUNT);

    // ****************************** OrigPrefix Address Block ******************************
    Ptr<PbbAddressBlockIp> msg1a1 = Create<PbbAddressBlockIp>();
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
    Ptr<PbbAddressBlockIp> msg1a2 = Create<PbbAddressBlockIp>();
    msg1a2->AddressPushBack(this->m_targIp);
    msg1a2->PrefixPushBack(this->m_targMask);

    // Add ADDRESS_TYPE TLV
    Ptr<PbbAddressTlv> msg1a2tlv1 = Create<PbbAddressTlv>();
    msg1a2tlv1->SetType(AODVV2_ADDRESS_TYPE);
    uint8_t msg1a2tlv1val[] = {AODVV2_TARGPREFIX};
    msg1a2tlv1->SetValue(msg1a2tlv1val, sizeof(msg1a2tlv1val));
    msg1a2->TlvPushBack(msg1a2tlv1);

    // Add SEQ_NUM TLV
    /* optional, use only with invalid route
    Ptr<PbbAddressTlv> msg1a2tlv2 = Create<PbbAddressTlv>();
    msg1a2tlv2->SetType(AODVV2_SEQ_NUM);
    uint8_t msg1a2tlv2val[] = {0};
    msg1a2tlv2->SetValue(msg1a2tlv2val, sizeof(msg1a2tlv2val));
    msg1a2->TlvPushBack(msg1a2tlv2); */

    msg1->AddressBlockPushBack(msg1a2);
    m_tlvHeader->MessagePushBack(msg1);
}

template <typename T>
void
RreqHeader<T>::SetTlvHeader(PbbPacket tlvHeader)
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
                    this->SetOrigIp(T::ConvertFrom(addressBlock->AddressFront()));
                    this->SetOrigMask(addressBlock->PrefixFront());
                }
                else if (tlv->GetValue().Begin().ReadU8() == AODVV2_TARGPREFIX)
                {
                    this->SetTargIp(T::ConvertFrom(addressBlock->AddressFront()));
                    this->SetTargMask(addressBlock->PrefixFront());
                }
            }

            // TODO finire lettura del TLV
        }
    }

    CreateTlvHeader();
}

template <typename T>
uint32_t
RreqHeader<T>::Deserialize(Buffer::Iterator start)
{
    Buffer::Iterator i = start;

    m_tlvHeader = Create<PbbPacket>();
    m_tlvHeader->Deserialize(i);

    uint32_t dist = i.GetDistanceFrom(start);
    std::cout << dist << " " << GetSerializedSize() << std::endl;
    // NS_ASSERT(dist == GetSerializedSize());
    return dist;
}

template <typename T>
void
RreqHeader<T>::Print(std::ostream& os) const
{
    os << "sequence number " << m_seqNo << " hop count " << m_hopCount << " originator ip "
       << m_origIp << " originator mask " << m_origMask << " target ip " << m_targIp
       << " target mask " << m_targMask;
}

template <typename T>
std::ostream&
operator<<(std::ostream& os, const RreqHeader<T>& h)
{
    h.Print(os);
    return os;
}

template <typename T>
bool
RreqHeader<T>::operator==(const RreqHeader<T>& o) const
{
    return (m_seqNo == o.m_seqNo && m_hopCount == o.m_hopCount && m_origIp == o.m_origIp &&
            m_origMask == o.m_origMask && m_targIp == o.m_targIp && m_targMask == o.m_targMask);
}

template class RreqHeader<Ipv4Address>;
template class RreqHeader<Ipv6Address>;

//-----------------------------------------------------------------------------
// RREP
//-----------------------------------------------------------------------------

template <typename T>
RrepHeader<T>::RrepHeader(T origIp,
                          uint16_t origMask,
                          T targIp,
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

// NS_OBJECT_ENSURE_REGISTERED(RrepHeader);

template <typename T>
TypeId
RrepHeader<T>::GetTypeId()
{
    static TypeId tid = TypeId("ns3::aodvv2::RrepHeader")
                            .SetParent<Header>()
                            .SetGroupName("Aodvv2")
                            .AddConstructor<RrepHeader<T>>();
    return tid;
}

template <typename T>
TypeId
RrepHeader<T>::GetInstanceTypeId() const
{
    return GetTypeId();
}

template <typename T>
uint32_t
RrepHeader<T>::GetSerializedSize() const
{
    return m_tlvHeader->GetSerializedSize();
}

template <typename T>
void
RrepHeader<T>::Serialize(Buffer::Iterator i) const
{
    m_tlvHeader->Serialize(i);
}

template <typename T>
void
RrepHeader<T>::CreateTlvHeader() const
{
    m_tlvHeader = Create<PbbPacket>();
    m_tlvHeader->SetSequenceNumber(0); // TODO

    Ptr<PbbMessageIp> msg1 = Create<PbbMessageIp>();
    msg1->SetType(AODVV2_TYPE_RREP);
    msg1->SetHopLimit(AODVV2_MAX_HOP_COUNT - this->m_hopCount);

    // ****************************** OrigPrefix Address Block ******************************
    Ptr<PbbAddressBlockIp> msg1a1 = Create<PbbAddressBlockIp>();
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
    Ptr<PbbAddressBlockIp> msg1a2 = Create<PbbAddressBlockIp>();
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

template <typename T>
void
RrepHeader<T>::SetTlvHeader(PbbPacket tlvHeader)
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
                    this->SetOrigIp(T::ConvertFrom(addressBlock->AddressFront()));
                    this->SetOrigMask(addressBlock->PrefixFront());
                }
                else if (tlv->GetValue().Begin().ReadU8() == AODVV2_TARGPREFIX)
                {
                    this->SetTargIp(T::ConvertFrom(addressBlock->AddressFront()));
                    this->SetTargMask(addressBlock->PrefixFront());
                }
            }

            // TODO finire lettura del TLV
        }
    }

    CreateTlvHeader();
}

template <typename T>
uint32_t
RrepHeader<T>::Deserialize(Buffer::Iterator start)
{
    Buffer::Iterator i = start;

    m_tlvHeader = Create<PbbPacket>();
    m_tlvHeader->Deserialize(i);

    uint32_t dist = i.GetDistanceFrom(start);
    std::cout << dist << " " << GetSerializedSize() << std::endl;
    // NS_ASSERT(dist == GetSerializedSize());
    return dist;
}

template <typename T>
void
RrepHeader<T>::Print(std::ostream& os) const
{
    os << " originator ip " << m_origIp << " originator mask " << m_origMask << " target ip "
       << m_targIp << " target mask " << m_targMask;
}

template <typename T>
bool
RrepHeader<T>::operator==(const RrepHeader<T>& o) const
{
    return (m_origIp == o.m_origIp && m_origMask == o.m_origMask && m_targIp == o.m_targIp &&
            m_targMask == o.m_targMask);
}

template <typename T>
std::ostream&
operator<<(std::ostream& os, const RrepHeader<T>& h)
{
    h.Print(os);
    return os;
}

template class RrepHeader<Ipv4Address>;
template class RrepHeader<Ipv6Address>;

//-----------------------------------------------------------------------------
// RREP-ACK
//-----------------------------------------------------------------------------

template <typename T>
RrepAckHeader<T>::RrepAckHeader()
    : m_reserved(0)
{
}

// NS_OBJECT_ENSURE_REGISTERED(RrepAckHeader);

template <typename T>
TypeId
RrepAckHeader<T>::GetTypeId()
{
    static TypeId tid = TypeId("ns3::aodvv2::RrepAckHeader")
                            .SetParent<Header>()
                            .SetGroupName("Aodvv2")
                            .AddConstructor<RrepAckHeader<T>>();
    return tid;
}

template <typename T>
TypeId
RrepAckHeader<T>::GetInstanceTypeId() const
{
    return GetTypeId();
}

template <typename T>
uint32_t
RrepAckHeader<T>::GetSerializedSize() const
{
    return m_tlvHeader->GetSerializedSize();
}

template <typename T>
void
RrepAckHeader<T>::Serialize(Buffer::Iterator i) const
{
    m_tlvHeader->Serialize(i);
}

template <typename T>
void
RrepAckHeader<T>::CreateTlvHeader() const
{
    m_tlvHeader = Create<PbbPacket>();
    m_tlvHeader->SetSequenceNumber(this->m_seqNo);

    Ptr<PbbMessageIp> msg1 = Create<PbbMessageIp>();
    msg1->SetType(AODVV2_TYPE_RREP_ACK);
    Ptr<PbbTlv> msg1tlv1 = Create<PbbTlv>();
    msg1tlv1->SetType(AODVV2_ACK_REQ);
    msg1->TlvPushBack(msg1tlv1);

    m_tlvHeader->MessagePushBack(msg1);
}

template <typename T>
void
RrepAckHeader<T>::SetTlvHeader(PbbPacket tlvHeader)
{
    this->SetSeqNo(tlvHeader.GetSequenceNumber());
    CreateTlvHeader();
}

template <typename T>
uint32_t
RrepAckHeader<T>::Deserialize(Buffer::Iterator start)
{
    Buffer::Iterator i = start;
    m_reserved = i.ReadU8();
    uint32_t dist = i.GetDistanceFrom(start);
    NS_ASSERT(dist == GetSerializedSize());
    return dist;
}

template <typename T>
void
RrepAckHeader<T>::Print(std::ostream& os) const
{
}

template <typename T>
bool
RrepAckHeader<T>::operator==(const RrepAckHeader& o) const
{
    return m_reserved == o.m_reserved;
}

template <typename T>
std::ostream&
operator<<(std::ostream& os, const RrepAckHeader<T>& h)
{
    h.Print(os);
    return os;
}

template class RrepAckHeader<Ipv4Address>;
template class RrepAckHeader<Ipv6Address>;

//-----------------------------------------------------------------------------
// RERR
//-----------------------------------------------------------------------------
template <typename T>
RerrHeader<T>::RerrHeader()
    : m_flag(0),
      m_reserved(0)
{
}

// NS_OBJECT_ENSURE_REGISTERED(RerrHeader);

template <typename T>
TypeId
RerrHeader<T>::GetTypeId()
{
    static TypeId tid = TypeId("ns3::aodvv2::RerrHeader")
                            .SetParent<Header>()
                            .SetGroupName("Aodvv2")
                            .AddConstructor<RerrHeader>();
    return tid;
}

template <typename T>
TypeId
RerrHeader<T>::GetInstanceTypeId() const
{
    return GetTypeId();
}

template <typename T>
uint32_t
RerrHeader<T>::GetSerializedSize() const
{
    return (3 + 8 * GetDestCount());
}

template <typename T>
void
RerrHeader<T>::Serialize(Buffer::Iterator i) const
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

template <typename T>
uint32_t
RerrHeader<T>::Deserialize(Buffer::Iterator start)
{
    Buffer::Iterator i = start;
    m_flag = i.ReadU8();
    m_reserved = i.ReadU8();
    uint8_t dest = i.ReadU8();
    m_unreachableDstSeqNo.clear();
    T address;
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

template <typename T>
void
RerrHeader<T>::Print(std::ostream& os) const
{
    os << "Unreachable destination (ip address, seq. number):";
    for (auto j = m_unreachableDstSeqNo.begin(); j != m_unreachableDstSeqNo.end(); ++j)
    {
        os << (*j).first << ", " << (*j).second;
    }
    os << "No delete flag " << (*this).GetNoDelete();
}

template <typename T>
void
RerrHeader<T>::SetNoDelete(bool f)
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

template <typename T>
bool
RerrHeader<T>::GetNoDelete() const
{
    return (m_flag & (1 << 0));
}

template <typename T>
bool
RerrHeader<T>::AddUnDestination(T dst, uint32_t seqNo)
{
    if (m_unreachableDstSeqNo.find(dst) != m_unreachableDstSeqNo.end())
    {
        return true;
    }

    NS_ASSERT(GetDestCount() < 255); // can't support more than 255 destinations in single RERR
    m_unreachableDstSeqNo.insert(std::make_pair(dst, seqNo));
    return true;
}

template <typename T>
bool
RerrHeader<T>::RemoveUnDestination(std::pair<T, uint32_t>& un)
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

template <typename T>
void
RerrHeader<T>::Clear()
{
    m_unreachableDstSeqNo.clear();
    m_flag = 0;
    m_reserved = 0;
}

template <typename T>
bool
RerrHeader<T>::operator==(const RerrHeader& o) const
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

template <typename T>
std::ostream&
operator<<(std::ostream& os, const RerrHeader<T>& h)
{
    h.Print(os);
    return os;
}

template class RerrHeader<Ipv4Address>;
template class RerrHeader<Ipv6Address>;

} // namespace aodvv2
} // namespace ns3
