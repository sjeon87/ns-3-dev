/*
 * Copyright (c) 2009 Drexel University
 *
 * SPDX-License-Identifier: GPL-2.0-only and NIST-Software
 *
 * Author: Tom Wambold <tom5760@gmail.com>
 */

#ifndef NHDP_INFO_BASE_H
#define NHDP_INFO_BASE_H

#include "ns3/address.h"
#include "ns3/nstime.h"

#include <stdint.h>
#include <utility>
#include <vector>

namespace ns3
{

namespace manet
{

enum class LinkStatus
{
    PENDING,
    HEARD,
    SYMMETRIC,
    LOST
};

/**
 * @ingroup nhdp
 * @brief Neighbor Tuple (RFC 6130, Sec. 8.1)
 *
 * Records all network addresses of a single 1-hop neighbor.  Addresses are
 * stored as generic ns3::Address so that the same structure serves IPv4 and
 * IPv6.  A neighbor may be reached at more than one address (e.g. an IPv6
 * link-local plus a routing Unique Local Address, RFC 4193).
 */
struct NeighborTuple
{
    NeighborTuple() = default;

    /**
     * Construct a Neighbor Tuple with a single address.
     * @param addr The neighbor address.
     */
    explicit NeighborTuple(const Address& addr)
    {
        m_neighborAddrList.push_back(addr);
    }

    /**
     * Construct a Neighbor Tuple from a list of addresses.
     * @param addrs The neighbor addresses (N_neighbor_addr_list).
     */
    explicit NeighborTuple(std::vector<Address> addrs)
        : m_neighborAddrList(std::move(addrs))
    {
    }

    std::vector<Address> m_neighborAddrList; ///< N_neighbor_addr_list
    bool m_symmetric{false};                 ///< N_symmetric
};

/**
 * @ingroup nhdp
 * @brief Lost Neighbor Tuple (RFC 6130, Sec. 8.2)
 *
 * Records a single network address of a router that recently was a symmetric
 * 1-hop neighbor but is now advertised as lost.
 */
struct LostNeighborTuple
{
    Address m_neighborAddr; ///< NL_neighbor_addr
    Time m_expirationTime;  ///< NL_time
};

/**
 * @ingroup nhdp
 * @brief Link Tuple (RFC 6130, Sec. 7.1)
 *
 * Represents a single link from a 1-hop neighbor.  The neighbor's MANET
 * interface may have more than one address, recorded in
 * L_neighbor_iface_addr_list.
 */
struct LinkTuple
{
    LinkTuple() = default;

    /**
     * Construct a Link Tuple with a single neighbor address.
     * @param addr The neighbor interface address.
     */
    explicit LinkTuple(const Address& addr)
    {
        m_neighborAddrList.push_back(addr);
    }

    /**
     * Construct a Link Tuple from a list of neighbor addresses.
     * @param addrs The neighbor interface addresses (L_neighbor_iface_addr_list).
     */
    explicit LinkTuple(std::vector<Address> addrs)
        : m_neighborAddrList(std::move(addrs))
    {
    }

    /**
     * Get the link status (L_status), per RFC 6130, Sec. 7.1.
     * @return The link status.
     */
    LinkStatus GetLinkStatus() const;

    std::vector<Address> m_neighborAddrList; ///< L_neighbor_iface_addr_list
    Time m_heardTime;                        ///< L_HEARD_time
    Time m_symTime;                          ///< L_SYM_time
    double m_quality{0};                     ///< L_quality
    bool m_pending{false};                   ///< L_pending
    bool m_lost{false};                      ///< L_lost
    Time m_expirationTime;                   ///< L_time
};

/**
 * @ingroup nhdp
 * @brief 2-Hop Tuple (RFC 6130, Sec. 7.2)
 *
 * Represents a single symmetric 2-hop neighbor address reachable via the
 * symmetric 1-hop neighbor whose interface addresses are in
 * N2_neighbor_iface_addr_list.
 */
struct TwoHopTuple
{
    TwoHopTuple() = default;

    /**
     * Construct a 2-Hop Tuple with a single 1-hop neighbor address.
     * @param via The 1-hop neighbor interface address.
     * @param twoHopAddr The 2-hop neighbor address.
     */
    TwoHopTuple(const Address& via, const Address& twoHopAddr)
        : m_twoHopAddr(twoHopAddr)
    {
        m_neighborAddrList.push_back(via);
    }

    /**
     * Construct a 2-Hop Tuple from a list of 1-hop neighbor addresses.
     * @param via The 1-hop neighbor interface addresses (N2_neighbor_iface_addr_list).
     * @param twoHopAddr The 2-hop neighbor address.
     */
    TwoHopTuple(std::vector<Address> via, const Address& twoHopAddr)
        : m_neighborAddrList(std::move(via)),
          m_twoHopAddr(twoHopAddr)
    {
    }

    std::vector<Address> m_neighborAddrList; ///< N2_neighbor_iface_addr_list
    Address m_twoHopAddr;                    ///< N2_2hop_addr
    Time m_expirationTime;                   ///< N2_time
};

std::ostream& operator<<(std::ostream& os, const LinkStatus& status);

} // namespace manet

} // namespace ns3

#endif /* NHDP_INFO_BASE_H */
