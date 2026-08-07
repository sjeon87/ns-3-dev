/*
 * Copyright (c) 2026 SRM Institute of Science and Technology, India
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Usham Roy <ushamroy80@gmail.com>
 */

#ifndef SIXLOWPAN_TRICKLE_FORWARDING_H
#define SIXLOWPAN_TRICKLE_FORWARDING_H

#include "sixlowpan-mesh-under-routing.h"

#include "ns3/event-id.h"
#include "ns3/nstime.h"
#include "ns3/traced-callback.h"
#include "ns3/traced-value.h"
#include "ns3/trickle-timer.h"

#include <deque>

namespace ns3
{

/**
 * @ingroup sixlowpan
 *
 * @brief Trickle-based mesh-under forwarding policy (\RFC{6206}).
 *
 * Where SixLowPanSimpleFlooding rebroadcasts every received packet
 * unconditionally, this policy uses the Trickle algorithm to suppress
 * redundant rebroadcasts (the broadcast-storm problem) while preserving
 * coverage. A single Trickle timer governs how often a node transmits
 * its pending forwards. The duplicate cache in the base class still
 * handles duplicate detection; this class only decides when, and whether,
 * to rebroadcast.
 *
 * Behaviour (the logic, independent of the implementation):
 *
 *  - Pending queue. A packet accepted for forwarding joins a FIFO pending
 *    queue. The first packet that makes the queue non-empty starts the
 *    timer with the minimum interval (a new packet is new information);
 *    a packet arriving while the timer is already running simply joins
 *    the queue and does NOT restart the timer (so an earlier packet is
 *    never starved).
 *
 *  - Consistent event (\RFC{6206} Rule 3). Overhearing a neighbour
 *    rebroadcast a packet we have also seen is evidence that the
 *    information is spreading without us. It increments the Trickle
 *    counter c. As consistency accumulates, the Trickle interval grows
 *    (Rule 5), so a well-covered neighbourhood transmits less often.
 *    With HeadOfLineConsistency enabled only duplicates of the packet at
 *    the head of the pending queue count (the next packet this node would
 *    forward); otherwise any duplicate counts (channel-level consistency).
 *
 *  - Transmit decision (\RFC{6206} Rule 4). When the timer fires it
 *    forwards only if fewer than @c k consistent events were heard in the
 *    interval (c < k). Otherwise nothing is forwarded at this firing.
 *    With ForwardOnePerFiring enabled a firing forwards only the packet
 *    at the head of the queue and the timer keeps running (its interval
 *    doubling per \RFC{6206}) until the queue drains; otherwise a firing
 *    forwards the whole queue at once.
 *
 *  - Per-packet deadline. Each packet records its own deadline on
 *    arrival (MaxForwardingDelay later). A packet still pending at its
 *    deadline (suppression won) is discarded individually; packets that
 *    arrived later are unaffected until their own deadlines.
 *
 *  - Reset on empty. The timer stops when the pending queue drains
 *    (everything forwarded or discarded), so the next arrival restarts
 *    it from the minimum interval. This is the adaptive backoff: react
 *    fast when new information appears, back off while the neighbourhood
 *    is covered.
 *
 * A zero RedundancyConstant disables suppression: the node always forwards,
 * behaving like jittered flooding driven by the Trickle interval.
 */
class SixLowPanTrickleForwarding : public SixLowPanMeshUnderRouting
{
  public:
    /**
     * @brief Get the type ID.
     * @return The object TypeId.
     */
    static TypeId GetTypeId();

    SixLowPanTrickleForwarding();
    ~SixLowPanTrickleForwarding() override;

    // Delete copy constructor and assignment operator to avoid misuse.
    SixLowPanTrickleForwarding(const SixLowPanTrickleForwarding&) = delete;
    SixLowPanTrickleForwarding& operator=(const SixLowPanTrickleForwarding&) = delete;

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
        Address originator;        ///< Mesh originator (for head-of-line consistency).
        uint8_t seqNo;             ///< Mesh sequence number (for head-of-line consistency).
        Time deadline;             ///< Discard time: arrival + MaxForwardingDelay.
    };

    /**
     * @brief Start the Trickle timer for a fresh batch of pending packets.
     *
     * The timer starts from the minimum interval: the packet that starts
     * it is new information, so the node must react quickly (in the spirit
     * of \RFC{6206} inconsistency and \RFC{7731} new-data handling).
     */
    void StartTimer();

    /**
     * @brief Stop the Trickle timer and cancel the discard deadline.
     */
    void StopTimer();

    /**
     * @brief Trickle transmit callback, invoked only when c < k.
     *
     * Forwards the whole pending queue and stops the timer, or, with
     * ForwardOnePerFiring, forwards only the head of the queue and keeps
     * the timer running until the queue drains.
     */
    void Transmit();

    /**
     * @brief Discard the pending packets whose deadline has passed.
     *
     * Reached when suppression kept a packet silent for MaxForwardingDelay.
     * Only the expired packets are dropped; the timer stops when the queue
     * drains.
     */
    void DiscardExpired();

    /**
     * @brief Schedule the discard event for the head of the pending queue.
     *
     * The queue is FIFO and every packet waits the same MaxForwardingDelay,
     * so deadlines are monotonic and a single event (for the head) suffices.
     */
    void ScheduleDiscard();

    Time m_minInterval;        ///< RFC 6206 Imin: the minimum Trickle interval.
    uint8_t m_doublings;       ///< Imax = MinInterval * 2^Doublings.
    uint16_t m_redundancy;     ///< RFC 6206 k. Forward iff c < k; zero disables suppression.
    Time m_maxForwardingDelay; ///< Per-packet deadline: discard if not forwarded within this time.
    bool m_onePerFiring;       ///< Forward one packet per firing instead of the whole queue.
    bool m_headOfLine;         ///< Count only duplicates of the head-of-queue packet.

    TrickleTimer m_timer;                ///< The single per-node Trickle timer.
    bool m_timerRunning;                 ///< True while m_timer is enabled.
    EventId m_discardEvent;              ///< Fires at the head-of-queue packet's deadline.
    std::deque<PendingPacket> m_pending; ///< FIFO queue of packets awaiting a decision.

    TracedValue<uint32_t> m_pendingSize;              ///< Traced size of the pending queue.
    TracedCallback<Ptr<const Packet>> m_discardTrace; ///< Fired for each discarded packet.
};

} // namespace ns3

#endif /* SIXLOWPAN_TRICKLE_FORWARDING_H */
