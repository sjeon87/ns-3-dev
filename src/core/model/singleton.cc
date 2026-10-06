/*
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include "singleton.h"

#include <map>
#include <mutex>
#include <typeindex>

namespace ns3
{

void*
GetSingletonInstance(const std::type_info& type, void* (*create)())
{
    static std::recursive_mutex mutex;
    static std::map<std::type_index, void*> instances;
    std::lock_guard lock(mutex);
    const std::type_index key(type);
    auto instance = instances.find(key);
    if (instance == instances.end())
    {
        instance = instances.emplace(key, create()).first;
    }
    return instance->second;
}

} // namespace ns3
