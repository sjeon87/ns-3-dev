/*
 * Copyright (c) 2024 Ishaan Lagwankar
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Ishaan Lagwankar <lagwanka@msu.edu>
 */

#ifndef LOCAL_CLOCK_HELPER_H
#define LOCAL_CLOCK_HELPER_H

#include "ns3/local-clock.h"
#include "ns3/node-container.h"
#include "ns3/node.h"
#include "ns3/object-factory.h"

namespace ns3
{

/**
 * @brief Unified helper to aggregate any LocalClock implementation to nodes.
 */
class LocalClockHelper
{
  public:
    LocalClockHelper();

    /**
     * @brief Set the exact type of clock to create (e.g., "ns3::MultiRateClock").
     *
     * @param type The TypeId string of the derived clock class.
     */
    void SetClockType(std::string type);

    /**
     * @brief Set an attribute for the chosen clock type.
     *
     * @param name The name of the attribute to set.
     * @param value The value of the attribute to set.
     */
    void SetAttribute(std::string name, const AttributeValue& value);

    /**
     * @brief Install the configured clock on the provided node.
     *
     * @param node The node to install the clock on.
     */
    void Install(Ptr<Node> node) const;

    /**
     * @brief Install the configured clock on all nodes in the container.
     *
     * @param c The container of nodes.
     */
    void Install(NodeContainer c) const;

  private:
    ObjectFactory m_factory; //!< Object factory to create the specific clock instances.
};

} // namespace ns3

#endif /* LOCAL_CLOCK_HELPER_H */
