/*
 * Copyright (c) 2019-2026 NITK Surathkal
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 *
 *
 * Author: Shikha Bakshi <shikhabakshi912@gmail.com>
 *         Mohit P. Tahiliani <tahiliani@nitk.edu.in>
 *         Keerthana Polkampally <keerthana.keetu.p@gmail.com>
 *         Archana Priyadarshani Sahoo <archana98priya@gmail.com>
 *         Durvesh Shyam  Bhalekar <durvesh.5.db@gmail.com>
 *         Mohnish Hemanth Kumar <mohnishhemanthkumar@gmail.com>
 *         Nikhil Kottoli <nikhilkottoli2005@gmail.com>
 *         Manish Agarwal <manishagarwal428728@gmail.com>
 *         Patel Pal Bharat <ppal61679@gmail.com>
 */
#pragma once

#include "tcp-option-sack.h"
#include "tcp-socket-state.h"

#include "ns3/nstime.h"
#include "ns3/object.h"
#include "ns3/packet.h"
#include "ns3/sequence-number.h"
#include "ns3/simulator.h"
#include "ns3/traced-value.h"

namespace ns3
{

class TcpTlp : public Object
{
  public:
    /**
     * @brief Get the type ID.
     * @return the object TypeId
     */
    static TypeId GetTypeId();

    /**
     * @brief Constructor
     */
    TcpTlp();

    /**
     * @brief Copy constructor.
     * @param other object to copy.
     */
    TcpTlp(const TcpTlp& other);

    /**
     * @brief Deconstructor
     */
    virtual ~TcpTlp();

    /**
     * @brief Calculates Pto
     *
     * @param srtt smoother round trip time
     * @param inflight flight size
     * @param rto retransmission timeout
     * @return Probe Timeout
     */
    Time CalculatePto(Time srtt, uint32_t inflight, double rto);

    /**
     * @brief Reset TLP state.
     *
     * Clears outstanding probe state and retransmission flags.
     * Called when entering connection establishment, fast recovery,
     * or RTO recovery (RFC 8985 Section 7.1).
     *
     * @param currentSamples current number of RTT samples
     */
    void Reset(uint32_t currentSamples);

    /**
     * @brief Record transmission of a loss probe.
     *
     * Updates internal state to indicate that a probe is outstanding.
     *
     * @param endSeq Ending sequence number of the probe.
     * @param isRetrans True if probe is a retransmission.
     * @param rttSamples Current RTT sample count.
     */
    void OnProbeSent(SequenceNumber32 endSeq, bool isRetrans, uint32_t rttSamples);

    /**
     * @brief Process ACK for TLP recovery detection.
     *
     * Implements RFC 8985 Section 7.4 to detect whether a loss probe
     * repaired a packet loss or was spurious.
     *
     * @param ack Cumulative ACK number.
     * @param hasDsack True if ACK contains a DSACK option.
     * @param isDupAckNoSack True if ACK is a duplicate ACK without SACK.
     * @return True if TLP recovery is detected
     */
    bool OnAckReceived(SequenceNumber32 ack, bool hasDsack, bool isDupAckNoSack);

    /**
     * @brief Check whether a loss probe may be sent.
     *
     * Verifies that no previous probe is outstanding and that
     * a new RTT sample has been obtained since the last probe.
     *
     * @param currRttSamples Current number of RTT samples.
     * @return True if a probe may be sent.
     */
    bool CanSendProbe(uint32_t currRttSamples) const;
    /**
     * @brief Check whether a loss probe is currently outstanding.
     *
     * @return True if a probe has been sent and not yet acknowledged.
     */
    bool IsProbeOutstanding() const;

  private:
    Time m_maxAckDelay{MilliSeconds(200)}; //!< Maximum delayed ACK allowance (RFC 8985)
    Time m_pto{0};                         //!< Last computed probe timeout (PTO)
    SequenceNumber32 m_tlpEndSeq{0};       //!< End sequence number of outstanding TLP probe
    bool m_tlpIsRetrans{false};            //!< True if current probe is a retransmission
    uint32_t m_lastRttSamples{0};          //!< RTT sample count at last probe transmission
};
} // namespace ns3

/* TCP_TLP_H */
