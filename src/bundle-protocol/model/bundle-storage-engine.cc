/*
 * Copyright (c) 2026 Michigan State University
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Ishaan Lagwankar <lagwanka@msu.edu>
 */
#include "bundle-storage-engine.h"
#include "ns3/log.h"
#include "ns3/simulator.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("BundleStorageEngine");
NS_OBJECT_ENSURE_REGISTERED(BundleStorageEngine);

BundleStorageEngine::BundleStorageEngine()
    : m_currentSize(0),
      m_totalSize(0),
      m_nextHandle(1)
{
    NS_LOG_FUNCTION(this);
}

BundleStorageEngine::~BundleStorageEngine()
{
    NS_LOG_FUNCTION(this);
    m_bundleMap.clear();
}

TypeId
BundleStorageEngine::GetTypeId()
{
    static TypeId tid = TypeId("ns3::BundleStorageEngine")
                            .SetParent<Object>()
                            .SetGroupName("BundleProtocol")
                            .AddConstructor<BundleStorageEngine>();
    return tid;
}

uint32_t
BundleStorageEngine::GetCurrentSize() const
{
    NS_LOG_FUNCTION(this);
    return m_currentSize;
}

uint32_t
BundleStorageEngine::GetTotalSize() const
{
    NS_LOG_FUNCTION(this);
    return m_totalSize;
}

void
BundleStorageEngine::SetTotalSize(uint32_t size)
{
    NS_LOG_FUNCTION(this << size);
    m_totalSize = size;
}

uint32_t
BundleStorageEngine::StoreBundle(Ptr<Bundle> bundle)
{
    NS_LOG_FUNCTION(this << bundle);
    NS_ASSERT_MSG(bundle, "BundleStorageEngine::StoreBundle called with null bundle");

    uint32_t bundleSize = bundle->GetTotalSize();

    if (m_totalSize > 0 && m_currentSize + bundleSize > m_totalSize)
    {
        NS_LOG_WARN("Storage full: cannot store bundle of size " << bundleSize
                    << " (used=" << m_currentSize << " total=" << m_totalSize << ")");
        return 0;
    }

    uint32_t handle = m_nextHandle++;
    m_bundleMap[handle] = bundle;
    m_currentSize += bundleSize;

    NS_LOG_DEBUG("Stored bundle with handle " << handle
                 << " size=" << bundleSize
                 << " totalUsed=" << m_currentSize);
    return handle;
}

Ptr<Bundle>
BundleStorageEngine::RetrieveBundle(uint32_t handle) const
{
    NS_LOG_FUNCTION(this << handle);

    auto it = m_bundleMap.find(handle);
    if (it == m_bundleMap.end())
    {
        NS_LOG_WARN("RetrieveBundle: handle " << handle << " not found");
        return nullptr;
    }
    return it->second;
}

uint32_t
BundleStorageEngine::DeleteBundle(uint32_t handle)
{
    NS_LOG_FUNCTION(this << handle);

    auto it = m_bundleMap.find(handle);
    if (it == m_bundleMap.end())
    {
        NS_LOG_WARN("DeleteBundle: handle " << handle << " not found");
        return 1;
    }

    uint32_t bundleSize = it->second->GetTotalSize();
    m_bundleMap.erase(it);

    if (bundleSize > m_currentSize)
    {
        NS_LOG_WARN("DeleteBundle: size accounting inconsistency, resetting to 0");
        m_currentSize = 0;
    }
    else
    {
        m_currentSize -= bundleSize;
    }

    NS_LOG_DEBUG("Deleted bundle handle=" << handle
                 << " freed=" << bundleSize
                 << " totalUsed=" << m_currentSize);
    return 0;
}

std::vector<Ptr<Bundle>>
BundleStorageEngine::GetExpiredBundles() const
{
    NS_LOG_FUNCTION(this);
    std::vector<Ptr<Bundle>> expired;
    Time now = Simulator::Now();

    for (auto& entry : m_bundleMap)
    {
        if (entry.second->GetExpiry() <= now)
        {
            NS_LOG_DEBUG("Bundle handle=" << entry.first
                         << " expired at " << entry.second->GetExpiry().As(Time::S));
            expired.push_back(entry.second);
        }
    }
    return expired;
}

std::vector<uint32_t>
BundleStorageEngine::GetHandlesForDestination(const std::string& eid) const
{
    NS_LOG_FUNCTION(this << eid);
    std::vector<uint32_t> handles;

    for (auto& entry : m_bundleMap)
    {
        if (entry.second->GetDestinationEID() == eid)
        {
            handles.push_back(entry.first);
        }
    }
    return handles;
}

bool
BundleStorageEngine::HasBundle(uint32_t handle) const
{
    NS_LOG_FUNCTION(this << handle);
    return m_bundleMap.find(handle) != m_bundleMap.end();
}

} // namespace ns3