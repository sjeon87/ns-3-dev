/*
 * Copyright (c) 2024 University of Florence
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Based on
 *      NS-3 AODV model developed by Elena Buchatskaya and Pavel Boyko of IITP RAS
 *
 * Authors: Francesco Todino <francesco.todino@edu.unifi.it>
 *          Tommaso Pecorella <tommaso.pecorella@unifi.it>
 */

#ifndef AODVV2_NEIGHBOR_H
#define AODVV2_NEIGHBOR_H

#include "ns3/callback.h"
#include "ns3/internet-module.h"
#include "ns3/simulator.h"
#include "ns3/timer.h"

#include <vector>

namespace ns3
{

namespace aodvv2
{
/**
 * \ingroup aodvv2
 * \brief Route record states
 */
enum NeighborStates
{
    BLACKLISTED = 0, //!< link is invalid and unidirectional
    HEARD = 1,       //!< initial state
    CONFIRMED = 2,   //!< link is valid and bidirectional
};

const Time INFINITY_TIME = Seconds(99999);

class RoutingProtocol;

/**
 * \ingroup aodvv2
 * \brief maintain list of active neighbors
 */
template <typename T>
class NeighborSet
    : public std::enable_if_t<std::is_same_v<Ipv4Address, T> || std::is_same_v<Ipv6Address, T>, T>
{
    /// Alias for determining whether the parent is Ipv4Address or Ipv6Address
    static constexpr bool IsIpv4 = std::is_same_v<Ipv4Address, T>;
    /// Alias for Ipv4InterfaceAddress and Ipv6InterfaceAddress classes
    using IpInterfaceAddress =
        typename std::conditional_t<IsIpv4, Ipv4InterfaceAddress, Ipv6InterfaceAddress>;

  public:
    /**
     * constructor
     */
    NeighborSet(Time maxBlacklistTime);

    /// Neighbor description
    struct Neighbor
    {
        /// Neighbor T address
        T m_neighborAddress;
        /// Neighbor NeighborStates state
        NeighborStates m_state;
        /// Neighbor Time timeout
        Time m_timeout;
        /// Neighbor IpInterfaceAddress interface
        IpInterfaceAddress m_interface;
        /// Neighbor uint16_t Ack Sequence Number
        uint16_t m_ackSeqNo;
        /// Neighbor uint16_t Heard RERR Sequence Number
        uint16_t m_heardRERRSeqNo;

        /**
         * \brief Neighbor structure constructor
         *
         * \param ip T entry
         * \param interface IpInterfaceAddress entry
         * \param t Time timeout
         */
        Neighbor(T ip, IpInterfaceAddress interface)
            : m_neighborAddress(ip),
              m_state(HEARD),
              m_timeout(Simulator::Now() + INFINITY_TIME),
              m_interface(interface),
              m_ackSeqNo(rand() % 1000),
              m_heardRERRSeqNo(0)
        {
        }
    };

    /**
     * Return timeout for neighbor node with address addr, if exists, else return 0.
     * \param addr the IP address of the neighbor node
     * \returns the timeout for the neighbor node
     */
    Time GetTimeout(T addr);
    /**
     * Return state for neighbor node with address addr.
     * \param addr the IP address of the neighbor node
     * \returns the state for the neighbor node
     */
    NeighborStates GetState(T addr);
    /**
     * Check that node with address addr is neighbor
     * \param addr the IP address to check
     * \returns true if the node with IP address is a neighbor
     */
    bool IsNeighbor(T addr);
    /**
     * Update timeout for entry with address addr, if it exists, else add new entry
     * \param addr the IP address to check
     * \param iface the interface address
     */
    void AddNeighbor(T addr, IpInterfaceAddress iface);
    /**
     * Update state for entry
     * \param addr the IP address to check
     * \param iface the interface address
     * \param timeout the timeout for the address
     */
    void UpdateState(T addr, IpInterfaceAddress iface, Time timeout);

    /// Remove all entries
    void Clear()
    {
        m_nb.clear();
    }

  private:
    /// vector of entries
    std::vector<Neighbor> m_nb;
    /// max blacklist time
    Time m_maxBlacklistTime;
};

} // namespace aodvv2
} // namespace ns3

#endif /* AODVV2_NEIGHBOR_H */
