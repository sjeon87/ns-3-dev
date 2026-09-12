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

#ifndef DUAL_PI2_QUEUE_DISC_H
#define DUAL_PI2_QUEUE_DISC_H

#include "queue-disc.h"

#include "ns3/boolean.h"
#include "ns3/data-rate.h"
#include "ns3/event-id.h"
#include "ns3/nstime.h"
#include "ns3/packet.h"
#include "ns3/random-variable-stream.h"
#include "ns3/simulator.h"
#include "ns3/string.h"
#include "ns3/timer.h"
#include "ns3/traced-value.h"

#include <queue>

namespace ns3
{

class UniformRandomVariable;

/**
 * @ingroup traffic-control
 *
 * @brief A DualQ Coupled PI2 queue disc for L4S
 *
 * DualPi2QueueDisc implements the DualPI2 AQM defined in RFC 9332. It maintains
 * two internal queues that share a single buffer: an L4S queue for ECT(1)/CE
 * traffic and a Classic queue for ECT(0)/Not-ECT traffic. A single base
 * probability couples the two queues (Classic drop probability is the square of
 * the base, L4S marking probability is the base scaled by the coupling factor
 * k) along with a step AQM for the L4S queue. Furthermore, a credit-based
 * weighted round-robin scheduler arbitrates between them. The implementation
 * follows the Linux kernel sch_dualpi2.
 */
class DualPi2QueueDisc : public QueueDisc
{
  public:
    /**
     * @brief Get the type ID.
     * @return the object TypeId
     */
    static TypeId GetTypeId(void);

    /**
     * @brief DualPi2QueueDisc Constructor
     */
    DualPi2QueueDisc();

    /**
     * @brief  Destructor
     */
    ~DualPi2QueueDisc() override;

    /**
     * @brief Get the current value of the queue in bytes.
     *
     * @returns The queue size in bytes.
     */
    uint32_t GetQueueSize() const;

    /**
     * @brief Set the limit of the queue in bytes.
     *
     * @param lim The limit in bytes.
     */
    void SetQueueLimit(uint32_t lim);

    // Reasons for dropping packets
    static constexpr const char* FORCED_DROP =
        "Forced drop"; //!< Drops due to queue limit: reactive
    static constexpr const char* OVERLOAD_DROP =
        "Overload drop"; //!< Drops due to overload: reactive
    static constexpr const char* PROBABILISTIC_CLASSIC_MARK =
        "Unforced classic mark"; //!< Mark a classic packet from pi2 probability
    static constexpr const char* PROBABILISTIC_L4S_MARK =
        "Unforced mark in L4S queue"; //!< Mark an L4S packet from pi2 probability
    static constexpr const char* STEP_L4S_MARK =
        "Step L4S mark"; //!< Mark an L4S packet from step AQM

    /**
     * Assign a fixed random variable stream number to the random variables
     * used by this model.  Return the number of streams (possibly zero) that
     * have been assigned.
     *
     * @param stream first stream index to use
     * @return the number of stream indices assigned by this model
     */
    int64_t AssignStreams(int64_t stream);

  protected:
    /**
     * @brief Dispose of the object
     */
    void DoDispose() override;

  private:
    // Documented in base class
    bool DoEnqueue(Ptr<QueueDiscItem> item) override;
    Ptr<QueueDiscItem> DoDequeue() override;
    Ptr<const QueueDiscItem> DoPeek() override;
    bool CheckConfig() override;

    /**
     * @brief Initialize the queue parameters.
     */
    void InitializeParams() override;

    /**
     * @brief check if traffic is classified as L4S (ECT(1) or CE)
     * @param item the QueueDiscItem to check
     * @return true if ECT(1) or CE, false otherwise
     */
    bool IsL4S(Ptr<QueueDiscItem> item);

    /**
     * @brief Periodically calculate the drop probability
     */
    void DualPi2Update();

    /**
     * @brief Apply the L4S step-threshold AQM, marking the item with CE if eligible
     *
     * @param item the QueueDiscItem to check
     * @return true if the item was CE-marked, false otherwise
     */
    bool StepAqm(Ptr<QueueDiscItem> item);

    /**
     * @brief Decide whether to drop the item based on the calculated drop probability
     *
     * @param item the QueueDiscItem to check
     * @return true if the item should be dropped, false otherwise
     */
    bool MustDrop(Ptr<QueueDiscItem> item);

    /**
     * @brief Weight round-robin scheduler to decide which queue to dequeue from
     *
     * @param credit the credit used in the scheduler decision
     * @return the QueueDiscItem to be dequeued, or null if no item should be dequeued
     */
    Ptr<QueueDiscItem> Scheduler(int32_t& credit);

    // Values supplied by user
    Time m_target;          //!< Queue delay target
    Time m_tUpdate;         //!< Time period after which CalculateP() is called
    uint32_t m_mtu;         //!< Device MTU (bytes)
    double m_alpha;         //!< Gain factor for the integral rate response (PI2 controller)
    double m_beta;          //!< Gain factor for the proportional response  (PI2 controller)
    Time m_minTh;           //!< L4S step marking threshold (in time)
    uint32_t m_minThPkts;   //!< L4S step marking threshold (in packets)
    bool m_stepInPackets;   //!< Whether to apply the step based on queue length instead of delay
    double m_k;             //!< Coupling factor
    uint32_t m_queueLimit;  //!< Queue limit in bytes
    Time m_startTime;       //!< Start time of the update timer
    bool m_dropEarly;       //!< Drop at enqueue instead of dequeue
    bool m_dropOverload;    //!< Drop on overload (1) or overflow (0)
    uint32_t m_minQLenStep; //!< Minimum queue length to apply step threshold

    // Scheduler state variables
    uint32_t m_wClassic;  //!< Classic queue weight
    uint32_t m_wL4S;      //!< L4S queue weight
    int32_t m_credit;     //!< Credit for scheduler decision (sign indicates which queue)
    int32_t m_creditInit; //!< Initial credit for scheduler decision

    // Variables maintained by DualPI2
    uint32_t m_thLen;         //!< Minimum threshold (in bytes) for marking
    double m_baseProb;        //!< Variable used in calculation of drop probability
    TracedValue<double> m_pC; //!< Classic drop/mark probability
    TracedValue<double> m_pL; //!< L4S mark probability
    TracedCallback<Time> m_traceClassicSojourn; //!< Classic sojourn time
    TracedCallback<Time> m_traceL4sSojourn;     //!< L4S sojourn time
    Time m_prevQ;                               //!< Old value of queue delay
    EventId m_rtrsEvent; //!< Event used to decide the decision of interval of drop probability
                         //!< calculation
    Ptr<UniformRandomVariable> m_uv; //!< Rng stream
};

} // namespace ns3

#endif
