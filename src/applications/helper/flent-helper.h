/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2008 INRIA
 * Copyright (c) 2022 NITK Surathkal
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * This file is adapted from bulk-send-helper.h.
 *
 * Author: Ameya Deshpande <ameyanrd@outlook.com>
 */

#ifndef FLENT_HELPER_H
#define FLENT_HELPER_H

#include <string>
#include "ns3/object-factory.h"
#include "ns3/address.h"
#include "ns3/attribute.h"
#include "ns3/node-container.h"
#include "ns3/application-container.h"

namespace ns3 {

/**
 * \ingroup flent
 * \brief A helper to make it easier to instantiate an ns3::FlentApplication
 */
class FlentHelper
{
public:
  /**
   * Create an FlentHelper to make it easier to work with FlentApplication
   *
   * \param [in] testname the name of the test to run on FlentApplication
   *        The supported tests are ping, tcp_upload, tcp_download and rrul.
   * \param [in] address the address of the remote host to send traffic to.
   */
  FlentHelper (std::string testname, Address address);

  /**
   * Helper function used to set the underlying application attributes.
   *
   * \param name the name of the application attribute to set
   * \param value the value of the application attribute to set
   */
  void SetAttribute (std::string name, const AttributeValue &value);

  /**
   * Install an ns3::FlentApplication on the node configured with all the
   * attributes set with SetAttribute.
   *
   * \param node The node on which an FlentApplication will be installed.
   * \returns Container of Ptr to the applications installed.
   */
  ApplicationContainer Install (Ptr<Node> node) const;

  /**
   * Install an ns3::FlentApplication on the node configured with all the
   * attributes set with SetAttribute.
   *
   * \param nodeName The node on which an FlentApplication will be installed.
   * \returns Container of Ptr to the applications installed.
   */
  ApplicationContainer Install (std::string nodeName) const;

private:
  /**
   * Install an ns3::FlentApplication on the node configured with all the
   * attributes set with SetAttribute.
   *
   * \param node The node on which an FlentApplication will be installed.
   * \returns Ptr to the application installed.
   */
  Ptr<Application> InstallPriv (Ptr<Node> node) const;

  ObjectFactory m_factory; //!< Object factory.

};

} // namespace ns3

#endif /* ON_OFF_HELPER_H */