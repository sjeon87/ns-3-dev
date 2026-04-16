/*
 * Copyright (c) 2013 University of New Brunswick
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 *
 *
 * Author: Dizhi Zhou <dizhi.zhou@gmail.com>
 */
#ifndef BP_STATIC_ROUTING_PROTOCOL_H
#define BP_STATIC_ROUTING_PROTOCOL_H

#include "bp-routing-protocol.h"
#include "bundle-protocol.h"

#include "ns3/inet-socket-address.h"

namespace ns3
{

/**
 * @brief This is an abstract base class of bundle routing protocol
 *
 */
class BpStaticRoutingProtocol : public BpRoutingProtocol
{
  public:
    static TypeId GetTypeId(void);

    /**
     * Constructor
     */
    BpStaticRoutingProtocol();

    /**
     * Destroy
     */
    virtual ~BpStaticRoutingProtocol();

    /**
     * @brief Set static bundle protocol
     *
     * @param bundleProtocol static bundle protocol
     */
    virtual void SetBundleProtocol(Ptr<BundleProtocol> bundleProtocol);

    /**
     * @brief Add a static route
     */
    virtual int AddRoute(BpEndpointId eid, InetSocketAddress address);

    /**
     *  @return the internet socket address of matched eid; If there is no
     *  match route, return the 127.0.0.1 with port 0
     */
    virtual InetSocketAddress GetRoute(BpEndpointId eid);

  private:
    std::map<BpEndpointId, InetSocketAddress> m_routeMap; /// routing table
    Ptr<BundleProtocol> m_bp;                             /// bundle protocol
};

} // namespace ns3

#endif /* BP_STATIC_ROUTING_PROTOCOL_H */
