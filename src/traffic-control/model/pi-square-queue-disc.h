/*
 * Copyright (c) 2017 Trinity College Dublin
 * Copyright (c) 2025-26 NITK Surathkal (Porting to ns-3)
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Authors: Rohit P. Tahiliani <rohit.tahil@gmail.com>
 *
 */

#ifndef PI_SQUARE_QUEUE_DISC_H
#define PI_SQUARE_QUEUE_DISC_H

#include "queue-disc.h"

#include "ns3/boolean.h"
#include "ns3/data-rate.h"
#include "ns3/event-id.h"
#include "ns3/nstime.h"
#include "ns3/packet.h"
#include "ns3/random-variable-stream.h"
#include "ns3/timer.h"

#include <queue>

namespace ns3
{

class TraceContainer;
class UniformRandomVariable;

/**
 * @ingroup traffic-control
 *
 * @brief Implements PI Square queue discipline.
 *
 * This model does not support PIE features such as ECN,
 * autotuning, and optional design elements from RFC 8033.
 */
class PiSquareQueueDisc : public QueueDisc
{
  public:
    /**
     * @brief Get the type ID.
     * @return the object TypeId
     */
    static TypeId GetTypeId();

    /**
     * @brief PiSquareQueueDisc Constructor
     */
    PiSquareQueueDisc();

    /**
     * @brief PiSquareQueueDisc Destructor
     */
    ~PiSquareQueueDisc() override;

    /**
     * @brief Get queue delay
     *
     * @returns The current queue delay.
     */
    Time GetQueueDelay();

    /**
     * Assign a fixed random variable stream number to the random variables
     * used by this model.  Return the number of streams (possibly zero) that
     * have been assigned.
     *
     * @param stream first stream index to use
     * @return the number of stream indices assigned by this model
     */
    int64_t AssignStreams(int64_t stream);

    static constexpr const char* UNFORCED_DROP =
        "Unforced drop"; //!< Early probability drops: proactive
    static constexpr const char* FORCED_DROP =
        "Forced drop"; //!< Drops due to queue limit: reactive
    static constexpr const char* UNFORCED_MARK =
        "Unforced mark"; //!< Early probability marks: proactive
    static constexpr const char* CE_THRESHOLD_EXCEEDED_MARK =
        "CE threshold exceeded mark"; //!< Early probability marks: proactive

  protected:
    /**
     * @brief Dispose of the object
     */
    void DoDispose() override;

  private:
    bool DoEnqueue(Ptr<QueueDiscItem> item) override;
    Ptr<QueueDiscItem> DoDequeue() override;
    bool CheckConfig() override;

    /**
     * @brief Initialize the queue parameters.
     */
    void InitializeParams() override;

    /**
     * @brief Check if a packet needs to be dropped due to probability drop
     * @param item queue item
     * @param qSize queue size
     * @returns 0 for no drop, 1 for drop
     */
    bool DropEarly(Ptr<QueueDiscItem> item, uint32_t qSize);

    /**
     * @brief Periodically calculate the drop probability
     */
    void CalculateP();

    Stats m_stats; //!< PI Square statistics

    // ** Variables supplied by user
    Time m_sUpdate;         //!< Start time of the update timer
    Time m_tUpdate;         //!< Time period after which CalculateP () is called
    Time m_qDelayRef;       //!< Desired queue delay
    uint32_t m_meanPktSize; //!< Average packet size in bytes
    double m_a;             //!< Parameter to PI Square controller
    double m_b;             //!< Parameter to PI Square controller
    uint32_t m_dqThreshold; //!< Minimum queue size in bytes before dequeue rate is measured

    // ** Variables maintained by PI Square
    TracedValue<double> m_dropProb; //!< Variable used in calculation of drop probability
    Time m_qDelayOld;               //!< Old value of queue delay
    Time m_qDelay;                  //!< Current value of queue delay
    bool m_inMeasurement;           //!< Indicates whether we are in a measurement cycle
    double m_avgDqRate;             //!< Time averaged dequeue rate
    double m_dqStart;               //!< Start timestamp of current measurement cycle
    uint32_t m_dqCount;  //!< Number of bytes departed since current measurement cycle starts
    EventId m_rtrsEvent; //!< Event used to decide the decision of interval of drop probability
                         //!< calculation
    Ptr<UniformRandomVariable> m_uv; //!< Rng stream
};

}; // namespace ns3

#endif
