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
 * Authors: Pavel Boyko <boyko@iitp.ru>, written after OlsrHelper by Mathieu Lacage
 * <mathieu.lacage@sophia.inria.fr>
 */
#include "aodvv2-helper.h"

#include "../model/aodvv2-routing-protocol.h"

#include "ns3/ipv4-list-routing.h"
#include "ns3/names.h"
#include "ns3/node-list.h"
#include "ns3/ptr.h"

namespace ns3
{

template <typename T>
Aodvv2Helper<T>::Aodvv2Helper()
{
    std::string name;
    if constexpr (IsIpv4)
    {
        name = "Ipv4";
    }
    else
    {
        name = "Ipv6";
    }

    m_agentFactory.SetTypeId("ns3::aodvv2::" + name + "Aodvv2RoutingProtocol");
}

template <typename T>
Aodvv2Helper<T>*
Aodvv2Helper<T>::Copy() const
{
    return new Aodvv2Helper<T>(*this);
}

template <typename T>
Ptr<typename Aodvv2Helper<T>::IpRoutingProtocol>
Aodvv2Helper<T>::Create(Ptr<Node> node) const
{
    if constexpr (std::is_same<T, Ipv4RoutingHelper>::value)
    {
        Ptr<aodvv2::Aodvv2RoutingProtocol<IpRoutingProtocol>> agent =
            m_agentFactory.Create<aodvv2::Aodvv2RoutingProtocol<IpRoutingProtocol>>();
        node->AggregateObject(agent);
        return agent;
    }
    return nullptr;
}

template <typename T>
void
Aodvv2Helper<T>::Set(std::string name, const AttributeValue& value)
{
    m_agentFactory.Set(name, value);
}

template <typename T>
int64_t
Aodvv2Helper<T>::AssignStreams(NodeContainer c, int64_t stream)
{
    int64_t currentStream = stream;
    Ptr<Node> node;
    for (auto i = c.Begin(); i != c.End(); ++i)
    {
        node = (*i);
        Ptr<Ip> ip = node->GetObject<Ip>();
        NS_ASSERT_MSG(ip, "Ip not installed on node");
        Ptr<IpRoutingProtocol> proto = ip->GetRoutingProtocol();
        NS_ASSERT_MSG(proto, "Ip routing not installed on node");
        Ptr<aodvv2::Aodvv2RoutingProtocol<IpRoutingProtocol>> aodv =
            DynamicCast<aodvv2::Aodvv2RoutingProtocol<IpRoutingProtocol>>(proto);
        if (aodv)
        {
            currentStream += aodv->AssignStreams(currentStream);
            continue;
        }
        // Aodv may also be in a list
        Ptr<IpListRouting> list = DynamicCast<IpListRouting>(proto);
        if (list)
        {
            int16_t priority;
            Ptr<IpRoutingProtocol> listProto;
            Ptr<aodvv2::Aodvv2RoutingProtocol<IpRoutingProtocol>> listAodv;
            for (uint32_t i = 0; i < list->GetNRoutingProtocols(); i++)
            {
                listProto = list->GetRoutingProtocol(i, priority);
                listAodv = DynamicCast<aodvv2::Aodvv2RoutingProtocol<IpRoutingProtocol>>(listProto);
                if (listAodv)
                {
                    currentStream += listAodv->AssignStreams(currentStream);
                    break;
                }
            }
        }
    }
    return (currentStream - stream);
}

template class Aodvv2Helper<Ipv4RoutingHelper>;
template class Aodvv2Helper<Ipv6RoutingHelper>;

} // namespace ns3
