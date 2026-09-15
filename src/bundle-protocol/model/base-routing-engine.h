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
    std::string fromEID;  //!< Source EID
    std::string toEID;    //!< Destination EID
    Time startTime;       //!< Simulation time when the contact opens
    Time endTime;         //!< Simulation time when the contact closes
    uint32_t dataRate;    //!< Nominal data rate (bps)
    Time delay;           //!< Propagation delay
    uint32_t totalVolume; //!< Maximum bytes this specific window can carry
    uint32_t usedVolume;  //!< Bytes already committed during this window
};

/**
 * @ingroup BundleProtocol
 * @brief Base class for Bundle Protocol routing engines.
 *
 * This class defines the common interface for routing engines used within
 * the Bundle Protocol. Implementations of this class are responsible for
 * maintaining contact states and determining the next hop for a given bundle
 * based on the current network topology and/or contact plan.
 */
class BaseRoutingEngine : public Object
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
    BaseRoutingEngine();

    /**
     * @brief Destructor
     */
    ~BaseRoutingEngine() override;

    /**
     * @brief Initialise the engine with the full list of EIDs in the simulation.
     *
     * @param eidList List of all Endpoint IDs (EIDs) present in the simulation.
     */
    virtual void InitializeMap(const std::vector<std::string>& eidList) = 0;

    /**
     * @brief Add a directed contact edge with unknown total volume.
     *
     * @param fromEID Source EID of the contact.
     * @param toEID Destination EID of the contact.
     * @param dataRate Nominal data rate of the contact in bps.
     */
    virtual void AddContact(const std::string& fromEID,
                            const std::string& toEID,
                            uint32_t dataRate) = 0;

    /**
     * @brief Remove a directed contact edge.
     *
     * @param fromEID Source EID of the contact to remove.
     * @param toEID Destination EID of the contact to remove.
     */
    virtual void RemoveContact(const std::string& fromEID, const std::string& toEID) = 0;

    /**
     * @brief Return the EID of the best next hop toward the bundle's destination.
     *
     * @param bundle The bundle that needs to be routed.
     * @param currEID The Endpoint ID of the current node holding the bundle.
     * @return The Endpoint ID of the best next hop, or an empty string if no path is known.
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
     * Record that bytes have been committed on the fromEID -> toEID link.
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
     * @param fromEID     Source EID.
     * @param toEID       Destination EID.
     * @param startTime   Simulation time when the contact opens.
     * @param endTime     Simulation time when the contact closes.
     * @param dataRate    Nominal link data rate (bps).
     * @param delay       Propagation delay.
     * @param totalVolume Maximum bytes this contact window can carry.
     */
    virtual void AddTimedContact(const std::string& fromEID,
                                 const std::string& toEID,
                                 Time startTime,
                                 Time endTime,
                                 uint32_t dataRate,
                                 Time delay,
                                 uint32_t totalVolume);

    /**
     * Return the full list of contact windows recorded from the contact plan.
     * @return vector of all contact windows
     */
    const std::vector<ContactWindow>& GetContactWindows() const;

  protected:
    std::vector<ContactWindow> m_contactWindows; //!< All parsed contact windows
};

} // namespace ns3

#endif /* BASE_ROUTING_ENGINE_H */
