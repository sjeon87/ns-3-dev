/*
 * Copyright (c) 2025
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Devansh Gupta <devanshg308@gmail.com>
 */

#include "tpc-report-element.h"

#include "ns3/log.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("TpcReportElement");

WifiInformationElementId
TpcReportElement::ElementId() const
{
    return IE_TPC_REPORT;
}

void
TpcReportElement::Print(std::ostream& os) const
{
    os << "TPC Report: txPower=" << +m_transmitPower << " linkMargin=" << +m_linkMargin;
}

void
TpcReportElement::SetTransmitPower(int8_t power)
{
    m_transmitPower = power;
}

int8_t
TpcReportElement::GetTransmitPower() const
{
    return m_transmitPower;
}

void
TpcReportElement::SetLinkMargin(int8_t margin)
{
    m_linkMargin = margin;
}

int8_t
TpcReportElement::GetLinkMargin() const
{
    return m_linkMargin;
}

uint16_t
TpcReportElement::GetInformationFieldSize() const
{
    return 2;
}

void
TpcReportElement::SerializeInformationField(Buffer::Iterator start) const
{
    start.WriteU8(static_cast<uint8_t>(m_transmitPower));
    start.WriteU8(static_cast<uint8_t>(m_linkMargin));
}

uint16_t
TpcReportElement::DeserializeInformationField(Buffer::Iterator start, uint16_t length)
{
    m_transmitPower = static_cast<int8_t>(start.ReadU8());
    m_linkMargin = static_cast<int8_t>(start.ReadU8());
    return 2;
}

} // namespace ns3
