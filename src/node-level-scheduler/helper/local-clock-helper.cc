/*
 * Copyright (c) 2024 Ishaan Lagwankar
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Ishaan Lagwankar <lagwanka@msu.edu>
 */

#include "local-clock-helper.h"

#include "ns3/log.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("LocalClockHelper");

LocalClockHelper::LocalClockHelper()
{
    m_factory.SetTypeId("ns3::LocalClock");
}

void
LocalClockHelper::SetClockType(std::string type)
{
    m_factory.SetTypeId(type);
}

void
LocalClockHelper::SetAttribute(std::string name, const AttributeValue& value)
{
    m_factory.Set(name, value);
}

void
LocalClockHelper::Install(Ptr<Node> node) const
{
    if (node->GetObject<LocalClock>())
    {
        NS_FATAL_ERROR("A LocalClock is already installed on node " << node->GetId());
    }
    Ptr<LocalClock> clock = m_factory.Create<LocalClock>();
    node->AggregateObject(clock);
}

void
LocalClockHelper::Install(NodeContainer c) const
{
    for (auto i = c.Begin(); i != c.End(); ++i)
    {
        Install(*i);
    }
}

} // namespace ns3
