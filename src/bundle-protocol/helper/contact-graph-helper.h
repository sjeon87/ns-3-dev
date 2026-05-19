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

/**
 * @brief A helper class to parse contact plans and instantiate Contact Graph Routing (CGR) engines.
 *
 * This helper simplifies the process of reading an ION-format contact plan
 * file and configuring a routing engine object (derived from BaseRoutingEngine).
 */
class ContactGraphHelper
{
  public:
    /**
     * @brief Create a ContactGraphHelper.
     */
    ContactGraphHelper();

    /**
     * @brief Destroy the ContactGraphHelper.
     */
    ~ContactGraphHelper() = default;

    /**
     * @brief Define the ION contact plan text file to be parsed.
     * @param filename Path to the contact plan file.
     */
    void SetContactPlan(const std::string& filename);

    /**
     * @brief Define the CGR Engine.
     * @param typeId Factory object type of the CGR Engine.
     */
    void SetRoutingEngine(const std::string& typeId);

    /**
     * @brief Install a routing engine.
     * @return Pointer to installed routing engine.
     */
    Ptr<BaseRoutingEngine> Install();

  private:
    std::string m_filename;  //!< Filename of the contact plan
    ObjectFactory m_factory; //!< ObjectFactory for the CGR engines
};

} // namespace ns3

#endif /* CONTACT_GRAPH_HELPER_H */
