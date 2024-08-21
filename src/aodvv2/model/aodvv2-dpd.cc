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
 *
 * Authors: Elena Buchatskaia <borovkovaes@iitp.ru>
 *          Pavel Boyko <boyko@iitp.ru>
 */

#include "aodvv2-dpd.h"

namespace ns3
{
namespace aodvv2
{

template <typename T>
bool
DuplicatePacketDetection<T>::IsDuplicate(Ptr<const Packet> p, const T& header)
{
    return m_idCache.IsDuplicate(header.GetSource(), p->GetUid());
}

template <typename T>
void
DuplicatePacketDetection<T>::SetLifetime(Time lifetime)
{
    m_idCache.SetLifetime(lifetime);
}

template <typename T>
Time
DuplicatePacketDetection<T>::GetLifetime() const
{
    return m_idCache.GetLifeTime();
}

template class DuplicatePacketDetection<Ipv4Header>;
template class DuplicatePacketDetection<Ipv6Header>;

} // namespace aodvv2
} // namespace ns3
