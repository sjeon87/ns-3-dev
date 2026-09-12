/*
 * Copyright (c) 2026 University contributors
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */

#ifndef NS3_LOG_FILTER_H
#define NS3_LOG_FILTER_H

#include <cstdint>
#include <vector>

/**
 * @file
 * @ingroup logging
 * Declaration of log filtering functions and provider callback types.
 *
 * The log filter system allows suppressing log messages based on
 * simulation time and/or node context. Filters are configured either
 * programmatically or via the NS_LOG_FILTER_TIME and NS_LOG_FILTER_NODES
 * environment variables.
 *
 * @par Architecture Note:
 * The logging module sits at the bottom of the ns-3 dependency tree and
 * must not depend on the Simulator or Time classes. Access to the current
 * simulation time and node context is obtained through provider callbacks
 * that are registered by simulator.cc during initialization, following the
 * same pattern used by TimePrinter and NodePrinter.
 */

namespace ns3
{

/**
 * @ingroup logging
 * @defgroup logfilter Log Filtering
 * @brief Functions and types for filtering log output by time window
 *        and node context.
 */
/** @{ */

/**
 * Function signature for a provider that returns the current simulation
 * time as a raw int64_t time step value.
 *
 * @return The current simulation time in internal time step units.
 */
typedef int64_t (*LogTimeProvider)();

/**
 * Function signature for a provider that returns the current simulation
 * context (node ID).
 *
 * @return The current simulation context (node ID), or 0xffffffff for
 *         NO_CONTEXT.
 */
typedef uint32_t (*LogNodeProvider)();

/**
 * Register the function that provides the current simulation time.
 *
 * This is called from simulator.cc during initialization. Until a
 * provider is registered, time-based filtering is not applied even
 * if a time filter has been configured.
 *
 * @param [in] provider The time provider function, or nullptr to clear.
 */
void LogSetTimeFilterProvider(LogTimeProvider provider);

/**
 * Register the function that provides the current node context.
 *
 * This is called from simulator.cc during initialization. Until a
 * provider is registered, node-based filtering is not applied even
 * if a node filter has been configured.
 *
 * @param [in] provider The node context provider function, or nullptr to
 *                      clear.
 */
void LogSetNodeFilterProvider(LogNodeProvider provider);

/**
 * Set the time window for log filtering.
 *
 * Only log messages where the time provider returns a value in
 * [start, end] (inclusive) will be printed. Use
 * std::numeric_limits<int64_t>::min() for an open start bound and
 * std::numeric_limits<int64_t>::max() for an open end bound.
 *
 * @param [in] start Start of the time window (inclusive), in raw
 *                   time step units.
 * @param [in] end   End of the time window (inclusive), in raw
 *                   time step units.
 */
void LogSetTimeFilter(int64_t start, int64_t end);

/**
 * Clear the time window filter. All simulation times will be logged.
 */
void LogClearTimeFilter();

/**
 * Set the node ID filter for logging.
 *
 * Only log messages whose node context matches one of the specified
 * node IDs will be printed. An empty vector clears the node filter.
 *
 * @param [in] nodes Vector of node IDs to include in log output.
 * @param [in] includeNoContext If true, messages with NO_CONTEXT
 *             (0xffffffff) also pass the filter.
 */
void LogSetNodeFilter(const std::vector<uint32_t>& nodes, bool includeNoContext = false);

/**
 * Add a single node ID to the node filter.
 *
 * If no node filter is currently active, this activates one
 * containing only the specified node ID.
 *
 * @param [in] nodeId The node ID to include.
 */
void LogAddNodeFilter(uint32_t nodeId);

/**
 * Clear the node ID filter. All nodes will be logged.
 */
void LogClearNodeFilter();

/**
 * Check if the current simulation state passes the active log filters.
 *
 * This is called by the NS_LOG macro after the log level check.
 * It returns true immediately if no filter is active (single boolean
 * check, zero overhead in the common case).
 *
 * @return true if the message should be logged, false if filtered out.
 */
bool LogFilterCheck();

/**
 * Parse the NS_LOG_FILTER_NODES environment variable and configure
 * the node filter accordingly.
 *
 * Expected format: comma-separated list of node IDs, optionally
 * including "*" to also pass NO_CONTEXT events.
 * Example: "0,2,5" or "0,2,*"
 *
 * This function does not require any simulator dependency and can
 * be called from any compilation unit.
 */
void LogFilterParseNodeEnvironment();

/** @} */ // logfilter

} // namespace ns3

#endif /* NS3_LOG_FILTER_H */
