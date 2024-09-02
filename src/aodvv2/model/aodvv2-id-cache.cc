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
 *      NS-3 AODV model developed by Elena Buchatskaya and Pavel Boyko of IITP RAS
 *
 * Authors: Francesco Todino <francesco.todino@edu.unifi.it>
 *          Tommaso Pecorella <tommaso.pecorella@unifi.it>
 */
#include "aodvv2-id-cache.h"

#include <algorithm>

namespace ns3
{
namespace aodvv2
{
template <typename T>
bool
IdCache<T>::IsDuplicate(T addr, uint32_t id)
{
    Purge();
    for (auto i = m_idCache.begin(); i != m_idCache.end(); ++i)
    {
        if (i->m_context == addr && i->m_id == id)
        {
            return true;
        }
    }
    UniqueId uniqueId = {addr, id, m_lifetime + Simulator::Now()};
    m_idCache.push_back(uniqueId);
    return false;
}

template <typename T>
void
IdCache<T>::Purge()
{
    m_idCache.erase(remove_if(m_idCache.begin(), m_idCache.end(), IsExpired()), m_idCache.end());
}

template <typename T>
uint32_t
IdCache<T>::GetSize()
{
    Purge();
    return m_idCache.size();
}

template class IdCache<Ipv4Address>;
template class IdCache<Ipv6Address>;

} // namespace aodvv2
} // namespace ns3
