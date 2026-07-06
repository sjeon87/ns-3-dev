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
#ifndef BUNDLE_AGENT_H
#define BUNDLE_AGENT_H

#include "bundle-header.h"
#include "bundle-storage-engine.h"
#include "bundle.h"

#include "ns3/event-id.h"
#include "ns3/nstime.h"
#include "ns3/object.h"
#include "ns3/ptr.h"

#include <map>
#include <set>
#include <string>
#include <tuple>
#include <vector>

namespace ns3
{

class BundleCla;
class BaseRoutingEngine;

/**
 * @ingroup dtn
 *
 * @brief An Agent handling the Bundle Protocol functionalities
 *
 * This class implements the Bundle Agent which handles the BPv7 functions. It
 * is responsible for transmitting bundles through registered CLAs and relaying
 * received bundles up the stack to a bundle protocol enabled application.
 */
class BundleAgent : public Object
{
  public:
    /**
     * @brief Get the type ID.
     * @return the object TypeId
     */
    static TypeId GetTypeId();

    BundleAgent();
    ~BundleAgent() override;

    /**
     * @brief Set the local EID
     * @param eid the local node's EID
     */
    void SetLocalEID(const std::string& eid);

    /**
     * @brief Set the bundle storage engine
     * @param bundleStorageEngine the bundle storage engine
     */
    void SetBundleStorageEngine(Ptr<BundleStorageEngine> bundleStorageEngine);

    /**
     * @brief Set the Contact Graph routing oracle
     * @param contactGraph The routing engine to use for Contact Graph Routing
     */
    void SetContactGraph(Ptr<BaseRoutingEngine> contactGraph);

    /**
     * @brief Get the local EID
     * @return the local node's EID
     */
    std::string GetLocalEID() const;

    /**
     * @brief Get the storage engine size
     * @return the size of the storage engine
     */
    uint32_t GetStorageEngineSize() const;

    /**
     * @brief Register a CLA
     * @param destinationEID the EID for which the CLA should be set
     * @param cla the CLA identifier for the EID
     * @return whether the CLA was set correctly
     */
    bool RegisterCla(const std::string& destinationEID, Ptr<BundleCla> cla);

    /**
     * @brief Unregister a CLA for a destination
     * @param destinationEID the destination node's EID
     */
    void UnregisterCla(const std::string& destinationEID);

    /**
     * @brief Transmit a bundle from source to destination; called by applications
     * @param destinationEID the destination node's EID
     * @param reportToEID the reporting node's EID
     * @param data the payload of the bundle
     * @param size the size of the payload
     * @param ttl the Time To Live to expire bundles
     * @param procFlags the set procflags for the bundle
     * @return 0 on failure, 1 on success
     */
    uint32_t TransmitBundle(const std::string& destinationEID,
                            const std::string& reportToEID,
                            uint8_t* data,
                            uint32_t size,
                            Time ttl,
                            uint32_t procFlags = 0);

    /**
     * @brief Receive a bundle from a destination, called by CLA when a bundle arrives from the
     * network
     * @param bundle the bundle received
     * @return 0 on failure, 1 on success
     */
    uint32_t RecvBundle(Ptr<Bundle> bundle);

    /**
     * @brief Forward a bundle to a destination
     * @param handle the bundle's handle to be forwarded
     * @return 0 on failure, 1 on success
     */
    uint32_t ForwardBundle(uint32_t handle);

    /**
     * @brief Remove timed-out bundles
     * @param handle the bundle's handle to be removed
     * @return 0 on failure, 1 on success
     */
    uint32_t ExpireBundle(uint32_t handle);

    /**
     * @brief Generate a status report bundle
     * @param bundle bundle to be sent as a status report
     * @param statusFlags the status flags indicating report properties
     * @param reasonCode the reason to generate a status code
     * @return bundle with the status report
     */
    Ptr<Bundle> GenerateStatusReport(Ptr<Bundle> bundle, uint8_t statusFlags, uint8_t reasonCode);

    /**
     * @brief Attempt to forward any stored bundles destined for a specific EID.
     * @param destinationEID The EID that just became available.
     */
    void ProcessBacklog(const std::string& destinationEID);

    /**
     * @brief Attempt to forward all stored bundles.
     */
    void ProcessAllBacklog();

    /**
     * @brief Callback definition for the receipt of a bundle.
     */
    typedef Callback<void, Ptr<Bundle>> BundleReceiveCallback;

    /**
     * @brief Registers a callback for the receipt of a bundle.
     * @param cb BundleReceiveCallback that calls a function upon receive of a bundle.
     */
    void SetReceiveCallback(BundleReceiveCallback cb);

    /**
     * @brief Sets the storage limit of the BSE
     * @param limit The storage limit to be set.
     */
    void SetStorageLimitFromAttribute(uint32_t limit);

  private:
    /**
     * @brief Get the CLA for a destination EID
     * @param eid EID of destination
     * @return CLA of the destination EID
     */
    Ptr<BundleCla> GetClaForDestination(const std::string& eid) const;

    /**
     * @brief Schedule an event for expiry check
     */
    void ScheduleExpiryCheck();

    /**
     * @brief Check for any expired bundles
     */
    void CheckExpiredBundles();

    /**
     * @brief Placeholder for local node destinations
     * @param eid EID of local process to check
     * @return true if it is a local destination, false otherwise
     */
    bool IsLocalDestination(const std::string& eid) const;

    /**
     * @brief Processes the result of a transaction
     * @param handle Handle for a bundle sent
     * @param success Whether or not the transaction was successful
     */
    void OnTxResult(uint32_t handle, bool success);

    /**
     * @brief Identifies the logical bundle (and thus fragment set) a fragment belongs to. Fragments of the same original
     * bundle share their source EID, creation timestamp, and
     * sequence number.
     */
    using FragmentKey = std::tuple<std::string, int64_t, uint32_t>;

    /**
     * @brief Tracks the partial state of a fragmented bundle awaiting reassembly.
     */
    struct FragmentAssembly
    {
        uint32_t totalLength = 0;                 //!< Total application data unit length
        std::map<uint32_t, Ptr<Packet>> pieces;   //!< Received payload chunks, keyed by offset
        Ptr<PrimaryBlock> templatePrimary;        //!< Primary block used to build the reassembled bundle
        EventId expiryEvent;                      //!< Event that drops the buffer once expired
    };

    /**
     * @brief Buffer an incoming fragment and attempt to reassemble its bundle.
     * @param fragment the received fragment bundle (IS_FRG must be set)
     * @return the reassembled bundle if all fragments have been received, nullptr otherwise
     */
    Ptr<Bundle> TryReassembleFragment(Ptr<Bundle> fragment);

    /**
     * @brief Drop an incomplete fragment reassembly buffer whose lifetime has expired.
     * @param key the fragment set's identifying key
     */
    void ExpireFragmentBuffer(FragmentKey key);

    std::string m_localEID;                         //!< Local EID of process
    uint32_t m_seqNumber = 0;                       //!< Sequence number of messages sent
    uint32_t m_fragmentationMtu = 0;                //!< Max payload bytes per fragment (0 = disabled)
    Ptr<BundleStorageEngine> m_bundleStorageEngine; //!< Storage engine for node
    std::map<std::string, Ptr<BundleCla>> m_clas;   //!< Map of CLAs with destination EIDs
    std::map<uint32_t, EventId> m_expiryEvents;     //!< Expiry event tracker
    BundleReceiveCallback m_receiveCallback;        //!< Receiving application callback
    Ptr<BaseRoutingEngine> m_contactGraph;          //!< Routing oracle
    EventId m_backlogCheckEvent;                    //!< Event to periodically check backlog
    std::set<uint32_t> m_inTransit;                 //!< Set of bundle handles currently in transit
    std::map<FragmentKey, FragmentAssembly> m_fragmentBuffers; //!< Pending fragment reassembly state
};

} // namespace ns3
#endif /* BUNDLE_AGENT_H */
