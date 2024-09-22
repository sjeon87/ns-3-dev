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

#ifndef AODVV2_RERR_SET_H
#define AODVV2_RERR_SET_H

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
 * \brief maintain list of sent rerr messages
 */
template <typename T>
class RerrSet
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
    RerrSet();

    /// Rerr description
    struct Rerr
    {
        /// Rerr Time timeout
        Time m_timeout;
        /// Rerr T unreachable address
        T m_unreachableAddr;
        /// Rerr T packet source address
        T m_pktSource;

        /**
         * \brief Rerr structure constructor
         *
         * \param t Time timeout after which the entry should be removed
         * \param unreachableAddr T unreachable address
         * \param pktSource T the packet source address
         */
        Rerr(Time t, T unreachableAddr, T pktSource)
            : m_timeout(t),
              m_unreachableAddr(unreachableAddr),
              m_pktSource(pktSource)
        {
        }
    };

    /**
     * Return timeout for error packet with address addr, if exists, else return 0.
     * \param addr the IP address of the unreachable node
     * \returns the timeout for the error packet
     */
    Time GetTimeout(T addr);
    /**
     * Check that node with address addr is already in the list
     * \param unreachableAddr the unreachable address
     * \param pktSource the packet source address
     * \returns true if the node with inputs is in the list
     */
    bool HasRerr(T unreachableAddr, T pktSource);
    /**
     * Add new entry to the list
     * \param unreachableAddr the unreachable address
     * \param pktSource the packet source address
     * \param timeout the timeout for the entry
     */
    void Add(T unreachableAddr, T pktSource, Time timeout);

    /// Remove all entries
    void Clear()
    {
        m_rerr.clear();
    }

  private:
    /// vector of entries
    std::vector<Rerr> m_rerr;
};

} // namespace aodvv2
} // namespace ns3

#endif /* AODVV2_RERR_SET_H */
