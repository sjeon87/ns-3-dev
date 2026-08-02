/*
 * Copyright (c) 2026 SRM Institute of Science and Technology, India
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Usham Roy <ushamroy80@gmail.com>
 */

#ifndef SIXLOWPAN_TRICKLE_SUPPRESSION_H
#define SIXLOWPAN_TRICKLE_SUPPRESSION_H

#include "sixlowpan-mesh-under-routing.h"

#include "ns3/event-id.h"
#include "ns3/nstime.h"
#include "ns3/trickle-timer.h"

#include <vector>

namespace ns3
{

/**
 * @ingroup sixlowpan
 *
 * @brief Trickle-based mesh-under forwarding strategy (\RFC{6206}).
 *
 * Where SixLowPanSimpleFlooding rebroadcasts every received packet
 * unconditionally, this strategy uses the Trickle algorithm to suppress
 * redundant rebroadcasts (the broadcast-storm problem) while preserving
 * coverage. A single Trickle timer governs how often a node transmits
 * its pending forwards. The duplicate cache in the base class still
 * handles duplicate detection; this class only decides when, and whether,
 * to rebroadcast.
 *
 * Behaviour (the logic, independent of the implementation):
 *
 *  - Pending set. A packet accepted for forwarding is added to a pending
 *    set. The first packet that makes the set non-empty starts the timer;
 *    a packet arriving while the timer is already running simply joins the
 *    set and does NOT restart the timer (so an earlier packet is never
 *    starved).
 *
 *  - Consistent event (\RFC{6206} Rule 3). Overhearing a neighbour
 *    rebroadcast a packet we have also seen is evidence that the
 *    information is spreading without us. It increments the Trickle
 *    counter c. As consistency accumulates, the Trickle interval grows
 *    (Rule 5), so a well-covered neighbourhood transmits less often.
 *
 *  - Transmit decision (\RFC{6206} Rule 4). When the timer fires it
 *    forwards the pending set only if fewer than @c k consistent events
 *    were heard in the interval (c < k). Otherwise the packet is not
 *    forwarded.
 *
 *  - Suppressed packets. A packet suppressed at a firing stays in the
 *    pending set: it may be forwarded at a later firing, and it is
 *    discarded once it has waited MaxForwardingDelay in total.
 *
 *  - Reset on resolution. The timer is reset (restarted from the minimum
 *    interval) when the pending work is resolved: either after a successful
 *    forward, or after a packet is discarded because it waited too long
 *    (MaxForwardingDelay) without being forwarded. Arrivals never reset the
 *    timer; only resolution does.
 *
 * A zero RedundancyConstant disables suppression: the node always forwards,
 * behaving like jittered flooding driven by the Trickle interval.
 */
class SixLowPanTrickleSuppression : public SixLowPanMeshUnderRouting
{
  public:
    /**
     * @brief Get the type ID.
     * @return The object TypeId.
     */
    static TypeId GetTypeId();

    SixLowPanTrickleSuppression();
    ~SixLowPanTrickleSuppression() override;

    // Delete copy constructor and assignment operator to avoid misuse.
    SixLowPanTrickleSuppression(const SixLowPanTrickleSuppression&) = delete;
    SixLowPanTrickleSuppression& operator=(const SixLowPanTrickleSuppression&) = delete;

    void OnPacketForward(Ptr<Packet> packet,
                         const Address& originator,
                         uint8_t seqNo,
                         uint8_t hopsLeft,
                         ForwardCallback forwardCb) override;

    void OnDuplicateReceived(const Address& originator, uint8_t seqNo) override;

    int64_t AssignStreams(int64_t stream) override;

  protected:
    void DoDispose() override;

  private:
    /// A packet awaiting a forward decision, with the callback that sends it.
    struct PendingPacket
    {
        Ptr<Packet> packet;        ///< The packet to forward.
        ForwardCallback forwardCb; ///< Callback that performs the rebroadcast.
    };

    /**
     * @brief Start the Trickle timer for a fresh batch of pending packets.
     */
    void StartTimer();

    /**
     * @brief Stop the Trickle timer and cancel the discard deadline.
     */
    void StopTimer();

    /**
     * @brief Trickle transmit callback: forward all pending packets (c < k).
     *
     * Invoked by the TrickleTimer only when the redundancy constant has not
     * been reached. Forwards every pending packet, then resets by stopping
     * the timer until the next packet arrives.
     */
    void Transmit();

    /**
     * @brief Discard pending packets that waited too long without forwarding.
     *
     * Reached when suppression kept the node silent for MaxForwardingDelay.
     * The packets are dropped and the timer is reset.
     */
    void DiscardPending();

    Time m_minInterval;        ///< RFC 6206 Imin: the minimum Trickle interval.
    uint8_t m_doublings;       ///< Imax = MinInterval * 2^Doublings.
    uint16_t m_redundancy;     ///< RFC 6206 k. Forward iff c < k; zero disables suppression.
    Time m_maxForwardingDelay; ///< Discard a pending packet not forwarded within this time.

    TrickleTimer m_timer;                 ///< The single per-node Trickle timer.
    bool m_timerRunning;                  ///< True while m_timer is enabled.
    EventId m_discardEvent;               ///< Fires at MaxForwardingDelay to discard stuck packets.
    std::vector<PendingPacket> m_pending; ///< Packets awaiting a forward decision.
};

} // namespace ns3

#endif /* SIXLOWPAN_TRICKLE_SUPPRESSION_H */
