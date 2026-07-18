/*
 * Copyright (c) 2006,2007 INRIA
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 */
#include "log.h"

#include "assert.h"
#include "environment-variable.h"
#include "fatal-error.h"
#include "nstime.h"
#include "string.h"

#include <algorithm> // transform
#include <charconv>
#include <ctype.h> // toupper
#include <iostream>
#include <map>
#include <memory>
#include <stdexcept>
#include <utility>
#include <vector>

/**
 * @file
 * @ingroup logging
 * ns3::LogComponent and related implementations.
 */

/**
 * @ingroup logging
 * Unnamed namespace for log.cc
 */
namespace
{
/** Mapping of log level text names to values. */
const std::map<std::string, ns3::LogLevel> LOG_LABEL_LEVELS = {
    // clang-format off
        {"none",           ns3::LOG_NONE},
        {"error",          ns3::LOG_ERROR},
        {"level_error",    ns3::LOG_LEVEL_ERROR},
        {"warn",           ns3::LOG_WARN},
        {"level_warn",     ns3::LOG_LEVEL_WARN},
        {"debug",          ns3::LOG_DEBUG},
        {"level_debug",    ns3::LOG_LEVEL_DEBUG},
        {"info",           ns3::LOG_INFO},
        {"level_info",     ns3::LOG_LEVEL_INFO},
        {"function",       ns3::LOG_FUNCTION},
        {"level_function", ns3::LOG_LEVEL_FUNCTION},
        {"logic",          ns3::LOG_LOGIC},
        {"level_logic",    ns3::LOG_LEVEL_LOGIC},
        {"all",            ns3::LOG_ALL},
        {"level_all",      ns3::LOG_LEVEL_ALL},
        {"func",           ns3::LOG_PREFIX_FUNC},
        {"prefix_func",    ns3::LOG_PREFIX_FUNC},
        {"time",           ns3::LOG_PREFIX_TIME},
        {"prefix_time",    ns3::LOG_PREFIX_TIME},
        {"node",           ns3::LOG_PREFIX_NODE},
        {"prefix_node",    ns3::LOG_PREFIX_NODE},
        {"level",          ns3::LOG_PREFIX_LEVEL},
        {"prefix_level",   ns3::LOG_PREFIX_LEVEL},
        {"prefix_all",     ns3::LOG_PREFIX_ALL}
    // clang-format on
};

/** Inverse mapping of level values to log level text names. */
const std::map<ns3::LogLevel, std::string> LOG_LEVEL_LABELS = {[]() {
    std::map<ns3::LogLevel, std::string> labels;
    for (const auto& [label, lev] : LOG_LABEL_LEVELS)
    {
        // Only keep the first label for a level
        if (labels.find(lev) == labels.end())
        {
            std::string pad{label};
            // Add whitespace for alignment with "ERROR", "DEBUG" etc.
            if (pad.size() < 5)
            {
                pad.insert(pad.size(), 5 - pad.size(), ' ');
            }
            std::transform(pad.begin(), pad.end(), pad.begin(), ::toupper);
            labels[lev] = pad;
        }
    }
    return labels;
}()};

} // Unnamed namespace

namespace ns3
{

/**
 * @ingroup logging
 * The Log TimePrinter.
 * This is private to the logging implementation.
 */
static TimePrinter g_logTimePrinter = nullptr;
/**
 * @ingroup logging
 * The Log NodePrinter.
 */
static NodePrinter g_logNodePrinter = nullptr;

/**
 * @ingroup logging
 * The log filter simulation time source.
 * This is private to the logging implementation.
 */
static LogTimeSource g_logTimeSource = nullptr;
/**
 * @ingroup logging
 * The log filter simulator context source.
 * This is private to the logging implementation.
 */
static LogContextSource g_logContextSource = nullptr;
/**
 * @ingroup logging
 * Whether a time window or context filter is configured.
 * Kept as a simple flag so LogIsFiltered() is trivially cheap when no
 * filter is in use.
 * This is private to the logging implementation.
 */
static bool g_logFilterConfigured = false;

/**
 * @ingroup logging
 * The simulator context value indicating "no context", selected by `-1`
 * in a context filter.  Matches Simulator::NO_CONTEXT, which cannot be
 * referenced from here.
 */
static constexpr uint32_t LOG_NO_CONTEXT = 0xffffffff;

/**
 * @ingroup logging
 * Log filter configuration.
 * This is private to the logging implementation.
 */
struct LogFilterConfig
{
    bool timeWindow{false}; //!< Whether a time window is configured.
    Time minTime;           //!< Time window lower bound (inclusive).
    Time maxTime;           //!< Time window upper bound (inclusive).
    /** Context id ranges (inclusive bounds); single ids have equal bounds. */
    std::vector<std::pair<uint32_t, uint32_t>> contexts;
};

/**
 * @ingroup logging
 * Get the log filter configuration, constructed on first use so no
 * Time objects are created during static initialization.
 * This is private to the logging implementation.
 *
 * @return The log filter configuration.
 */
static LogFilterConfig&
GetLogFilterConfig()
{
    static LogFilterConfig config;
    return config;
}

/**
 * @ingroup logging
 * Recompute \c g_logFilterConfigured from the filter configuration.
 * This is private to the logging implementation.
 */
static void
UpdateLogFilterConfigured()
{
    const auto& config = GetLogFilterConfig();
    g_logFilterConfigured = config.timeWindow || !config.contexts.empty();
}

/**
 * @ingroup logging
 * Handler for the undocumented \c print-list token in NS_LOG
 * which triggers printing of the list of log components, then exits.
 *
 * A static instance of this class is instantiated below, so the
 * \c print-list token is handled before any other logging action
 * can take place.
 *
 * This is private to the logging implementation.
 */
class PrintList
{
  public:
    PrintList(); //<! Constructor, prints the list and exits.
};

/**
 * Invoke handler for \c print-list in NS_LOG environment variable.
 * This is private to the logging implementation.
 */
static PrintList g_printList;

/* static */
LogComponent::ComponentList*
LogComponent::GetComponentList()
{
    static LogComponent::ComponentList components;
    return &components;
}

PrintList::PrintList()
{
    auto [found, value] = EnvironmentVariable::Get("NS_LOG", "print-list", ":");
    if (found)
    {
        LogComponentPrintList();
        exit(0);
    }
}

LogComponent::LogComponent(const std::string& name,
                           const std::string& file,
                           const LogLevel mask /* = 0 */)
    : m_levels(0),
      m_mask(mask),
      m_name(name),
      m_file(file)
{
    // Check if we're mentioned in NS_LOG, and set our flags appropriately
    EnvVarCheck();

    LogComponent::ComponentList* components = GetComponentList();

    if (components->find(name) != components->end())
    {
        NS_FATAL_ERROR("Log component \"" << name << "\" has already been registered once.");
    }

    components->insert(std::make_pair(name, this));
}

LogComponent&
GetLogComponent(const std::string name)
{
    LogComponent::ComponentList* components = LogComponent::GetComponentList();
    LogComponent* ret;

    try
    {
        ret = components->at(name);
    }
    catch (std::out_of_range&)
    {
        NS_FATAL_ERROR("Log component \"" << name << "\" does not exist.");
    }
    return *ret;
}

/** Unnamed namespace for log line assembly buffers. */
namespace
{

/**
 * A streambuf appending to a std::string whose capacity is reused
 * between log lines, so steady-state logging does not allocate.
 */
class LogLineBuf : public std::streambuf
{
  public:
    std::string m_line; //!< The log line being assembled.

  protected:
    /**
     * Append a character sequence to the line.
     *
     * @param s The characters to append.
     * @param n The number of characters to append.
     * @return The number of characters appended.
     */
    std::streamsize xsputn(const char* s, std::streamsize n) override
    {
        m_line.append(s, static_cast<std::size_t>(n));
        return n;
    }

    /**
     * Append a single character to the line.
     *
     * @param c The character to append, or EOF.
     * @return A value other than EOF on success.
     */
    int_type overflow(int_type c) override
    {
        if (!traits_type::eq_int_type(c, traits_type::eof()))
        {
            m_line.push_back(traits_type::to_char_type(c));
        }
        return traits_type::not_eof(c);
    }
};

/** A memory buffer and the ostream assembling a log line into it. */
struct LogLine
{
    LogLineBuf buf;        //!< The line buffer.
    std::ostream os{&buf}; //!< The stream assembling the line.
};

/**
 * Whether this thread's LogLine has been destroyed.
 *
 * Trivially destructible, so it remains readable during program shutdown,
 * after the buffer's own thread_local destructor has run.  Core itself logs
 * during static destruction (e.g. Time::Clear() from ~Time of static
 * attribute defaults), so this must be handled.
 */
thread_local bool g_logLineDestroyed = false;

/** Arm g_logLineDestroyed when the thread's LogLine is destroyed. */
struct LogLineHolder
{
    LogLine line; //!< The line buffer and stream.

    ~LogLineHolder()
    {
        g_logLineDestroyed = true;
    }
};

/** @return The thread-local log line buffer. */
LogLine&
GetLogLine()
{
    thread_local LogLineHolder holder;
    return holder.line;
}

} // unnamed namespace

std::ostream&
LogLineBegin()
{
    if (g_logLineDestroyed)
    {
        // Logging during program shutdown, after this thread's buffer is
        // gone: stream directly to std::clog, which is kept alive by
        // std::ios_base::Init.
        return std::clog;
    }
    // The buffer is empty here except for nested logging (a user-defined
    // operator<< that itself logs while a log message is being assembled);
    // then the inner line is appended to the outer line in progress and
    // LogLineCommit() flushes both, matching the historical interleaving of
    // direct std::clog streaming.
    return GetLogLine().os;
}

void
LogLineCommit(std::ostream& os)
{
    if (&os == &std::clog)
    {
        std::clog << std::endl;
        return;
    }
    auto& line = static_cast<LogLineBuf*>(os.rdbuf())->m_line;
    line.push_back('\n');
    std::clog.write(line.data(), static_cast<std::streamsize>(line.size()));
    std::clog.flush();
    line.clear();
}

void
LogComponent::EnvVarCheck()
{
    auto [found, value] = EnvironmentVariable::Get("NS_LOG", m_name, ":");
    if (!found)
    {
        std::tie(found, value) = EnvironmentVariable::Get("NS_LOG", "*", ":");
    }
    if (!found)
    {
        std::tie(found, value) = EnvironmentVariable::Get("NS_LOG", "***", ":");
    }

    if (!found)
    {
        return;
    }

    if (value.empty())
    {
        // Default is enable all levels, all prefixes
        value = "**";
    }

    // Got a value, might have flags
    int level = 0;
    StringVector flags = SplitString(value, "|");
    NS_ASSERT_MSG(!flags.empty(), "Unexpected empty flags from non-empty value");
    bool pre_pipe{true};

    for (const auto& lev : flags)
    {
        if (lev == "**")
        {
            level |= LOG_LEVEL_ALL | LOG_PREFIX_ALL;
        }
        else if (lev == "all" || lev == "*")
        {
            level |= (pre_pipe ? LOG_LEVEL_ALL : LOG_PREFIX_ALL);
        }
        else if (LOG_LABEL_LEVELS.find(lev) != LOG_LABEL_LEVELS.end())
        {
            level |= LOG_LABEL_LEVELS.at(lev);
        }
        pre_pipe = false;
    }
    Enable(static_cast<LogLevel>(level));
}

bool
LogComponent::IsNoneEnabled() const
{
    return m_levels == 0;
}

void
LogComponent::SetMask(const LogLevel level)
{
    m_mask |= level;
}

void
LogComponent::Enable(const LogLevel level)
{
    m_levels |= (level & ~m_mask);
}

void
LogComponent::Disable(const LogLevel level)
{
    m_levels &= ~level;
}

std::string
LogComponent::Name() const
{
    return m_name;
}

std::string
LogComponent::File() const
{
    return m_file;
}

/* static */
std::string
LogComponent::GetLevelLabel(const LogLevel level)
{
    auto it = LOG_LEVEL_LABELS.find(level);
    if (it != LOG_LEVEL_LABELS.end())
    {
        return it->second;
    }
    return "unknown";
}

void
LogComponentEnable(const std::string& name, LogLevel level)
{
    LogComponent::ComponentList* components = LogComponent::GetComponentList();
    auto logComponent = components->find(name);

    if (logComponent == components->end())
    {
        NS_LOG_UNCOND("Logging component \"" << name << "\" not found.");
        LogComponentPrintList();
        NS_FATAL_ERROR("Logging component \""
                       << name << "\" not found."
                       << " See above for a list of available log components");
    }

    logComponent->second->Enable(level);
}

void
LogComponentEnableAll(LogLevel level)
{
    LogComponent::ComponentList* components = LogComponent::GetComponentList();
    for (auto i = components->begin(); i != components->end(); i++)
    {
        i->second->Enable(level);
    }
}

void
LogComponentDisable(const std::string& name, LogLevel level)
{
    LogComponent::ComponentList* components = LogComponent::GetComponentList();
    auto logComponent = components->find(name);

    if (logComponent != components->end())
    {
        logComponent->second->Disable(level);
    }
}

void
LogComponentDisableAll(LogLevel level)
{
    LogComponent::ComponentList* components = LogComponent::GetComponentList();
    for (auto i = components->begin(); i != components->end(); i++)
    {
        i->second->Disable(level);
    }
}

void
LogComponentPrintList()
{
    // Create sorted map of components by inserting them into a map
    std::map<std::string, LogComponent*> componentsSorted;

    for (const auto& component : *LogComponent::GetComponentList())
    {
        componentsSorted.insert(component);
    }

    // Iterate through sorted components
    for (const auto& [name, component] : componentsSorted)
    {
        std::cout << name << "=";
        if (component->IsNoneEnabled())
        {
            std::cout << "0" << std::endl;
            continue;
        }
        if (component->IsEnabled(LOG_LEVEL_ALL))
        {
            std::cout << "all";
        }
        else
        {
            if (component->IsEnabled(LOG_ERROR))
            {
                std::cout << "error";
            }
            if (component->IsEnabled(LOG_WARN))
            {
                std::cout << "|warn";
            }
            if (component->IsEnabled(LOG_DEBUG))
            {
                std::cout << "|debug";
            }
            if (component->IsEnabled(LOG_INFO))
            {
                std::cout << "|info";
            }
            if (component->IsEnabled(LOG_FUNCTION))
            {
                std::cout << "|function";
            }
            if (component->IsEnabled(LOG_LOGIC))
            {
                std::cout << "|logic";
            }
        }
        if (component->IsEnabled(LOG_PREFIX_ALL))
        {
            std::cout << "|prefix_all";
        }
        else
        {
            if (component->IsEnabled(LOG_PREFIX_FUNC))
            {
                std::cout << "|func";
            }
            if (component->IsEnabled(LOG_PREFIX_TIME))
            {
                std::cout << "|time";
            }
            if (component->IsEnabled(LOG_PREFIX_NODE))
            {
                std::cout << "|node";
            }
            if (component->IsEnabled(LOG_PREFIX_LEVEL))
            {
                std::cout << "|level";
            }
        }
        std::cout << std::endl;
    }
}

/**
 * @ingroup logging
 * Check if a log component exists.
 * This is private to the logging implementation.
 *
 * @param [in] componentName The putative log component name.
 * @returns \c true if \c componentName exists.
 */
static bool
ComponentExists(std::string componentName)
{
    LogComponent::ComponentList* components = LogComponent::GetComponentList();

    return components->find(componentName) != components->end();
}

/**
 * @ingroup logging
 * Parse a log filter time window of the form `min/max`, where either
 * bound (but not both) may be omitted.
 * This is private to the logging implementation.
 *
 * @param [in] window The time window specification.
 */
static void
ParseTimeWindow(const std::string& window)
{
    auto slash = window.find('/');
    if (slash == std::string::npos)
    {
        NS_FATAL_ERROR("Invalid log time window \"" << window << "\": expected the form min/max");
    }
    std::string minStr = window.substr(0, slash);
    std::string maxStr = window.substr(slash + 1);
    if (minStr.empty() && maxStr.empty())
    {
        NS_FATAL_ERROR("Invalid log time window \"" << window
                                                    << "\": at least one bound is required");
    }
    auto& config = GetLogFilterConfig();
    config.minTime = minStr.empty() ? Time::Min() : Time(minStr);
    config.maxTime = maxStr.empty() ? Time::Max() : Time(maxStr);
    if (config.minTime > config.maxTime)
    {
        NS_FATAL_ERROR("Invalid log time window \"" << window << "\": min is later than max");
    }
    config.timeWindow = true;
    UpdateLogFilterConfigured();
}

/**
 * @ingroup logging
 * Parse a single context id for the log context filter.
 * This is private to the logging implementation.
 *
 * @param [in] item The context id string.
 * @param [in] contexts The full filter specification, for error messages.
 * @return The context id.
 */
static uint32_t
ParseContextId(const std::string& item, const std::string& contexts)
{
    uint32_t id{};
    auto [ptr, ec] = std::from_chars(item.data(), item.data() + item.size(), id);
    if (ec != std::errc() || ptr != item.data() + item.size())
    {
        NS_FATAL_ERROR("Invalid context id \"" << item << "\" in log context filter \"" << contexts
                                               << "\"");
    }
    return id;
}

/**
 * @ingroup logging
 * Parse a log context filter: a comma-separated list of context ids,
 * `[min-max]` ranges, and `-1` (no context).  An empty string clears
 * the filter.
 * This is private to the logging implementation.
 *
 * @param [in] contexts The context filter specification.
 */
static void
ParseContextFilter(const std::string& contexts)
{
    auto& config = GetLogFilterConfig();
    config.contexts.clear();
    if (!contexts.empty())
    {
        for (const auto& item : SplitString(contexts, ","))
        {
            if (item == "-1")
            {
                config.contexts.emplace_back(LOG_NO_CONTEXT, LOG_NO_CONTEXT);
            }
            else if (item.size() > 1 && item.front() == '[' && item.back() == ']')
            {
                std::string inner = item.substr(1, item.size() - 2);
                auto dash = inner.find('-');
                if (dash == std::string::npos)
                {
                    NS_FATAL_ERROR("Invalid context range \""
                                   << item << "\" in log context filter \"" << contexts
                                   << "\": expected [min-max]");
                }
                uint32_t min = ParseContextId(inner.substr(0, dash), contexts);
                uint32_t max = ParseContextId(inner.substr(dash + 1), contexts);
                if (min > max)
                {
                    NS_FATAL_ERROR("Invalid context range \""
                                   << item << "\" in log context filter \"" << contexts
                                   << "\": min is greater than max");
                }
                config.contexts.emplace_back(min, max);
            }
            else
            {
                uint32_t id = ParseContextId(item, contexts);
                config.contexts.emplace_back(id, id);
            }
        }
    }
    UpdateLogFilterConfigured();
}

void
LogSetTimeWindow(const Time& minTime, const Time& maxTime)
{
    if (minTime > maxTime)
    {
        NS_FATAL_ERROR("Invalid log time window: min " << minTime << " is later than max "
                                                       << maxTime);
    }
    auto& config = GetLogFilterConfig();
    config.minTime = minTime;
    config.maxTime = maxTime;
    config.timeWindow = true;
    UpdateLogFilterConfigured();
}

void
LogSetTimeWindow(const std::string& window)
{
    if (window.empty())
    {
        GetLogFilterConfig().timeWindow = false;
        UpdateLogFilterConfigured();
        return;
    }
    ParseTimeWindow(window);
}

void
LogSetContextFilter(const std::string& contexts)
{
    ParseContextFilter(contexts);
}

void
LogSetFilterSources(LogTimeSource timeSource, LogContextSource contextSource)
{
    g_logTimeSource = timeSource;
    g_logContextSource = contextSource;
}

bool
LogIsFiltered()
{
    if (!g_logFilterConfigured)
    {
        return false;
    }
    const auto& config = GetLogFilterConfig();
    if (config.timeWindow && g_logTimeSource)
    {
        Time now = (*g_logTimeSource)();
        if (now < config.minTime || now > config.maxTime)
        {
            return true;
        }
    }
    if (!config.contexts.empty() && g_logContextSource)
    {
        uint32_t context = (*g_logContextSource)();
        auto inRange = [context](const auto& range) {
            return context >= range.first && context <= range.second;
        };
        if (std::none_of(config.contexts.begin(), config.contexts.end(), inRange))
        {
            return true;
        }
    }
    return false;
}

/**
 * @ingroup logging
 * Parse the \c NS_LOG environment variable.
 * This is private to the logging implementation.
 */
static void
CheckEnvironmentVariables()
{
    auto dict = EnvironmentVariable::GetDictionary("NS_LOG", ":")->GetStore();

    for (auto& [component, value] : dict)
    {
        if (component.find('/') != std::string::npos)
        {
            // A global time window filter token, e.g. "1.2s/1.5s"
            if (!value.empty())
            {
                NS_FATAL_ERROR("Invalid time window \""
                               << component << "=" << value
                               << "\" in env variable NS_LOG: flags are not allowed");
            }
            ParseTimeWindow(component);
            continue;
        }
        if (component == "ContextId")
        {
            // The global context (node id) filter token
            if (value.empty())
            {
                NS_FATAL_ERROR("Empty ContextId filter in env variable NS_LOG; "
                               "expected e.g. ContextId=0,[2-4],6");
            }
            ParseContextFilter(value);
            continue;
        }
        if (component != "*" && component != "***" && !ComponentExists(component))
        {
            NS_LOG_UNCOND("Invalid or unregistered component name \"" << component << "\"");
            LogComponentPrintList();
            NS_FATAL_ERROR(
                "Invalid or unregistered component name \""
                << component
                << "\" in env variable NS_LOG, see above for a list of valid components");
        }

        // No valid component or wildcard
        if (value.empty())
        {
            continue;
        }

        // We have a valid component or wildcard, check the flags present in value
        StringVector flags = SplitString(value, "|");
        for (const auto& flag : flags)
        {
            // Handle wild cards
            if (flag == "*" || flag == "**")
            {
                continue;
            }
            bool ok = LOG_LABEL_LEVELS.find(flag) != LOG_LABEL_LEVELS.end();
            if (!ok)
            {
                NS_FATAL_ERROR("Invalid log level \""
                               << flag << "\" in env variable NS_LOG for component name "
                               << component);
            }
        }
    }
}

void
LogSetTimePrinter(TimePrinter printer)
{
    g_logTimePrinter = printer;
    /**
     * @internal
     * This is the only place where we are more or less sure that all log variables
     * are registered. See \bugid{1082} for details.
     */
    CheckEnvironmentVariables();
}

TimePrinter
LogGetTimePrinter()
{
    return g_logTimePrinter;
}

void
LogSetNodePrinter(NodePrinter printer)
{
    g_logNodePrinter = printer;
}

NodePrinter
LogGetNodePrinter()
{
    return g_logNodePrinter;
}

ParameterLogger::ParameterLogger(std::ostream& os)
    : m_os(os)
{
}

void
ParameterLogger::CommaRest()
{
    if (m_first)
    {
        m_first = false;
    }
    else
    {
        m_os << ", ";
    }
}

} // namespace ns3
