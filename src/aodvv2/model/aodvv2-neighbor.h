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

#ifndef AODVV2NEIGHBOR_H
#define AODVV2NEIGHBOR_H

#include "ns3/arp-cache.h"
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

class RoutingProtocol;

/**
 * \ingroup aodvv2
 * \brief maintain list of active neighbors
 */
template <typename T>
class Neighbors
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
     * \param delay the delay time for purging the list of neighbors
     */
    Neighbors(Time delay);

    /// Neighbor description
    struct Neighbor
    {
        /// Neighbor T address
        T m_neighborAddress;
        /// Neighbor state
        NeighborStates m_state;
        /// Neighbor timeout
        Time m_timeout;
        /// Neighbor interface
        IpInterfaceAddress m_interface;
        /// Neighbor Ack Sequence Number
        uint32_t m_ackSeqNo;
        /// Neighbor Heard RERR Sequence Number
        uint32_t m_heardRERRSeqNo;

        /**
         * \brief Neighbor structure constructor
         *
         * \param ip T entry
         * \param interface IpInterfaceAddress entry
         * \param t Time timeout
         */
        Neighbor(T ip, IpInterfaceAddress interface, Time t)
            : m_neighborAddress(ip),
              m_state(HEARD),
              m_timeout(t),
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
     * Check that node with address addr is neighbor
     * \param addr the IP address to check
     * \returns true if the node with IP address is a neighbor
     */
    bool IsNeighbor(T addr);
    /**
     * Update timeout for entry with address addr, if it exists, else add new entry
     * \param addr the IP address to check
     * \param timeout the timeout for the address
     */
    void Update(T addr, Time timeout);
    /// Remove all expired entries
    void Purge();
    /// Schedule m_ntimer.
    void ScheduleTimer();

    /// Remove all entries
    void Clear()
    {
        m_nb.clear();
    }

    /**
     * Add ARP cache to be used to allow layer 2 notifications processing
     * \param a pointer to the ARP cache to add
     */
    void AddArpCache(Ptr<ArpCache> a);
    /**
     * Don't use given ARP cache any more (interface is down)
     * \param a pointer to the ARP cache to delete
     */
    void DelArpCache(Ptr<ArpCache> a);

    /**
     * Get callback to ProcessTxError
     * \returns the callback function
     */
    Callback<void, const Header&> GetTxErrorCallback() const
    {
        return m_txErrorCallback;
    }

    /**
     * Set link failure callback
     * \param cb the callback function
     */
    void SetCallback(Callback<void, T> cb)
    {
        m_handleLinkFailure = cb;
    }

    /**
     * Get link failure callback
     * \returns the link failure callback
     */
    Callback<void, T> GetCallback() const
    {
        return m_handleLinkFailure;
    }

  private:
    /// link failure callback
    Callback<void, T> m_handleLinkFailure;
    /// TX error callback
    Callback<void, const Header&> m_txErrorCallback;
    /// Timer for neighbor's list. Schedule Purge().
    Timer m_ntimer;
    /// vector of entries
    std::vector<Neighbor> m_nb;
    /// list of ARP cached to be used for layer 2 notifications processing
    std::vector<Ptr<ArpCache>> m_arp;
};

} // namespace aodvv2
} // namespace ns3

#endif /* AODVV2NEIGHBOR_H */
