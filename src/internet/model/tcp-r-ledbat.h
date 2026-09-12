/*
 * Copyright (c) 2026 NITK Surathkal
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author:  Jayesh Akot <akotjayesh@gmail.com>
 *          S B L Prateek <sblprateek@gmail.com>
 *          A R Sharan Kumar <arsharankumar99@gmail.com>
 *          Mohit P. Tahiliani <tahiliani@nitk.edu.in>
 */

#ifndef TCP_RLEDBAT_H
#define TCP_RLEDBAT_H

#include "tcp-socket-base.h"

#include <vector>

namespace ns3
{

/**
 * @ingroup socket
 * @brief Receiver-driven Low Extra Delay Background Transport (rLEDBAT)
 *
 * Implementation of the receiver-side LEDBAT congestion controller as
 * specified in RFC 9840. The receiver controls the sender's transmit rate
 * by advertising a receiver-computed window (RLWND) in place of the
 * standard receive window:
 *
 *   RCV.WND = min(RLWND, fcwnd)
 *
 * where fcwnd is the flow-control window computed from available buffer
 * space. RLWND is adjusted each time a data segment arrives based on the
 * one-way queuing delay (OWD) estimated from TCP Timestamps (RFC 7323),
 * a retransmission signal, and a slow-start phase identical in structure
 * to sender-side LEDBAT.
 *
 * References:
 *   - RFC 9840: Receiver-Driven LEDBAT (rLEDBAT)
 *   - RFC 6817: Low Extra Delay Background Transport (LEDBAT)
 *   - RFC 7323: TCP Extensions for High Performance (TCP Timestamps)
 *   - RFC 9293: Transmission Control Protocol (TCP)
 */
class TcpRLedbat : public TcpSocketBase
{
  public:
    /**
     * @brief Slow-start behaviour selector.
     */
    enum SlowStartType
    {
        DO_NOT_SLOWSTART, //!< Disable slow start; enter CA immediately.
        DO_SLOWSTART,     //!< Enable slow start (default).
    };

    /**
     * @brief Get the TypeId.
     * @return The TypeId for this class.
     */
    static TypeId GetTypeId();

    TcpRLedbat();

    /**
     * @brief Copy constructor.
     * @param sock The socket object to copy.
     */
    TcpRLedbat(const TcpRLedbat& sock);

    ~TcpRLedbat() override;

    /**
     * @brief Return the current receiver-computed window (RLWND) in bytes.
     * @return Current RLWND value.
     */
    uint32_t GetRLWND() const
    {
        return m_RLWND;
    }

    /**
     * @brief Process an arriving data segment and update RLWND.
     * @param p         The received packet.
     * @param tcpHeader The TCP header of the received packet.
     */
    void ReceivedData(Ptr<Packet> p, const TcpHeader& tcpHeader) override;

    /**
     * @brief Returns the window size to be advertised to the sender.
     *
     * Computes min(RLWND, fcwnd), floored at the configured minimum.
     *
     * @param scale If true, return scaled window.
     * @return The advertised window size in bytes.
     */
    uint16_t AdvertisedWindowSize(bool scale) const override;

    /**
     * @brief Enable or disable slow start.
     * @param doSS DO_SLOWSTART to enable, DO_NOT_SLOWSTART to disable.
     */
    void SetDoSs(SlowStartType doSS);

    /**
     * @brief Create a copy of this socket (used by the accept path).
     * @return A new TcpRLedbat socket that is a deep copy of this one.
     */
    Ptr<TcpSocketBase> Fork() override;

  private:
    /**
     * @brief Circular buffer for OWD noise filtering and base-delay tracking.
     */
    struct OwdCircBuf
    {
        std::vector<uint32_t> buffer; //!< Delay samples (milliseconds).
        size_t min;                   //!< Index of the current minimum sample.
    };

    /**
     * @brief Initialise a circular buffer to empty.
     * @param buffer Buffer to initialise.
     */
    void InitCircBuf(OwdCircBuf& buffer);

    /**
     * @brief Return the minimum value stored in a circular buffer.
     * @param b The buffer to query.
     * @return Minimum OWD sample (ms), or UINT32_MAX if the buffer is empty.
     */
    static uint32_t MinCircBuf(OwdCircBuf& b);

    /**
     * @brief Add a new OWD sample to a circular buffer.
     * @param cb     Buffer to update.
     * @param owd    New OWD sample in milliseconds.
     * @param maxlen Maximum number of samples to retain.
     */
    void AddDelay(OwdCircBuf& cb, uint32_t owd, uint32_t maxlen);

    /**
     * @brief Update the base-delay history with a new OWD sample.
     *
     * Within each 60-second epoch only the minimum OWD is retained
     * (per RFC 6817 Section 4.3).
     *
     * @param owd New OWD sample in milliseconds.
     */
    void UpdateBaseDelay(uint32_t owd);

    /**
     * @brief Detect whether the arriving segment is a retransmission.
     *
     * Per RFC 9840 Section 4.3, a segment is a retransmission when:
     *   SEG.SEQ < RCV.HGH  AND  TSV.SEQ > TSV.HGH
     *
     * @param segSeq Sequence number (SEG.SEQ) of the arriving segment.
     * @param tsvSeq TSval (TSV.SEQ) of the arriving segment.
     * @return true if the segment is identified as a retransmission.
     */
    bool DetectRetransmission(SequenceNumber32 segSeq, uint32_t tsvSeq);

    /**
     * @brief Increase RLWND by amount bytes.
     * @param amount Bytes to add to RLWND.
     */
    void IncreaseWindow(uint32_t amount);

    /**
     * @brief Drain RLWND gradually by at most ackedBytes per call.
     *
     * Implements the window-shrinking avoidance rule of RFC 9293 as
     * described in RFC 9840 Section 4.1.1.
     *
     * @param ackedBytes Payload bytes in the received segment.
     */
    void DecreaseWindow(uint32_t ackedBytes);

    Time m_target;             //!< Target queuing delay (default 100 ms).
    double m_gain;             //!< Offset-from-target gain factor.
    SlowStartType m_doSs;      //!< Whether slow start is enabled.
    uint32_t m_baseHistoLen;   //!< Maximum number of base-delay epochs.
    uint32_t m_noiseFilterLen; //!< Maximum number of noise-filter samples.
    Time m_lastRollover;       //!< Timestamp of the last epoch rollover.
    OwdCircBuf m_baseHistory;  //!< Per-epoch minimum OWD samples.
    OwdCircBuf m_noiseFilter;  //!< Recent OWD samples for current-delay estimation.
    uint32_t m_RLWND;          //!< Receiver-computed window (bytes).
    uint32_t m_minRlwnd;       //!< Minimum RLWND in segments.
    uint32_t m_initRlwnd;      //!< Initial RLWND in segments.

    SequenceNumber32 m_rcvHgh; //!< Highest sequence number received so far (RCV.HGH).
    uint32_t m_tsvHgh;         //!< TSval recorded at m_rcvHgh (TSV.HGH).

    int32_t m_pendingReduction; //!< Bytes by which RLWND must still be reduced,
                                //!< drained at most ackedBytes per segment (RFC 9293).
                                //!< Also gates multiplicative decrease to once per RTT
                                //!< (RFC 9840 §4.3): a new MD is only permitted when
                                //!< this reaches zero.
    uint32_t m_ssthresh;        //!< Slow-start threshold (bytes).
};

} // namespace ns3

#endif /* TCP_RLEDBAT_H */
