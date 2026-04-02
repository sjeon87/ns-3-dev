/*
 * Copyright (c) 2014 Universitat Autònoma de Barcelona
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 *
 *
 * Author: Rubén Martínez <rmartinez@deic.uab.cat>
 */

#include "ltp-convergence-layer-adapter.h"

#include "ns3/log.h"
#include "ns3/uinteger.h"

NS_LOG_COMPONENT_DEFINE("LtpConvergenceLayerAdapter");

namespace ns3
{
namespace ltp
{

NS_OBJECT_ENSURE_REGISTERED(LtpConvergenceLayerAdapter);

TypeId
LtpConvergenceLayerAdapter::GetTypeId(void)
{
    static TypeId tid =
        TypeId("ns3::LtpConvergenceLayerAdapter")
            .SetParent<Object>()
            .AddAttribute("RemotePeer",
                          "Engine ID of remote peer",
                          UintegerValue(0),
                          MakeUintegerAccessor(&LtpConvergenceLayerAdapter::m_peerLtpEngineId),
                          MakeUintegerChecker<uint64_t>());
    return tid;
}

LtpConvergenceLayerAdapter::LtpConvergenceLayerAdapter()
    : m_linkState(false),
      m_peerLtpEngineId(0),
      m_activeSessionId(SessionId())
{
    NS_LOG_FUNCTION(this);
}

LtpConvergenceLayerAdapter::~LtpConvergenceLayerAdapter()
{
    NS_LOG_FUNCTION(this);
}

void
LtpConvergenceLayerAdapter::SetLinkUpCallback(Callback<void, Ptr<LtpConvergenceLayerAdapter>> cb)
{
    NS_LOG_FUNCTION(this << &cb);
    m_linkUp = cb;
}

void
LtpConvergenceLayerAdapter::SetLinkDownCallback(Callback<void, Ptr<LtpConvergenceLayerAdapter>> cb)
{
    NS_LOG_FUNCTION(this << &cb);
    m_linkDown = cb;
}

void
LtpConvergenceLayerAdapter::SetCheckPointSentCallback(Callback<void, SessionId, RedSegmentInfo> cb)
{
    NS_LOG_FUNCTION(this << &cb);
    m_checkpointSent = cb;
}

void
LtpConvergenceLayerAdapter::SetReportSentCallback(Callback<void, SessionId, RedSegmentInfo> cb)
{
    NS_LOG_FUNCTION(this << &cb);
    m_reportSent = cb;
}

void
LtpConvergenceLayerAdapter::SetEndOfBlockSentCallback(Callback<void, SessionId> cb)
{
    NS_LOG_FUNCTION(this << &cb);
    m_endOfBlockSent = cb;
}

void
LtpConvergenceLayerAdapter::SetCancellationCallback(Callback<void, SessionId> cb)
{
    NS_LOG_FUNCTION(this << &cb);
    m_cancelSent = cb;
}

void
LtpConvergenceLayerAdapter::SetLinkUp()
{
    NS_LOG_FUNCTION(this);

    if (!m_linkState)
    {
        m_linkState = true;
        m_linkUp(this);
    }
}

void
LtpConvergenceLayerAdapter::SetLinkDown()
{
    NS_LOG_FUNCTION(this);
    if (m_linkState)
    {
        m_linkState = false;
        m_linkDown(this);
    }
}

uint64_t
LtpConvergenceLayerAdapter::GetRemoteEngineId() const
{
    NS_LOG_FUNCTION(this);
    return m_peerLtpEngineId;
}

void
LtpConvergenceLayerAdapter::SetRemoteEngineId(uint64_t id)
{
    NS_LOG_FUNCTION(this);
    m_peerLtpEngineId = id;
}

SessionId
LtpConvergenceLayerAdapter::GetSessionId() const
{
    NS_LOG_FUNCTION(this);
    return m_activeSessionId;
}

void
LtpConvergenceLayerAdapter::SetSessionId(SessionId id)
{
    NS_LOG_FUNCTION(this);
    m_activeSessionId = id;
}

} // namespace ltp
} // namespace ns3
