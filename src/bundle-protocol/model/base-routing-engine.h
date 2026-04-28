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

struct ContactWindow
{
    std::string fromEID;
    std::string toEID;
    Time startTime;
    Time endTime;
    uint32_t dataRate;
    Time delay;
};

class BaseRoutingEngine : public Object
{
  public:
    /**
     * @brief Get the type ID.
     * @return the object TypeId
     */
    static TypeId GetTypeId();
    BaseRoutingEngine();
    ~BaseRoutingEngine() override;

    /**
     * Returns the next best hop according to contact graph fed.
     * @param bundle Bundle to be transmitted (provides destinationEID).
     * @param currEID Current bundle holder's EID.
     * @return EID of the next best hop
     */
    virtual std::string GetNextHop(Ptr<Bundle> bundle, const std::string& currEID) = 0;

    /**
     * Initializes contact graph as custom structure.
     * @param eidList List of EIDs in the network.
     */
    virtual void InitializeMap(const std::vector<std::string>& eidList) = 0;

    /**
     * Adds a contact edge between fromEID and toEID with weight dataRate.
     * @param fromEID EID 1 (src)
     * @param toEID EID 2 (dest)
     * @param dataRate weight of the edge
     */
    virtual void AddContact(const std::string& fromEID,
                            const std::string& toEID,
                            uint32_t dataRate) = 0;

    /**
     * Removes a contact edge between fromEID and toEID
     * @param fromEID EID 1 (src)
     * @param toEID EID 2 (dest)
     */
    virtual void RemoveContact(const std::string& fromEID, const std::string& toEID) = 0;

    /**
     * Adds a contact edge between fromEID and toEID with weight dataRate from a given time to
     * another time.
     * @param fromEID EID 1 (src)
     * @param toEID EID 2 (dest)
     * @param dataRate weight of the edge
     * @param startTime Contact open time
     * @param endTime Contact end time
     * @param delay Channel delay of the contact
     */
    void AddTimedContact(const std::string& fromEID,
                         const std::string& toEID,
                         Time startTime,
                         Time endTime,
                         uint32_t dataRate,
                         Time delay);

    /**
     * Returns a list of all contact windows.
     * @return vector of all contact windows available.
     */
    const std::vector<ContactWindow>& GetContactWindows() const;

  protected:
    std::vector<ContactWindow> m_contactWindows; // !< Contact Window list holder
};

} // namespace ns3

#endif /* BASE_ROUTING_ENGINE_H */
