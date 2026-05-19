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
#ifndef BUNDLE_CLA_H
#define BUNDLE_CLA_H

#include "bundle.h"

#include "ns3/callback.h"
#include "ns3/object.h"
#include "ns3/packet.h"

namespace ns3
{

/**
 * @ingroup dtn
 *
 * @brief Abstract base class for Bundle Protocol Convergence Layer Adapters.
 *
 * Concrete implementations (e.g., TcpBundleCla, UdpBundleCla) will inherit
 * from this class and implement the underlying socket logic.
 */
class BundleCla : public Object
{
  public:
    /**
     * @brief Get the type ID.
     * @return the object TypeId
     */
    static TypeId GetTypeId();

    BundleCla();
    ~BundleCla() override;

    /**
     * @brief Callback signature for passing received bundles up to the agent.
     */
    using RxCallback = Callback<uint32_t, Ptr<Bundle>>;

    /**
     * @brief Set the callback to be fired when a bundle is received.
     * @param callback The callback (usually bound to BundleAgent::RecvBundle)
     */
    void SetRxCallback(RxCallback callback);

    /**
     * @brief Fire callback registered
     * @param bundle The bundle that was received and needs to be passed up.
     * @return The result of the callback execution.
     */
    uint32_t NotifyReception(Ptr<Bundle> bundle);

    /**
     * @brief Send a serialized bundle packet out over the convergence layer.
     * @param packet The serialized bundle to send.
     * @param bundleHandle The handle for the bundle to be sent.
     * @note This is a pure virtual function and must be implemented by concrete CLAs.
     */
    virtual void Send(Ptr<Packet> packet, uint32_t bundleHandle = 0) = 0;

    /**
     * @brief Check if the CLA is ready to send data.
     * @return true if the underlying link/socket is up.
     */
    virtual bool IsUp() const = 0;

    /**
     * @brief Sets the callback for the result of a Tx
     * @param cb Callback for the TxResult
     */
    void SetTxResultCallback(Callback<void, uint32_t, bool> cb);

  protected:
    /**
     * @brief Helper function called by concrete CLAs to pass a parsed bundle
     * to the upper layer.
     *
     * @param bundle The deserialized bundle object.
     */
    void ForwardUp(Ptr<Bundle> bundle);

  protected:
    RxCallback m_rxCallback;                     //!< The callback to trigger on bundle reception
    Callback<void, uint32_t, bool> m_txResultCb; //!< The callback to report transmission results
};

} // namespace ns3

#endif /* BUNDLE_CLA_H */
