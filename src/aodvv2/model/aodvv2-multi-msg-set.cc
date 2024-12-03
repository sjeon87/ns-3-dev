/*
 * Copyright (c) 2024 University of Florence
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Based on
 *      NS-3 AODV model developed by Elena Buchatskaya and Pavel Boyko of IITP RAS
 *
 * Authors: Francesco Todino <todinofrancesco97@gmail.com>
 *          Tommaso Pecorella <tommaso.pecorella@unifi.it>
 */
#include "aodvv2-multi-msg-set.h"

#include <algorithm>
#include <iomanip>

namespace ns3
{
namespace aodvv2
{
template <typename T>
bool
MultiMsgSet<T>::IsDuplicate(T origIp,
                            uint16_t origMask,
                            T targIp,
                            T seqNoRtr,
                            uint16_t origSeqNum,
                            IpInterfaceAddress interface,
                            Metric<IpAddress> metric,
                            uint8_t* metricValue)
{
    Purge();
    for (auto i = m_msgCache.begin(); i != m_msgCache.end(); ++i)
    {
        if (i->m_origIp == origIp && i->m_origMask == origMask && i->m_targIp == targIp &&
            /* i->m_seqNoRtr == seqNoRtr &&  */ i->m_metric.GetMetricType() ==
                metric.GetMetricType())
        {
            i->m_timestamp = Simulator::Now();
            i->m_removalTime = m_maxSeqnoLifetime + Simulator::Now();

            if (origSeqNum <= i->m_origSeqNum)
            {
                return true;
            }
            else
            {
                if (metric.LoopFree(i->m_metricValue, metricValue))
                {
                    return true;
                }
                else
                {
                    i->m_origSeqNum = origSeqNum;
                    i->m_metricValue = metricValue;
                    return false;
                }
            }
        }
    }

    MultiMsgEntry entry = {origIp,
                           origMask,
                           targIp,
                           seqNoRtr,
                           origSeqNum,
                           interface,
                           metric,
                           metricValue,
                           Simulator::Now(),
                           m_maxSeqnoLifetime + Simulator::Now()};
    m_msgCache.push_back(entry);
    return false;
}

template <typename T>
void
MultiMsgSet<T>::Purge()
{
    m_msgCache.erase(remove_if(m_msgCache.begin(), m_msgCache.end(), IsExpired()),
                     m_msgCache.end());
}

template <typename T>
uint32_t
MultiMsgSet<T>::GetSize()
{
    Purge();
    return m_msgCache.size();
}

template <typename T>
void
MultiMsgSet<T>::Print(Ptr<OutputStreamWrapper> stream, Time::Unit unit /* = Time::S */) const
{
    std::vector<MultiMsgEntry> table = m_msgCache;
    std::ostream* os = stream->GetStream();
    // Copy the current ostream state
    std::ios oldState(nullptr);
    oldState.copyfmt(*os);

    *os << std::resetiosflags(std::ios::adjustfield) << std::setiosflags(std::ios::left);

    *os << "AODVv2 Multicast Msg\n";
    *os << std::setw(16) << "Orig Addr";
    *os << std::setw(16) << "Targ Addr";
    *os << std::setw(16) << "SeqNoRtr Addr";
    *os << std::setw(16) << "Orig SeqNum";
    *os << std::setw(16) << "Metric Type";
    *os << std::setw(16) << "Metric Value" << std::endl;
    for (auto i = table.begin(); i != table.end(); ++i)
    {
        std::ostringstream orig;
        std::ostringstream targ;
        orig << i->m_origIp << "/" << i->m_origMask;
        targ << i->m_targIp << "/" << i->m_origMask;

        *os << std::setw(16) << orig.str();
        *os << std::setw(16) << targ.str();
        *os << std::setw(16) << i->m_seqNoRtr;
        *os << std::setw(16) << i->m_origSeqNum;
        *os << std::setw(16) << static_cast<uint16_t>(i->m_metric.GetMetricType()) << ": ";
        for (uint8_t j = 0; j < i->m_metric.GetMaxMetric(); j++)
        {
            *os << static_cast<uint16_t>(i->m_metricValue[j]) << " ";
        }
        *os << std::setw(16);
    }
    *stream->GetStream() << "\n";
}

template class MultiMsgSet<Ipv4Address>;
template class MultiMsgSet<Ipv6Address>;

} // namespace aodvv2
} // namespace ns3
