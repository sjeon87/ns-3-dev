/*
 * Copyright (c) 2026 Michigan State University
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Ishaan Lagwankar <lagwanka@msu.edu>
 */
#include "contact-graph-helper.h"

#include "ns3/contact-parser.h"
#include "ns3/fatal-error.h"
#include "ns3/log.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("ContactGraphHelper");

ContactGraphHelper::ContactGraphHelper()
    : m_filename("")
{
    m_factory.SetTypeId("ns3::PerPacketDijkstraCGR");
}

void
ContactGraphHelper::SetContactPlan(const std::string& filename)
{
    m_filename = filename;
}

void
ContactGraphHelper::SetRoutingEngine(const std::string& typeId)
{
    m_factory.SetTypeId(typeId);
}

Ptr<BaseRoutingEngine>
ContactGraphHelper::Install()
{
    if (m_filename.empty())
    {
        NS_FATAL_ERROR("Contact plan filename not set.");
    }

    Ptr<BaseRoutingEngine> graph = m_factory.Create<BaseRoutingEngine>();

    if (!graph)
    {
        NS_FATAL_ERROR("CRASH AVERTED: ObjectFactory failed to build '"
                       << m_factory.GetTypeId().GetName());
    }

    bool success = ContactParser::ParseFile(m_filename, graph);

    if (!success)
    {
        NS_FATAL_ERROR("Failed to parse the contact plan: " << m_filename);
    }

    NS_LOG_INFO(
        "ContactGraphHelper successfully installed: " << graph->GetInstanceTypeId().GetName());

    return graph;
}

} // namespace ns3
