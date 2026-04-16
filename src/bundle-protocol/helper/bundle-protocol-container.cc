/*
 * Copyright (c) 2008 INRIA
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 *
 *
 * Author: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 * Modified by Dizhi Zhou <dizhi.zhou@gmail.com>
 */

#include "bundle-protocol-container.h"

#include "ns3/names.h"

namespace ns3
{

BundleProtocolContainer::BundleProtocolContainer()
{
}

BundleProtocolContainer::BundleProtocolContainer(Ptr<BundleProtocol> bundleProtocol)
{
    m_bundleProtocols.push_back(bundleProtocol);
}

BundleProtocolContainer::BundleProtocolContainer(std::string name)
{
    Ptr<BundleProtocol> bundleProtocol = Names::Find<BundleProtocol>(name);
    m_bundleProtocols.push_back(bundleProtocol);
}

BundleProtocolContainer::Iterator
BundleProtocolContainer::Begin(void) const
{
    return m_bundleProtocols.begin();
}

BundleProtocolContainer::Iterator
BundleProtocolContainer::End(void) const
{
    return m_bundleProtocols.end();
}

uint32_t
BundleProtocolContainer::GetN(void) const
{
    return m_bundleProtocols.size();
}

Ptr<BundleProtocol>
BundleProtocolContainer::Get(uint32_t i) const
{
    return m_bundleProtocols[i];
}

void
BundleProtocolContainer::Add(BundleProtocolContainer other)
{
    for (Iterator i = other.Begin(); i != other.End(); i++)
    {
        m_bundleProtocols.push_back(*i);
    }
}

void
BundleProtocolContainer::Add(Ptr<BundleProtocol> bundleProtocol)
{
    m_bundleProtocols.push_back(bundleProtocol);
}

void
BundleProtocolContainer::Add(std::string name)
{
    Ptr<BundleProtocol> bundleProtocol = Names::Find<BundleProtocol>(name);
    m_bundleProtocols.push_back(bundleProtocol);
}

void
BundleProtocolContainer::Start(Time start)
{
    for (Iterator i = Begin(); i != End(); ++i)
    {
        Ptr<BundleProtocol> bp = *i;
        bp->SetStartTime(start);
    }
}

void
BundleProtocolContainer::Stop(Time stop)
{
    for (Iterator i = Begin(); i != End(); ++i)
    {
        Ptr<BundleProtocol> bp = *i;
        bp->SetStopTime(stop);
    }
}

} // namespace ns3
