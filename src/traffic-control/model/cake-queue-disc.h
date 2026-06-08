/*
 * Copyright (c) 2026 Shivang Upadhyay
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * CAKE queue discipline implementation for ns-3.
 *
 * Developed with reference to the Linux kernel
 * sch_cake.c implementation.
 */

#ifndef CAKE_QUEUE_DISC_H
#define CAKE_QUEUE_DISC_H

#include "queue-disc.h"

#include "ns3/data-rate.h"
#include "ns3/nstime.h"
#include "ns3/random-variable-stream.h"
#include "ns3/tag.h"

#include <list>
#include <vector>

namespace ns3
{

/**
 * @brief Packet tag carrying the simulation time at which a packet was enqueued.
 *
 * DoEnqueue() stamps every packet with this tag.  DoDequeue() peeks it,
 * computes sojourn = Now() - enqueueTime, and stores the result in
 * CakeFlow::sojournTime.
 */
class CakeSojournTag : public Tag
{
  public:
    /**
     * @brief Get the TypeId for CakeSojournTag.
     * @return The TypeId.
     */
    static TypeId GetTypeId()
    {
        static TypeId tid = TypeId("ns3::CakeSojournTag")
                                .SetParent<Tag>()
                                .SetGroupName("TrafficControl")
                                .AddConstructor<CakeSojournTag>();
        return tid;
    }

    TypeId GetInstanceTypeId() const override
    {
        return GetTypeId();
    }

    /**
     * @brief Serialised size: one int64 nanosecond timestamp.
     * @return The serialized size in bytes.
     */
    uint32_t GetSerializedSize() const override
    {
        return 8;
    }

    /**
     * @brief Serialise the tag content into a buffer.
     * @param buf The buffer to write to.
     */
    void Serialize(TagBuffer buf) const override
    {
        buf.WriteU64(static_cast<uint64_t>(m_enqueueTime.GetNanoSeconds()));
    }

    /**
     * @brief Deserialise the tag content from a buffer.
     * @param buf The buffer to read from.
     */
    void Deserialize(TagBuffer buf) override
    {
        m_enqueueTime = NanoSeconds(static_cast<int64_t>(buf.ReadU64()));
    }

    /**
     * @brief Print the tag content.
     * @param os The output stream to print to.
     */
    void Print(std::ostream& os) const override
    {
        os << "CakeSojournTag enqueueTime=" << m_enqueueTime.GetNanoSeconds() << "ns";
    }

    /**
     * @brief Set the enqueue timestamp.
     * @param t The simulation time to set as enqueue time.
     */
    void SetEnqueueTime(Time t)
    {
        m_enqueueTime = t;
    }

    /**
     * @brief Retrieve the enqueue timestamp.
     * @return The simulation time when the packet was enqueued.
     */
    Time GetEnqueueTime() const
    {
        return m_enqueueTime;
    }

  private:
    Time m_enqueueTime{Seconds(0)}; //!< Wall-clock time of enqueue.
};

/**
 * @ingroup traffic-control
 * @brief CAKE (Common Applications Kept Enhanced) Queue Discipline.
 *
 * Implements the four framework pillars: rate-based shaping, 8-way flow hashing,
 * per-host DRR fairness, and DiffServ priority tins [cite: 21-25, 94].
 * DSCP-to-tin classification is handled via PacketFilter subclasses[cite: 96].
 */
class CakeQueueDisc : public QueueDisc
{
  public:
    /** Maximum number of DiffServ tins. */
    static const uint32_t CAKE_MAX_TINS = 8;
    /** Number of ways in the set-associative hash table. */
    static const uint32_t CAKE_SET_WAYS = 8;

    /** @brief Link layer framing modes for overhead calculation. */
    enum CellMode : uint32_t
    {
        CAKE_NONE = 0, //!< Standard Ethernet
        CAKE_ATM = 1,  //!< Asynchronous Transfer Mode (ADSL)
        CAKE_PTM = 2   //!< Packet Transfer Mode (VDSL2)
    };

    /** @brief Configuration flags for CAKE queue operations. */
    enum RateFlags : uint32_t
    {
        CAKE_FLAG_OVERHEAD = (1 << 0),         //!< Subtract MAC framing overhead
        CAKE_FLAG_AUTORATE_INGRESS = (1 << 1), //!< Auto-rate adjustment for ingress
        CAKE_FLAG_INGRESS = (1 << 2),          //!< Ingress shaping mode
        CAKE_FLAG_WASH = (1 << 3),             //!< Wash (clear) DiffServ/ECN marks
        CAKE_FLAG_SPLIT_GSO = (1 << 4)         //!< Split GSO super-packets
    };

    /** @brief DiffServ preset modes. */
    enum DiffServMode : uint32_t
    {
        DIFFSERV_BESTEFFORT = 0, //!< Single tin; all traffic treated equally
        DIFFSERV_DIFFSERV3 = 1,  //!< 3 tins: Bg / Best Effort / Latency Sensitive
        DIFFSERV_DIFFSERV4 = 2,  //!< 4 tins: Bg / Best Effort / Streaming / Latency Sensitive
        DIFFSERV_DIFFSERV8 = 3,  //!< 8 tins pruned from RFC 4594 (Network Ctrl … Bg)
        DIFFSERV_PRECEDENCE = 4, //!< 8 tins mapped by IP Precedence (CS0–CS7)
    };

    /** @brief Flow states for DRR rotation (CAKE_SET_* in Linux). */
    enum FlowSet : uint16_t
    {
        CAKE_SET_NONE = 0,        //!< No queue membership; inactive flow slot.
        CAKE_SET_SPARSE = 1,      //!< Sparse/low-rate flow in the new-flow rotation.
        CAKE_SET_SPARSE_WAIT = 2, //!< Sparse flow waiting for deficit/accounting update.
        CAKE_SET_BULK = 3,        //!< Bulk/high-rate flow in the old-flow rotation.
        CAKE_SET_DECAYING = 4     //!<  Empty flow retained temporarily for COBALT decay state.
    };

    /** @brief Host isolation modes (Algorithm 2 of the paper). */
    enum IsolationMode : uint32_t
    {
        ISOLATION_NONE = 0,   //!< Flow-level fairness only.
        ISOLATION_SRC = 1,    //!< Isolate by source host.
        ISOLATION_DST = 2,    //!< Isolate by destination host.
        ISOLATION_TRIPLE = 3, //!< Isolate by max of src and dst refcount.
    };

    /** @brief TCP ACK filter aggressiveness. Not yet implemented. */
    enum AckFilterMode : uint32_t
    {
        ACK_FILTER_NONE = 0,         //!< No ACK filtering.
        ACK_FILTER_CONSERVATIVE = 1, //!< Keep at least two ACKs per flow.
        ACK_FILTER_AGGRESSIVE = 2,   //!< Keep only the newest ACK per flow.
    };

    /**
     * @brief Per-flow state stored in the 8-way hash table.
     */
    struct CakeFlow
    {
        uint32_t flowHash{0};     //!< Hash of the 5-tuple.
        uint32_t srcHash{0};      //!< Source host bucket index.
        uint32_t dstHash{0};      //!< Destination host bucket index.
        int32_t tinDeficit{0};    //!< DRR deficit counter (signed).
        uint32_t quantum{1514};   //!< DRR quantum in bytes.
        uint32_t backlogBytes{0}; //!< Bytes currently queued for this flow.
        uint8_t tinIndex{0};      //!< DiffServ tin this flow belongs to.

        FlowSet set{CAKE_SET_NONE}; //!< Current flow state (Sparse, Bulk, Decaying)
        uint32_t dropped{0};        //!< Number of dropped packets for this flow

        bool cobaltDropping{false};      //!< CoDel dropping state active.
        Time cobaltDropNext{Seconds(0)}; //!< Scheduled time of next CoDel drop.
        uint32_t cobaltCount{0};         //!< Drop count for CoDel control law.
        double blueProb{0.0};            //!< BLUE drop probability in [0, 1].
        bool ecnMarked{false};           //!< True if the packet was ECN-marked instead of dropped
    };

    /**
     * @brief Per-IP reference counts used for host isolation.
     */
    struct CakeHostBucket
    {
        uint32_t srchostTag{0}; //!< Hash tag for the source host
        uint32_t dsthostTag{0}; //!< Hash tag for the destination host

        uint32_t srchostBulkFlowCount{0}; //!< Bulk flows sourced from this host
        uint32_t dsthostBulkFlowCount{0}; //!< Bulk flows destined to this host
    };

    /**
     * @brief One DiffServ priority tier with its own rate and AQM state.
     */
    struct CakeTin
    {
        DataRate targetRate;                   //!< Target shaper rate for this specific tin.
        Time virtualClock{Seconds(0)};         //!< Virtual clock for inter-tin Deficit Round Robin.
        Time cobaltTarget{MicroSeconds(5000)}; //!< COBALT sojourn target delay.
        Time cobaltInterval{MilliSeconds(100)}; //!< COBALT control loop interval.
        uint32_t backlogBytes{0};   //!< Total bytes queued across all flows in this tin.
        uint32_t tinQuantum{1514};  //!< Base DRR quantum for flows in this tin.
        uint32_t flowQuantum{1514}; //!< per-flow deficit quantum.

        // DRR flow balancers
        uint32_t sparseFlowCount{0};   //!< Number of active sparse flows (new or short bursts).
        uint32_t bulkFlowCount{0};     //!< Number of active bulk flows (heavy, continuous traffic).
        uint32_t decayingFlowCount{0}; //!< Number of empty flows kept alive to decay AQM state.
        uint32_t unresponsiveFlowCount{0}; //!< Number of flows not responding to COBALT drops.

        // Tin shaping and DRR variables
        int32_t tinDeficit{0};           //!< DRR deficit counter for unshaped tin balancing.
        uint32_t tinDropped{0};          //!< Total number of packets dropped from this tin.
        Time timeNextPacket{Seconds(0)}; //!< Earliest time this tin is scheduled to transmit.

        // Delay EWMA (Exponentially Weighted Moving Average) stats
        Time avgeDelay{Seconds(0)}; //!< Moving average of packet sojourn time (queuing delay).
        Time baseDelay{Seconds(0)}; //!< Moving average of the minimum/base queuing delay.
        Time peakDelay{Seconds(0)}; //!< Moving average of the maximum/peak queuing delay.

        uint32_t tinEcnMark{0}; //!< Total packets ECN-marked in this tin.
    };

    /**
     * @brief Get the TypeId for this class.
     * @return The TypeId.
     */
    static TypeId GetTypeId();

    CakeQueueDisc();
    ~CakeQueueDisc() override;

  protected:
    /**
     * @brief Enqueue a packet into the CAKE queue disc.
     * @param item The packet to enqueue.
     * @return True if the packet was enqueued, false otherwise.
     */
    bool DoEnqueue(Ptr<QueueDiscItem> item) override;

    /**
     * @brief Dequeue a packet from the CAKE queue disc.
     * @return The dequeued packet, or nullptr if none available.
     */
    Ptr<QueueDiscItem> DoDequeue() override;

    /**
     * @brief Enforce the global queue limit by dropping from the head of the fattest flow.
     * * Scans all flows to find the one with the largest byte backlog, dequeues the
     * head packet from that flow, updates byte counters, and records the drop.
     */
    void CakeDrop();

    /**
     * @brief Check the configuration of the CAKE queue disc.
     * @return True if the configuration is valid, false otherwise.
     */
    bool CheckConfig() override;

    /** @brief Initialize the CAKE queue disc parameters. */
    void InitializeParams() override;

  private:
    /**
     * @brief Update shaper tracking timers and charge packet transmission duration.
     * @param b Pointer to the CakeTin currently processing the transmission.
     * @param pkt The packet being evaluated.
     * @param now Current simulation time.
     * @param drop True if this calculation is for a dropped packet.
     * @return Size of the packet in bytes.
     */
    uint32_t CakeAdvanceShaper(CakeTin* b, Ptr<QueueDiscItem> pkt, Time now, bool drop);

    /**
     * @brief Calculate packet size including CAKE overhead compensation.
     * @param item Queue disc item being evaluated.
     * @return Packet size including configured overhead adjustments.
     */
    uint32_t CakeOverhead(Ptr<QueueDiscItem> item);

    /**
     * @brief Calculate the physical on-the-wire packet size including framing overhead.
     * @param len Packet length in bytes.
     * @param off Network offset adjustment.
     * @return Adjusted packet size including CAKE overhead and framing compensation.
     */
    uint32_t CakeCalcOverhead(uint32_t len, uint32_t off);

    /**
     * @brief Check if destination host isolation is active for the current mode.
     * @return True if destination host isolation is enabled, false otherwise.
     */
    bool CakeDdst() const;

    /**
     * @brief Check if source host isolation is active for the current mode.
     * @return True if source host isolation is enabled, false otherwise.
     */
    bool CakeDsrc() const;

    /**
     * @brief Compute the Exponentially Weighted Moving Average (EWMA) for latency tracking.
     * @param avg Current running average state.
     * @param sample The new time measurement.
     * @param shift The bit-shift weight parameter.
     * @return The updated moving average as a Time object.
     */

    Time CakeEwma(Time avg, Time sample, uint32_t shift) const;
    /**
     * @brief Compute a flow hash using QueueDiscItem::Hash().
     * @param item The packet to hash.
     * @return A flow bucket index in [0, m_flows).
     */
    uint32_t FlowHash(Ptr<const QueueDiscItem> item) const;

    /**
     * @brief Hash a 32-bit value into a host bucket index.
     * @param val Input value.
     * @return A host bucket index in [0, m_flows).
     */
    uint32_t HostHash(uint32_t val) const;

    /**
     * @brief Find or claim a flow slot using 8-way set-associative lookup.
     * @param flowHash Flow hash value.
     * @param srcHash Source host hash.
     * @param dstHash Destination host hash.
     * @param tin Tin index for a newly initialised slot.
     * @return Index into m_flowBuckets.
     */
    uint32_t SetAssocHashLookup(uint32_t flowHash, uint32_t srcHash, uint32_t dstHash, uint8_t tin);

    /**
     * @brief Map a DSCP value to a tin index for the current DiffServMode.
     * @param dscp 6-bit DSCP value.
     * @return Tin index.
     */
    uint8_t DscpToTin(uint8_t dscp) const;

    /** @brief Configure per-tin bandwidth fractions for the current DiffServMode. */
    void SetupTins();

    /** @brief Called by Simulator::Schedule when the shaper gate opens. */
    void ShaperWakeup();

    /**
     * @brief Compute the next CoDel drop time using the control law.
     * @param t Base time (last drop or now).
     * @param interval COBALT control interval for this tin.
     * @param count Current drop count.
     * @return Scheduled time for the next drop event.
     */
    Time CobaltControlLaw(Time t, Time interval, uint32_t count) const;

    /**
     * @brief Reset COBALT flow variables when a queue becomes entirely empty.
     * @param flow Reference to the CakeFlow entry being emptied.
     * @param now Current simulation time.
     * @return True if the flow was previously in an active cobalt dropping loop.
     */
    bool CobaltQueueEmpty(CakeFlow& flow, Time now) const;

    /**
     * @brief Compute the DRR quantum for a flow, accounting for host isolation.
     *
     * Direct equivalent of cake_get_flow_quantum() in sch_cake.c (line 688).
     * Called on both initial sparse assignment (DoEnqueue) and deficit
     * top-up (DoDequeue) to ensure consistent host-load weighting.
     *
     * @param tinIndex Tin index the flow belongs to.
     * @param flowIndex Index into m_flowBuckets.
     * @return Quantum in bytes, with dithering applied.
     */
    uint16_t CakeGetFlowQuantum(uint32_t tinIndex, uint32_t flowIndex) const;

    /**
     * @brief Run the COBALT state machine for one dequeued packet.
     *
     * Updates per-tin CoDel and BLUE state. Attempts ECN marking before
     * dropping. Must be called after the tin backlog has been decremented.
     *
     * @param tin Tin index.
     * @param flowIndex Flow queue index associated with the packet.
     * @param sojourn Measured packet sojourn time.
     * @param item Dequeued packet which may be ECN-marked in-place.
     * @param bulkFlows Number of currently active bulk flows.
     * @return True if the packet should be dropped.
     */
    bool CobaltShouldDrop(uint32_t tin,
                          uint32_t flowIndex,
                          Time sojourn,
                          Ptr<QueueDiscItem> item,
                          uint32_t bulkFlows);

    DataRate m_bandwidth;     //!< Target shaper rate.
    Time m_target;            //!< AQM sojourn target.
    Time m_interval;          //!< AQM control interval.
    uint32_t m_flows;         //!< Number of flow hash buckets.
    uint32_t m_overhead;      //!< Per-packet framing overhead in bytes.
    uint32_t m_diffServMode;  //!< DiffServ preset (0, 3, or 4).
    uint32_t m_isolationMode; //!< Host isolation mode.
    uint32_t m_ackFilterMode; //!< ACK filter aggressiveness.
    uint32_t m_perturbation;  //!< Hash perturbation seed.

    Time m_mtuTime; //!< Serialisation delay of maximum-size packet.

    uint32_t m_numTins;     //!< Number of active tins.
    Time m_tNext;           //!< Earliest time the shaper allows dequeue.
    bool m_shaperScheduled; //!< Whether a ShaperWakeup event is pending.

    Time m_failsafeNext{Seconds(0)}; //!< Global failsafe shaper time
    uint32_t m_curTin{0};            //!< Tin index currently being serviced
    uint32_t m_curFlow{0};           //!< Flow index currently being serviced
    uint16_t m_overflowTimeout{0};   //!< Heap rebuilding timeout
    bool m_isIngress{false};         //!< Ingress shaping flag
    uint32_t m_rateFlags{0};         //!< Bitmask of active CAKE_FLAG_* settings

    uint32_t m_mpu{64};             //!< Minimum Packet Unit in bytes
    CellMode m_cellMode{CAKE_NONE}; //!< Link layer cell framing mode
    uint32_t m_avgNetoff{0};        //!< EWMA tracker for network offset

    uint32_t m_maxNetlen{0};   //!< Maximum observed network layer length
    uint32_t m_maxAdjlen{0};   //!< Maximum observed adjusted link length
    uint32_t m_minNetlen{~0U}; //!< Minimum observed network layer length (init to max uint32)
    uint32_t m_minAdjlen{~0U}; //!< Minimum observed adjusted link length (init to max uint32)

    std::vector<CakeFlow> m_flowBuckets;              //!< Per-flow state table.
    std::vector<CakeHostBucket> m_srcHosts;           //!< Source host reference counts.
    std::vector<CakeHostBucket> m_dstHosts;           //!< Destination host reference counts.
    std::vector<CakeTin> m_tins;                      //!< Per-tin state.
    std::vector<std::list<uint32_t>> m_newFlows;      //!< Sparse flows
    std::vector<std::list<uint32_t>> m_oldFlows;      //!< Bulk flows
    std::vector<std::list<uint32_t>> m_decayingFlows; //!< Empty flows maintaining AQM state

    Ptr<UniformRandomVariable> m_uv;    //!< RNG used to auto-generate perturbation seed.
    std::vector<uint32_t> m_quantumDiv; //!< Pre-computed division array for Triple Isolate
};

} // namespace ns3

#endif /* CAKE_QUEUE_DISC_H */
