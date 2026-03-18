/*
 * Copyright (c) 2025
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Devansh Gupta <devanshg308@gmail.com>
 */

#include "link-measurement.h"

#include "ns3/log.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("LinkMeasurement");

TypeId
LinkMeasurementRequestHeader::GetTypeId()
{
    static TypeId tid = TypeId("ns3::LinkMeasurementRequestHeader")
                            .SetParent<Header>()
                            .SetGroupName("Wifi")
                            .AddConstructor<LinkMeasurementRequestHeader>();
    return tid;
}

TypeId
LinkMeasurementRequestHeader::GetInstanceTypeId() const
{
    return GetTypeId();
}

void
LinkMeasurementRequestHeader::Print(std::ostream& os) const
{
    os << "dialogToken=" << +m_dialogToken << " txPowerUsed=" << +m_txPowerUsed
       << " maxTxPower=" << +m_maxTxPower;
}

uint32_t
LinkMeasurementRequestHeader::GetSerializedSize() const
{
    return 3;
}

void
LinkMeasurementRequestHeader::Serialize(Buffer::Iterator start) const
{
    Buffer::Iterator i = start;
    i.WriteU8(m_dialogToken);
    i.WriteU8(static_cast<uint8_t>(m_txPowerUsed));
    i.WriteU8(static_cast<uint8_t>(m_maxTxPower));
}

uint32_t
LinkMeasurementRequestHeader::Deserialize(Buffer::Iterator start)
{
    Buffer::Iterator i = start;
    m_dialogToken = i.ReadU8();
    m_txPowerUsed = static_cast<int8_t>(i.ReadU8());
    m_maxTxPower = static_cast<int8_t>(i.ReadU8());
    return i.GetDistanceFrom(start);
}

void
LinkMeasurementRequestHeader::SetDialogToken(uint8_t token)
{
    m_dialogToken = token;
}

uint8_t
LinkMeasurementRequestHeader::GetDialogToken() const
{
    return m_dialogToken;
}

void
LinkMeasurementRequestHeader::SetTransmitPowerUsed(int8_t power)
{
    m_txPowerUsed = power;
}

int8_t
LinkMeasurementRequestHeader::GetTransmitPowerUsed() const
{
    return m_txPowerUsed;
}

void
LinkMeasurementRequestHeader::SetMaxTransmitPower(int8_t power)
{
    m_maxTxPower = power;
}

int8_t
LinkMeasurementRequestHeader::GetMaxTransmitPower() const
{
    return m_maxTxPower;
}

} // namespace ns3
