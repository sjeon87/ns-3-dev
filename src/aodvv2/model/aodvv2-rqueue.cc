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
#include "aodvv2-rqueue.h"

#include "ns3/ipv4-route.h"
#include "ns3/log.h"
#include "ns3/socket.h"

#include <algorithm>
#include <functional>

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("Aodvv2RequestQueue");

namespace aodvv2
{
template <typename T>
uint32_t
RequestQueue<T>::GetSize()
{
    Purge();
    return m_queue.size();
}

template <typename T>
bool
RequestQueue<T>::Enqueue(QueueEntry<IpHeader>& entry)
{
    Purge();
    for (auto i = m_queue.begin(); i != m_queue.end(); ++i)
    {
        if ((i->GetPacket()->GetUid() == entry.GetPacket()->GetUid()) &&
            (i->GetIpHeader().GetDestination() == entry.GetIpHeader().GetDestination()))
        {
            return false;
        }
    }
    entry.SetExpireTime(m_queueTimeout);
    if (m_queue.size() == m_maxLen)
    {
        Drop(m_queue.front(), "Drop the most aged packet"); // Drop the most aged packet
        m_queue.erase(m_queue.begin());
    }
    m_queue.push_back(entry);
    return true;
}

template <typename T>
void
RequestQueue<T>::DropPacketWithDst(T dst)
{
    NS_LOG_FUNCTION(this << dst);
    Purge();
    for (auto i = m_queue.begin(); i != m_queue.end(); ++i)
    {
        if constexpr (std::is_same<T, Ipv4Address>::value)
        {
            if (i->GetIpHeader().GetDestination() == dst)
            {
                Drop(*i, "DropPacketWithDst ");
            }
        }
        else if constexpr (std::is_same<T, Ipv6Address>::value)
        {
            if (i->GetIpHeader().GetDestination() == dst)
            {
                Drop(*i, "DropPacketWithDst ");
            }
        }
    }
    auto new_end =
        std::remove_if(m_queue.begin(), m_queue.end(), [&](const QueueEntry<IpHeader>& en) {
            if constexpr (std::is_same<T, Ipv4Address>::value)
            {
                return en.GetIpHeader().GetDestination() == dst;
            }
            else if constexpr (std::is_same<T, Ipv6Address>::value)
            {
                return en.GetIpHeader().GetDestination() == dst;
            }
            return false;
        });
    m_queue.erase(new_end, m_queue.end());
}

template <typename T>
bool
RequestQueue<T>::Dequeue(T dst, QueueEntry<IpHeader>& entry)
{
    Purge();
    for (auto i = m_queue.begin(); i != m_queue.end(); ++i)
    {
        if constexpr (std::is_same<T, Ipv4Address>::value)
        {
            if (i->GetIpHeader().GetDestination() == dst)
            {
                entry = *i;
                m_queue.erase(i);
                return true;
            }
        }
        else if constexpr (std::is_same<T, Ipv6Address>::value)
        {
            if (i->GetIpHeader().GetDestination() == dst)
            {
                entry = *i;
                m_queue.erase(i);
                return true;
            }
        }
    }
    return false;
}

template <typename T>
bool
RequestQueue<T>::Find(T dst)
{
    for (auto i = m_queue.begin(); i != m_queue.end(); ++i)
    {
        if constexpr (std::is_same<T, Ipv4Address>::value)
        {
            return i->GetIpHeader().GetDestination() == dst;
        }
        else if constexpr (std::is_same<T, Ipv6Address>::value)
        {
            return i->GetIpHeader().GetDestination() == dst;
        }
    }
    return false;
}

/**
 * \brief IsExpired structure
 */
struct IsExpired
{
    /**
     * Check if the entry is expired
     *
     * \param e QueueEntry entry
     * \return true if expired, false otherwise
     */
    bool operator()(const QueueEntry<Ipv4Header>& e) const
    {
        return (e.GetExpireTime() < Seconds(0));
    }

    /**
     * Check if the entry is expired
     *
     * \param e QueueEntry entry
     * \return true if expired, false otherwise
     */
    bool operator()(const QueueEntry<Ipv6Header>& e) const
    {
        return (e.GetExpireTime() < Seconds(0));
    }
};

template <typename T>
void
RequestQueue<T>::Purge()
{
    IsExpired pred;
    for (auto i = m_queue.begin(); i != m_queue.end(); ++i)
    {
        if (pred(*i))
        {
            Drop(*i, "Drop outdated packet ");
        }
    }
    m_queue.erase(std::remove_if(m_queue.begin(), m_queue.end(), pred), m_queue.end());
}

template <typename T>
void
RequestQueue<T>::Drop(QueueEntry<IpHeader> en, std::string reason)
{
    if constexpr (std::is_same<T, Ipv4Address>::value)
    {
        NS_LOG_LOGIC(reason << en.GetPacket()->GetUid() << " "
                            << en.GetIpHeader().GetDestination());
        en.GetErrorCallback()(en.GetPacket(), en.GetIpHeader(), Socket::ERROR_NOROUTETOHOST);
    }
    else if constexpr (std::is_same<T, Ipv6Address>::value)
    {
        NS_LOG_LOGIC(reason << en.GetPacket()->GetUid() << " "
                            << en.GetIpHeader().GetDestination());
        en.GetErrorCallback()(en.GetPacket(), en.GetIpHeader(), Socket::ERROR_NOROUTETOHOST);
    }
}

template class RequestQueue<Ipv4Address>;
template class RequestQueue<Ipv4Route>;

} // namespace aodvv2
} // namespace ns3
