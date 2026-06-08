/*
 * Copyright (c) 2026 Shivang Upadhyay
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * CAKE queue discipline implementation for ns-3.
 *
 * Portions of the scheduler structure and behavior are
 * developed with reference to the Linux kernel sch_cake.c
 * implementation.
 */

#include "cake-queue-disc.h"

#include "ns3/drop-tail-queue.h"
#include "ns3/ipv4-queue-disc-item.h"
#include "ns3/log.h"
#include "ns3/object-factory.h"
#include "ns3/simulator.h"
#include "ns3/string.h"
#include "ns3/uinteger.h"

#include <algorithm>
#include <cmath>

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("CakeQueueDisc");
NS_OBJECT_ENSURE_REGISTERED(CakeQueueDisc);

/** Drop reason used when the internal queue is full. */
static constexpr const char* LIMIT_EXCEEDED_DROP = "Queue disc limit exceeded";
/**
 * @brief Drop reason string used for COBALT AQM drops.
 */
static constexpr const char* COBALT_DROP = "COBALT AQM drop";

TypeId
CakeQueueDisc::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::CakeQueueDisc")
            .SetParent<QueueDisc>()
            .SetGroupName("TrafficControl")
            .AddConstructor<CakeQueueDisc>()
            .AddAttribute("Bandwidth",
                          "Target shaper bandwidth (0 = disabled).",
                          DataRateValue(DataRate(0)),
                          MakeDataRateAccessor(&CakeQueueDisc::m_bandwidth),
                          MakeDataRateChecker())
            .AddAttribute("Target",
                          "AQM target sojourn time.",
                          TimeValue(MicroSeconds(5000)),
                          MakeTimeAccessor(&CakeQueueDisc::m_target),
                          MakeTimeChecker())
            .AddAttribute("Interval",
                          "AQM control interval.",
                          TimeValue(MilliSeconds(100)),
                          MakeTimeAccessor(&CakeQueueDisc::m_interval),
                          MakeTimeChecker())
            .AddAttribute("Flows",
                          "Hash table size — must be a power of 2 and >= 8.",
                          UintegerValue(1024),
                          MakeUintegerAccessor(&CakeQueueDisc::m_flows),
                          MakeUintegerChecker<uint32_t>(8, 65536))
            .AddAttribute("Overhead",
                          "Per-packet overhead bytes added for framing compensation.",
                          UintegerValue(0),
                          MakeUintegerAccessor(&CakeQueueDisc::m_overhead),
                          MakeUintegerChecker<uint32_t>())
            .AddAttribute("DiffServMode",
                          "DiffServ preset: 0=besteffort 3=diffserv3 4=diffserv4.",
                          UintegerValue(static_cast<uint32_t>(DIFFSERV_BESTEFFORT)),
                          MakeUintegerAccessor(&CakeQueueDisc::m_diffServMode),
                          MakeUintegerChecker<uint32_t>())
            .AddAttribute("IsolationMode",
                          "Host isolation: 0=none 1=src 2=dst 3=triple.",
                          UintegerValue(static_cast<uint32_t>(ISOLATION_NONE)),
                          MakeUintegerAccessor(&CakeQueueDisc::m_isolationMode),
                          MakeUintegerChecker<uint32_t>())
            .AddAttribute("AckFilterMode",
                          "TCP ACK filter: 0=none 1=conservative 2=aggressive.",
                          UintegerValue(static_cast<uint32_t>(ACK_FILTER_NONE)),
                          MakeUintegerAccessor(&CakeQueueDisc::m_ackFilterMode),
                          MakeUintegerChecker<uint32_t>())
            .AddAttribute("Perturbation",
                          "Hash perturbation seed (0 = auto-randomise).",
                          UintegerValue(0),
                          MakeUintegerAccessor(&CakeQueueDisc::m_perturbation),
                          MakeUintegerChecker<uint32_t>())
            .AddAttribute("MaxSize",
                          "The maximum number of packets accepted by this queue disc.",
                          QueueSizeValue(QueueSize("10240p")),
                          MakeQueueSizeAccessor(&QueueDisc::SetMaxSize, &QueueDisc::GetMaxSize),
                          MakeQueueSizeChecker());
    return tid;
}

CakeQueueDisc::CakeQueueDisc()
    : QueueDisc(QueueDiscSizePolicy::MULTIPLE_QUEUES, QueueSizeUnit::PACKETS),
      m_flows(1024),
      m_overhead(0),
      m_diffServMode(static_cast<uint32_t>(DIFFSERV_BESTEFFORT)),
      m_isolationMode(static_cast<uint32_t>(ISOLATION_NONE)),
      m_ackFilterMode(static_cast<uint32_t>(ACK_FILTER_NONE)),
      m_perturbation(0),
      m_numTins(1),
      m_tNext(Seconds(0)),
      m_shaperScheduled(false)
{
    NS_LOG_FUNCTION(this);
    m_uv = CreateObject<UniformRandomVariable>();
}

CakeQueueDisc::~CakeQueueDisc()
{
    NS_LOG_FUNCTION(this);
}

uint32_t
CakeQueueDisc::FlowHash(Ptr<const QueueDiscItem> item) const
{
    return item->Hash(m_perturbation) % m_flows;
}

uint32_t
CakeQueueDisc::HostHash(uint32_t val) const
{
    uint32_t h = m_perturbation ^ val;
    h ^= h >> 16;
    h *= 0x45d9f3b;
    h ^= h >> 16;
    return h % m_flows;
}

uint32_t
CakeQueueDisc::SetAssocHashLookup(uint32_t flowHash,
                                  uint32_t srcHash,
                                  uint32_t dstHash,
                                  uint8_t tin)
{
    uint32_t reducedHash = flowHash % m_flows;

    /* Fast path: Direct hit on the primary hashed slot */
    if (m_flowBuckets[reducedHash].flowHash == flowHash &&
        m_flowBuckets[reducedHash].set != CAKE_SET_NONE)
    {
        return reducedHash;
    }

    uint32_t innerHash = reducedHash % CAKE_SET_WAYS;
    uint32_t outerHash = reducedHash - innerHash;
    bool allocateSrc = false;
    bool allocateDst = false;
    uint32_t foundIdx = m_flows;

    /* Search for an existing matching flow tag */
    for (uint32_t i = 0, k = innerHash; i < CAKE_SET_WAYS; i++, k = (k + 1) % CAKE_SET_WAYS)
    {
        if (m_flowBuckets[outerHash + k].flowHash == flowHash)
        {
            if (m_flowBuckets[outerHash + k].set == CAKE_SET_NONE)
            {
                allocateSrc = CakeDsrc();
                allocateDst = CakeDdst();
            }
            foundIdx = outerHash + k;
            break;
        }
    }

    /* If no match, search for an empty slot */
    if (foundIdx == m_flows)
    {
        for (uint32_t i = 0, k = innerHash; i < CAKE_SET_WAYS; i++, k = (k + 1) % CAKE_SET_WAYS)
        {
            if (m_flowBuckets[outerHash + k].set == CAKE_SET_NONE)
            {
                allocateSrc = CakeDsrc();
                allocateDst = CakeDdst();
                foundIdx = outerHash + k;
                break;
            }
        }
    }

    /* Set is full. Accept the collision in the original hashed slot */
    if (foundIdx == m_flows)
    {
        foundIdx = outerHash + innerHash;
        if (m_flowBuckets[foundIdx].set == CAKE_SET_BULK)
        {
            m_srcHosts[m_flowBuckets[foundIdx].srcHash].srchostBulkFlowCount--;
            m_dstHosts[m_flowBuckets[foundIdx].dstHash].dsthostBulkFlowCount--;
        }
        allocateSrc = CakeDsrc();
        allocateDst = CakeDdst();
    }

    reducedHash = foundIdx;
    m_flowBuckets[reducedHash].flowHash = flowHash;
    m_flowBuckets[reducedHash].tinIndex = tin;

    if (allocateSrc)
    {
        uint32_t srcIdx = srcHash % m_flows;
        uint32_t srcInner = srcIdx % CAKE_SET_WAYS;
        uint32_t srcOuter = srcIdx - srcInner;
        uint32_t foundSrcIdx = m_flows;

        // 8-way search for existing Source Host
        for (uint32_t i = 0, k = srcInner; i < CAKE_SET_WAYS; i++, k = (k + 1) % CAKE_SET_WAYS)
        {
            if (m_srcHosts[srcOuter + k].srchostTag == srcHash)
            {
                foundSrcIdx = srcOuter + k;
                break;
            }
        }

        // 8-way search for empty Source Host slot
        if (foundSrcIdx == m_flows)
        {
            for (uint32_t i = 0, k = srcInner; i < CAKE_SET_WAYS; i++, k = (k + 1) % CAKE_SET_WAYS)
            {
                if (m_srcHosts[srcOuter + k].srchostBulkFlowCount == 0)
                {
                    foundSrcIdx = srcOuter + k;
                    break;
                }
            }
            // Collision fallback
            if (foundSrcIdx == m_flows)
            {
                foundSrcIdx = srcOuter + srcInner;
            }
            m_srcHosts[foundSrcIdx].srchostTag = srcHash;
        }

        if (m_flowBuckets[reducedHash].set == CAKE_SET_BULK)
        {
            m_srcHosts[foundSrcIdx].srchostBulkFlowCount++;
        }
        m_flowBuckets[reducedHash].srcHash = foundSrcIdx;
    }

    if (allocateDst)
    {
        uint32_t dstIdx = dstHash % m_flows;
        uint32_t dstInner = dstIdx % CAKE_SET_WAYS;
        uint32_t dstOuter = dstIdx - dstInner;
        uint32_t foundDstIdx = m_flows;

        // 8-way search for existing Dest Host
        for (uint32_t i = 0, k = dstInner; i < CAKE_SET_WAYS; i++, k = (k + 1) % CAKE_SET_WAYS)
        {
            if (m_dstHosts[dstOuter + k].dsthostTag == dstHash)
            {
                foundDstIdx = dstOuter + k;
                break;
            }
        }

        // 8-way search for empty Dest Host slot
        if (foundDstIdx == m_flows)
        {
            for (uint32_t i = 0, k = dstInner; i < CAKE_SET_WAYS; i++, k = (k + 1) % CAKE_SET_WAYS)
            {
                if (m_dstHosts[dstOuter + k].dsthostBulkFlowCount == 0)
                {
                    foundDstIdx = dstOuter + k;
                    break;
                }
            }
            // Collision fallback
            if (foundDstIdx == m_flows)
            {
                foundDstIdx = dstOuter + dstInner;
            }
            m_dstHosts[foundDstIdx].dsthostTag = dstHash;
        }

        if (m_flowBuckets[reducedHash].set == CAKE_SET_BULK)
        {
            m_dstHosts[foundDstIdx].dsthostBulkFlowCount++;
        }
        m_flowBuckets[reducedHash].dstHash = foundDstIdx;
    }

    return reducedHash;
}

uint8_t
CakeQueueDisc::DscpToTin(uint8_t dscp) const
{
    NS_ASSERT_MSG(dscp < 64, "DSCP value out of range: " << +dscp);

    static const uint8_t cake_dscp_besteffort[64] = {
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    };

    static const uint8_t cake_dscp_precedence[64] = {
        0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2,
        2, 2, 3, 3, 3, 3, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4, 4, 4, 5, 5, 5, 5,
        5, 5, 5, 5, 6, 6, 6, 6, 6, 6, 6, 6, 7, 7, 7, 7, 7, 7, 7, 7,
    };

    static const uint8_t cake_dscp_diffserv8[64] = {
        2, 0, 1, 2, 4, 2, 2, 2, 1, 2, 1, 2, 1, 2, 1, 2, 5, 2, 4, 2, 4, 2,
        4, 2, 3, 2, 3, 2, 3, 2, 3, 2, 6, 2, 3, 2, 3, 2, 3, 2, 6, 2, 2, 2,
        6, 2, 6, 2, 7, 2, 2, 2, 2, 2, 2, 2, 7, 2, 2, 2, 2, 2, 2, 2,
    };

    static const uint8_t cake_dscp_diffserv4[64] = {
        0, 1, 0, 0, 2, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 2, 0, 2, 0, 2, 0,
        2, 0, 2, 0, 2, 0, 2, 0, 2, 0, 3, 0, 2, 0, 2, 0, 2, 0, 3, 0, 0, 0,
        3, 0, 3, 0, 3, 0, 0, 0, 0, 0, 0, 0, 3, 0, 0, 0, 0, 0, 0, 0,
    };

    static const uint8_t cake_dscp_diffserv3[64] = {
        0, 1, 0, 0, 2, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        2, 0, 2, 0, 2, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0,
    };

    switch (m_diffServMode)
    {
    case DIFFSERV_BESTEFFORT:
        return cake_dscp_besteffort[dscp];
    case DIFFSERV_PRECEDENCE:
        return cake_dscp_precedence[dscp];
    case DIFFSERV_DIFFSERV8:
        return cake_dscp_diffserv8[dscp];
    case DIFFSERV_DIFFSERV4:
        return cake_dscp_diffserv4[dscp];
    case DIFFSERV_DIFFSERV3:
    default:
        return cake_dscp_diffserv3[dscp];
    }
}

void
CakeQueueDisc::SetupTins()
{
    switch (m_diffServMode)
    {
    case DIFFSERV_BESTEFFORT: {
        m_tins.assign(1, CakeTin());
        m_tins[0].targetRate = m_bandwidth;
        m_tins[0].tinQuantum = 65535; // kernel: b->tin_quantum = 65535
        break;
    }

    case DIFFSERV_PRECEDENCE:
    case DIFFSERV_DIFFSERV8: {
        m_tins.assign(8, CakeTin());
        uint64_t rate = m_bandwidth.GetBitRate();
        uint32_t quantum = 256;
        for (uint32_t i = 0; i < 8; i++)
        {
            m_tins[i].targetRate = DataRate(rate);
            m_tins[i].tinQuantum = static_cast<uint16_t>(std::max(1u, quantum));
            rate = (rate * 7) >> 3;       // kernel: rate *= 7; rate >>= 3;
            quantum = (quantum * 7) >> 3; // kernel: quantum *= 7; quantum >>= 3;
        }
        break;
    }

    case DIFFSERV_DIFFSERV4: {
        m_tins.assign(4, CakeTin());
        uint64_t rate = m_bandwidth.GetBitRate();
        m_tins[0].targetRate = DataRate(rate);
        m_tins[1].targetRate = DataRate(rate >> 4);
        m_tins[2].targetRate = DataRate(rate >> 1);
        m_tins[3].targetRate = DataRate(rate >> 2);
        m_tins[0].tinQuantum = 1024;
        m_tins[1].tinQuantum = 1024 >> 4; // 64
        m_tins[2].tinQuantum = 1024 >> 1; // 512
        m_tins[3].tinQuantum = 1024 >> 2; // 256
        break;
    }

    case DIFFSERV_DIFFSERV3:
    default: {
        m_tins.assign(3, CakeTin());
        uint64_t rate = m_bandwidth.GetBitRate();
        m_tins[0].targetRate = DataRate(rate);
        m_tins[1].targetRate = DataRate(rate >> 4);
        m_tins[2].targetRate = DataRate(rate >> 2);
        m_tins[0].tinQuantum = 1024;
        m_tins[1].tinQuantum = 1024 >> 4; // 64
        m_tins[2].tinQuantum = 1024 >> 2; // 256
        break;
    }
    }
}

bool
CakeQueueDisc::CheckConfig()
{
    NS_LOG_FUNCTION(this);

    if (m_target >= m_interval)
    {
        NS_LOG_ERROR("CakeQueueDisc: Target must be < Interval.");
        return false;
    }

    if (m_flows < CAKE_SET_WAYS || (m_flows & (m_flows - 1)) != 0)
    {
        NS_LOG_ERROR("CakeQueueDisc: Flows must be a power of 2 and >= " << CAKE_SET_WAYS);
        return false;
    }

    if (m_diffServMode > static_cast<uint32_t>(DIFFSERV_PRECEDENCE))
    {
        NS_LOG_ERROR("CakeQueueDisc: Unknown DiffServMode "
                     << m_diffServMode << " (max " << static_cast<uint32_t>(DIFFSERV_PRECEDENCE)
                     << ").");
        return false;
    }

    return true;
}

void
CakeQueueDisc::CakeDrop()
{
    uint32_t maxBytes = 0;
    uint32_t fattestFlowIdx = 0;
    bool found = false;

    // Find the flow with the largest byte backlog
    for (uint32_t i = 0; i < m_flows; ++i)
    {
        if (m_flowBuckets[i].backlogBytes > maxBytes && GetInternalQueue(i)->GetNPackets() > 0)
        {
            maxBytes = m_flowBuckets[i].backlogBytes;
            fattestFlowIdx = i;
            found = true;
        }
    }

    if (found)
    {
        // Drop from the HEAD of the fattest flow's queue
        Ptr<QueueDiscItem> pkt = GetInternalQueue(fattestFlowIdx)->Dequeue();
        if (pkt)
        {
            uint32_t len = pkt->GetSize();
            CakeFlow& flow = m_flowBuckets[fattestFlowIdx];
            CakeTin& b = m_tins[flow.tinIndex];

            flow.backlogBytes -= len;
            b.backlogBytes -= len;

            DropAfterDequeue(pkt, LIMIT_EXCEEDED_DROP);
        }
    }
}

void
CakeQueueDisc::InitializeParams()
{
    NS_LOG_FUNCTION(this);

    NS_ABORT_MSG_IF(m_ackFilterMode != static_cast<uint32_t>(ACK_FILTER_NONE),
                    "CakeQueueDisc: ACK filtering is not yet implemented");

    switch (m_diffServMode)
    {
    case DIFFSERV_BESTEFFORT:
        m_numTins = 1;
        break;
    case DIFFSERV_DIFFSERV3:
        m_numTins = 3;
        break;
    case DIFFSERV_DIFFSERV4:
        m_numTins = 4;
        break;
    case DIFFSERV_DIFFSERV8:
        m_numTins = 8;
        break;
    case DIFFSERV_PRECEDENCE:
        m_numTins = 8;
        break;
    default:
        m_numTins = 1;
        break;
    }

    SetupTins();

    m_flowBuckets.assign(m_flows, CakeFlow());
    m_srcHosts.assign(m_flows, CakeHostBucket());
    m_dstHosts.assign(m_flows, CakeHostBucket());
    m_newFlows.assign(m_numTins, std::list<uint32_t>());
    m_oldFlows.assign(m_numTins, std::list<uint32_t>());
    m_decayingFlows.assign(m_numTins, std::list<uint32_t>());

    m_quantumDiv.resize(m_flows + 1);
    m_quantumDiv[0] = 0;
    for (uint32_t i = 1; i <= m_flows; i++)
    {
        m_quantumDiv[i] = 65536 / i;
    }

    ObjectFactory factory;
    factory.SetTypeId("ns3::DropTailQueue<QueueDiscItem>");
    factory.Set("MaxSize", QueueSizeValue(GetMaxSize()));
    for (uint32_t i = 0; i < m_flows; ++i)
    {
        AddInternalQueue(factory.Create<QueueDisc::InternalQueue>());
    }

    m_tNext = Simulator::Now();
    m_shaperScheduled = false;

    if (m_perturbation == 0)
    {
        m_perturbation = static_cast<uint32_t>(m_uv->GetInteger(1, 0xffffffff));
    }

    if (m_bandwidth.GetBitRate() > 0)
    {
        m_mtuTime =
            Seconds(static_cast<double>(1514 * 8) / static_cast<double>(m_bandwidth.GetBitRate()));
    }
    else
    {
        m_mtuTime = Seconds(0);
    }
}

bool
CakeQueueDisc::DoEnqueue(Ptr<QueueDiscItem> item)
{
    NS_LOG_FUNCTION(this << item);
    Time now = Simulator::Now();
    uint32_t len = item->GetSize();

    /* Classify packet and determine the target tin */
    uint8_t tin = 0;
    int32_t filterResult = Classify(item);
    if (filterResult != PacketFilter::PF_NO_MATCH)
    {
        auto candidate = static_cast<uint8_t>(filterResult);
        if (candidate < m_numTins)
        {
            tin = candidate;
        }
    }
    else if (m_numTins > 1)
    {
        uint8_t dsField = 0;
        item->GetUint8Value(QueueItem::IP_DSFIELD, dsField);
        tin = DscpToTin(dsField >> 2);
    }

    /* Calculate flow and host hashes for isolation */
    uint32_t flowH = FlowHash(item);
    uint32_t srcH;
    uint32_t dstH;
    Ptr<const Ipv4QueueDiscItem> ipItem = DynamicCast<const Ipv4QueueDiscItem>(item);
    if (ipItem)
    {
        srcH = HostHash(ipItem->GetHeader().GetSource().Get());
        dstH = HostHash(ipItem->GetHeader().GetDestination().Get());
    }
    else
    {
        srcH = HostHash(flowH);
        dstH = HostHash(~flowH);
    }

    /* Retrieve the flow state via 8-way set-associative hash */
    uint32_t idx = SetAssocHashLookup(flowH, srcH, dstH, tin);
    CakeFlow& flow = m_flowBuckets[idx];
    CakeTin& b = m_tins[tin];

    /* Ensure shaper state isn't stale when queue transitions from empty */
    if (b.backlogBytes == 0)
    {
        if (b.timeNextPacket < now)
        {
            b.timeNextPacket = now;
        }

        if (GetNPackets() == 0)
        {
            if (m_tNext < now)
            {
                m_failsafeNext = now;
                m_tNext = now;
            }
            else if (m_tNext > now && m_failsafeNext > now)
            {
                uint64_t next = std::min(m_tNext.GetNanoSeconds(), m_failsafeNext.GetNanoSeconds());
                if (!m_shaperScheduled)
                {
                    m_shaperScheduled = true;
                    Simulator::Schedule(NanoSeconds(next) - now,
                                        &CakeQueueDisc::ShaperWakeup,
                                        this);
                }
            }
        }
    }

    /* Stamp enqueue time for AQM sojourn calculation */
    CakeSojournTag sojournTag;
    sojournTag.SetEnqueueTime(now);
    item->GetPacket()->AddPacketTag(sojournTag);

    GetInternalQueue(idx)->Enqueue(item);

    /* Update backlog statistics */
    flow.backlogBytes += len;
    b.backlogBytes += len;

    /* Manage the flowchain state machine for DRR */
    if (flow.set == CAKE_SET_NONE || flow.set == CAKE_SET_DECAYING)
    {
        CakeHostBucket& srchost = m_srcHosts[flow.srcHash];
        CakeHostBucket& dsthost = m_dstHosts[flow.dstHash];
        uint16_t hostLoad = 1;

        if (flow.set == CAKE_SET_NONE)
        {
            m_newFlows[tin].push_back(idx);
        }
        else
        {
            b.decayingFlowCount--;
            m_decayingFlows[tin].remove(idx);
            m_newFlows[tin].push_back(idx);
        }

        flow.set = CAKE_SET_SPARSE;
        b.sparseFlowCount++;

        /* Calculate host load for fairness deficit */
        if (CakeDsrc())
        {
            hostLoad = std::max(hostLoad, static_cast<uint16_t>(srchost.srchostBulkFlowCount));
        }
        if (CakeDdst())
        {
            hostLoad = std::max(hostLoad, static_cast<uint16_t>(dsthost.dsthostBulkFlowCount));
        }

        flow.tinDeficit = CakeGetFlowQuantum(tin, idx);
    }
    else if (flow.set == CAKE_SET_SPARSE_WAIT)
    {
        /* Promote sparse flow to bulk rotation */
        CakeHostBucket& srchost = m_srcHosts[flow.srcHash];
        CakeHostBucket& dsthost = m_dstHosts[flow.dstHash];

        flow.set = CAKE_SET_BULK;
        b.sparseFlowCount--;
        b.bulkFlowCount++;

        if (CakeDsrc())
        {
            srchost.srchostBulkFlowCount++;
        }
        if (CakeDdst())
        {
            dsthost.dsthostBulkFlowCount++;
        }
    }

    /* Enforce global memory limit by dropping from the fattest flow */
    while (GetNPackets() > GetMaxSize().GetValue())
    {
        CakeDrop();
    }

    return true;
}

Time
CakeQueueDisc::CobaltControlLaw(Time t, Time interval, uint32_t count) const
{
    if (count <= 1)
    {
        return t + interval;
    }

    {
        return t + Time(static_cast<int64_t>(static_cast<double>(interval.GetTimeStep()) /
                                             std::sqrt(static_cast<double>(count))));
    }
}

bool
CakeQueueDisc::CobaltShouldDrop(uint32_t tin,
                                uint32_t fi,
                                Time sojourn,
                                Ptr<QueueDiscItem> item,
                                uint32_t bulkFlows)
{
    CakeTin& tk = m_tins[tin];
    CakeFlow& flow = m_flowBuckets[fi];
    Time now = Simulator::Now();

    bool nextDue;
    bool overTarget;
    bool drop = false;

    Time schedule = now - flow.cobaltDropNext;

    overTarget = (sojourn > tk.cobaltTarget) && (sojourn > m_mtuTime * bulkFlows * 2) &&
                 (sojourn > m_mtuTime * 4);

    nextDue = (flow.cobaltCount > 0) && (schedule >= Time(0));
    flow.ecnMarked = false;

    if (overTarget)
    {
        if (!flow.cobaltDropping)
        {
            flow.cobaltDropping = true;

            if (!flow.cobaltCount)
            {
                flow.cobaltCount = 1;
            }
            flow.cobaltDropNext = CobaltControlLaw(now, tk.cobaltInterval, flow.cobaltCount);
        }
        else if (!flow.cobaltCount)
        {
            flow.cobaltCount = 1;
        }
    }
    else if (flow.cobaltDropping)
    {
        flow.cobaltDropping = false;
    }

    if (nextDue && flow.cobaltDropping)
    {
        drop = !(flow.ecnMarked = item->Mark());

        flow.cobaltCount++;
        if (!flow.cobaltCount)
        {
            flow.cobaltCount--;
        }

        flow.cobaltDropNext =
            CobaltControlLaw(flow.cobaltDropNext, tk.cobaltInterval, flow.cobaltCount);

        schedule = now - flow.cobaltDropNext;
    }
    else
    {
        while (nextDue)
        {
            flow.cobaltCount--;
            flow.cobaltDropNext =
                CobaltControlLaw(flow.cobaltDropNext, tk.cobaltInterval, flow.cobaltCount);

            schedule = now - flow.cobaltDropNext;
            nextDue = (flow.cobaltCount > 0) && (schedule >= Time(0));
        }
    }

    if (flow.blueProb > 0.0)
    {
        drop = drop || (m_uv->GetValue() < flow.blueProb);
    }

    /* Overload the drop_next field as an activity timeout */
    if (flow.cobaltCount == 0)
    {
        flow.cobaltDropNext = now + tk.cobaltInterval;
    }
    else if ((schedule > Time(0)) && !drop)
    {
        flow.cobaltDropNext = now;
    }

    return drop;
}

bool
CakeQueueDisc::CakeDsrc() const
{
    return (m_isolationMode == ISOLATION_SRC || m_isolationMode == ISOLATION_TRIPLE);
}

bool
CakeQueueDisc::CakeDdst() const
{
    return (m_isolationMode == ISOLATION_DST || m_isolationMode == ISOLATION_TRIPLE);
}

bool
CakeQueueDisc::CobaltQueueEmpty(CakeFlow& flow, Time now) const
{
    bool dropped = flow.cobaltDropping && (now < flow.cobaltDropNext);
    flow.cobaltDropping = false;
    flow.cobaltCount = 0;
    flow.cobaltDropNext = Seconds(0);
    return dropped;
}

Time
CakeQueueDisc::CakeEwma(Time avg, Time sample, uint32_t shift) const
{
    int64_t avgNs = avg.GetNanoSeconds();
    int64_t sampleNs = sample.GetNanoSeconds();
    avgNs = avgNs - (avgNs >> shift) + (sampleNs >> shift);
    return NanoSeconds(avgNs);
}

uint16_t
CakeQueueDisc::CakeGetFlowQuantum(uint32_t tinIndex, uint32_t flowIndex) const
{
    const CakeTin& b = m_tins[tinIndex];
    const CakeFlow& flow = m_flowBuckets[flowIndex];
    uint16_t hostLoad = 1;

    if (CakeDsrc())
    {
        hostLoad = std::max(hostLoad,
                            static_cast<uint16_t>(m_srcHosts[flow.srcHash].srchostBulkFlowCount));
    }
    if (CakeDdst())
    {
        hostLoad = std::max(hostLoad,
                            static_cast<uint16_t>(m_dstHosts[flow.dstHash].dsthostBulkFlowCount));
    }

    return static_cast<uint16_t>(
        (b.flowQuantum * m_quantumDiv[hostLoad] + m_uv->GetInteger(0, 65535)) >> 16);
}

uint32_t
CakeQueueDisc::CakeAdvanceShaper(CakeTin* b, Ptr<QueueDiscItem> pkt, Time now, bool drop)
{
    /* Retrieve the packet length adjusted for link-layer overhead */
    uint32_t len = CakeOverhead(pkt);

    /* Charge packet bandwidth to this tin and to the global shaper */
    if (m_bandwidth.GetBitRate() > 0)
    {
        Time tinDur = b->targetRate.CalculateBytesTxTime(len);
        Time globalDur = m_bandwidth.CalculateBytesTxTime(len);
        Time failsafeDur = globalDur + (globalDur / 2);

        if (b->timeNextPacket < now)
        {
            b->timeNextPacket += tinDur;
        }
        else if (b->timeNextPacket < (now + tinDur))
        {
            b->timeNextPacket = now + tinDur;
        }

        m_tNext += globalDur;

        if (!drop)
        {
            m_failsafeNext += failsafeDur;
        }
    }

    return len;
}

uint32_t
CakeQueueDisc::CakeCalcOverhead(uint32_t len, uint32_t off)
{
    if (m_rateFlags & CAKE_FLAG_OVERHEAD)
    {
        len -= off;
    }

    if (m_maxNetlen < len)
    {
        m_maxNetlen = len;
    }
    if (m_minNetlen > len)
    {
        m_minNetlen = len;
    }

    len += m_overhead;

    if (len < m_mpu)
    {
        len = m_mpu;
    }

    if (m_cellMode == CAKE_ATM)
    {
        len += 47;
        len /= 48;
        len *= 53;
    }
    else if (m_cellMode == CAKE_PTM)
    {
        /* Add one byte per 64 bytes or part thereof.
         * This is conservative and easier to calculate than the precise value.
         */
        len += (len + 63) / 64;
    }

    if (m_maxAdjlen < len)
    {
        m_maxAdjlen = len;
    }
    if (m_minAdjlen > len)
    {
        m_minAdjlen = len;
    }

    return len;
}

uint32_t
CakeQueueDisc::CakeOverhead(Ptr<QueueDiscItem> item)
{
    uint32_t len = item->GetSize();

    uint32_t off = 0;

    /* Update EWMA of network offset */
    m_avgNetoff = m_avgNetoff - (m_avgNetoff >> 8) + ((off << 16) >> 8);
    return CakeCalcOverhead(len, off);
}

Ptr<QueueDiscItem>
CakeQueueDisc::DoDequeue()
{
    NS_LOG_FUNCTION(this);

    Time now = Simulator::Now();
    CakeTin* b = &m_tins[m_curTin];
    std::list<uint32_t>* head = nullptr;
    uint32_t fi = 0;
    bool firstFlow = true;
    uint16_t hostLoad = 1;

begin:
    if (GetNPackets() == 0)
    {
        return nullptr;
    }

    /* global hard shaper */
    if (now < m_tNext && now < m_failsafeNext)
    {
        uint64_t next = std::min(m_tNext.GetNanoSeconds(), m_failsafeNext.GetNanoSeconds());
        if (!m_shaperScheduled)
        {
            m_shaperScheduled = true;
            Simulator::Schedule(NanoSeconds(next) - now, &CakeQueueDisc::ShaperWakeup, this);
        }
        return nullptr;
    }

    /* Choose a tin to work on */
    if (m_bandwidth.GetBitRate() == 0)
    {
        /* In unlimited mode, balance with DRR */
        bool wrapped = false;
        bool empty = true;
        while (b->tinDeficit < 0 || !(b->sparseFlowCount + b->bulkFlowCount))
        {
            if (b->tinDeficit <= 0)
            {
                b->tinDeficit += b->tinQuantum;
            }
            if (b->sparseFlowCount + b->bulkFlowCount)
            {
                empty = false;
            }
            if (++m_curTin >= m_numTins)
            {
                m_curTin = 0;
                if (wrapped)
                {
                    if (empty)
                    {
                        return nullptr;
                    }
                }
                else
                {
                    wrapped = true;
                }
            }
            b = &m_tins[m_curTin];
        }
    }
    else
    {
        /* In shaped mode, choose earliest-scheduled tin with queue. */
        Time bestTime = Time::Max();
        uint32_t bestTin = 0;
        for (uint32_t tin = 0; tin < m_numTins; tin++)
        {
            b = &m_tins[tin];
            if ((b->sparseFlowCount + b->bulkFlowCount) > 0)
            {
                Time timeToPkt = b->timeNextPacket - now;
                if (timeToPkt.GetNanoSeconds() <= 0 || timeToPkt <= bestTime)
                {
                    bestTime = timeToPkt;
                    bestTin = tin;
                }
            }
        }
        m_curTin = bestTin;
        b = &m_tins[bestTin];

        if (!(b->sparseFlowCount + b->bulkFlowCount))
        {
            return nullptr;
        }
    }

retry:
    /* service this tin — walk: decaying → new → old → decaying */
    head = &m_decayingFlows[m_curTin];
    if (!firstFlow || head->empty())
    {
        head = &m_newFlows[m_curTin];
        if (head->empty())
        {
            head = &m_oldFlows[m_curTin];
            if (head->empty())
            {
                head = &m_decayingFlows[m_curTin];
                if (head->empty())
                {
                    goto begin;
                }
            }
        }
    }

    fi = head->front();
    m_curFlow = fi;
    firstFlow = false;
    hostLoad = 1;

    {
        auto& flow = m_flowBuckets[fi];
        auto& srchost = m_srcHosts[flow.srcHash];
        auto& dsthost = m_dstHosts[flow.dstHash];

        /* triple isolation (modified DRR++) */
        if (flow.tinDeficit <= 0)
        {
            if (flow.set == CAKE_SET_SPARSE)
            {
                if (GetInternalQueue(fi)->GetNPackets() > 0)
                {
                    b->sparseFlowCount--;
                    b->bulkFlowCount++;
                    if (CakeDsrc())
                    {
                        srchost.srchostBulkFlowCount++;
                    }
                    if (CakeDdst())
                    {
                        dsthost.dsthostBulkFlowCount++;
                    }
                    flow.set = CAKE_SET_BULK;
                }
                else
                {
                    flow.set = CAKE_SET_SPARSE_WAIT;
                }
            }

            if (CakeDsrc())
            {
                hostLoad = std::max(hostLoad, static_cast<uint16_t>(srchost.srchostBulkFlowCount));
            }
            if (CakeDdst())
            {
                hostLoad = std::max(hostLoad, static_cast<uint16_t>(dsthost.dsthostBulkFlowCount));
            }

            NS_ASSERT_MSG(hostLoad <= m_flows, "host_load exceeds m_flows");

            flow.tinDeficit += static_cast<int32_t>(CakeGetFlowQuantum(m_curTin, fi));
            m_oldFlows[m_curTin].splice(m_oldFlows[m_curTin].end(), *head, head->begin());
            goto retry;
        }

        /* Retrieve a packet via the AQM */
        Ptr<QueueDiscItem> pkt;
        uint64_t delay = 0;

        while (true)
        {
            pkt = GetInternalQueue(fi)->Dequeue();
            if (!pkt)
            {
                /* this queue was actually empty */
                if (CobaltQueueEmpty(flow, now))
                {
                    b->unresponsiveFlowCount--;
                }

                if (flow.blueProb > 0.0 || flow.cobaltCount || now < flow.cobaltDropNext)

                {
                    m_decayingFlows[m_curTin].splice(m_decayingFlows[m_curTin].end(),
                                                     *head,
                                                     head->begin());

                    if (flow.set == CAKE_SET_BULK)
                    {
                        b->bulkFlowCount--;
                        if (CakeDsrc())
                        {
                            srchost.srchostBulkFlowCount--;
                        }
                        if (CakeDdst())
                        {
                            dsthost.dsthostBulkFlowCount--;
                        }
                        b->decayingFlowCount++;
                    }
                    else if (flow.set == CAKE_SET_SPARSE || flow.set == CAKE_SET_SPARSE_WAIT)
                    {
                        b->sparseFlowCount--;
                        b->decayingFlowCount++;
                    }
                    flow.set = CAKE_SET_DECAYING;
                }
                else
                {
                    head->pop_front();
                    if (flow.set == CAKE_SET_SPARSE || flow.set == CAKE_SET_SPARSE_WAIT)
                    {
                        b->sparseFlowCount--;
                    }
                    else if (flow.set == CAKE_SET_BULK)
                    {
                        b->bulkFlowCount--;
                        if (CakeDsrc())
                        {
                            srchost.srchostBulkFlowCount--;
                        }
                        if (CakeDdst())
                        {
                            dsthost.dsthostBulkFlowCount--;
                        }
                    }
                    else
                    {
                        b->decayingFlowCount--;
                    }

                    flow.set = CAKE_SET_NONE;
                }
                goto begin;
            }

            /* Read sojourn time */
            CakeSojournTag sojournTag;
            if (pkt->GetPacket()->PeekPacketTag(sojournTag))
            {
                Time d = now - sojournTag.GetEnqueueTime();
                delay = static_cast<uint64_t>(d.IsNegative() ? 0 : d.GetNanoSeconds());
            }

            bool flowHasMore = (GetInternalQueue(fi)->GetNPackets() > 0);
            uint32_t bulkFlows = b->bulkFlowCount * static_cast<uint32_t>(m_isIngress);

            if (!CobaltShouldDrop(m_curTin, fi, NanoSeconds(delay), pkt, bulkFlows) || !flowHasMore)
            {
                break;
            }

            /* drop this packet, get another one */
            if (m_isIngress)
            {
                uint32_t len = CakeAdvanceShaper(b, pkt, now, true);
                flow.tinDeficit -= static_cast<int32_t>(len);
                b->tinDeficit -= static_cast<int32_t>(len);
            }

            flow.dropped++;
            b->tinDropped++;
            DropAfterDequeue(pkt, COBALT_DROP);

            if (m_isIngress)
            {
                goto retry;
            }
        }

        b->tinEcnMark += static_cast<uint32_t>(!!flow.ecnMarked);

        /* collect delay stats */
        Time delayTime = NanoSeconds(delay);
        b->avgeDelay = CakeEwma(b->avgeDelay, delayTime, 8);
        b->peakDelay = CakeEwma(b->peakDelay, delayTime, (delayTime > b->peakDelay) ? 2 : 8);
        b->baseDelay = CakeEwma(b->baseDelay, delayTime, (delayTime < b->baseDelay) ? 2 : 8);

        uint32_t len = CakeAdvanceShaper(b, pkt, now, false);
        flow.tinDeficit -= static_cast<int32_t>(len);
        b->tinDeficit -= static_cast<int32_t>(len);

        if (now < m_tNext && GetNPackets() > 0)
        {
            uint64_t next = std::min(m_tNext.GetNanoSeconds(), m_failsafeNext.GetNanoSeconds());
            if (!m_shaperScheduled)
            {
                m_shaperScheduled = true;
                Simulator::Schedule(NanoSeconds(next) - now, &CakeQueueDisc::ShaperWakeup, this);
            }
        }
        else if (GetNPackets() == 0)
        {
            for (uint32_t i = 0; i < m_numTins; i++)
            {
                if (m_tins[i].decayingFlowCount)
                {
                    if (!m_shaperScheduled)
                    {
                        m_shaperScheduled = true;
                        Simulator::Schedule(m_tins[i].cobaltTarget,
                                            &CakeQueueDisc::ShaperWakeup,
                                            this);
                    }
                    break;
                }
            }
        }

        if (m_overflowTimeout)
        {
            m_overflowTimeout--;
        }

        return pkt;
    }
}

void
CakeQueueDisc::ShaperWakeup()
{
    NS_LOG_FUNCTION(this);
    m_shaperScheduled = false;
    Run();
}

} // namespace ns3
