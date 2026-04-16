/*
 * Copyright (c) 2013 University of New Brunswick
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 *
 *
 * Author: Dizhi Zhou <dizhi.zhou@gmail.com>
 */
#ifndef BP_ROUTING_PROTOCOL_H
#define BP_ROUTING_PROTOCOL_H

#include "ns3/object.h"

namespace ns3
{

class BundleProtocol;

/**
 * @brief This is an abstract base class of bundle routing protocol
 *
 */
class BpRoutingProtocol : public Object
{
  public:
    static TypeId GetTypeId(void);

    /**
     * Constructor
     */
    BpRoutingProtocol();

    /**
     * Destroy
     */
    virtual ~BpRoutingProtocol();

    /**
     * Set bundle protocol
     *
     * @param bundleProtocol bundle protocol
     */
    virtual void SetBundleProtocol(Ptr<BundleProtocol> bundleProtocol) = 0;
};

} // namespace ns3

#endif /* BP_ROUTING_PROTOCOL_H */
