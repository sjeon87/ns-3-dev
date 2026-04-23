/*
 * Copyright (c) 2026 Michigan State University
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Ishaan Lagwankar <lagwanka@msu.edu>
 */

#ifndef CONTACT_GRAPH_HELPER_H
#define CONTACT_GRAPH_HELPER_H

#include "ns3/base-routing-engine.h"
#include "ns3/object-factory.h"
#include "ns3/ptr.h"

#include <string>

namespace ns3
{

class ContactGraphHelper
{
  public:
    ContactGraphHelper();
    ~ContactGraphHelper() = default;

    /**
     * @brief Define the ION contact plan text file to be parsed.
     * @param filename Path to the contact plan file.
     */
    void SetContactPlan(const std::string& filename);

    void SetRoutingEngine(const std::string& typeId);

    Ptr<BaseRoutingEngine> Install();

  private:
    std::string m_filename;
    ObjectFactory m_factory;
};

} // namespace ns3

#endif /* CONTACT_GRAPH_HELPER_H */
