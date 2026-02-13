/*
 * Copyright (c) 2018-2026 NITK Surathkal
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Authors: Shikha Bakshi <shikhabakshi912@gmail.com>
 *          Mohit P. Tahiliani <tahiliani@nitk.edu.in>
 *          Mohnish Hemanth Kumar <mohnishhemanthkumar@gmail.com>
 *          Nikhil Kottoli <nikhilkottoli2005@gmail.com>
 *          Manish Agarwal <manishagarwal428728@gmail.com>
 *          Patel Pal Bharat <ppal61679@gmail.com>
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

#include <deque>
#include <utility>

namespace ns3
{

class TcpRack : public Object
{
  public:
    /**
     * @brief Get the type ID.
     * @return the object TypeId
     */
    static TypeId GetTypeId(void);

    /**
     * @brief Constructor
     */
    TcpRack();

    /**
     * @brief Copy constructor.
     * @param other object to copy.
     */
    TcpRack(const TcpRack& other);

    /**
     * @brief Deconstructor
     */
    virtual ~TcpRack();

    /**
     * @brief checks if packet1 sent after packet2
     *
     * @param t1 transmission time of packet1
     * @param t2 transmission time of packet2
     * @param seq1 sequence number of packet1
     * @param seq2 sequence number of packet2
     * @return true if packet1 sent after packet2
     */
    bool SentAfter(Time t1, Time t2, uint32_t seq1, uint32_t seq2);

    /**
     * @brief updates reo_wnd
     *
     * @param reorderSeen whether re-ordering is seen or not
     * @param dsackSeen whether D-SACK block is seen or not
     * @param sndNxt SND.NXT
     * @param sndUna SND.UNA
     * @param tcb Socket State object
     * @param sacked Number of packets sacked
     * @param dupAckThresh Threshold for number of dupacks to enter recovery
     * @param exiting if the connection is exiting Recovery State
     */
    void UpdateReoWnd(bool reorderSeen,
                      bool dsackSeen,
                      SequenceNumber32 sndNxt,
                      SequenceNumber32 sndUna,
                      Ptr<TcpSocketState> tcb,
                      uint32_t sacked,
                      uint32_t dupAckThresh,
                      bool exiting);

    /**
     * @brief Updates the RACK parameters based on the most recently (S)ACKed packet.
     *
     * @param tser echo timestamp
     * @param retrans whether the packet is retransmitted or not
     * @param xmitTs transmission timestamp of the packet
     * @param endSseq end sequence number of the packet
     * @param sndNxt SND.NXT when the RTT is updated
     * @param rtt estimated round trip time
     */
    virtual void UpdateStats(uint32_t tser,
                             bool retrans,
                             Time xmitTs,
                             SequenceNumber32 endSseq,
                             SequenceNumber32 sndNxt,
                             Time rtt);

    /**
     * @brief returns Reordering Window
     * @return Reordering Window
     */
    double GetReoWnd() const
    {
        return m_reoWnd;
    }

    /**
     * @brief returns recent transmission time of Rack.segment
     * @return recent transmission time of Rack.segment
     */
    Time GetXmitTs()
    {
        return m_rackXmitTs;
    }

    /**
     * @brief returns Ending sequence of Rack.segment
     * @return Ending sequence of Rack.segment
     */
    uint32_t GetEndSeq()
    {
        return m_rackEndSeq.GetValue();
    }

    /**
     * @brief returns the RTT of the most recently transmitted segment
     *        that has been acknowledged
     * @return RTT of the most recently transmitted segment
     */
    Time GetRtt()
    {
        return m_rackRtt;
    }

  private:
    Time m_rackXmitTs{0};             //!< Latest transmission timestamp of Rack.segment
    SequenceNumber32 m_rackEndSeq{0}; //!< Ending sequence number of Rack.segment
    Time m_rackRtt{0};  //!< RTT of the most recently transmitted segment that has been acknowledged
    double m_reoWnd{0}; //!< Re-ordering Window
    Time m_minRtt{0};   //!< Minimum RTT
    Time m_minRttWindow{Seconds(300)};              //!< Window size for min-RTT filter
    std::deque<std::pair<Time, Time>> m_rttSamples; //!< Sliding window of RTT samples
    SequenceNumber32 m_dsackRound{
        0}; //!< Indicates if a DSACK option has been received in the latest round trip.
    uint32_t m_reoWndMult{1};     //!< Multiplier applied to adjust RACK.reo_wnd
    uint32_t m_reoWndPersist{16}; //!< Number of loss recoveries before resetting RACK.reo_wnd

    /**
     * @brief Remove expired RTT samples from the sliding window.
     *
     * Discards RTT measurements that fall outside the configured
     * minimum RTT observation window and updates internal state
     * accordingly.
     *
     * @param now Current simulation time.
     * @return True if any samples were removed.
     */
    bool PruneRttWindow(Time now);

    /**
     * @brief Recompute the minimum RTT estimate.
     *
     * Updates the minimum RTT value based on the current set
     * of valid RTT samples maintained in the sliding window.
     */
    void RecomputeMinRtt();
};
} // namespace ns3

/* TCP_RACK_H */
