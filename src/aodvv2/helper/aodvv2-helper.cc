/*
 * Copyright (c) 2024 University of Florence
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Authors: Francesco Todino <todinofrancesco97@gmail.com>
 *          Tommaso Pecorella <tommaso.pecorella@unifi.it>
 */
#include "aodvv2-helper.h"

#include "ns3/aodvv2-routing-protocol.h"
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

        agent->SetUseDefaultMetric(m_useDefaultMetric);
        for (auto& m : m_metrics)
        {
            agent->AddMetric(m);
        }
        for (auto& [node, metricNode] : m_metricNodes)
        {
            agent->AddMetricNode(node, metricNode);
        }
        node->AggregateObject(agent);
        return agent;
    }
    return nullptr;
}

template <typename T>
void
Aodvv2Helper<T>::SetUseDefaultMetric(bool useDefaultMetric)
{
    m_useDefaultMetric = useDefaultMetric;
}

template <typename T>
bool
Aodvv2Helper<T>::AddMetric(const aodvv2::Metric<IpAddress>& metric)
{
    for (auto& m : m_metrics)
    {
        if (m.GetMetricType() == metric.GetMetricType())
        {
            return false;
        }
    }
    m_metrics.push_back(metric);
    return true;
}

template <typename T>
bool
Aodvv2Helper<T>::AddMetricNode(Ptr<Node> node, const aodvv2::MetricNode& metricNode)
{
    auto result = m_metricNodes.insert(std::make_pair(node, metricNode));
    return result.second;
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
        Ptr<aodvv2::Aodvv2RoutingProtocol<IpRoutingProtocol>> aodvv2 =
            DynamicCast<aodvv2::Aodvv2RoutingProtocol<IpRoutingProtocol>>(proto);
        if (aodvv2)
        {
            currentStream += aodvv2->AssignStreams(currentStream);
            continue;
        }
        // Aodvv2 may also be in a list
        Ptr<IpListRouting> list = DynamicCast<IpListRouting>(proto);
        if (list)
        {
            int16_t priority;
            Ptr<IpRoutingProtocol> listProto;
            Ptr<aodvv2::Aodvv2RoutingProtocol<IpRoutingProtocol>> listAodvv2;
            for (uint32_t i = 0; i < list->GetNRoutingProtocols(); i++)
            {
                listProto = list->GetRoutingProtocol(i, priority);
                listAodvv2 =
                    DynamicCast<aodvv2::Aodvv2RoutingProtocol<IpRoutingProtocol>>(listProto);
                if (listAodvv2)
                {
                    currentStream += listAodvv2->AssignStreams(currentStream);
                    break;
                }
            }
        }
    }
    return (currentStream - stream);
}

template <typename T>
void
Aodvv2Helper<T>::PrintRoutingTable(Time printTime,
                                   Ptr<Node> source,
                                   Ptr<OutputStreamWrapper> stream,
                                   Time::Unit unit)
{
    Simulator::Schedule(printTime, &Aodvv2Helper<T>::PrintRoute, source, stream, unit);
}

template <typename T>
void
Aodvv2Helper<T>::PrintRoute(Ptr<Node> source, Ptr<OutputStreamWrapper> stream, Time::Unit unit)
{
    Ptr<aodvv2::Aodvv2RoutingProtocol<IpRoutingProtocol>> rp =
        T::template GetRouting<aodvv2::Aodvv2RoutingProtocol<IpRoutingProtocol>>(
            source->GetObject<Ip>()->GetRoutingProtocol());
    NS_ASSERT(rp);
    rp->PrintRoutingTable(stream, unit);
}

template class Aodvv2Helper<Ipv4RoutingHelper>;
template class Aodvv2Helper<Ipv6RoutingHelper>;

} // namespace ns3
