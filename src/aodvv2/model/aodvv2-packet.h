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
 * \brief AODVv2 enums
 */
enum Aodvv2Type
{
    AODVV2_MAX_HOP_COUNT = 20, //!< MAX_HOP_COUNT
};

/**
 * \ingroup aodvv2
 * \brief AODVv2 timers
 */
enum Aodvv2Timers
{
    AODVV2_ACTIVE_INTERVAL = 5,       //!< ACTIVE_INTERVAL
    AODVV2_MAX_IDLETIME = 200,        //!< MAX_IDLETIME
    AODVV2_MAX_BLACKLIST_TIME = 200,  //!< MAX_BLACKLIST_TIME
    AODVV2_MAX_SEQNUM_LIFETIME = 300, //!< MAX_SEQNUM_LIFETIME
    AODVV2_RERR_TIMEOUT = 3,          //!< RERR_TIMEOUT
    AODVV2_RTEMSG_ENTRY_TIME = 12,    //!< RteMsg_ENTRY_TIME
    AODVV2_RREQ_WAIT_TIME = 2,        //!< RREQ_WAIT_TIME
    AODVV2_RREP_ACK_SENT_TIMEOUT = 1, //!< RREP_Ack_SENT_TIMEOUT
    AODVV2_RREQ_HOLDDOWN_TIME = 10,   //!< RREQ_HOLDDOWN_TIME
};

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

/**
* \ingroup aodv
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
class RreqHeader : public Header

{
  public:
    /**
     * constructor
     * \param origIp the origin IP address
     * \param origMask the origin mask
     * \param targIp the target IP address
     * \param targMask the target mask
     * \param seqNo the sequence number
     * \param hopCount the hop count
     */
    RreqHeader(Ipv4Address origIp = Ipv4Address(),
               uint16_t origMask = 0,
               Ipv4Address targIp = Ipv4Address(),
               uint16_t targMask = 0,
               uint32_t seqNo = 0,
               uint8_t hopCount = 0);

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
     * \brief Set the origin IP address
     * \param ip the origin IP address
     */
    void SetOrigIp(Ipv4Address ip)
    {
        m_origIp = ip;
    }

    /**
     * \brief Get the origin IP address
     * \return the origin IP address
     */
    Ipv4Address GetOrigIp() const
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
    void SetTargIp(Ipv4Address ip)
    {
        m_targIp = ip;
    }

    /**
     * \brief Get the target IP address
     * \return the target IP address
     */
    Ipv4Address GetTargIp() const
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
     * \brief Set the sequence number
     * \param seq the sequence number
     */
    void SetSeqNo(uint32_t seq)
    {
        m_seqNo = seq;
    }

    /**
     * \brief Get the sequence number
     * \return the sequence number
     */
    uint32_t GetSeqNo() const
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
     * \param o RREQ header to compare
     * \return true if the RREQ headers are equal
     */
    bool operator==(const RreqHeader& o) const;

  private:
    Ipv4Address m_origIp; ///< Origin IP Address
    uint16_t m_origMask;  ///< Origin Mask
    Ipv4Address m_targIp; ///< Target IP Address
    uint16_t m_targMask;  ///< Target Mask
    uint8_t m_seqNo;      ///< Sequence number
    uint8_t m_hopCount;   ///< Hop Count

    mutable Ptr<PbbPacket> m_tlvHeader; ///< TLV header
};

/**
 * \brief Stream output operator
 * \param os output stream
 * \return updated stream
 */
std::ostream& operator<<(std::ostream& os, const RreqHeader&);

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
class RrepHeader : public Header
{
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
     */
    RrepHeader(Ipv4Address origIp = Ipv4Address(),
               uint16_t origMask = 0,
               Ipv4Address targIp = Ipv4Address(),
               uint16_t targMask = 0,
               uint32_t seqNo = 0,
               uint8_t hopCount = 0);
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
    void SetOrigIp(Ipv4Address ip)
    {
        m_origIp = ip;
    }

    /**
     * \brief Get the origin IP address
     * \return the origin IP address
     */
    Ipv4Address GetOrigIp() const
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
    void SetTargIp(Ipv4Address ip)
    {
        m_targIp = ip;
    }

    /**
     * \brief Get the target IP address
     * \return the target IP address
     */
    Ipv4Address GetTargIp() const
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
     * \brief Set the sequence number
     * \param seq the sequence number
     */
    void SetSeqNo(uint32_t seq)
    {
        m_seqNo = seq;
    }

    /**
     * \brief Get the sequence number
     * \return the sequence number
     */
    uint32_t GetSeqNo() const
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
     * \brief Get the destination sequence number
     * \return the destination sequence number
     */
    uint32_t GetDstSeqno() const
    {
        return 0; // TODO
    }

    /**
     * \brief Get the lifetime
     * \return the lifetime
     */
    Time GetLifeTime() const
    {
        Time t(MilliSeconds(0)); // TODO
        return t;
    }

    /**
     * \brief Comparison operator
     * \param o RREP header to compare
     * \return true if the RREP headers are equal
     */
    bool operator==(const RrepHeader& o) const;

  private:
    Ipv4Address m_origIp;               ///< Origin IP Address
    uint16_t m_origMask;                ///< Origin Mask
    Ipv4Address m_targIp;               ///< Target IP Address
    uint16_t m_targMask;                ///< Target Mask
    uint8_t m_seqNo;                    ///< Sequence number
    uint8_t m_hopCount;                 ///< Hop Count
    mutable Ptr<PbbPacket> m_tlvHeader; ///< TLV header
};

/**
 * \brief Stream output operator
 * \param os output stream
 * \return updated stream
 */
std::ostream& operator<<(std::ostream& os, const RrepHeader&);

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
class RrepAckHeader : public Header
{
  public:
    /// constructor
    RrepAckHeader();

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
    void SetSeqNo(uint32_t seq)
    {
        m_seqNo = seq;
    }

    /**
     * \brief Get the sequence number
     * \return the sequence number
     */
    uint32_t GetSeqNo() const
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
    uint8_t m_reserved;                 ///< Not used (must be 0)
    uint8_t m_seqNo;                    ///< Sequence number
    mutable Ptr<PbbPacket> m_tlvHeader; ///< TLV header
};

/**
 * \brief Stream output operator
 * \param os output stream
 * \return updated stream
 */
std::ostream& operator<<(std::ostream& os, const RrepAckHeader&);

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
class RerrHeader : public Header
{
  public:
    /// constructor
    RerrHeader();

    /**
     * \brief Get the type ID.
     * \return the object TypeId
     */
    static TypeId GetTypeId();
    TypeId GetInstanceTypeId() const override;
    uint32_t GetSerializedSize() const override;
    void Serialize(Buffer::Iterator i) const override;
    uint32_t Deserialize(Buffer::Iterator start) override;
    void Print(std::ostream& os) const override;

    // No delete flag
    /**
     * \brief Set the no delete flag
     * \param f the no delete flag
     */
    void SetNoDelete(bool f);
    /**
     * \brief Get the no delete flag
     * \return the no delete flag
     */
    bool GetNoDelete() const;

    /**
     * \brief Add unreachable node address and its sequence number in RERR header
     * \param dst unreachable IPv4 address
     * \param seqNo unreachable sequence number
     * \return false if we already added maximum possible number of unreachable destinations
     */
    bool AddUnDestination(Ipv4Address dst, uint32_t seqNo);
    /**
     * \brief Delete pair (address + sequence number) from REER header, if the number of unreachable
     * destinations > 0
     * \param un unreachable pair (address + sequence number)
     * \return true on success
     */
    bool RemoveUnDestination(std::pair<Ipv4Address, uint32_t>& un);
    /// Clear header
    void Clear();

    /**
     * \returns number of unreachable destinations in RERR message
     */
    uint8_t GetDestCount() const
    {
        return (uint8_t)m_unreachableDstSeqNo.size();
    }

    /**
     * \brief Comparison operator
     * \param o RERR header to compare
     * \return true if the RERR headers are equal
     */
    bool operator==(const RerrHeader& o) const;

  private:
    uint8_t m_flag;     ///< No delete flag
    uint8_t m_reserved; ///< Not used (must be 0)

    /// List of Unreachable destination: IP addresses and sequence numbers
    std::map<Ipv4Address, uint32_t> m_unreachableDstSeqNo;
};

/**
 * \brief Stream output operator
 * \param os output stream
 * \return updated stream
 */
std::ostream& operator<<(std::ostream& os, const RerrHeader&);

} // namespace aodvv2
} // namespace ns3

#endif /* AODVV2PACKET_H */
