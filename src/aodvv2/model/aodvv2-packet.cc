/*
 * Copyright (c) 2024 University of Florence
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
 *      NS-3 AODV model developed by Elena Buchatskaya and Pavel Boyko of IITP RAS
 *
 * Authors: Francesco Todino <francesco.todino@edu.unifi.it>
 *          Tommaso Pecorella <tommaso.pecorella@unifi.it>
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
                          uint16_t seqNo,
                          uint8_t hopCount,
                          uint8_t maxHopCount,
                          uint8_t metricType,
                          uint8_t origMetric)
    : m_origIp(origIp),
      m_origMask(origMask),
      m_origSeqNo(1),
      m_targIp(targIp),
      m_targMask(targMask),
      m_targSeqNo(1),
      m_metricType(metricType),
      m_origMetric(origMetric),
      m_seqNo(seqNo),
      m_hopCount(hopCount),
      m_maxHopCount(maxHopCount)
{
}

template <typename T>
RreqHeader<T>::RreqHeader(PbbPacket tlvHeader)
{
    SetTlvHeader(tlvHeader);
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
    this->CreateTlvHeader();
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
    msg1->SetHopLimit(this->m_maxHopCount);
    msg1->SetHopCount(this->m_hopCount);

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
    // TODO me: understand if the packetbb lib has to be updated to uint16_t
    uint8_t msg1a1tlv2val[] = {(uint8_t)this->m_seqNo};
    msg1a1tlv2->SetValue(msg1a1tlv2val, sizeof(msg1a1tlv2val));
    msg1a1->TlvPushBack(msg1a1tlv2);

    // Add PATH_METRIC TLV
    Ptr<PbbAddressTlv> msg1a1tlv3 = Create<PbbAddressTlv>();
    msg1a1tlv3->SetType(AODVV2_PATH_METRIC);
    msg1a1tlv3->SetTypeExt(this->m_metricType);
    uint8_t msg1a1tlv3val[] = {this->m_origMetric};
    msg1a1tlv3->SetValue(msg1a1tlv3val, sizeof(msg1a1tlv3val));
    msg1a1->TlvPushBack(msg1a1tlv3);

    msg1->AddressBlockPushBack(msg1a1);
    // **************************************************************************************

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
    if (this->m_sendTargSeqNum)
    {
        Ptr<PbbAddressTlv> msg1a2tlv2 = Create<PbbAddressTlv>();
        msg1a2tlv2->SetType(AODVV2_SEQ_NUM);
        // TODO me: understand if the packetbb lib has to be updated to uint16_t
        uint8_t msg1a2tlv2val[] = {(uint8_t)this->m_origSeqNo};
        msg1a2tlv2->SetValue(msg1a2tlv2val, sizeof(msg1a2tlv2val));
        msg1a2->TlvPushBack(msg1a2tlv2);
    }

    msg1->AddressBlockPushBack(msg1a2);
    // **************************************************************************************

    // ****************************** SeqNoRtr Address Block ******************************
    if (this->m_rtrIp != T() && this->m_rtrIp != this->m_origIp)
    {
        Ptr<PbbAddressBlockIp> msg1a3 = Create<PbbAddressBlockIp>();
        msg1a3->AddressPushBack(this->m_rtrIp);
        msg1a3->PrefixPushBack(this->m_rtrMask);

        msg1->AddressBlockPushBack(msg1a3);
    }
    // **************************************************************************************

    // Add msg to tlv header
    m_tlvHeader->MessagePushBack(msg1);
}

template <typename T>
void
RreqHeader<T>::SetTlvHeader(PbbPacket tlvHeader)
{
    Ptr<PbbMessage> msg1 = tlvHeader.MessageFront();
    this->SetSeqNo(tlvHeader.GetSequenceNumber());
    if (msg1->HasHopCount())
    {
        this->SetHopCount(msg1->GetHopCount());
    }
    if (msg1->HasHopLimit())
    {
        this->SetMaxHopCount(msg1->GetHopLimit());
    }

    for (auto i = msg1->AddressBlockBegin(); i != msg1->AddressBlockEnd(); i++)
    {
        bool hasAddrType = false;
        bool hasSeqNum = false;
        bool hasMetric = false;
        uint8_t addrType = 0;
        uint8_t seqNum = 0;
        uint8_t metric = 0;
        uint8_t metricType = 0;

        Ptr<PbbAddressBlock> addressBlock = *i;
        for (auto j = addressBlock->TlvBegin(); j != addressBlock->TlvEnd(); j++)
        {
            Ptr<PbbAddressTlv> tlv = *j;

            if (tlv->GetType() == AODVV2_ADDRESS_TYPE)
            {
                addrType = tlv->GetValue().Begin().ReadU8();
                hasAddrType = true;
            }
            else if (tlv->GetType() == AODVV2_SEQ_NUM)
            {
                seqNum = tlv->GetValue().Begin().ReadU8();
                hasSeqNum = true;
            }
            else if (tlv->GetType() == AODVV2_PATH_METRIC)
            {
                metric = tlv->GetValue().Begin().ReadU8();
                metricType = tlv->GetTypeExt();
                hasMetric = true;
            }
        }

        if (hasAddrType)
        {
            switch (addrType)
            {
            case AODVV2_ORIGPREFIX:
                this->SetOrigIp(T::ConvertFrom(addressBlock->AddressFront()));
                this->SetOrigMask(addressBlock->PrefixFront());
                if (hasSeqNum)
                {
                    this->SetOrigSeqNo(seqNum);
                }
                if (hasMetric)
                {
                    this->SetOrigMetric(metric);
                    this->SetMetricType(metricType);
                }
                break;
            case AODVV2_TARGPREFIX:
                this->SetTargIp(T::ConvertFrom(addressBlock->AddressFront()));
                this->SetTargMask(addressBlock->PrefixFront());
                if (hasSeqNum)
                {
                    this->SetTargSeqNo(seqNum);
                }
                break;
            }
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
    NS_ASSERT(dist == GetSerializedSize());
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
                          uint16_t seqNo,
                          uint8_t hopCount,
                          uint8_t maxHopCount,
                          uint8_t metricType,
                          uint8_t targMetric)
    : m_origIp(origIp),
      m_origMask(origMask),
      m_targIp(targIp),
      m_targMask(targMask),
      m_seqNo(seqNo),
      m_hopCount(hopCount),
      m_maxHopCount(maxHopCount),
      m_metricType(metricType),
      m_targMetric(targMetric)
{
}

template <typename T>
RrepHeader<T>::RrepHeader(PbbPacket tlvHeader)
{
    SetTlvHeader(tlvHeader);
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
    this->CreateTlvHeader();
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
    m_tlvHeader->SetSequenceNumber(this->m_seqNo);

    Ptr<PbbMessageIp> msg1 = Create<PbbMessageIp>();
    msg1->SetType(AODVV2_TYPE_RREP);
    msg1->SetHopLimit(this->m_maxHopCount - this->m_hopCount);

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
    // **************************************************************************************

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
    // TODO me: understand if the packetbb lib has to be updated to uint16_t
    uint8_t msg1a2tlv2val[] = {(uint8_t)this->m_seqNo};
    msg1a2tlv2->SetValue(msg1a2tlv2val, sizeof(msg1a2tlv2val));
    msg1a1->TlvPushBack(msg1a2tlv2);

    // Add PATH_METRIC TLV
    Ptr<PbbAddressTlv> msg1a2tlv3 = Create<PbbAddressTlv>();
    msg1a2tlv3->SetType(AODVV2_PATH_METRIC);
    msg1a2tlv3->SetTypeExt(this->m_metricType);
    uint8_t msg1a2tlv3val[] = {this->m_targMetric};
    msg1a2tlv3->SetValue(msg1a2tlv3val, sizeof(msg1a2tlv3val));
    msg1a1->TlvPushBack(msg1a2tlv3);

    msg1->AddressBlockPushBack(msg1a2);
    // **************************************************************************************

    // Add msg to tlv header
    m_tlvHeader->MessagePushBack(msg1);
}

template <typename T>
void
RrepHeader<T>::SetTlvHeader(PbbPacket tlvHeader)
{
    Ptr<PbbMessage> msg1 = tlvHeader.MessageFront();
    this->SetSeqNo(tlvHeader.GetSequenceNumber());
    this->SetHopCount(msg1->GetHopLimit());

    for (auto i = msg1->AddressBlockBegin(); i != msg1->AddressBlockEnd(); i++)
    {
        bool hasAddrType = false;
        bool hasSeqNum = false;
        bool hasMetric = false;
        uint8_t addrType = 0;
        uint8_t seqNum = 0;
        uint8_t metric = 0;
        uint8_t metricType = 0;

        Ptr<PbbAddressBlock> addressBlock = *i;
        for (auto j = addressBlock->TlvBegin(); j != addressBlock->TlvEnd(); j++)
        {
            Ptr<PbbAddressTlv> tlv = *j;

            if (tlv->GetType() == AODVV2_ADDRESS_TYPE)
            {
                addrType = tlv->GetValue().Begin().ReadU8();
                hasAddrType = true;
            }
            else if (tlv->GetType() == AODVV2_SEQ_NUM)
            {
                seqNum = tlv->GetValue().Begin().ReadU8();
                hasSeqNum = true;
            }
            else if (tlv->GetType() == AODVV2_PATH_METRIC)
            {
                metric = tlv->GetValue().Begin().ReadU8();
                metricType = tlv->GetTypeExt();
                hasMetric = true;
            }
        }

        if (hasAddrType)
        {
            switch (addrType)
            {
            case AODVV2_ORIGPREFIX:
                this->SetOrigIp(T::ConvertFrom(addressBlock->AddressFront()));
                this->SetOrigMask(addressBlock->PrefixFront());
                break;
            case AODVV2_TARGPREFIX:
                this->SetTargIp(T::ConvertFrom(addressBlock->AddressFront()));
                this->SetTargMask(addressBlock->PrefixFront());
                if (hasSeqNum)
                {
                    this->SetTargSeqNo(seqNum);
                }
                if (hasMetric)
                {
                    this->SetTargMetric(metric);
                    this->SetMetricType(metricType);
                }
                break;
            }
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
    NS_ASSERT(dist == GetSerializedSize());
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
RrepAckHeader<T>::RrepAckHeader(uint8_t maxHopCount)
    : m_maxHopCount(maxHopCount)
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
    this->CreateTlvHeader();
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
    CreateTlvHeader();
}

template <typename T>
uint32_t
RrepAckHeader<T>::Deserialize(Buffer::Iterator start)
{
    Buffer::Iterator i = start;

    m_tlvHeader = Create<PbbPacket>();
    m_tlvHeader->Deserialize(i);

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
    return m_seqNo == o.m_seqNo;
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
RerrHeader<T>::RerrHeader(uint8_t maxHopCount)
    : m_maxHopCount(maxHopCount)
{
}

template <typename T>
RerrHeader<T>::RerrHeader(PbbPacket tlvHeader, uint8_t maxHopCount)
    : m_maxHopCount(maxHopCount)
{
    SetTlvHeader(tlvHeader);
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
    this->CreateTlvHeader();
    return m_tlvHeader->GetSerializedSize();
}

template <typename T>
void
RerrHeader<T>::Serialize(Buffer::Iterator i) const
{
    m_tlvHeader->Serialize(i);
}

template <typename T>
void
RerrHeader<T>::CreateTlvHeader() const
{
    m_tlvHeader = Create<PbbPacket>();
    // m_tlvHeader->SetSequenceNumber(this->m_seqNo);

    Ptr<PbbMessageIp> msg1 = Create<PbbMessageIp>();
    msg1->SetType(AODVV2_TYPE_RERR);

    // ****************************** PktSource Address Block *******************************
    Ptr<PbbAddressBlockIp> msg1a1 = Create<PbbAddressBlockIp>();
    msg1a1->AddressPushBack(this->m_origIp);
    msg1a1->PrefixPushBack(this->m_origMask);

    // Add ADDRESS_TYPE TLV
    Ptr<PbbAddressTlv> msg1a1tlv1 = Create<PbbAddressTlv>();
    msg1a1tlv1->SetType(AODVV2_ADDRESS_TYPE);
    uint8_t msg1a1tlv1val[] = {AODVV2_PKTSOURCE};
    msg1a1tlv1->SetValue(msg1a1tlv1val, sizeof(msg1a1tlv1val));
    msg1a1->TlvPushBack(msg1a1tlv1);

    msg1->AddressBlockPushBack(msg1a1);
    // **************************************************************************************

    // ****************************** AddressList Address Block ******************************
    // for each unreachable destination
    for (auto j = m_unreachableDst.begin(); j != m_unreachableDst.end(); ++j)
    {
        Ptr<PbbAddressBlockIp> msg1a2 = Create<PbbAddressBlockIp>();
        msg1a2->AddressPushBack((*j).first);
        msg1a2->PrefixPushBack(32); // TODO me: update if needed

        // Add ADDRESS_TYPE TLV
        Ptr<PbbAddressTlv> msg1a2tlv1 = Create<PbbAddressTlv>();
        msg1a2tlv1->SetType(AODVV2_ADDRESS_TYPE);
        uint8_t msg1a2tlv1val[] = {AODVV2_UNREACHABLE};
        msg1a2tlv1->SetValue(msg1a2tlv1val, sizeof(msg1a2tlv1val));
        msg1a2->TlvPushBack(msg1a2tlv1);

        // Add SEQ_NUM TLV
        Ptr<PbbAddressTlv> msg1a2tlv2 = Create<PbbAddressTlv>();
        msg1a2tlv2->SetType(AODVV2_SEQ_NUM);
        uint8_t msg1a2tlv2val[] = {(uint8_t)(*j).second.m_seqNo};
        msg1a2tlv2->SetValue(msg1a2tlv2val, sizeof(msg1a2tlv2val));
        msg1a1->TlvPushBack(msg1a2tlv2);

        // Add PATH_METRIC TLV
        Ptr<PbbAddressTlv> msg1a2tlv3 = Create<PbbAddressTlv>();
        msg1a2tlv3->SetType(AODVV2_PATH_METRIC);
        msg1a2tlv3->SetTypeExt((*j).second.m_metricType);
        msg1a1->TlvPushBack(msg1a2tlv3);

        msg1->AddressBlockPushBack(msg1a2);
    }
    // **************************************************************************************

    // Add msg to tlv header
    m_tlvHeader->MessagePushBack(msg1);
}

template <typename T>
void
RerrHeader<T>::SetTlvHeader(PbbPacket tlvHeader)
{
    Ptr<PbbMessage> msg1 = tlvHeader.MessageFront();

    for (auto i = msg1->AddressBlockBegin(); i != msg1->AddressBlockEnd(); i++)
    {
        bool hasAddrType = false;
        bool hasMetric = false;
        uint8_t addrType = 0;
        uint8_t seqNum = 0;
        uint8_t metricType = 0;

        Ptr<PbbAddressBlock> addressBlock = *i;
        for (auto j = addressBlock->TlvBegin(); j != addressBlock->TlvEnd(); j++)
        {
            Ptr<PbbAddressTlv> tlv = *j;

            if (tlv->GetType() == AODVV2_ADDRESS_TYPE)
            {
                addrType = tlv->GetValue().Begin().ReadU8();
                hasAddrType = true;
            }
            else if (tlv->GetType() == AODVV2_SEQ_NUM)
            {
                seqNum = tlv->GetValue().Begin().ReadU8();
            }
            else if (tlv->GetType() == AODVV2_PATH_METRIC)
            {
                metricType = tlv->GetTypeExt();
                hasMetric = true;
            }
        }

        if (hasAddrType)
        {
            switch (addrType)
            {
            case AODVV2_PKTSOURCE:
                this->SetOrigIp(T::ConvertFrom(addressBlock->AddressFront()));
                this->SetOrigMask(addressBlock->PrefixFront());
                break;
            case AODVV2_TARGPREFIX:
                if (hasMetric)
                {
                    this->AddUnDestination(T::ConvertFrom(addressBlock->AddressFront()),
                                           seqNum,
                                           metricType);
                }
                else
                {
                    this->AddUnDestination(T::ConvertFrom(addressBlock->AddressFront()), seqNum);
                }

                break;
            }
        }
    }

    CreateTlvHeader();
}

template <typename T>
uint32_t
RerrHeader<T>::Deserialize(Buffer::Iterator start)
{
    Buffer::Iterator i = start;

    m_tlvHeader = Create<PbbPacket>();
    m_tlvHeader->Deserialize(i);

    uint32_t dist = i.GetDistanceFrom(start);
    NS_ASSERT(dist == GetSerializedSize());
    return dist;
}

template <typename T>
void
RerrHeader<T>::Print(std::ostream& os) const
{
    os << "Unreachable destination (ip address, seq. number):";
    for (auto j = m_unreachableDst.begin(); j != m_unreachableDst.end(); ++j)
    {
        os << (*j).first << ", " << (*j).second.m_seqNo << ", " << (*j).second.m_metricType;
    }
}

template <typename T>
bool
RerrHeader<T>::AddUnDestination(T dst, uint16_t seqNo, uint8_t metricType)
{
    if (m_unreachableDst.find(dst) != m_unreachableDst.end())
    {
        return true;
    }

    NS_ASSERT(GetDestCount() < 255); // can't support more than 255 destinations in single RERR
    m_unreachableDst.insert(std::make_pair(dst, UnreachableDst{seqNo, metricType}));
    return true;
}

template <typename T>
bool
RerrHeader<T>::RemoveUnDestination(std::pair<T, UnreachableDst>& un)
{
    if (m_unreachableDst.empty())
    {
        return false;
    }
    auto i = m_unreachableDst.begin();
    un = *i;
    m_unreachableDst.erase(i);
    return true;
}

template <typename T>
void
RerrHeader<T>::Clear()
{
    m_unreachableDst.clear();
}

template <typename T>
bool
RerrHeader<T>::operator==(const RerrHeader& o) const
{
    if (GetDestCount() != o.GetDestCount())
    {
        return false;
    }

    auto j = m_unreachableDst.begin();
    auto k = o.m_unreachableDst.begin();
    for (uint8_t i = 0; i < GetDestCount(); ++i)
    {
        if ((j->first != k->first) || (j->second.m_seqNo != k->second.m_seqNo &&
                                       j->second.m_metricType != k->second.m_metricType))
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
