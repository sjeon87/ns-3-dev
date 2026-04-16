/*
 * Copyright (c) 2008 INRIA
 *                  2013 University of New Brunswick
 *                  2026 Michigan State University
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 *           Dizhi Zhou <dizhi.zhou@gmail.com>
 *           Gerard Garcia <ggarcia@deic.uab.cat>
 *           Ishaan Lagwankar <lagwanka@msu.edu>
 */

#include "bundle-protocol-helper.h"

#include "ns3/fatal-error.h"
#include "ns3/generic-convergence-layer-adapter.h"
#include "ns3/names.h"
#include "ns3/object.h"

namespace ns3
{

BundleAgentContainer::BundleAgentContainer()
{
}

BundleAgentContainer::BundleAgentContainer(Ptr<BundleAgent> bundleAgent)
{
    m_bundleAgents.push_back(bundleAgent);
}

BundleAgentContainer::BundleAgentContainer(std::string name)
{
    Ptr<BundleAgent> bundleAgent = Names::Find<BundleAgent>(name);
    m_bundleAgents.push_back(bundleAgent);
}

BundleAgentContainer::Iterator
BundleAgentContainer::Begin(void) const
{
    return m_bundleAgents.begin();
}

BundleAgentContainer::Iterator
BundleAgentContainer::End(void) const
{
    return m_bundleAgents.end();
}

uint32_t
BundleAgentContainer::GetN(void) const
{
    return m_bundleAgents.size();
}

Ptr<BundleAgent>
BundleAgentContainer::Get(uint32_t i) const
{
    return m_bundleAgents[i];
}

void
BundleAgentContainer::Add(BundleAgentContainer other)
{
    for (Iterator i = other.Begin(); i != other.End(); i++)
    {
        m_bundleAgents.push_back(*i);
    }
}

void
BundleAgentContainer::Add(Ptr<BundleAgent> bundleAgent)
{
    m_bundleAgents.push_back(bundleAgent);
}

void
BundleAgentContainer::Add(std::string name)
{
    Ptr<BundleAgent> bundleAgent = Names::Find<BundleAgent>(name);
    m_bundleAgents.push_back(bundleAgent);
}

BundleAgentHelper::BundleAgentHelper()
    : m_eid("dtn:none")
{
}

BundleAgentContainer
BundleAgentHelper::Install(Ptr<Node> node)
{
    return BundleAgentContainer(InstallPriv(node));
}

BundleAgentContainer
BundleAgentHelper::Install(std::string nodeName)
{
    Ptr<Node> node = Names::Find<Node>(nodeName);
    return BundleAgentContainer(InstallPriv(node));
}

BundleAgentContainer
BundleAgentHelper::Install(NodeContainer c)
{
    BundleAgentContainer apps;
    for (NodeContainer::Iterator i = c.Begin(); i != c.End(); ++i)
    {
        apps.Add(InstallPriv(*i));
    }

    return apps;
}

Ptr<BundleAgent>
BundleAgentHelper::InstallPriv(Ptr<Node> node)
{
    if (m_eid == "dtn:none")
    {
        NS_FATAL_ERROR("BundleAgentHelper::InstallPriv (): do not have endpoint id!");
    }

    Ptr<BundleAgent> bundleAgent = CreateObject<BundleAgent>();
    bundleAgent->SetLocalEID(m_eid);

    return bundleAgent;
}

void
BundleAgentHelper::SetBpEndpointId(std::string eid)
{
    m_eid = eid;
}

BundleClaContainer::BundleClaContainer()
{
}

BundleClaContainer::BundleClaContainer(Ptr<BundleCla> cla)
{
    m_clas.push_back(cla);
}

BundleClaContainer::BundleClaContainer(std::string name)
{
    Ptr<BundleCla> cla = Names::Find<BundleCla>(name);
    m_clas.push_back(cla);
}

BundleClaContainer::Iterator
BundleClaContainer::Begin(void) const
{
    return m_clas.begin();
}

BundleClaContainer::Iterator
BundleClaContainer::End(void) const
{
    return m_clas.end();
}

uint32_t
BundleClaContainer::GetN(void) const
{
    return m_clas.size();
}

Ptr<BundleCla>
BundleClaContainer::Get(uint32_t i) const
{
    return m_clas[i];
}

void
BundleClaContainer::Add(BundleClaContainer other)
{
    for (Iterator i = other.Begin(); i != other.End(); i++)
    {
        m_clas.push_back(*i);
    }
}

void
BundleClaContainer::Add(Ptr<BundleCla> cla)
{
    m_clas.push_back(cla);
}

void
BundleClaContainer::Add(std::string name)
{
    Ptr<BundleCla> cla = Names::Find<BundleCla>(name);
    m_clas.push_back(cla);
}

BundleClaHelper::BundleClaHelper(std::string type)
{
    m_factory.SetTypeId(type);
}

void
BundleClaHelper::SetAttribute(std::string name, const AttributeValue& value)
{
    m_factory.Set(name, value);
}

BundleClaContainer
BundleClaHelper::Install(Ptr<Node> node)
{
    return BundleClaContainer(InstallPriv(node));
}

BundleClaContainer
BundleClaHelper::Install(std::string nodeName)
{
    Ptr<Node> node = Names::Find<Node>(nodeName);
    return BundleClaContainer(InstallPriv(node));
}

BundleClaContainer
BundleClaHelper::Install(NodeContainer c)
{
    BundleClaContainer clas;
    for (NodeContainer::Iterator i = c.Begin(); i != c.End(); ++i)
    {
        clas.Add(InstallPriv(*i));
    }

    return clas;
}

Ptr<BundleCla>
BundleClaHelper::InstallPriv(Ptr<Node> node)
{
    Ptr<BundleCla> cla = m_factory.Create<BundleCla>();
    return cla;
}

} // namespace ns3
