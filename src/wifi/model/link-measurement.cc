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

TypeId
LinkMeasurementReportHeader::GetTypeId()
{
    static TypeId tid = TypeId("ns3::LinkMeasurementReportHeader")
                            .SetParent<Header>()
                            .SetGroupName("Wifi")
                            .AddConstructor<LinkMeasurementReportHeader>();
    return tid;
}

TypeId
LinkMeasurementReportHeader::GetInstanceTypeId() const
{
    return GetTypeId();
}

void
LinkMeasurementReportHeader::Print(std::ostream& os) const
{
    os << "dialogToken=" << +m_dialogToken << " tpcTxPower=" << +m_tpcReport.GetTransmitPower()
       << " tpcLinkMargin=" << +m_tpcReport.GetLinkMargin() << " rxAntennaId=" << +m_rxAntennaId
       << " txAntennaId=" << +m_txAntennaId << " rcpi=" << +m_rcpi << " rsni=" << +m_rsni;
}

uint32_t
LinkMeasurementReportHeader::GetSerializedSize() const
{
    return 1 + m_tpcReport.GetSerializedSize() + 4;
}

void
LinkMeasurementReportHeader::Serialize(Buffer::Iterator start) const
{
    Buffer::Iterator i = start;
    i.WriteU8(m_dialogToken);
    i = m_tpcReport.Serialize(i);
    i.WriteU8(m_rxAntennaId);
    i.WriteU8(m_txAntennaId);
    i.WriteU8(m_rcpi);
    i.WriteU8(m_rsni);
}

uint32_t
LinkMeasurementReportHeader::Deserialize(Buffer::Iterator start)
{
    Buffer::Iterator i = start;
    m_dialogToken = i.ReadU8();
    i = m_tpcReport.Deserialize(i);
    m_rxAntennaId = i.ReadU8();
    m_txAntennaId = i.ReadU8();
    m_rcpi = i.ReadU8();
    m_rsni = i.ReadU8();
    return i.GetDistanceFrom(start);
}

void
LinkMeasurementReportHeader::SetDialogToken(uint8_t token)
{
    m_dialogToken = token;
}

uint8_t
LinkMeasurementReportHeader::GetDialogToken() const
{
    return m_dialogToken;
}

void
LinkMeasurementReportHeader::SetTpcReport(const TpcReportElement& tpc)
{
    m_tpcReport = tpc;
}

const TpcReportElement&
LinkMeasurementReportHeader::GetTpcReport() const
{
    return m_tpcReport;
}

void
LinkMeasurementReportHeader::SetTpcTransmitPower(int8_t power)
{
    m_tpcReport.SetTransmitPower(power);
}

int8_t
LinkMeasurementReportHeader::GetTpcTransmitPower() const
{
    return m_tpcReport.GetTransmitPower();
}

void
LinkMeasurementReportHeader::SetTpcLinkMargin(int8_t margin)
{
    m_tpcReport.SetLinkMargin(margin);
}

int8_t
LinkMeasurementReportHeader::GetTpcLinkMargin() const
{
    return m_tpcReport.GetLinkMargin();
}

void
LinkMeasurementReportHeader::SetRxAntennaId(uint8_t id)
{
    m_rxAntennaId = id;
}

uint8_t
LinkMeasurementReportHeader::GetRxAntennaId() const
{
    return m_rxAntennaId;
}

void
LinkMeasurementReportHeader::SetTxAntennaId(uint8_t id)
{
    m_txAntennaId = id;
}

uint8_t
LinkMeasurementReportHeader::GetTxAntennaId() const
{
    return m_txAntennaId;
}

void
LinkMeasurementReportHeader::SetRcpi(uint8_t rcpi)
{
    m_rcpi = rcpi;
}

uint8_t
LinkMeasurementReportHeader::GetRcpi() const
{
    return m_rcpi;
}

void
LinkMeasurementReportHeader::SetRsni(uint8_t rsni)
{
    m_rsni = rsni;
}

uint8_t
LinkMeasurementReportHeader::GetRsni() const
{
    return m_rsni;
}

} // namespace ns3
