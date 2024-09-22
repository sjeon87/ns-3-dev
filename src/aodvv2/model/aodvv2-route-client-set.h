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

#ifndef AODVV2_ROUTE_CLIENT_SET_H
#define AODVV2_ROUTE_CLIENT_SET_H

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
 * \brief maintain list of addresses used
 */
template <typename T>
class RouteClientSet
    : public std::enable_if_t<std::is_same_v<Ipv4Address, T> || std::is_same_v<Ipv6Address, T>, T>
{
    /// Alias for determining whether the parent is Ipv4Address or Ipv6Address
    static constexpr bool IsIpv4 = std::is_same_v<Ipv4Address, T>;

  public:
    /**
     * constructor
     */
    RouteClientSet();

    /// RouteClient description
    struct RouteClient
    {
        /// RouteClient T ip address
        T m_ip;
        /// RouteClient uint16_t mask
        uint16_t m_mask;
        /// RouteClient uint16_t cost
        uint16_t m_cost;

        /**
         * \brief RouteClient structure constructor
         *
         * \param ip T ip address
         * \param mask uint16_t mask
         * \param cost uint16_t cost
         */
        RouteClient(T ip, uint16_t mask, uint16_t cost)
            : m_ip(ip),
              m_mask(mask),
              m_cost(cost)
        {
        }
    };

    /**
     * Return cost for address addr, if exists, else return 0.
     * \param addr the IP address of the client
     * \returns the cost for the address
     */
    uint16_t GetCost(T addr);
    /**
     * Check that node with address ip is already in the list
     * \param ip the ip address
     * \returns true if the node with IP address is in the list
     */
    bool HasClient(T ip);
    /**
     * Add new entry to the list
     * \param ip the ip address
     * \param mask the mask of the address
     * \param cost the cost of the address
     */
    void Add(T ip, uint16_t mask, uint16_t cost);

    /// Remove all entries
    void Clear()
    {
        m_rc.clear();
    }

  private:
    /// vector of entries
    std::vector<RouteClient> m_rc;
};

} // namespace aodvv2
} // namespace ns3

#endif /* AODVV2_ROUTE_CLIENT_SET_H */
