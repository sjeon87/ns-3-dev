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
#ifndef AODVV2PACKET_H
#define AODVV2PACKET_H

#include "ns3/enum.h"
#include "ns3/header.h"
#include "ns3/ipv4-address.h"
#include "ns3/nstime.h"
#include "ns3/packetbb.h"

#include <iostream>
#include <map>

namespace ns3
{
namespace aodvv2
{

/**
 * \ingroup aodvv2
 * \brief MessageType enumeration
 */
enum MessageType
{
    AODVV2_TYPE_RREQ = 10,    //!< AODVV2_TYPE_RREQ
    AODVV2_TYPE_RREP = 11,    //!< AODVV2_TYPE_RREP
    AODVV2_TYPE_RERR = 12,    //!< AODVV2_TYPE_RERR
    AODVV2_TYPE_RREP_ACK = 13 //!< AODVV2_TYPE_RREP_ACK
};

/**
 * \ingroup aodvv2
 * \brief Message TLV Type
 */
enum MessageTlvType
{
    AODVV2_ACK_REQ = 128
};

/**
 * \ingroup aodvv2
 * \brief Address Block TLV Type
 */
enum AddressTlvType
{
    AODVV2_PATH_METRIC = 129,
    AODVV2_SEQ_NUM = 130,
    AODVV2_ADDRESS_TYPE = 131,
};

/**
 * \ingroup aodvv2
 * \brief Address Block TLV Value
 */
enum AddressTlvValue
{
    AODVV2_ORIGPREFIX = 0,
    AODVV2_TARGPREFIX = 1,
    AODVV2_UNREACHABLE = 2,
    AODVV2_PKTSOURCE = 3,
};

/// Unreachable destination structure
struct UnreachableDst
{
    uint16_t m_seqNo;
    uint8_t m_metricType;
};

/**
* \ingroup aodvv2
* \brief   Route Request (RREQ) Message Format
  \verbatim
  0                   1                   2                   3
  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                         msg_hop_limit                         |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                          AddressList                          |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                  PrefixLengthList (optional)                  |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |               OrigSeqNum, (optional) TargSeqNum               |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                          MetricType                           |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                          OrigMetric                           |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  \endverbatim
*/
template <typename T>
class RreqHeader : public Header

{
    /// Alias for determining whether the parent is Ipv4Address or Ipv6Address
    static constexpr bool IsIpv4 = std::is_same_v<Ipv4Address, T>;

    /// Alias for PbbMessageIpv4 and PbbMessageIpv6 classes
    using PbbMessageIp = typename std::conditional_t<IsIpv4, PbbMessageIpv4, PbbMessageIpv6>;
    /// Alias for PbbAddressBlockIpv4 and PbbAddressBlockIpv6 classes
    using PbbAddressBlockIp =
        typename std::conditional_t<IsIpv4, PbbAddressBlockIpv4, PbbAddressBlockIpv6>;

  public:
    /**
     * constructor
     * \param origIp the origin IP address
     * \param origMask the origin mask
     * \param targIp the target IP address
     * \param targMask the target mask
     * \param seqNo the sequence number
     * \param hopCount the hop count
     * \param maxHopCount the maximum hop count
     * \param metricType the metric type
     * \param origMetric the origin metric
     */
    RreqHeader(T origIp = T(),
               uint16_t origMask = 0,
               T targIp = T(),
               uint16_t targMask = 0,
               uint16_t seqNo = 1,
               uint8_t hopCount = 0,
               uint8_t maxHopCount = 20,
               uint8_t metricType = 0,
               uint8_t origMetric = 1);

    /**
     * constructor
     * \param tlvHeader the TLV header
     */
    RreqHeader(PbbPacket tlvHeader);

    /**
     * \brief Get the type ID.
     * \return the object TypeId
     */
    static TypeId GetTypeId();
    /**
     * \brief Create TLV header for RREQ
     */
    void CreateTlvHeader() const;
    /**
     * \brief Dispatch TLV header inside the RREQ header
     * \param tlvHeader the TLV header
     */
    void SetTlvHeader(PbbPacket tlvHeader);
    TypeId GetInstanceTypeId() const override;
    uint32_t GetSerializedSize() const override;
    void Serialize(Buffer::Iterator start) const override;
    uint32_t Deserialize(Buffer::Iterator start) override;
    void Print(std::ostream& os) const override;

    // Fields

    /**
     * \brief Set the router IP address
     * \param ip the router IP address
     */
    void SetRtrIp(T ip)
    {
        m_rtrIp = ip;
    }

    /**
     * \brief Get the router IP address
     * \return the router IP address
     */
    T GetRtrIp() const
    {
        return m_rtrIp;
    }

    /**
     * \brief Set the router mask
     * \param mask the router mask
     */
    void SetRtrMask(uint16_t mask)
    {
        m_rtrMask = mask;
    }

    /**
     * \brief Get the router mask
     * \return the router mask
     */
    uint16_t GetRtrMask() const
    {
        return m_rtrMask;
    }

    /**
     * \brief Set the origin IP address
     * \param ip the origin IP address
     */
    void SetOrigIp(T ip)
    {
        m_origIp = ip;
    }

    /**
     * \brief Get the origin IP address
     * \return the origin IP address
     */
    T GetOrigIp() const
    {
        return m_origIp;
    }

    /**
     * \brief Set the origin mask
     * \param mask the origin mask
     */
    void SetOrigMask(uint16_t mask)
    {
        m_origMask = mask;
    }

    /**
     * \brief Get the origin mask
     * \return the origin mask
     */
    uint16_t GetOrigMask() const
    {
        return m_origMask;
    }

    /**
     * \brief Set the origin sequence number
     * \param seq the origin sequence number
     */
    void SetOrigSeqNo(uint16_t seq)
    {
        m_origSeqNo = seq;
    }

    /**
     * \brief Set the origin sequence number
     * \return the origin sequence number
     */
    uint16_t GetOrigSeqNo() const
    {
        return m_origSeqNo;
    }

    /**
     * \brief Set the target IP address
     * \param ip the target IP address
     */
    void SetTargIp(T ip)
    {
        m_targIp = ip;
    }

    /**
     * \brief Get the target IP address
     * \return the target IP address
     */
    T GetTargIp() const
    {
        return m_targIp;
    }

    /**
     * \brief Set the target mask
     * \param mask the target mask
     */
    void SetTargMask(uint16_t mask)
    {
        m_targMask = mask;
    }

    /**
     * \brief Get the target mask
     * \return the target mask
     */
    uint16_t GetTargMask() const
    {
        return m_targMask;
    }

    /**
     * \brief Set the metric type
     * \param type the metric type
     */
    void SetMetricType(uint8_t type)
    {
        m_metricType = type;
    }

    /**
     * \brief Get the metric type
     * \return the metric type
     */
    uint8_t GetMetricType() const
    {
        return m_metricType;
    }

    /**
     * \brief Set the origin metric
     * \param metric the origin metric
     */
    void SetOrigMetric(uint8_t metric)
    {
        m_origMetric = metric;
    }

    /**
     * \brief Get the origin metric
     * \return the origin metric
     */
    uint8_t GetOrigMetric() const
    {
        return m_origMetric;
    }

    /**
     * \brief Set the target sequence number
     * \param seq the target sequence number
     */
    void SetTargSeqNo(uint16_t seq)
    {
        m_targSeqNo = seq;
    }

    /**
     * \brief Get the target sequence number
     * \return the target sequence number
     */
    uint16_t GetTargSeqNo() const
    {
        return m_targSeqNo;
    }

    /**
     * \brief Set the sequence number
     * \param seq the sequence number
     */
    void SetSeqNo(uint16_t seq)
    {
        m_seqNo = seq;
    }

    /**
     * \brief Get the sequence number
     * \return the sequence number
     */
    uint16_t GetSeqNo() const
    {
        return m_seqNo;
    }

    /**
     * \brief Set the hop count
     * \param count the hop count
     */
    void SetHopCount(uint8_t count)
    {
        m_hopCount = count;
    }

    /**
     * \brief Get the hop count
     * \return the hop count
     */
    uint8_t GetHopCount() const
    {
        return m_hopCount;
    }

    /**
     * \brief Set the send target sequence number flag
     * \param send the send target sequence number flag
     */
    void SetSendTargSeqNum(bool send)
    {
        m_sendTargSeqNum = send;
    }

    /**
     * \brief Get the send target sequence number flag
     * \return the send target sequence number flag
     */
    bool GetSendTargSeqNum() const
    {
        return m_sendTargSeqNum;
    }

    /**
     * \brief Comparison operator
     * \param o RREQ header to compare
     * \return true if the RREQ headers are equal
     */
    bool operator==(const RreqHeader& o) const;

  private:
    T m_rtrIp;            ///< Router IP Address
    uint16_t m_rtrMask;   ///< Router Mask
    T m_origIp;           ///< Origin IP Address
    uint16_t m_origMask;  ///< Origin Mask
    uint16_t m_origSeqNo; ///< Origin Sequence number
    T m_targIp;           ///< Target IP Address
    uint16_t m_targMask;  ///< Target Mask
    uint16_t m_targSeqNo; ///< Target Sequence number

    uint8_t m_metricType; ///< Metric Type
    uint8_t m_origMetric; ///< Origin Path Metric
    uint16_t m_seqNo;     ///< Sequence number
    uint8_t m_hopCount;   ///< Hop Count

    uint8_t m_maxHopCount;              ///< Max Hop Count
    bool m_sendTargSeqNum;              ///< Send Target Sequence Number
    mutable Ptr<PbbPacket> m_tlvHeader; ///< TLV header
};

/**
 * \brief Stream output operator
 * \param os output stream
 * \return updated stream
 */
template <typename T>
std::ostream& operator<<(std::ostream& os, const RreqHeader<T>&);

/**
* \ingroup aodvv2
* \brief Route Reply (RREP) Message Format
  \verbatim
  0                   1                   2                   3
  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                         msg_hop_limit                         |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                          AddressList                          |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                  PrefixLengthList (optional)                  |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                          TargSeqNum                           |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                          MetricType                           |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                          TargMetric                           |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  \endverbatim
*/
template <typename T>
class RrepHeader : public Header
{
    /// Alias for determining whether the parent is Ipv4Address or Ipv6Address
    static constexpr bool IsIpv4 = std::is_same_v<Ipv4Address, T>;

    /// Alias for PbbMessageIpv4 and PbbMessageIpv6 classes
    using PbbMessageIp = typename std::conditional_t<IsIpv4, PbbMessageIpv4, PbbMessageIpv6>;
    /// Alias for PbbAddressBlockIpv4 and PbbAddressBlockIpv6 classes
    using PbbAddressBlockIp =
        typename std::conditional_t<IsIpv4, PbbAddressBlockIpv4, PbbAddressBlockIpv6>;

  public:
    /**
     * constructor
     *
     * \param origIp the origin IP address
     * \param origMask the origin mask
     * \param targIp the target IP address
     * \param targMask the target mask
     * \param seqNo the sequence number
     * \param hopCount the hop count
     * \param maxHopCount the maximum hop count
     * \param metricType the metric type
     * \param targMetric the target metric
     */
    RrepHeader(T origIp = T(),
               uint16_t origMask = 0,
               T targIp = T(),
               uint16_t targMask = 0,
               uint16_t seqNo = 1,
               uint8_t hopCount = 0,
               uint8_t maxHopCount = 20,
               uint8_t metricType = 0,
               uint8_t targMetric = 1);
    /**
     * constructor
     * \param tlvHeader the TLV header
     */
    RrepHeader(PbbPacket tlvHeader);

    /**
     * \brief Get the type ID.
     * \return the object TypeId
     */
    static TypeId GetTypeId();
    /**
     * \brief Create TLV header for RREP
     */
    void CreateTlvHeader() const;
    /**
     * \brief Dispatch TLV header inside the RREP header
     * \param tlvHeader the TLV header
     */
    void SetTlvHeader(PbbPacket tlvHeader);
    TypeId GetInstanceTypeId() const override;
    uint32_t GetSerializedSize() const override;
    void Serialize(Buffer::Iterator start) const override;
    uint32_t Deserialize(Buffer::Iterator start) override;
    void Print(std::ostream& os) const override;

    // Fields
    /**
     * \brief Set the origin IP address
     * \param ip the origin IP address
     */
    void SetOrigIp(T ip)
    {
        m_origIp = ip;
    }

    /**
     * \brief Get the origin IP address
     * \return the origin IP address
     */
    T GetOrigIp() const
    {
        return m_origIp;
    }

    /**
     * \brief Set the origin mask
     * \param mask the origin mask
     */
    void SetOrigMask(uint16_t mask)
    {
        m_origMask = mask;
    }

    /**
     * \brief Get the origin mask
     * \return the origin mask
     */
    uint16_t GetOrigMask() const
    {
        return m_origMask;
    }

    /**
     * \brief Set the target IP address
     * \param ip the target IP address
     */
    void SetTargIp(T ip)
    {
        m_targIp = ip;
    }

    /**
     * \brief Get the target IP address
     * \return the target IP address
     */
    T GetTargIp() const
    {
        return m_targIp;
    }

    /**
     * \brief Set the target mask
     * \param mask the target mask
     */
    void SetTargMask(uint16_t mask)
    {
        m_targMask = mask;
    }

    /**
     * \brief Get the target mask
     * \return the target mask
     */
    uint16_t GetTargMask() const
    {
        return m_targMask;
    }

    /**
     * \brief Set the target sequence number
     * \param seq the target sequence number
     */
    void SetTargSeqNo(uint16_t seq)
    {
        m_targSeqNo = seq;
    }

    /**
     * \brief Get the target sequence number
     * \return the target sequence number
     */
    uint16_t GetTargSeqNo() const
    {
        return m_targSeqNo;
    }

    /**
     * \brief Set the metric type
     * \param type the metric type
     */
    void SetMetricType(uint8_t type)
    {
        m_metricType = type;
    }

    /**
     * \brief Get the metric type
     * \return the metric type
     */
    uint8_t GetMetricType() const
    {
        return m_metricType;
    }

    /**
     * \brief Set the target metric
     * \param metric the target metric
     */
    void SetTargMetric(uint8_t metric)
    {
        m_targMetric = metric;
    }

    /**
     * \brief Get the target metric
     * \return the target metric
     */
    uint8_t GetTargMetric() const
    {
        return m_targMetric;
    }

    /**
     * \brief Set the sequence number
     * \param seq the sequence number
     */
    void SetSeqNo(uint16_t seq)
    {
        m_seqNo = seq;
    }

    /**
     * \brief Get the sequence number
     * \return the sequence number
     */
    uint16_t GetSeqNo() const
    {
        return m_seqNo;
    }

    /**
     * \brief Set the hop count
     * \param count the hop count
     */
    void SetHopCount(uint8_t count)
    {
        m_hopCount = count;
    }

    /**
     * \brief Get the hop count
     * \return the hop count
     */
    uint8_t GetHopCount() const
    {
        return m_hopCount;
    }

    /**
     * \brief Comparison operator
     * \param o RREP header to compare
     * \return true if the RREP headers are equal
     */
    bool operator==(const RrepHeader& o) const;

  private:
    T m_origIp;           ///< Origin IP Address
    uint16_t m_origMask;  ///< Origin Mask
    T m_targIp;           ///< Target IP Address
    uint16_t m_targMask;  ///< Target Mask
    uint16_t m_targSeqNo; ///< Target Sequence number
    uint8_t m_metricType; ///< Metric Type
    uint8_t m_targMetric; ///< Target Path Metric
    uint16_t m_seqNo;     ///< Sequence number
    uint8_t m_hopCount;   ///< Hop Count

    uint8_t m_maxHopCount;              ///< Max Hop Count
    mutable Ptr<PbbPacket> m_tlvHeader; ///< TLV header
};

/**
 * \brief Stream output operator
 * \param os output stream
 * \return updated stream
 */
template <typename T>
std::ostream& operator<<(std::ostream& os, const RrepHeader<T>&);

/**
* \ingroup aodvv2
* \brief Route Reply Acknowledgment (RREP-ACK) Message Format
  \verbatim
  0                   1
  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |       AckReq (optional)       |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  \endverbatim
*/
template <typename T>
class RrepAckHeader : public Header
{
    /// Alias for determining whether the parent is Ipv4Address or Ipv6Address
    static constexpr bool IsIpv4 = std::is_same_v<Ipv4Address, T>;

    /// Alias for PbbMessageIpv4 and PbbMessageIpv6 classes
    using PbbMessageIp = typename std::conditional_t<IsIpv4, PbbMessageIpv4, PbbMessageIpv6>;

  public:
    /**
     * constructor
     *
     * \param maxHopCount the maximum hop count
     */
    RrepAckHeader(uint8_t maxHopCount = 20);

    /**
     * \brief Get the type ID.
     * \return the object TypeId
     */
    static TypeId GetTypeId();
    /**
     * \brief Create TLV header for RREP_ACK
     */
    void CreateTlvHeader() const;
    /**
     * \brief Dispatch TLV header inside the RREP_ACK header
     * \param tlvHeader the TLV header
     */
    void SetTlvHeader(PbbPacket tlvHeader);
    TypeId GetInstanceTypeId() const override;
    uint32_t GetSerializedSize() const override;
    void Serialize(Buffer::Iterator start) const override;
    uint32_t Deserialize(Buffer::Iterator start) override;
    void Print(std::ostream& os) const override;

    /**
     * \brief Set the sequence number
     * \param seq the sequence number
     */
    void SetSeqNo(uint16_t seq)
    {
        m_seqNo = seq;
    }

    /**
     * \brief Get the sequence number
     * \return the sequence number
     */
    uint16_t GetSeqNo() const
    {
        return m_seqNo;
    }

    /**
     * \brief Comparison operator
     * \param o RREP header to compare
     * \return true if the RREQ headers are equal
     */
    bool operator==(const RrepAckHeader& o) const;

  private:
    uint16_t m_seqNo; ///< Sequence number

    uint8_t m_maxHopCount;              ///< Max Hop Count
    mutable Ptr<PbbPacket> m_tlvHeader; ///< TLV header
};

/**
 * \brief Stream output operator
 * \param os output stream
 * \return updated stream
 */
template <typename T>
std::ostream& operator<<(std::ostream& os, const RrepAckHeader<T>&);

/**
* \ingroup aodvv2
* \brief Route Error (RERR) Message Format
  \verbatim
  0                   1                   2                   3
  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                      PktSource (optional)                     |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                          AddressList                          |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                 PrefixLengthList (optional)                   |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-|
  |                     SeqNumList (optional)                     |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                         MetricTypeList                        |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  \endverbatim
*/
template <typename T>
class RerrHeader : public Header
{
    /// Alias for determining whether the parent is Ipv4Address or Ipv6Address
    static constexpr bool IsIpv4 = std::is_same_v<Ipv4Address, T>;
    /// Alias for PbbMessageIpv4 and PbbMessageIpv6 classes
    using PbbMessageIp = typename std::conditional_t<IsIpv4, PbbMessageIpv4, PbbMessageIpv6>;
    /// Alias for PbbAddressBlockIpv4 and PbbAddressBlockIpv6 classes
    using PbbAddressBlockIp =
        typename std::conditional_t<IsIpv4, PbbAddressBlockIpv4, PbbAddressBlockIpv6>;

  public:
    /**
     * constructor
     *
     * \param maxHopCount the maximum hop count
     */
    RerrHeader(uint8_t maxHopCount = 20);
    /**
     * constructor
     * \param tlvHeader the TLV header
     * \param maxHopCount the maximum hop count
     */
    RerrHeader(PbbPacket tlvHeader, uint8_t maxHopCount = 20);

    /**
     * \brief Get the type ID.
     * \return the object TypeId
     */
    static TypeId GetTypeId();
    /**
     * \brief Create TLV header for RRER
     */
    void CreateTlvHeader() const;
    /**
     * \brief Dispatch TLV header inside the RRER header
     * \param tlvHeader the TLV header
     */
    void SetTlvHeader(PbbPacket tlvHeader);
    TypeId GetInstanceTypeId() const override;
    uint32_t GetSerializedSize() const override;
    void Serialize(Buffer::Iterator i) const override;
    uint32_t Deserialize(Buffer::Iterator start) override;
    void Print(std::ostream& os) const override;

    // Fields
    /**
     * \brief Set the origin IP address
     * \param ip the origin IP address
     */
    void SetOrigIp(T ip)
    {
        m_origIp = ip;
    }

    /**
     * \brief Get the origin IP address
     * \return the origin IP address
     */
    T GetOrigIp() const
    {
        return m_origIp;
    }

    /**
     * \brief Set the origin mask
     * \param mask the origin mask
     */
    void SetOrigMask(uint16_t mask)
    {
        m_origMask = mask;
    }

    /**
     * \brief Get the origin mask
     * \return the origin mask
     */
    uint16_t GetOrigMask() const
    {
        return m_origMask;
    }

    /**
     * \brief Add unreachable node address and its sequence number in RERR header
     * \param dst unreachable IP address
     * \param seqNo unreachable sequence number
     * \param metricType metric type
     * \return false if we already added maximum possible number of unreachable destinations
     */
    bool AddUnDestination(T dst, uint16_t seqNo, uint8_t metricType = 0);
    /**
     * \brief Delete pair (address + sequence number) from REER header, if the number of unreachable
     * destinations > 0
     * \param un unreachable pair (address + sequence number)
     * \return true on success
     */
    bool RemoveUnDestination(std::pair<T, UnreachableDst>& un);
    /// Clear header
    void Clear();

    /**
     * \returns number of unreachable destinations in RERR message
     */
    uint8_t GetDestCount() const
    {
        return (uint8_t)m_unreachableDst.size();
    }

    /**
     * \brief Comparison operator
     * \param o RERR header to compare
     * \return true if the RERR headers are equal
     */
    bool operator==(const RerrHeader& o) const;

  private:
    /// List of Unreachable destination: IP addresses, sequence numbers and metric type
    std::map<T, UnreachableDst> m_unreachableDst;

    T m_origIp;          ///< Origin IP Address
    uint16_t m_origMask; ///< Origin Mask

    uint8_t m_maxHopCount;              ///< Max Hop Count
    mutable Ptr<PbbPacket> m_tlvHeader; ///< TLV header
};

/**
 * \brief Stream output operator
 * \param os output stream
 * \return updated stream
 */
template <typename T>
std::ostream& operator<<(std::ostream& os, const RerrHeader<T>&);

} // namespace aodvv2
} // namespace ns3

#endif /* AODVV2PACKET_H */
