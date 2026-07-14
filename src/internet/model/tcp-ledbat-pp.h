/*
 * Copyright (c) 2026 NITK Surathkal
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Authors: Ayush Nigam <ash12521198@gmail.com>
 *          S B L Prateek <sblprateek@gmail.com>
 *          A R Sharan Kumar <arsharankumar99@gmail.com>
 *          Yashwanth R <ryashwanth990@gmail.com>
 *          Mohit P. Tahiliani <tahiliani@nitk.edu.in>
 */

#ifndef TCP_LEDBATPP_H
#define TCP_LEDBATPP_H

#include "tcp-congestion-ops.h"

#include "ns3/traced-value.h"

#include <vector>

namespace ns3
{

class TcpLedbatPpSlowStartTest;
class TcpLedbatPpCongestionAvoidanceTest;
class TcpLedbatPpSlowdownTest;

/**
 * @ingroup congestionOps
 *
 * @brief An implementation of LEDBAT++
 *
 * LEDBAT++ is a low priority congestion control. It keeps the queuing delay it
 * induces near a fixed target and yields to standard TCP, which makes it a good
 * fit for background transfers. It builds on LEDBAT (RFC 6817), adding a gain
 * that adapts to the base delay, a modified slow start, and periodic slowdowns
 * that give it a chance to re-measure the base delay.
 */
class TcpLedbatPp : public TcpCongestionOps
{
  public:
    friend class TcpLedbatPpSlowStartTest;
    friend class TcpLedbatPpCongestionAvoidanceTest;
    friend class TcpLedbatPpSlowdownTest;

    /**
     * @brief The phase of the LEDBAT++ state machine
     *
     * A connection opens in the modified slow start of Section 4.3 of the
     * LEDBAT++ draft. Once it leaves, it alternates between congestion avoidance
     * (Sections 4.1 and 4.2) and the periodic slowdowns of Section 4.4.
     */
    enum Phase
    {
        INITIAL_SLOW_START,   //!< Gain-scaled slow start, left early if the queue delay grows
        CONGESTION_AVOIDANCE, //!< Delay-based increase, multiplicative decrease past the target
        SLOWDOWN,             //!< cWnd held at 2 segments, then ramped back up to ssThresh
    };

    /**
     * @brief TracedCallback signature for the LEDBAT++ phase
     *
     * @param oldPhase the previous phase
     * @param newPhase the new phase
     */
    typedef void (*PhaseTracedCallback)(Phase oldPhase, Phase newPhase);

    /**
     * @brief Get the type ID.
     * @return the object TypeId
     */
    static TypeId GetTypeId();

    /**
     * @brief Constructor
     */
    TcpLedbatPp();

    /**
     * @brief Copy constructor
     * @param sock the object to copy
     */
    TcpLedbatPp(const TcpLedbatPp& sock);

    /**
     * @brief Destructor
     */
    ~TcpLedbatPp() override;

    /**
     * @brief Get the name of the TCP flavour
     *
     * @return The name of the TCP
     */
    std::string GetName() const override;

    /**
     * @brief Feed the RTT of the acked packets into the delay history
     *
     * Section 4.5 of the LEDBAT++ draft filters raw round trip samples, so this
     * method reads tcb->m_lastRtt, the RTT of the last acknowledged packet. The
     * rtt argument is left alone: TcpSocketBase passes the smoothed RTT there, and
     * feeding an already averaged value to a minimum filter would defeat it.
     *
     * A duplicate or partial ACK yields no fresh sample, so tcb->m_lastRtt still
     * holds the previous one and it is counted twice. Taking a minimum makes the
     * repeat harmless.
     *
     * @param tcb internal congestion state
     * @param segmentsAcked count of segments ACKed
     * @param rtt the smoothed RTT, unused for the reason given above
     */
    void PktsAcked(Ptr<TcpSocketState> tcb, uint32_t segmentsAcked, const Time& rtt) override;

    // Inherited
    Ptr<TcpCongestionOps> Fork() override;

    /**
     * @brief Adjust cWnd following the LEDBAT++ algorithm
     *
     * Refreshes the delay estimates and the gain, moves the connection between
     * phases as the draft requires, and then leaves the window update itself to
     * the slow start, congestion avoidance or slowdown code.
     *
     * @param tcb internal congestion state
     * @param segmentsAcked count of segments ACKed
     */
    void IncreaseWindow(Ptr<TcpSocketState> tcb, uint32_t segmentsAcked) override;

    /**
     * @brief Compute the slow start threshold after a congestion signal
     *
     * LEDBAT++ backs off the way standard TCP does: it halves the congestion
     * window, but never below MinCwnd segments. Section 1.3 of RFC 6817 asks for
     * an ECN mark to be treated exactly as a loss, so unlike TcpNewReno and
     * TcpLinuxReno this method does not apply the gentler ABE back-off of
     * RFC 8511.
     *
     * @param tcb internal congestion state
     * @param bytesInFlight total bytes in flight
     * @return the new slow start threshold
     */
    uint32_t GetSsThresh(Ptr<const TcpSocketState> tcb, uint32_t bytesInFlight) override;

  protected:
    /**
     * @brief Grow cWnd exponentially, scaled by the dynamic GAIN and capped at ssThresh
     *
     * This is the modified slow start of Section 4.3 of the LEDBAT++ draft. The
     * window grows by a factor of 1 + GAIN every round trip rather than doubling,
     * which keeps the queue LEDBAT++ builds on the way up within reason.
     *
     * @param tcb internal congestion state
     * @param segmentsAcked count of segments ACKed
     * @return the segments left over once the window has been increased
     */
    uint32_t SlowStart(Ptr<TcpSocketState> tcb, uint32_t segmentsAcked);

    /**
     * @brief Run the LEDBAT++ congestion avoidance controller
     *
     * As long as the queue delay stays under the target, the window grows by GAIN
     * segments per round trip. Past the target it shrinks in proportion to the
     * excess delay, by at most half the window per round trip. See Sections 4.1
     * and 4.2 of the LEDBAT++ draft.
     *
     * @param tcb internal congestion state
     * @param segmentsAcked count of segments ACKed
     */
    void CongestionAvoidance(Ptr<TcpSocketState> tcb, uint32_t segmentsAcked);

  private:
    /**
     * @brief A bounded history of RTT samples that keeps track of its own minimum
     */
    struct RttCircBuf
    {
        std::vector<Time> buffer; //!< The RTT samples, oldest first
        uint32_t min;             //!< Index into buffer of the smallest sample
    };

    /**
     * @brief Empty a buffer so that it can be filled from scratch
     *
     * @param buffer The buffer to reset
     */
    void InitCircBuf(RttCircBuf& buffer);

    /**
     * @brief Return the smallest RTT sample a buffer holds
     *
     * @param b The buffer
     * @return The smallest sample, or Time::Max() when the buffer is empty
     */
    static Time MinCircBuf(RttCircBuf& b);

    /**
     * @brief Return the current delay estimate
     *
     * Section 4.5 of the LEDBAT++ draft filters the noise out of the RTT by taking
     * the minimum over the most recent samples.
     *
     * @return The smallest RTT in the noise filter
     */
    Time CurrentDelay();

    /**
     * @brief Return the base delay estimate
     *
     * @return The smallest RTT in the base delay history, which is the best guess
     *         at what the path costs with an empty queue
     */
    Time BaseDelay();

    /**
     * @brief Append an RTT sample to a buffer, dropping the oldest if it is full
     *
     * @param cb The buffer
     * @param rttVal The sample to add
     * @param maxlen The number of samples the buffer may hold
     */
    void AddDelay(RttCircBuf& cb, Time rttVal, uint32_t maxlen);

    /**
     * @brief Carry out the periodic slowdown of Section 4.4 of the LEDBAT++ draft
     *
     * cWnd is pinned at 2 segments for two round trips and then ramped back up to
     * ssThresh. Draining the bottleneck this way lets the next base delay sample
     * be taken on an empty path, so a standing queue cannot pass for base delay.
     *
     * @param tcb internal congestion state
     * @param segmentsAcked count of segments ACKed
     * @param currTime current time
     */
    void SlowDown(Ptr<TcpSocketState> tcb, uint32_t segmentsAcked, Time currTime);

    /**
     * @brief Fold an RTT sample into the base delay history
     *
     * Every entry of the history is the smallest RTT seen over one minute. A fresh
     * sample either pulls the current minute's entry down or, once that minute has
     * elapsed, opens a new entry.
     *
     * @param rttVal The RTT sample
     */
    void UpdateBaseDelay(Time rttVal);

    uint32_t m_baseHistoLen;   //!< Number of one-minute minima kept in the base delay history
    uint32_t m_noiseFilterLen; //!< Number of recent RTT samples kept in the noise filter
    Time m_lastRollover;       //!< When the base delay history last opened a new entry
    RttCircBuf m_baseHistory;  //!< Per-minute minimum RTTs, from which the base delay is taken
    RttCircBuf m_noiseFilter;  //!< Recent RTT samples, from which the current delay is taken
    uint32_t m_minCwnd;        //!< Smallest cWnd LEDBAT++ will settle for, in segments
    Time m_nextSdStart{Time::Max()};  //!< When the next slowdown is due to start
    Time m_slowdownEntry;             //!< When the current slowdown started
    Time m_freeze;                    //!< Time until which cWnd is held at 2 segments
    Time m_duration;                  //!< How long the previous slowdown lasted
    TracedValue<Time> m_currentDelay; //!< Filtered RTT, the minimum over the noise filter
    Time m_target;                    //!< Queue delay LEDBAT++ aims to hold
    TracedValue<double> m_gain{1.0};  //!< Dynamic GAIN value (LEDBAT++ draft Section 4.1)
    TracedValue<Time> m_queueDelay;   //!< Current delay less base delay
    TracedValue<Time> m_baseDelay;    //!< Smallest RTT across the whole base delay history
    double m_constant;                //!< Scales the multiplicative decrease (draft Section 4.2)
    TracedValue<Phase> m_state{INITIAL_SLOW_START}; //!< Phase the connection is currently in
};

} // namespace ns3

#endif /* TCP_LEDBATPP_H */
