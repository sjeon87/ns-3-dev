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
#ifndef BUNDLE_PROTOCOL_HELPER_H
#define BUNDLE_PROTOCOL_HELPER_H

#include "ns3/bundle-agent.h"
#include "ns3/node-container.h"
#include "ns3/nstime.h"
#include "ns3/object-factory.h"

#include <string>
#include <vector>

namespace ns3
{

/**
 * @brief holds a vector of ns3::BundleAgent pointers.
 *
 * Typically ns-3 BundleAgents are installed on nodes using an BundleAgent
 * helper.  The helper Install method takes a NodeContainer which holds
 * some number of Ptr<Node>.  For each of the Nodes in the NodeContainer
 * the helper will instantiate an application, install it in a node and
 * add a Ptr<BundleAgent> to that application into a Container for use
 * by the caller.  This is that container used to hold the Ptr<BundleAgent>
 * which are instantiated by the BundleAgent helper.
 */
class BundleAgentContainer
{
  public:
    /**
     * Create an empty BundleAgent.
     */
    BundleAgentContainer();

    /**
     * Create an BundleAgentContainer with exactly one BundleAgent which has
     * been previously instantiated.  The single application is specified
     * by a smart pointer.
     *
     * @param bpa The Ptr<BundleAgent> to add to the container.
     */
    BundleAgentContainer(Ptr<BundleAgent> bpa);

    /**
     * Create an BundleAgentContainer with exactly one BundleAgent which has
     * been previously instantiated and assigned a name using the Object Name
     * Service.  This BundleAgent is then specified by its assigned name.
     *
     * @param name The name of the BundleAgent Object to add to the container.
     */
    BundleAgentContainer(std::string name);

    /**
     * iterator for BundleAgent
     */
    typedef std::vector<Ptr<BundleAgent>>::const_iterator Iterator;

    /**
     * @brief Get an iterator which refers to the first BundleAgent in the
     * container.
     *
     * BundleAgents can be retrieved from the container in two ways.  First,
     * directly by an index into the container, and second, using an iterator.
     * This method is used in the iterator method and is typically used in a
     * for-loop to run through the BundleAgent
     *
     * @code
     *   BundleAgentContainer::Iterator i;
     *   for (i = container.Begin (); i != container.End (); ++i)
     *     {
     *       (*i)->method ();  // some BundleAgent method
     *     }
     * @endcode
     *
     * @returns an iterator which refers to the first BundleAgent in the container.
     */
    Iterator Begin(void) const;

    /**
     * @brief Get an iterator which indicates past-the-last BundleAgent in the
     * container.
     *
     * BundleAgents can be retrieved from the container in two ways.  First,
     * directly by an index into the container, and second, using an iterator.
     * This method is used in the iterator method and is typically used in a
     * for-loop to run through the BundleAgents
     *
     * @code
     *   BundleAgentContainer::Iterator i;
     *   for (i = container.Begin (); i != container.End (); ++i)
     *     {
     *       (*i)->method ();  // some BundleAgent method
     *     }
     * @endcode
     *
     * @returns an iterator which indicates an ending condition for a loop.
     */
    Iterator End(void) const;

    /**
     * @brief Get the number of Ptr<BundleAgent> stored in this container.
     *
     * BundleAgents can be retrieved from the container in two ways.  First,
     * directly by an index into the container, and second, using an iterator.
     * This method is used in the direct method and is typically used to
     * define an ending condition in a for-loop that runs through the stored
     * BundleAgents
     *
     * @code
     *   uint32_t nBundleAgents = container.GetN ();
     *   for (i = container.Begin (); i < nBundleAgents; ++i)
     *     {
     *       Ptr<BundleAgent> p = container.Get (i)
     *       (*i)->method ();  // some BundleAgent method
     *     }
     * @endcode
     *
     * @returns the number of Ptr<BundleAgent> stored in this container.
     */
    uint32_t GetN(void) const;

    /**
     * @brief Get the Ptr<BundleAgent> stored in this container at a given
     * index.
     *
     * BundleAgents can be retrieved from the container in two ways.  First,
     * directly by an index into the container, and second, using an iterator.
     * This method is used in the direct method and is used to retrieve the
     * indexed Ptr<Application>.
     *
     * @code
     *   uint32_t nBundleAgents = container.GetN ();
     *   for (uint32_t i = 0 i < nBundleAgents; ++i)
     *     {
     *       Ptr<BundleAgent> p = container.Get (i)
     *       i->method ();  // some BundleAgent method
     *     }
     * @endcode
     *
     * @param i the index of the requested application pointer.
     * @returns the requested application pointer.
     */
    Ptr<BundleAgent> Get(uint32_t i) const;

    /**
     * @brief Append the contents of another BundleAgent to the end of
     * this container.
     *
     * @param other The BundleAgent to append.
     */
    void Add(BundleAgentContainer other);

    /**
     * @brief Append a single Ptr<BundleAgent> to this container.
     *
     * @param application The Ptr<BundleAgent> to append.
     */
    void Add(Ptr<BundleAgent> application);

    /**
     * @brief Append to this container the single Ptr<BundleAgent> referred to
     * via its object name service registered name.
     *
     * @param name The name of the BundleAgent Object to add to the container.
     */
    void Add(std::string name);

  private:
    std::vector<Ptr<BundleAgent>> m_bundleAgents; /// vector of bundle protocols
};

/**
 * @brief A helper to make it easier to instantiate an ns3::BundleAgent
 * on a set of nodes.
 */
class BundleAgentHelper
{
  public:
    /**
     * Create an BundleAgentHelper to make it easier to work with BundleAgent
     */
    BundleAgentHelper();

    /**
     * Install an ns3::BundleAgent on each node of the input container
     * configured with all the attributes set with SetAttribute.
     *
     * @param c NodeContainer of the set of nodes on which an BundleAgent
     * will be installed.
     *
     * @returns Container of Ptr to the BundleAgents installed.
     */
    BundleAgentContainer Install(NodeContainer c);

    /**
     * Install an ns3::BundleAgent on the node configured with all the
     * attributes set with SetAttribute.
     *
     * @param node The node on which an BundleAgent will be installed.
     * @returns Container of Ptr to the BundleAgents installed.
     */
    BundleAgentContainer Install(Ptr<Node> node);

    /**
     * Install an ns3::BundleAgent on the node configured with all the
     * attributes set with SetAttribute.
     *
     * @param nodeName The node on which an BundleAgent will be installed.
     * @returns Container of Ptr to the BundleAgents installed.
     */
    BundleAgentContainer Install(std::string nodeName);

    /**
     * Set endpoint id
     *
     * @param eid endpoint id
     */
    void SetBpEndpointId(std::string eid);

  private:
    /**
     * @internal
     * Install an ns3::BundleAgent on the node
     *
     * @param node The node on which an BundleAgent will be installed.
     * @returns Ptr to the BundleAgent installed.
     */
    Ptr<BundleAgent> InstallPriv(Ptr<Node> node);

  private:
    std::string m_eid; /// endpoint id
};

/**
 * @brief holds a vector of ns3::BundleCla pointers.
 */
class BundleClaContainer
{
  public:
    BundleClaContainer();
    BundleClaContainer(Ptr<BundleCla> cla);
    BundleClaContainer(std::string name);

    typedef std::vector<Ptr<BundleCla>>::const_iterator Iterator;

    Iterator Begin(void) const;
    Iterator End(void) const;
    uint32_t GetN(void) const;
    Ptr<BundleCla> Get(uint32_t i) const;

    void Add(BundleClaContainer other);
    void Add(Ptr<BundleCla> cla);
    void Add(std::string name);

  private:
    std::vector<Ptr<BundleCla>> m_clas; /// vector of convergence layer adapters
};

/**
 * @brief A helper to make it easier to instantiate concrete implementations
 * of ns3::BundleCla on a set of nodes.
 */
class BundleClaHelper
{
  public:
    /**
     * Create a BundleClaHelper configured to create a specific concrete CLA.
     * @param type The TypeId string of the concrete CLA (e.g., "ns3::TcpBundleCla")
     */
    BundleClaHelper(std::string type);

    /**
     * Set an attribute on the underlying concrete CLA.
     * @param name Name of the attribute
     * @param value Value of the attribute
     */
    void SetAttribute(std::string name, const AttributeValue& value);

    /**
     * Install the concrete BundleCla on each node of the input container.
     * @param c NodeContainer of the set of nodes.
     * @returns Container of Ptr to the CLAs installed.
     */
    BundleClaContainer Install(NodeContainer c);
    BundleClaContainer Install(Ptr<Node> node);
    BundleClaContainer Install(std::string nodeName);

  private:
    Ptr<BundleCla> InstallPriv(Ptr<Node> node);

    ObjectFactory m_factory; /// Factory used to generate concrete CLA objects
};

} // namespace ns3

#endif /* BUNDLE_PROTOCOL_HELPER_H */
