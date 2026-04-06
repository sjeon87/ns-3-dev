/*
 * Copyright (c) 2026 Michigan State University
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Ishaan Lagwankar <lagwanka@msu.edu>
 */
#ifndef BUNDLE_STORAGE_ENGINE_H
#define BUNDLE_STORAGE_ENGINE_H

#include "bundle.h"
#include "ns3/object.h"
#include "ns3/type-id.h"  

#include <map>
#include <vector>

namespace ns3
{

/**
 * 
 * @ingroup dtn
 *
 * @brief Storage database engine for bundles
 *
 * The storage engine stores bundles for later processing. The bundle engine keeps track
 * of bundles for a particular EID, and provides bundles depending on the handle given.
 *
 */
class BundleStorageEngine : public Object
{
public:

    /**
     * @brief Get the type ID.
     * @return the object TypeId
     */
    static TypeId GetTypeId();

    BundleStorageEngine();
    ~BundleStorageEngine() override;

    /**
     * @brief Get the current size of the database
     * @return the current size of the database
     */
    uint32_t GetCurrentSize() const;

    /**
     * @brief Get the total size of the database
     * @return the total size of the database
     */
    uint32_t GetTotalSize() const;

    /**
     * @brief Set the total size of the database
     * @param the total size of the database
     */
    void SetTotalSize(uint32_t size);

    /**
     * @brief Store the bundle and return a handle for the bundle
     * @param bundle reference to the bundle being stored
     * @return a handle for the bundle
     */
    uint32_t StoreBundle(Ptr<Bundle> bundle);

    /**
     * @brief Returns bundle if found, nullptr if handle is invalid
     * @param handle Handle for the bundle requested
     * @return a bundle for the handle
     */
    Ptr<Bundle> RetrieveBundle(uint32_t handle) const;

    /**
     * @brief Removes bundle from storage, returns 0 on success, 1 if not found
     * @param handle Handle for the bundle requested
     * @return 0 on success, 1 on failure
     */
    uint32_t DeleteBundle(uint32_t handle);

    /**
     * @brief Returns all bundles whose expiry time has passed
     * @return vector of all expired bundles
     */
    std::vector<Ptr<Bundle>> GetExpiredBundles() const;

    /**
     * @brief Returns all stored bundles destined for a given EID
     * @return vector of all stored bundles
     */
    std::vector<uint32_t> GetHandlesForDestination(const std::string& eid) const;

    /**
     * @brief Checks if a handle has a bundle
     * @param handle Handle for the bundle requested
     * @return true or false for the bndle
     */
    bool HasBundle(uint32_t handle) const;

private:
    uint32_t m_currentSize = 0;                         //!< Current size of the engine
    uint32_t m_totalSize = 0;                           //!< Total size of the engine
    uint32_t m_nextHandle = 1;                          //!< Next available handle
    std::map<uint32_t, Ptr<Bundle>> m_bundleMap;        //!< Map of all the bundles to handles
};

} // namespace ns3
#endif /* BUNDLE_STORAGE_ENGINE_H */