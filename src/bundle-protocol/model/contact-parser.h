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
#ifndef CONTACT_PARSER_H
#define CONTACT_PARSER_H

#include "base-routing-engine.h"
#include "bundle-agent.h"

#include "ns3/bundle-protocol-helper.h"
#include "ns3/net-device-container.h"
#include "ns3/node-container.h"
#include "ns3/ptr.h"

#include <string>

namespace ns3
{

/**
 * @ingroup BundleProtocol
 * @brief A utility class to parse contact plans for Delay Tolerant Networks.
 *
 */
class ContactParser
{
  public:
    /**
     * @brief Parses a contact plan file and populates the given ContactGraph.
     * @param filename The path to the text file containing the contact data.
     * @param contactGraph A pointer to the ContactGraph instance to populate.
     * @return true if successful, false if the file could not be read.
     */
    static bool ParseFile(const std::string& filename, Ptr<BaseRoutingEngine> contactGraph);
};

} // namespace ns3

#endif /* CONTACT_PARSER_H */
