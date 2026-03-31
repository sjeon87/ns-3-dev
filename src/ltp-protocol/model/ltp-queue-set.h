/*
 * Copyright (c) 2014 Universitat Autònoma de Barcelona
 *                2026 Michigan State University
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 *
 *
 * Author: Rubén Martínez <rmartinez@deic.uab.cat>
          Ishaan Lagwankar <lagwanka@msu.edu>
 */

#ifndef LTP_QUEUE_SET_H
#define LTP_QUEUE_SET_H

#include "ltp-header.h"

#include "ns3/object.h"
#include "ns3/packet.h"

#include <queue>

namespace ns3
{
namespace ltp
{

/**
 * @ingroup dtn
 *
 * @brief Queue set class containing the two queues for outbound traffic.
 * Represents the dual queue structure and priority en/de-queueing policy described in
 * RFC 5325 - 3.1.2 Deferred Transmission.
 */
class LtpQueueSet : public Object
{
  public:
    /**
     * @brief Get Type Id.
     * @return the object TypeId
     */
    static TypeId GetTypeId(void);

    /**
     * @brief LtpQueueSet Constructor
     *
     * Create a ltp queue pair
     */
    LtpQueueSet();

    /**
     * @brief Destructor
     * Destructor
     */
    virtual ~LtpQueueSet();
    /**
     * Push a packet in the queue set, this method checks the LTP Segment type and enqueues
     * it in the corresponding priority queue.
     * @param p the packet to enqueue
     * @return true if success, false on failure.
     */
    bool Enqueue(Ptr<Packet> p);
    /**
     * Pull a packet from the queue based on priority (internal operation queue packets are
     * extracted first)
     * @return the packet.
     */
    Ptr<Packet> Dequeue(void);
    /**
     * Peek the front packet based on priority
     * @return the packet.
     */
    Ptr<const Packet> Peek(void) const;
    /**
     * Pull a packet from the queue based on priority (internal operation queue packets are
     * extracted first)
     * @return the packet.
     */
    Ptr<Packet> Remove(void);
    /**
     * Get N packets from the queue
     * @return the packet.
     */
    uint32_t GetNPackets(void) const;

    std::queue<Ptr<Packet>> m_internalOps; //!< Internal Operation Queue.
    std::queue<Ptr<Packet>> m_appData;     //!< Application Data Queue.
};

} // namespace ltp
} // namespace ns3

#endif /* LTP_QUEUE_SET_H_ */
