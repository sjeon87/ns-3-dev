/*
 * Copyright (c) 2017 Trinity College Dublin
 * Copyright (c) 2025-26 NITK Surathkal (Porting to ns-3)
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Authors: Rohit P. Tahiliani <rohit.tahil@gmail.com>
 *
 */

#include "pi-square-queue-disc.h"

#include "ns3/abort.h"
#include "ns3/double.h"
#include "ns3/drop-tail-queue.h"
#include "ns3/enum.h"
#include "ns3/log.h"
#include "ns3/simulator.h"
#include "ns3/uinteger.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("PiSquareQueueDisc");

NS_OBJECT_ENSURE_REGISTERED(PiSquareQueueDisc);

TypeId
PiSquareQueueDisc::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::PiSquareQueueDisc")
            .SetParent<QueueDisc>()
            .SetGroupName("TrafficControl")
            .AddConstructor<PiSquareQueueDisc>()
            .AddAttribute("MeanPktSize",
                          "Average of packet size",
                          UintegerValue(1000),
                          MakeUintegerAccessor(&PiSquareQueueDisc::m_meanPktSize),
                          MakeUintegerChecker<uint32_t>())
            .AddAttribute("A",
                          "Value of alpha",
                          DoubleValue(0.625),
                          MakeDoubleAccessor(&PiSquareQueueDisc::m_a),
                          MakeDoubleChecker<double>())
            .AddAttribute("B",
                          "Value of beta",
                          DoubleValue(6.25),
                          MakeDoubleAccessor(&PiSquareQueueDisc::m_b),
                          MakeDoubleChecker<double>())
            .AddAttribute("Tupdate",
                          "Time period to calculate drop probability",
                          TimeValue(MilliSeconds(30)),
                          MakeTimeAccessor(&PiSquareQueueDisc::m_tUpdate),
                          MakeTimeChecker())
            .AddAttribute("Supdate",
                          "Start time of the update timer",
                          TimeValue(Seconds(0)),
                          MakeTimeAccessor(&PiSquareQueueDisc::m_sUpdate),
                          MakeTimeChecker())
            .AddAttribute("MaxSize",
                          "Queue limit in bytes/packets",
                          QueueSizeValue(QueueSize("1000p")),
                          MakeQueueSizeAccessor(&QueueDisc::SetMaxSize, &QueueDisc::GetMaxSize),
                          MakeQueueSizeChecker())
            .AddAttribute("DequeueThreshold",
                          "Minimum queue size in bytes before dequeue rate is measured",
                          UintegerValue(10000),
                          MakeUintegerAccessor(&PiSquareQueueDisc::m_dqThreshold),
                          MakeUintegerChecker<uint32_t>())
            .AddAttribute("QueueDelayReference",
                          "Desired queue delay",
                          TimeValue(MilliSeconds(20)),
                          MakeTimeAccessor(&PiSquareQueueDisc::m_qDelayRef),
                          MakeTimeChecker());

    return tid;
}

PiSquareQueueDisc::PiSquareQueueDisc()
    : QueueDisc()
{
    NS_LOG_FUNCTION(this);
    m_uv = CreateObject<UniformRandomVariable>();
    m_rtrsEvent = Simulator::Schedule(m_sUpdate, &PiSquareQueueDisc::CalculateP, this);
}

PiSquareQueueDisc::~PiSquareQueueDisc()
{
    NS_LOG_FUNCTION(this);
}

void
PiSquareQueueDisc::DoDispose()
{
    NS_LOG_FUNCTION(this);
    m_uv = nullptr;
    Simulator::Remove(m_rtrsEvent);
    QueueDisc::DoDispose();
}

Time
PiSquareQueueDisc::GetQueueDelay()
{
    NS_LOG_FUNCTION(this);
    return m_qDelay;
}

int64_t
PiSquareQueueDisc::AssignStreams(int64_t stream)
{
    NS_LOG_FUNCTION(this << stream);
    m_uv->SetStream(stream);
    return 1;
}

bool
PiSquareQueueDisc::DoEnqueue(Ptr<QueueDiscItem> item)
{
    NS_LOG_FUNCTION(this << item);

    QueueSize nQueued = GetCurrentSize();

    if (nQueued + item > GetMaxSize())
    {
        // Drops due to queue limit: reactive
        DropBeforeEnqueue(item, FORCED_DROP);
        return false;
    }
    else if (DropEarly(item, nQueued.GetValue()))
    {
        // Early probability drop: proactive
        DropBeforeEnqueue(item, UNFORCED_DROP);
        return false;
    }

    // No drop
    bool retval = GetInternalQueue(0)->Enqueue(item);

    // If Queue::Enqueue fails, QueueDisc::Drop is called by the internal queue
    // because QueueDisc::AddInternalQueue sets the drop callback

    NS_LOG_LOGIC("\t bytesInQueue  " << GetInternalQueue(0)->GetNBytes());
    NS_LOG_LOGIC("\t packetsInQueue  " << GetInternalQueue(0)->GetNPackets());

    return retval;
}

void
PiSquareQueueDisc::InitializeParams()
{
    // Initially queue is empty so variables are initialize to zero except m_dqCount
    m_inMeasurement = false;
    m_dqCount = -1;
    m_dropProb = 0;
    m_avgDqRate = 0.0;
    m_dqStart = 0;
    m_qDelayOld = Time(Seconds(0));
}

bool
PiSquareQueueDisc::DropEarly(Ptr<QueueDiscItem> item, uint32_t qSize)
{
    NS_LOG_FUNCTION(this << item << qSize);

    double p = m_dropProb;

    uint32_t packetSize = item->GetSize();

    if (GetMaxSize().GetUnit() == QueueSizeUnit::BYTES)
    {
        p = p * packetSize / m_meanPktSize;
    }

    if ((GetMaxSize().GetUnit() == QueueSizeUnit::BYTES && qSize <= 2 * m_meanPktSize) ||
        (GetMaxSize().GetUnit() == QueueSizeUnit::PACKETS && qSize <= 2))
    {
        return false;
    }

    // Apply the squared drop probability
    double u = m_uv->GetValue();
    return u <= (p * p);
}

void
PiSquareQueueDisc::CalculateP()
{
    NS_LOG_FUNCTION(this);
    Time qDelay;
    double p = 0.0;
    bool missingInitFlag = false;

    if (m_avgDqRate > 0)
    {
        qDelay = Time(Seconds(GetInternalQueue(0)->GetNBytes() / m_avgDqRate));
    }
    else
    {
        qDelay = Time(Seconds(0));
        missingInitFlag = true;
    }

    m_qDelay = qDelay;

    // Calculate the drop probability
    p = m_a * (qDelay.GetSeconds() - m_qDelayRef.GetSeconds()) +
        m_b * (qDelay.GetSeconds() - m_qDelayOld.GetSeconds());
    p += m_dropProb;

    // For non-linear drop in prob

    if (qDelay.GetSeconds() == 0 && m_qDelayOld.GetSeconds() == 0)
    {
        p *= 0.98;
    }

    m_dropProb = (p > 0) ? p : 0;

    if ((qDelay.GetSeconds() < 0.5 * m_qDelayRef.GetSeconds()) &&
        (m_qDelayOld.GetSeconds() < (0.5 * m_qDelayRef.GetSeconds())) && (m_dropProb == 0) &&
        !missingInitFlag)
    {
        m_dqCount = -1;
        m_avgDqRate = 0.0;
    }

    m_qDelayOld = qDelay;
    m_rtrsEvent = Simulator::Schedule(m_tUpdate, &PiSquareQueueDisc::CalculateP, this);
}

Ptr<QueueDiscItem>
PiSquareQueueDisc::DoDequeue()
{
    NS_LOG_FUNCTION(this);

    if (GetInternalQueue(0)->IsEmpty())
    {
        NS_LOG_LOGIC("Queue empty");
        return nullptr;
    }

    Ptr<QueueDiscItem> item = StaticCast<QueueDiscItem>(GetInternalQueue(0)->Dequeue());
    double now = Simulator::Now().GetSeconds();
    uint32_t pktSize = item->GetSize();

    // if not in a measurement cycle and the queue has built up to dq_threshold,
    // start the measurement cycle

    if ((GetInternalQueue(0)->GetNBytes() >= m_dqThreshold) && (!m_inMeasurement))
    {
        m_dqStart = now;
        m_dqCount = 0;
        m_inMeasurement = true;
    }

    if (m_inMeasurement)
    {
        m_dqCount += pktSize;

        // done with a measurement cycle
        if (m_dqCount >= m_dqThreshold)
        {
            double tmp = now - m_dqStart;

            if (tmp > 0)
            {
                if (m_avgDqRate == 0)
                {
                    m_avgDqRate = m_dqCount / tmp;
                }
                else
                {
                    m_avgDqRate = (0.5 * m_avgDqRate) + (0.5 * (m_dqCount / tmp));
                }
            }

            // restart a measurement cycle if there is enough data
            if (GetInternalQueue(0)->GetNBytes() > m_dqThreshold)
            {
                m_dqStart = now;
                m_dqCount = 0;
                m_inMeasurement = true;
            }
            else
            {
                m_dqCount = 0;
                m_inMeasurement = false;
            }
        }
    }

    return item;
}

bool
PiSquareQueueDisc::CheckConfig()
{
    NS_LOG_FUNCTION(this);
    if (GetNQueueDiscClasses() > 0)
    {
        NS_LOG_ERROR("PiSquareQueueDisc cannot have classes");
        return false;
    }

    if (GetNPacketFilters() > 0)
    {
        NS_LOG_ERROR("PiSquareQueueDisc cannot have packet filters");
        return false;
    }

    if (GetNInternalQueues() == 0)
    {
        // create a DropTail queue
        AddInternalQueue(
            CreateObjectWithAttributes<DropTailQueue<QueueDiscItem>>("MaxSize",
                                                                     QueueSizeValue(GetMaxSize())));
    }

    if (GetNInternalQueues() != 1)
    {
        NS_LOG_ERROR("PiSquareQueueDisc needs 1 internal queue");
        return false;
    }

    return true;
}

} // namespace ns3
