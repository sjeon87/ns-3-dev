/*
 * Copyright (c) 2017 NITK Surathkal
 * Copyright (c) 2019 Tom Henderson (update to IETF draft -10)
 * Copyright (c) 2026 GPRT, UFPE (update to Linux code)
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Authors: Shravya K.S. <shravya.ks0@gmail.com>
 *          Tom Henderson <tomh@tomh.org>
 * Modified by:
 *          Maria Eduarda Veras <eduarda.martins@gprt.ufpe.br>
 *          Eduardo Freitas <eduardo.freitas@gprt.ufpe.br>
 *          Djamel Fawzi Hadj Sadok <jamel@gprt.ufpe.br>
 */

#include "dualpi2-queue-disc.h"

#include "ns3/abort.h"
#include "ns3/assert.h"
#include "ns3/double.h"
#include "ns3/drop-tail-queue.h"
#include "ns3/enum.h"
#include "ns3/fatal-error.h"
#include "ns3/log.h"
#include "ns3/net-device-queue-interface.h"
#include "ns3/object-factory.h"
#include "ns3/simulator.h"
#include "ns3/string.h"
#include "ns3/uinteger.h"

#include <cmath>
#include <cstddef>

namespace ns3
{

/**
 * Tag used to carry the "apply_step" state from Enqueue to Dequeue.
 *
 * In the Linux kernel implementation (sch_dualpi2.c), the decision to apply
 * the Step AQM is made at enqueue time (checking if queue len >= min_qlen_step)
 * and stored in the skb control buffer (cb).
 *
 * Since ns-3 queues do not natively persist metadata outside the packet,
 * this Tag mimics the Linux "cb" behavior, ensuring the Step AQM logic
 * follows the reference implementation.
 */
class DualPi2StepTag : public Tag
{
  public:
    static TypeId GetTypeId(void)
    {
        static TypeId tid = TypeId("ns3::DualPi2StepTag")
                                .SetParent<Tag>()
                                .SetGroupName("TrafficControl")
                                .AddConstructor<DualPi2StepTag>();
        return tid;
    }

    virtual TypeId GetInstanceTypeId(void) const
    {
        return GetTypeId();
    }

    virtual uint32_t GetSerializedSize(void) const
    {
        return 1;
    }

    virtual void Serialize(TagBuffer i) const
    {
        i.WriteU8(m_applyStep);
    }

    virtual void Deserialize(TagBuffer i)
    {
        m_applyStep = i.ReadU8();
    }

    virtual void Print(std::ostream& os) const
    {
        os << "Apply step: " << (uint32_t)m_applyStep;
    }

    void SetApplyStep(uint8_t step)
    {
        m_applyStep = step;
    }

    bool GetApplyStep() const
    {
        return m_applyStep;
    }

  private:
    uint8_t m_applyStep{0}; //!< Whether to apply step or not
};

NS_OBJECT_ENSURE_REGISTERED(DualPi2StepTag);

NS_LOG_COMPONENT_DEFINE("DualPi2QueueDisc");

NS_OBJECT_ENSURE_REGISTERED(DualPi2QueueDisc);

const std::size_t CLASSIC = 0;
const std::size_t L4S = 1;
const double MAX_PROB = 1.0;

TypeId
DualPi2QueueDisc::GetTypeId(void)
{
    static TypeId tid =
        TypeId("ns3::DualPi2QueueDisc")
            .SetParent<QueueDisc>()
            .SetGroupName("TrafficControl")
            .AddConstructor<DualPi2QueueDisc>()
            .AddAttribute("Mtu",
                          "Device MTU (bytes); if zero, will be automatically configured",
                          UintegerValue(0),
                          MakeUintegerAccessor(&DualPi2QueueDisc::m_mtu),
                          MakeUintegerChecker<uint32_t>())
            .AddAttribute("A",
                          "Value of alpha (Hz)",
                          DoubleValue(0.156250),
                          MakeDoubleAccessor(&DualPi2QueueDisc::m_alpha),
                          MakeDoubleChecker<double>())
            .AddAttribute("B",
                          "Value of beta (Hz)",
                          DoubleValue(3.195312),
                          MakeDoubleAccessor(&DualPi2QueueDisc::m_beta),
                          MakeDoubleChecker<double>())
            .AddAttribute("Tupdate",
                          "Time period to calculate drop probability",
                          TimeValue(MilliSeconds(16)),
                          MakeTimeAccessor(&DualPi2QueueDisc::m_tUpdate),
                          MakeTimeChecker())
            .AddAttribute("QueueLimit",
                          "Queue limit in bytes",
                          UintegerValue(1562500), // 250 ms at 50 Mbps
                          MakeUintegerAccessor(&DualPi2QueueDisc::m_queueLimit),
                          MakeUintegerChecker<uint32_t>())
            .AddAttribute("Target",
                          "PI AQM Classic queue delay target",
                          TimeValue(MilliSeconds(15)),
                          MakeTimeAccessor(&DualPi2QueueDisc::m_target),
                          MakeTimeChecker())
            .AddAttribute("L4SMarkThreshold",
                          "L4S marking threshold in Time",
                          TimeValue(MilliSeconds(1)),
                          MakeTimeAccessor(&DualPi2QueueDisc::m_minTh),
                          MakeTimeChecker())
            .AddAttribute("MinQLenStep",
                          "Minimum L4S queue length (in packets) to apply step marking",
                          UintegerValue(0),
                          MakeUintegerAccessor(&DualPi2QueueDisc::m_minQLenStep),
                          MakeUintegerChecker<uint32_t>())
            .AddAttribute("StepInPackets",
                          "Whether to apply step marking based on queue length instead of delay",
                          BooleanValue(false),
                          MakeBooleanAccessor(&DualPi2QueueDisc::m_stepInPackets),
                          MakeBooleanChecker())
            .AddAttribute("DropEarly",
                          "Whether to drop at enqueue (true) or dequeue (false)",
                          BooleanValue(false),
                          MakeBooleanAccessor(&DualPi2QueueDisc::m_dropEarly),
                          MakeBooleanChecker())
            .AddAttribute("DropOverload",
                          "Whether to drop on overload (true) or overflow (false)",
                          BooleanValue(true),
                          MakeBooleanAccessor(&DualPi2QueueDisc::m_dropOverload),
                          MakeBooleanChecker())
            .AddAttribute("K",
                          "Coupling factor",
                          DoubleValue(2),
                          MakeDoubleAccessor(&DualPi2QueueDisc::m_k),
                          MakeDoubleChecker<double>())
            .AddAttribute("StartTime", // Only if user wants to change queue start time
                          "Simulation time to start scheduling the update timer",
                          TimeValue(Seconds(0.0)),
                          MakeTimeAccessor(&DualPi2QueueDisc::m_startTime),
                          MakeTimeChecker())
            .AddAttribute("ClassicWeight",
                          "Weight for classic queue in %.",
                          UintegerValue(10),
                          MakeUintegerAccessor(&DualPi2QueueDisc::m_wClassic),
                          MakeUintegerChecker<uint32_t>())
            .AddTraceSource("ProbL",
                            "L4S mark probability (p_L)",
                            MakeTraceSourceAccessor(&DualPi2QueueDisc::m_pL),
                            "ns3::TracedValueCallback::Double")
            .AddTraceSource("ProbC",
                            "Classic drop/mark probability (p_C)",
                            MakeTraceSourceAccessor(&DualPi2QueueDisc::m_pC),
                            "ns3::TracedValueCallback::Double")
            .AddTraceSource("ClassicSojournTime",
                            "Sojourn time of the last packet dequeued from the Classic queue",
                            MakeTraceSourceAccessor(&DualPi2QueueDisc::m_traceClassicSojourn),
                            "ns3::Time::TracedCallback")
            .AddTraceSource("L4sSojournTime",
                            "Sojourn time of the last packet dequeued from the L4S queue",
                            MakeTraceSourceAccessor(&DualPi2QueueDisc::m_traceL4sSojourn),
                            "ns3::Time::TracedCallback");
    return tid;
}

DualPi2QueueDisc::DualPi2QueueDisc()
    : QueueDisc()
{
    NS_LOG_FUNCTION(this);
    m_uv = CreateObject<UniformRandomVariable>();
    m_rtrsEvent = Simulator::Schedule(m_startTime, &DualPi2QueueDisc::DualPi2Update, this);
}

DualPi2QueueDisc::~DualPi2QueueDisc()
{
    NS_LOG_FUNCTION(this);
}

void
DualPi2QueueDisc::DoDispose(void)
{
    NS_LOG_FUNCTION(this);
    m_rtrsEvent.Cancel();
    QueueDisc::DoDispose();
}

void
DualPi2QueueDisc::SetQueueLimit(uint32_t lim)
{
    NS_LOG_FUNCTION(this << lim);
    m_queueLimit = lim;
}

uint32_t
DualPi2QueueDisc::GetQueueSize(void) const
{
    return (GetInternalQueue(CLASSIC)->GetNBytes() + GetInternalQueue(L4S)->GetNBytes());
}

int64_t
DualPi2QueueDisc::AssignStreams(int64_t stream)
{
    NS_LOG_FUNCTION(this << stream);
    m_uv->SetStream(stream);
    return 1;
}

bool
DualPi2QueueDisc::IsL4S(Ptr<QueueDiscItem> item)
{
    uint8_t tosByte = 0;
    if (item->GetUint8Value(QueueItem::IP_DSFIELD, tosByte))
    {
        // ECT(1) or CE
        if ((tosByte & 0x3) == 1 || (tosByte & 0x3) == 3)
        {
            return true;
        }
    }
    return false;
}

bool
DualPi2QueueDisc::DoEnqueue(Ptr<QueueDiscItem> item)
{
    NS_LOG_FUNCTION(this << item);
    std::size_t queueNumber = CLASSIC;

    uint32_t nQueued = GetQueueSize();
    if (nQueued + item->GetSize() > m_queueLimit)
    {
        // Drops due to queue limit
        DropBeforeEnqueue(item, FORCED_DROP);
        return false;
    }

    if (m_dropEarly && MustDrop(item))
    {
        DropBeforeEnqueue(item, OVERLOAD_DROP);
        return false;
    }

    if (IsL4S(item))
    {
        queueNumber = L4S;
        bool applyStep = (GetInternalQueue(L4S)->GetNPackets() >= m_minQLenStep);

        DualPi2StepTag tag;
        tag.SetApplyStep(applyStep);
        item->GetPacket()->AddPacketTag(tag);
    }

    if (queueNumber == CLASSIC)
    {
        NS_LOG_DEBUG("Classic enqueue at t=" << Simulator::Now().GetSeconds()
                                             << " qlen=" << GetInternalQueue(CLASSIC)->GetNPackets()
                                             << " item_ts=" << item->GetTimeStamp().GetSeconds());
    }
    else
    {
        NS_LOG_DEBUG("L4S enqueue at t=" << Simulator::Now().GetSeconds()
                                         << " qlen=" << GetInternalQueue(L4S)->GetNPackets()
                                         << " item_ts=" << item->GetTimeStamp().GetSeconds());
    }

    bool retval = GetInternalQueue(queueNumber)->Enqueue(item);
    NS_LOG_LOGIC("Packets enqueued in queue-" << queueNumber << ": "
                                              << GetInternalQueue(queueNumber)->GetNPackets());
    return retval;
}

void
DualPi2QueueDisc::InitializeParams(void)
{
    if (m_mtu == 0)
    {
        Ptr<NetDeviceQueueInterface> ndqi = GetNetDeviceQueueInterface();
        Ptr<NetDevice> dev;
        // if the NetDeviceQueueInterface object is aggregated to a
        // NetDevice, get the MTU of such NetDevice
        if (ndqi && (dev = ndqi->GetObject<NetDevice>()))
        {
            m_mtu = dev->GetMtu();
        }
    }
    NS_ABORT_MSG_IF(m_mtu < 68, "Error: MTU does not meet RFC 791 minimum");
    m_prevQ = Time(Seconds(0));
    m_baseProb = 0;
    m_pC = 0;
    m_pL = 0;
    m_minThPkts = 0;

    m_wL4S = 100 - m_wClassic;
    m_creditInit = (int32_t)m_mtu * ((int32_t)m_wClassic - (int32_t)m_wL4S);
    m_credit = m_creditInit;

    m_thLen = 2 * m_mtu;

    NS_LOG_INFO("DualQ Init: WRR Weights C/L=" << m_wClassic << "/" << m_wL4S
                                               << " InitCredit=" << m_creditInit);
}

void
DualPi2QueueDisc::DualPi2Update()
{
    NS_LOG_FUNCTION(this);

    Ptr<const QueueDiscItem> item;
    Time curQ = Seconds(0);
    Time lDelay = Seconds(0);
    Time cDelay = Seconds(0);

    if ((item = GetInternalQueue(CLASSIC)->Peek()))
    {
        cDelay = Simulator::Now() - item->GetTimeStamp();
    }
    if ((item = GetInternalQueue(L4S)->Peek()))
    {
        lDelay = Simulator::Now() - item->GetTimeStamp();
    }

    curQ = std::max(cDelay, lDelay);

    m_baseProb = m_baseProb + m_alpha * (curQ - m_target).GetSeconds() +
                 m_beta * (curQ - m_prevQ).GetSeconds();
    // clamp p' to within [0,1]; page 34 of Internet-Draft
    m_baseProb = std::max<double>(m_baseProb, 0);
    m_baseProb = std::min<double>(m_baseProb, MAX_PROB);

    if (!m_dropOverload)
    {
        // if we do not drop on overload, ensure we cap the L4S probability
        // to 100% to keep window fairness when overflowing.
        m_baseProb = std::min<double>(m_baseProb, MAX_PROB / m_k);
    }

    m_pC = m_baseProb * m_baseProb;
    m_pL = m_baseProb * m_k;
    m_prevQ = curQ;

    NS_LOG_INFO("PI2Update: t=" << Simulator::Now().GetSeconds()
                                << " cDelay=" << cDelay.GetMilliSeconds() << " lDelay="
                                << lDelay.GetMilliSeconds() << " curQ=" << curQ.GetMilliSeconds()
                                << " prevQ=" << m_prevQ.GetMilliSeconds() << " baseProb="
                                << m_baseProb << " cSize=" << GetInternalQueue(CLASSIC)->GetNBytes()
                                << " lSize=" << GetInternalQueue(L4S)->GetNBytes());

    m_rtrsEvent = Simulator::Schedule(m_tUpdate, &DualPi2QueueDisc::DualPi2Update, this);
}

Ptr<QueueDiscItem>
DualPi2QueueDisc::DoDequeue()
{
    NS_LOG_FUNCTION(this);
    Ptr<QueueDiscItem> item;
    int32_t credit = 0;

    while (GetQueueSize() > 0)
    {
        item = Scheduler(credit);

        if (!item)
        {
            NS_LOG_LOGIC("Scheduler returned no item");
            return 0;
        }

        if (!m_dropEarly && MustDrop(item))
        {
            DropAfterDequeue(item, OVERLOAD_DROP);
            continue;
        }

        m_credit += credit;

        if (IsL4S(item))
        {
            StepAqm(item);
        }

        return item;
    }
    return 0;
}

/**
 * Weight round-robin scheduler to decide which queue to dequeue from.
 * The credit is updated based on the queue weights and the size of the dequeued packet,
 * following the logic in the Linux implementation. This is similar to the dequeue_packet()
 * function in the Linux kernel.
 */
Ptr<QueueDiscItem>
DualPi2QueueDisc::Scheduler(int32_t& credit)
{
    NS_LOG_FUNCTION(this);

    credit = 0;
    Ptr<QueueDiscItem> item = 0;

    Ptr<InternalQueue> classicQueue = GetInternalQueue(CLASSIC);
    Ptr<InternalQueue> l4sQueue = GetInternalQueue(L4S);

    bool lHasPackets = l4sQueue->GetNPackets() > 0;
    bool cHasPackets = classicQueue->GetNPackets() > 0;

    if (lHasPackets && (!cHasPackets || m_credit <= 0))
    {
        item = l4sQueue->Dequeue();
        m_traceL4sSojourn(Simulator::Now() - item->GetTimeStamp());
        NS_LOG_DEBUG("L4S sojourn time: "
                     << (Simulator::Now() - item->GetTimeStamp()).GetMilliSeconds()
                     << " enqueued at: " << item->GetTimeStamp().GetSeconds()
                     << " dequeued at: " << Simulator::Now().GetSeconds() << " queue lengths: L4S="
                     << l4sQueue->GetNPackets() << " Classic=" << classicQueue->GetNPackets());
        NS_LOG_LOGIC("L4S queue packet dequeued");
        if (cHasPackets)
        {
            credit = (int32_t)m_wClassic;
        }
    }
    else if (cHasPackets)
    {
        item = classicQueue->Dequeue();
        m_traceClassicSojourn(Simulator::Now() - item->GetTimeStamp());
        NS_LOG_DEBUG("Classic sojourn time: "
                     << (Simulator::Now() - item->GetTimeStamp()).GetMilliSeconds()
                     << " enqueued at: " << item->GetTimeStamp().GetSeconds()
                     << " dequeued at: " << Simulator::Now().GetSeconds() << " queue lengths: L4S="
                     << l4sQueue->GetNPackets() << " Classic=" << classicQueue->GetNPackets());
        NS_LOG_LOGIC("Classic queue packet dequeued");
        if (lHasPackets)
        {
            credit = -((int32_t)m_wL4S);
        }
    }
    else
    {
        NS_LOG_LOGIC("No packets in either queue");
        m_credit = m_creditInit;
        return 0;
    }

    credit *= (int32_t)item->GetSize();
    return item;
}

bool
DualPi2QueueDisc::StepAqm(Ptr<QueueDiscItem> item)
{
    NS_LOG_FUNCTION(this << item);
    NS_ASSERT_MSG(IsL4S(item), "StepAqm should only be called for L4S packets");

    bool conditionStep = false;
    bool applyStep = false;

    DualPi2StepTag tag;
    if (item->GetPacket()->PeekPacketTag(tag))
    {
        applyStep = tag.GetApplyStep();
    }

    if (m_stepInPackets)
    {
        uint32_t qLen = GetInternalQueue(L4S)->GetNPackets();
        conditionStep = qLen > m_minThPkts;
    }
    else
    {
        Time qDelay = Simulator::Now() - item->GetTimeStamp();
        conditionStep = qDelay > m_minTh;
    }

    if (applyStep && conditionStep)
    {
        Mark(item, STEP_L4S_MARK);
    }
    return false;
}

bool
DualPi2QueueDisc::MustDrop(Ptr<QueueDiscItem> item)
{
    NS_LOG_FUNCTION(this << item);

    if (GetQueueSize() < 2 * m_mtu)
    {
        NS_LOG_DEBUG("Queue size less than 2 MTU, not dropping");
        return false;
    }

    m_pL = m_baseProb * m_k;
    m_pC = m_baseProb * m_baseProb;
    bool overload = m_baseProb * m_k > MAX_PROB;

    if (IsL4S(item))
    {
        if (overload)
        {
            if (!m_dropOverload || !(m_uv->GetValue() <= m_pC))
            {
                Mark(item, PROBABILISTIC_L4S_MARK);
                return false;
            }
            NS_LOG_DEBUG("Overload detected, dropping L4S packet");
            return true;
        }

        if (m_uv->GetValue() <= m_pL)
        {
            bool retval = Mark(item, PROBABILISTIC_L4S_MARK);
            NS_ASSERT_MSG(retval == true, "Make sure we can mark in L4S queue");
            return false;
        }

        return false;
    }
    else
    {
        if (m_uv->GetValue() <= m_pC)
        {
            if (overload || !Mark(item, PROBABILISTIC_CLASSIC_MARK))
            {
                NS_LOG_DEBUG("Overload detected, dropping Classic packet");
                return true;
            }
        }
        return false;
    }
    return false;
}

Ptr<const QueueDiscItem>
DualPi2QueueDisc::DoPeek()
{
    NS_LOG_FUNCTION(this);
    Ptr<const QueueDiscItem> item;

    for (std::size_t i = 0; i < GetNInternalQueues(); i++)
    {
        if ((item = GetInternalQueue(i)->Peek()))
        {
            NS_LOG_LOGIC("Peeked from queue number " << i << ": " << item);
            NS_LOG_LOGIC("Number packets queue number " << i << ": "
                                                        << GetInternalQueue(i)->GetNPackets());
            NS_LOG_LOGIC("Number bytes queue number " << i << ": "
                                                      << GetInternalQueue(i)->GetNBytes());
            return item;
        }
    }

    NS_LOG_LOGIC("Queue empty");
    return item;
}

bool
DualPi2QueueDisc::CheckConfig(void)
{
    NS_LOG_FUNCTION(this);
    if (GetNQueueDiscClasses() > 0)
    {
        NS_LOG_ERROR("DualPi2QueueDisc cannot have classes");
        return false;
    }

    if (GetNPacketFilters() > 0)
    {
        NS_LOG_ERROR("DualPi2QueueDisc cannot have packet filters");
        return false;
    }

    if (GetNInternalQueues() == 0)
    {
        // Create 2 DropTail queues
        Ptr<InternalQueue> queue0 =
            CreateObjectWithAttributes<DropTailQueue<QueueDiscItem>>("MaxSize",
                                                                     QueueSizeValue(GetMaxSize()));
        Ptr<InternalQueue> queue1 =
            CreateObjectWithAttributes<DropTailQueue<QueueDiscItem>>("MaxSize",
                                                                     QueueSizeValue(GetMaxSize()));
        QueueSize queueSize(BYTES, m_queueLimit);
        queue0->SetMaxSize(queueSize);
        queue1->SetMaxSize(queueSize);
        AddInternalQueue(queue0);
        AddInternalQueue(queue1);
    }

    if (GetNInternalQueues() != 2)
    {
        NS_LOG_ERROR("DualPi2QueueDisc needs 2 internal queue");
        return false;
    }

    if (GetInternalQueue(CLASSIC)->GetMaxSize().GetValue() < m_queueLimit)
    {
        NS_LOG_ERROR(
            "The size of the internal Classic traffic queue is less than the queue disc limit");
        return false;
    }

    if (GetInternalQueue(L4S)->GetMaxSize().GetValue() < m_queueLimit)
    {
        NS_LOG_ERROR(
            "The size of the internal L4S traffic queue is less than the queue disc limit");
        return false;
    }

    return true;
}

} // namespace ns3
