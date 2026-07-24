/*
 * Copyright (c) 2026 Somaiya Vidyavihar University, India
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Manmita Das <das.manmita12@gmail.com>
 */

#ifndef ETHERNET_SWITCH_SCHEDULER_H
#define ETHERNET_SWITCH_SCHEDULER_H

#include "ethernet-switch.h"

#include "ns3/ethernet-header.h"
#include "ns3/mac48-address.h"
#include "ns3/net-device.h"

#include <map>
#include <queue>

namespace ns3
{
namespace ethernet
{
/**
 * @brief The state of a switch port in the scheduler.
 */
enum class PortSchedulingState : uint8_t
{
    PORT_IDLE = 0,
    PORT_TRANSMITTING
};

/**
 * @ingroup ethernet
 * @brief Ethernet Switch Scheduler model
 *
 * Picks frames out of the buffer of an EthernetSwitch and hands them to the
 * output ports they are destined to.
 *
 * A frame can only be handed to an output port that has room left in its
 * transmit queue, which CanTransmit() establishes. Deciding what to do with a
 * frame that its output port cannot take is left to the subclasses: a frame
 * can be dropped through DropEntry(), or held back in the buffer to be
 * reconsidered once the port has drained.
 */
class EthernetSwitchScheduler : public Object
{
  public:
    /**
     * @brief Get the type ID.
     * @return the object TypeId
     */
    static TypeId GetTypeId();

    EthernetSwitchScheduler();
    ~EthernetSwitchScheduler() override;

    // Delete copy constructor and assignment operator to avoid misuse
    EthernetSwitchScheduler(const EthernetSwitchScheduler&) = delete;
    EthernetSwitchScheduler& operator=(const EthernetSwitchScheduler&) = delete;

    /**
     * @brief Attach scheduler to switch.
     * @param sw Ethernet switch instance
     */
    void SetSwitch(Ptr<EthernetSwitch> sw);

    /**
     * @brief Selects packets from the switch buffer according to the scheduling
     * algorithm and initiates transmission on available output ports.
     */
    virtual void ScheduleTransmission() = 0;

    /**
     * @brief Schedules the transmission event for a selected packet on its output port.
     * Calculates transmission time based on port data rate and marks the port busy.
     * @param entry The buffer entry for the selected packet.
     */
    void SchedulePortTransmission(const SwitchBufferEntry& entry);

    /**
     * @brief Send a packet to the selected output port.
     * Calls EthernetSwitch::TransmitToPort().
     * @param entry The buffer entry for the selected packet.
     */
    void Transmit(const SwitchBufferEntry& entry);

    /**
     * @brief Called when transmission of a packet is complete.
     * @param entry The buffer entry for the completed transmission.
     */
    void TransmissionComplete(SwitchBufferEntry entry);

  protected:
    void DoDispose() override;

    /**
     * @brief Check whether the output port of an entry can take it.
     *
     * A frame whose output port has no room left cannot be handed over, and
     * has to be dropped or held back according to the policy of the
     * scheduler.
     *
     * @param entry The buffer entry to check.
     * @return True if the output port can take the frame.
     */
    bool CanTransmit(const SwitchBufferEntry& entry) const;

    /**
     * @brief Check whether the output port of an entry is free.
     * @param entry The buffer entry to check.
     * @return True if no transmission is in progress on the output port.
     */
    bool IsPortIdle(const SwitchBufferEntry& entry) const;

    /**
     * @brief Give up on forwarding an entry and discard it.
     * @param entry The buffer entry to discard.
     */
    void DropEntry(const SwitchBufferEntry& entry);

    /**
     * @brief Parent Ethernet switch.
     */
    Ptr<EthernetSwitch> m_switch;

    /**
     * @brief State of each port in the scheduler.
     * Maps each port to its current scheduling state (idle or transmitting).
     */
    std::map<Ptr<EthernetNetDevice>, PortSchedulingState> m_portSchedulingState;
};

} // namespace ethernet
} // namespace ns3

#endif /* ETHERNET_SWITCH_SCHEDULER_H */
