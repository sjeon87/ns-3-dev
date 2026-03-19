/*
 * Copyright (c) 2026
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Devansh Gupta <devanshg308@gmail.com>
 */

#include "radio-measurement-report.h"

#include "ns3/log.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("RadioMeasurementReportHeader");
NS_OBJECT_ENSURE_REGISTERED(RadioMeasurementReportHeader);

TypeId
RadioMeasurementReportHeader::GetTypeId()
{
    static TypeId tid = TypeId("ns3::RadioMeasurementReportHeader")
                            .SetParent<Header>()
                            .SetGroupName("Wifi")
                            .AddConstructor<RadioMeasurementReportHeader>();
    return tid;
}

TypeId
RadioMeasurementReportHeader::GetInstanceTypeId() const
{
    return GetTypeId();
}

void
RadioMeasurementReportHeader::Print(std::ostream& os) const
{
    os << "DialogToken=" << +m_dialogToken
       << ", MeasurementReportElements=" << m_measurementReportElements.size();
}

uint32_t
RadioMeasurementReportHeader::GetSerializedSize() const
{
    uint32_t size = 1; // Dialog Token (1)
    for (const auto& elem : m_measurementReportElements)
    {
        size += elem.GetSerializedSize();
    }
    return size;
}

void
RadioMeasurementReportHeader::Serialize(Buffer::Iterator start) const
{
    Buffer::Iterator i = start;
    i.WriteU8(m_dialogToken);
    for (const auto& elem : m_measurementReportElements)
    {
        i = elem.Serialize(i);
    }
}

uint32_t
RadioMeasurementReportHeader::Deserialize(Buffer::Iterator start)
{
    Buffer::Iterator i = start;
    m_dialogToken = i.ReadU8();

    m_measurementReportElements.clear();
    while (true)
    {
        auto tmp = i;
        MeasurementReportElement elem;
        i = elem.DeserializeIfPresent(i);
        if (i.GetDistanceFrom(tmp) == 0)
        {
            break;
        }
        m_measurementReportElements.push_back(std::move(elem));
    }

    return i.GetDistanceFrom(start);
}

void
RadioMeasurementReportHeader::SetDialogToken(uint8_t token)
{
    m_dialogToken = token;
}

uint8_t
RadioMeasurementReportHeader::GetDialogToken() const
{
    return m_dialogToken;
}

void
RadioMeasurementReportHeader::AddMeasurementReportElement(const MeasurementReportElement& elem)
{
    m_measurementReportElements.push_back(elem);
}

const std::vector<MeasurementReportElement>&
RadioMeasurementReportHeader::GetMeasurementReportElements() const
{
    return m_measurementReportElements;
}

} // namespace ns3
