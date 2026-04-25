/*
 * Copyright (c) 2005,2006 INRIA
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 */
#include "simulator.h"

#include "environment-variable.h"
#include "event-impl.h"
#include "fatal-error.h"
#include "global-value.h"
#include "log-filter.h"
#include "log.h"
#include "map-scheduler.h"
#include "object-factory.h"
#include "ptr.h"
#include "scheduler.h"
#include "simulator-impl.h"
#include "string.h"

#ifdef ENABLE_DES_METRICS
#include "des-metrics.h"
#endif

/**
 * @file
 * @ingroup simulator
 * ns3::Simulator implementation, as well as implementation pointer,
 * global scheduler implementation.
 */

namespace ns3
{

// Note:  Logging in this file is largely avoided due to the
// number of calls that are made to these functions and the possibility
// of causing recursions leading to stack overflow
NS_LOG_COMPONENT_DEFINE("Simulator");

namespace
{

/**
 * @ingroup logfilter
 * Wrapper to return Simulator::Now() as raw int64_t time step.
 * Registered as a LogTimeProvider callback.
 *
 * @return The current simulation time in raw time step units.
 */
int64_t
LogTimeProviderImpl()
{
    return Simulator::Now().GetTimeStep();
}

/**
 * @ingroup logfilter
 * Wrapper to return Simulator::GetContext().
 * Registered as a LogNodeProvider callback.
 *
 * @return The current simulation context (node ID).
 */
uint32_t
LogNodeProviderImpl()
{
    return Simulator::GetContext();
}

/**
 * @ingroup logfilter
 * Parse the NS_LOG_FILTER_TIME environment variable and configure
 * the time window filter.
 *
 * Expected format: "start:end" where start and end are ns3::Time strings.
 * Either bound may be omitted for an open interval:
 *   - "1.5s:3.0s" -- log from 1.5s to 3.0s
 *   - "1.5s:"     -- log from 1.5s onward
 *   - ":3.0s"     -- log until 3.0s
 */
void
LogFilterParseTimeEnvironment()
{
    auto [found, value] = EnvironmentVariable::Get("NS_LOG_FILTER_TIME");
    if (!found || value.empty())
    {
        return;
    }

    StringVector parts = SplitString(value, ":");
    int64_t start = std::numeric_limits<int64_t>::min();
    int64_t end = std::numeric_limits<int64_t>::max();

    if (!parts.empty() && !parts[0].empty())
    {
        start = Time(parts[0]).GetTimeStep();
    }
    if (parts.size() >= 2 && !parts[1].empty())
    {
        end = Time(parts[1]).GetTimeStep();
    }

    LogSetTimeFilter(start, end);
}

} // unnamed namespace

EventId Simulator::m_stopEvent;

/**
 * @ingroup simulator
 * @anchor GlobalValueSimulatorImplementationType
 * The specific simulator implementation to use.
 *
 * Must be derived from SimulatorImpl.
 */
static GlobalValue g_simTypeImpl =
    GlobalValue("SimulatorImplementationType",
                "The object class to use as the simulator implementation",
                StringValue("ns3::DefaultSimulatorImpl"),
                MakeStringChecker());

/**
 * @ingroup scheduler
 * @anchor GlobalValueSchedulerType
 * The specific event scheduler implementation to use.
 *
 * Must be derived from Scheduler.
 */
static GlobalValue g_schedTypeImpl =
    GlobalValue("SchedulerType",
                "The object class to use as the scheduler implementation",
                TypeIdValue(MapScheduler::GetTypeId()),
                MakeTypeIdChecker());

/**
 * @ingroup simulator
 * @brief Get the static SimulatorImpl instance.
 * @return The SimulatorImpl instance pointer.
 */
static SimulatorImpl**
PeekImpl()
{
    static SimulatorImpl* impl = nullptr;
    return &impl;
}

/**
 * @ingroup simulator
 * @brief Get the SimulatorImpl singleton.
 * @return The singleton pointer.
 * @see Simulator::GetImplementation()
 */
static SimulatorImpl*
GetImpl()
{
    SimulatorImpl** pimpl = PeekImpl();
    /* Please, don't include any calls to logging macros in this function
     * or pay the price, that is, stack explosions.
     */
    if (*pimpl == nullptr)
    {
        {
            ObjectFactory factory;
            StringValue s;

            g_simTypeImpl.GetValue(s);
            factory.SetTypeId(s.Get());
            *pimpl = GetPointer(factory.Create<SimulatorImpl>());
        }
        {
            ObjectFactory factory;
            StringValue s;
            g_schedTypeImpl.GetValue(s);
            factory.SetTypeId(s.Get());
            (*pimpl)->SetScheduler(factory);
        }

        //
        // Note: we call LogSetTimePrinter _after_ creating the implementation
        // object because the act of creation can trigger calls to the logging
        // framework which would call the TimePrinter function which would call
        // Simulator::Now which would call Simulator::GetImpl, and, thus, get us
        // in an infinite recursion until the stack explodes.
        //
        LogSetTimePrinter(&DefaultTimePrinter);
        LogSetNodePrinter(&DefaultNodePrinter);
        LogSetTimeFilterProvider(&LogTimeProviderImpl);
        LogSetNodeFilterProvider(&LogNodeProviderImpl);
        LogFilterParseTimeEnvironment();
        LogFilterParseNodeEnvironment();
    }
    return *pimpl;
}

void
Simulator::Destroy()
{
    NS_LOG_FUNCTION_NOARGS();

    SimulatorImpl** pimpl = PeekImpl();
    if (*pimpl == nullptr)
    {
        return;
    }
    /* Note: we have to call LogSetTimePrinter (0) below because if we do not do
     * this, and restart a simulation after this call to Destroy, (which is
     * legal), Simulator::GetImpl will trigger again an infinite recursion until
     * the stack explodes.
     */
    LogSetTimePrinter(nullptr);
    LogSetNodePrinter(nullptr);
    LogSetTimeFilterProvider(nullptr);
    LogSetNodeFilterProvider(nullptr);
    LogClearTimeFilter();
    LogClearNodeFilter();
    (*pimpl)->Destroy();
    (*pimpl)->Unref();
    *pimpl = nullptr;
}

void
Simulator::SetScheduler(ObjectFactory schedulerFactory)
{
    NS_LOG_FUNCTION(schedulerFactory);
    GetImpl()->SetScheduler(schedulerFactory);
}

bool
Simulator::IsFinished()
{
    NS_LOG_FUNCTION_NOARGS();
    return GetImpl()->IsFinished();
}

void
Simulator::Run()
{
    NS_LOG_FUNCTION_NOARGS();
    Time::ClearMarkedTimes();
    GetImpl()->Run();
}

void
Simulator::Stop()
{
    NS_LOG_FUNCTION_NOARGS();
    NS_LOG_LOGIC("stop");
    GetImpl()->Stop();
}

EventId
Simulator::Stop(const Time& delay)
{
    NS_LOG_FUNCTION(delay);
    m_stopEvent = GetImpl()->Stop(delay);
    return m_stopEvent;
}

EventId
Simulator::GetStopEvent()
{
    return m_stopEvent;
}

Time
Simulator::Now()
{
    /* Please, don't include any calls to logging macros in this function
     * or pay the price, that is, stack explosions.
     */
    return GetImpl()->Now();
}

Time
Simulator::GetDelayLeft(const EventId& id)
{
    NS_LOG_FUNCTION(&id);
    return GetImpl()->GetDelayLeft(id);
}

EventId
Simulator::Schedule(const Time& delay, const Ptr<EventImpl>& event)
{
    return DoSchedule(delay, GetPointer(event));
}

EventId
Simulator::ScheduleNow(const Ptr<EventImpl>& ev)
{
    return DoScheduleNow(GetPointer(ev));
}

void
Simulator::ScheduleWithContext(uint32_t context, const Time& delay, EventImpl* impl)
{
#ifdef ENABLE_DES_METRICS
    DesMetrics::Get()->TraceWithContext(context, Now(), delay);
#endif
    return GetImpl()->ScheduleWithContext(context, delay, impl);
}

EventId
Simulator::ScheduleDestroy(const Ptr<EventImpl>& ev)
{
    return DoScheduleDestroy(GetPointer(ev));
}

EventId
Simulator::DoSchedule(const Time& time, EventImpl* impl)
{
#ifdef ENABLE_DES_METRICS
    DesMetrics::Get()->Trace(Now(), time);
#endif
    return GetImpl()->Schedule(time, impl);
}

EventId
Simulator::DoScheduleNow(EventImpl* impl)
{
#ifdef ENABLE_DES_METRICS
    DesMetrics::Get()->Trace(Now(), Time(0));
#endif
    return GetImpl()->ScheduleNow(impl);
}

EventId
Simulator::DoScheduleDestroy(EventImpl* impl)
{
    return GetImpl()->ScheduleDestroy(impl);
}

void
Simulator::Remove(const EventId& id)
{
    if (*PeekImpl() == nullptr)
    {
        return;
    }
    return GetImpl()->Remove(id);
}

void
Simulator::Cancel(const EventId& id)
{
    if (*PeekImpl() == nullptr)
    {
        return;
    }
    return GetImpl()->Cancel(id);
}

bool
Simulator::IsExpired(const EventId& id)
{
    if (*PeekImpl() == nullptr)
    {
        return true;
    }
    return GetImpl()->IsExpired(id);
}

Time
Now()
{
    return Simulator::Now();
}

Time
Simulator::GetMaximumSimulationTime()
{
    NS_LOG_FUNCTION_NOARGS();
    return GetImpl()->GetMaximumSimulationTime();
}

uint32_t
Simulator::GetContext()
{
    return GetImpl()->GetContext();
}

uint64_t
Simulator::GetEventCount()
{
    return GetImpl()->GetEventCount();
}

uint32_t
Simulator::GetSystemId()
{
    NS_LOG_FUNCTION_NOARGS();

    if (*PeekImpl() != nullptr)
    {
        return GetImpl()->GetSystemId();
    }
    else
    {
        return 0;
    }
}

void
Simulator::SetImplementation(Ptr<SimulatorImpl> impl)
{
    NS_LOG_FUNCTION(impl);
    if (*PeekImpl() != nullptr)
    {
        NS_FATAL_ERROR(
            "It is not possible to set the implementation after calling any Simulator:: function. "
            "Call Simulator::SetImplementation earlier or after Simulator::Destroy.");
    }
    *PeekImpl() = GetPointer(impl);
    // Set the default scheduler
    ObjectFactory factory;
    StringValue s;
    g_schedTypeImpl.GetValue(s);
    factory.SetTypeId(s.Get());
    impl->SetScheduler(factory);
    //
    // Note: we call LogSetTimePrinter _after_ creating the implementation
    // object because the act of creation can trigger calls to the logging
    // framework which would call the TimePrinter function which would call
    // Simulator::Now which would call Simulator::GetImpl, and, thus, get us
    // in an infinite recursion until the stack explodes.
    //
    LogSetTimePrinter(&DefaultTimePrinter);
    LogSetNodePrinter(&DefaultNodePrinter);
    LogSetTimeFilterProvider(&LogTimeProviderImpl);
    LogSetNodeFilterProvider(&LogNodeProviderImpl);
    LogFilterParseTimeEnvironment();
    LogFilterParseNodeEnvironment();
}

Ptr<SimulatorImpl>
Simulator::GetImplementation()
{
    NS_LOG_FUNCTION_NOARGS();
    return GetImpl();
}

} // namespace ns3
