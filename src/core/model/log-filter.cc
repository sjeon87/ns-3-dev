/*
 * Copyright (c) 2026 University contributors
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include "log-filter.h"

#include "environment-variable.h"
#include "string.h"

#include <algorithm> // find
#include <climits>
#include <cstdlib> // strtoul
#include <iostream>

/**
 * @file
 * @ingroup logging
 * ns3::LogFilter implementation.
 */

namespace ns3
{

/**
 * @ingroup logging
 * Unnamed namespace for log-filter.cc
 */
namespace
{

/** Whether any log filter (time or node) is currently active. */
bool g_logFilterActive = false;

// --- Time filter state ---

/** Whether the time filter is active. */
bool g_logFilterTimeActive = false;
/** Start of the time filter window (raw time step, inclusive). */
int64_t g_logFilterTimeStart = INT64_MIN;
/** End of the time filter window (raw time step, inclusive). */
int64_t g_logFilterTimeEnd = INT64_MAX;

// --- Node filter state ---

/** Whether the node filter is active. */
bool g_logFilterNodeActive = false;
/** Vector of allowed node IDs (contiguous for cache-friendly linear search). */
std::vector<uint32_t> g_logFilterNodes;
/** Whether to pass messages with NO_CONTEXT when node filter is active. */
bool g_logFilterIncludeNoContext = false;

// --- Provider callbacks ---

/** Registered time provider function, or nullptr. */
LogTimeProvider g_logTimeProvider = nullptr;
/** Registered node context provider function, or nullptr. */
LogNodeProvider g_logNodeProvider = nullptr;

/** Sentinel value for events not associated with any node. */
constexpr uint32_t NO_CONTEXT = 0xffffffff;

/**
 * Update the global g_logFilterActive flag based on current filter state.
 */
void
UpdateFilterActive()
{
    g_logFilterActive = g_logFilterTimeActive || g_logFilterNodeActive;
}

} // unnamed namespace

void
LogSetTimeFilterProvider(LogTimeProvider provider)
{
    g_logTimeProvider = provider;
}

void
LogSetNodeFilterProvider(LogNodeProvider provider)
{
    g_logNodeProvider = provider;
}

void
LogSetTimeFilter(int64_t start, int64_t end)
{
    g_logFilterTimeStart = start;
    g_logFilterTimeEnd = end;
    g_logFilterTimeActive = true;
    UpdateFilterActive();
}

void
LogClearTimeFilter()
{
    g_logFilterTimeActive = false;
    g_logFilterTimeStart = INT64_MIN;
    g_logFilterTimeEnd = INT64_MAX;
    UpdateFilterActive();
}

void
LogSetNodeFilter(const std::vector<uint32_t>& nodes, bool includeNoContext)
{
    g_logFilterNodes = nodes;
    g_logFilterIncludeNoContext = includeNoContext;
    g_logFilterNodeActive = !nodes.empty();
    UpdateFilterActive();
}

void
LogAddNodeFilter(uint32_t nodeId)
{
    if (std::find(g_logFilterNodes.begin(), g_logFilterNodes.end(), nodeId) ==
        g_logFilterNodes.end())
    {
        g_logFilterNodes.push_back(nodeId);
    }
    g_logFilterNodeActive = true;
    UpdateFilterActive();
}

void
LogClearNodeFilter()
{
    g_logFilterNodes.clear();
    g_logFilterIncludeNoContext = false;
    g_logFilterNodeActive = false;
    UpdateFilterActive();
}

bool
LogFilterCheck()
{
    // Fast path: no filter active at all
    if (!g_logFilterActive)
    {
        return true;
    }

    // Time filter check
    if (g_logFilterTimeActive && g_logTimeProvider)
    {
        int64_t now = g_logTimeProvider();
        if (now < g_logFilterTimeStart || now > g_logFilterTimeEnd)
        {
            return false;
        }
    }

    // Node filter check
    if (g_logFilterNodeActive && g_logNodeProvider)
    {
        uint32_t ctx = g_logNodeProvider();
        if (ctx == NO_CONTEXT)
        {
            return g_logFilterIncludeNoContext;
        }
        // Linear search on a small, contiguous vector (typically 1-5 elements)
        if (std::find(g_logFilterNodes.begin(), g_logFilterNodes.end(), ctx) ==
            g_logFilterNodes.end())
        {
            return false;
        }
    }

    return true;
}

void
LogFilterParseNodeEnvironment()
{
    auto [found, value] = EnvironmentVariable::Get("NS_LOG_FILTER_NODES");
    if (!found || value.empty())
    {
        return;
    }

    StringVector parts = SplitString(value, ",");
    std::vector<uint32_t> nodes;
    bool includeNoContext = false;

    for (const auto& part : parts)
    {
        if (part.empty())
        {
            continue;
        }
        if (part == "*")
        {
            includeNoContext = true;
            continue;
        }
        char* endPtr = nullptr;
        unsigned long id = std::strtoul(part.c_str(), &endPtr, 10);
        if (endPtr != part.c_str() + part.size())
        {
            std::cerr << "NS_LOG_FILTER_NODES: invalid node ID \"" << part << "\"" << std::endl;
            continue;
        }
        nodes.push_back(static_cast<uint32_t>(id));
    }

    if (!nodes.empty() || includeNoContext)
    {
        LogSetNodeFilter(nodes, includeNoContext);
    }
}

} // namespace ns3
