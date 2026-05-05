/*
 * Copyright (c) 2008 INRIA
 *                  2013 University of New Brunswick
 *                  2026 Michigan State University
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 *           Dizhi Zhou <dizhi.zhou@gmail.com>
 *           Gerard Garcia <ggarcia@deic.uab.cat>
 *           Ishaan Lagwankar <lagwanka@msu.edu>
 */
#ifndef BASE_ROUTING_ENGINE_H
#define BASE_ROUTING_ENGINE_H

#include "bundle.h"

#include "ns3/nstime.h"
#include "ns3/object.h"
#include "ns3/ptr.h"

#include <string>
#include <vector>

namespace ns3
{

/**
 * @brief Describes a single scheduled contact window parsed from a contact plan.
 */
struct ContactWindow
{
    std::string fromEID;    //!< Source EID
    std::string toEID;      //!< Destination EID
    Time startTime;         //!< Simulation time when the contact opens
    Time endTime;           //!< Simulation time when the contact closes
    uint32_t dataRate;      //!< Nominal data rate (bps)
    Time delay;             //!< Propagation delay
};

class BaseRoutingEngine : public Object
{
  public:
    static TypeId GetTypeId();
    BaseRoutingEngine();
    ~BaseRoutingEngine() override;

    /**
     * Initialise the engine with the full list of EIDs in the simulation.
     * Must be called before any AddContact / GetNextHop call.
     */
    virtual void InitializeMap(const std::vector<std::string>& eidList) = 0;

    /**
     * Add a directed contact edge with unknown total volume.
     * Engines that track volume will treat this contact as unconstrained
     * and fall back to raw dataRate for path-metric purposes.
     */
    virtual void AddContact(const std::string& fromEID,
                            const std::string& toEID,
                            uint32_t dataRate) = 0;

    /**
     * Remove a directed contact edge.
     * Volume accounting for the edge is discarded; it resets if re-added.
     */
    virtual void RemoveContact(const std::string& fromEID, const std::string& toEID) = 0;

    /**
     * Return the EID of the best next hop toward the bundle's destination.
     * Returns "" when no path is currently known.
     */
    virtual std::string GetNextHop(Ptr<Bundle> bundle, const std::string& currEID) = 0;

    /**
     * Add a directed contact edge with an explicit total volume budget.
     *
     * @param fromEID     Source EID.
     * @param toEID       Destination EID.
     * @param dataRate    Nominal link data rate (bps).
     * @param totalVolume Maximum bytes this contact window can carry
     */
    virtual void AddContactWithVolume(const std::string& fromEID,
                                      const std::string& toEID,
                                      uint32_t dataRate,
                                      uint32_t totalVolume);

    /**
     * Record that `bytes` have been committed on the fromEID -> toEID link.
     *
     * @param fromEID Source EID of the link.
     * @param toEID   Destination EID of the link.
     * @param bytes   Size of the forwarded bundle in bytes.
     */
    virtual void ReserveVolume(const std::string& fromEID,
                               const std::string& toEID,
                               uint32_t bytes);

    /**
     * Store a contact window record parsed from the contact plan.
     * Does not affect the live routing graph — use AddContact for that.
     */
    void AddTimedContact(const std::string& fromEID,
                         const std::string& toEID,
                         Time startTime,
                         Time endTime,
                         uint32_t dataRate,
                         Time delay);

    /**
     * Return the full list of contact windows recorded from the contact plan.
     */
    const std::vector<ContactWindow>& GetContactWindows() const;

  protected:
    std::vector<ContactWindow> m_contactWindows; //!< All parsed contact windows
};

} // namespace ns3

#endif /* BASE_ROUTING_ENGINE_H */