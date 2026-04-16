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
#include "generic-convergence-layer-adapter.h"

#include "ns3/log.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("BundleCla");
NS_OBJECT_ENSURE_REGISTERED(BundleCla);

TypeId
BundleCla::GetTypeId()
{
    static TypeId tid = TypeId("ns3::BundleCla").SetParent<Object>().SetGroupName("BundleProtocol");
    return tid;
}

BundleCla::BundleCla()
{
    NS_LOG_FUNCTION(this);
}

BundleCla::~BundleCla()
{
    NS_LOG_FUNCTION(this);
}

void
BundleCla::SetRxCallback(RxCallback callback)
{
    NS_LOG_FUNCTION(this);
    NS_ASSERT_MSG(!callback.IsNull(), "SetRxCallback: null callback provided");
    m_rxCallback = callback;
}

uint32_t
BundleCla::NotifyReception(Ptr<Bundle> bundle)
{
    if (!m_rxCallback.IsNull())
    {
        m_rxCallback(bundle);
        return 1;
    }
    return 0;
}

void
BundleCla::ForwardUp(Ptr<Bundle> bundle)
{
    NS_LOG_FUNCTION(this << bundle);
    NS_ASSERT_MSG(bundle, "ForwardUp: null bundle");

    if (m_rxCallback.IsNull())
    {
        NS_LOG_WARN("ForwardUp: no RxCallback set, bundle dropped. "
                    "Call SetRxCallback(MakeCallback(&BundleAgent::RecvBundle, agent)) "
                    "before receiving bundles.");
        return;
    }

    uint32_t result = m_rxCallback(bundle);
    if (result != 0)
    {
        NS_LOG_WARN("ForwardUp: BundleAgent::RecvBundle returned error " << result);
    }
}

} // namespace ns3
