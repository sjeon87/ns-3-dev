/*
 * Copyright (c) 2024 DERONNE SOFTWARE ENGINEERING
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Sébastien Deronne <sebastien.deronne@gmail.com>
 */

#include "uhr-ppdu.h"

#include "uhr-phy.h"

#include "ns3/log.h"
#include "ns3/wifi-phy-operating-channel.h"
#include "ns3/wifi-psdu.h"

#include <algorithm>
#include <numeric>

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("UhrPpdu");

UhrPpdu::UhrPpdu(const WifiConstPsduMap& psdus,
                 const WifiTxVector& txVector,
                 const WifiPhyOperatingChannel& channel,
                 Time ppduDuration,
                 uint64_t uid,
                 TxPsdFlag flag,
                 bool instantiateHeaders /* = true */)
    : EhtPpdu(psdus, txVector, channel, ppduDuration, uid, flag, false)
{
    NS_LOG_FUNCTION(this << psdus << txVector << channel << ppduDuration << uid << flag
                         << instantiateHeaders);
    if (instantiateHeaders)
    {
        SetPhyHeaders(txVector, ppduDuration);
    }
}

void
UhrPpdu::SetPhyHeaders(const WifiTxVector& txVector, Time ppduDuration)
{
    NS_LOG_FUNCTION(this << txVector << ppduDuration);
    SetLSigHeader(ppduDuration);
    SetUhrPhyHeader(txVector);
}

void
UhrPpdu::SetUhrPhyHeader(const WifiTxVector& txVector)
{
    const auto bssColor = txVector.GetBssColor();
    NS_ASSERT(bssColor < 64);
    if (ns3::IsDlMu(m_preamble))
    {
        const auto p20Index = m_operatingChannel.GetPrimaryChannelIndex(MHz_u{20});
        m_uhrPhyHeader.emplace<UhrMuPhyHeader>(UhrMuPhyHeader{
            .m_bandwidth =
                GetChannelWidthEncodingFromMhz(txVector.GetChannelWidth(), m_operatingChannel),
            .m_bssColor = bssColor,
            .m_ppduType = txVector.GetEhtPpduType(),
            // TODO: UHR PPDU should store U-SIG per 20 MHz band, assume it is the lowest 20 MHz
            // band for now
            .m_puncturedChannelInfo =
                GetPuncturedInfo(txVector.GetInactiveSubchannels(),
                                 txVector.GetEhtPpduType(),
                                 (txVector.IsDlMu() && (txVector.GetChannelWidth() > MHz_u{80}))
                                     ? std::optional{true}
                                     : std::nullopt),
            .m_uhrSigMcs = txVector.GetSigBMode().GetMcsValue(),
            .m_giLtfSize = GetGuardIntervalAndNltfEncoding(txVector.GetGuardInterval(),
                                                           2 /*NLTF currently unused*/),
            .m_ruAllocationA = txVector.IsMu() && !txVector.IsSigBCompression()
                                   ? std::optional{txVector.GetRuAllocation(p20Index)}
                                   : std::nullopt,
            // TODO: RU Allocation-B not supported yet
            .m_contentChannels = GetUhrSigContentChannels(txVector, p20Index)});
    }
    else if (ns3::IsUlMu(m_preamble))
    {
        m_uhrPhyHeader.emplace<UhrTbPhyHeader>(
            UhrTbPhyHeader{.m_bandwidth = GetChannelWidthEncodingFromMhz(txVector.GetChannelWidth(),
                                                                         m_operatingChannel),
                           .m_bssColor = bssColor,
                           .m_ppduType = txVector.GetEhtPpduType()});
    }
}

WifiPpduType
UhrPpdu::GetType() const
{
    if (m_psdus.contains(SU_STA_ID))
    {
        return WIFI_PPDU_TYPE_SU;
    }
    switch (m_preamble)
    {
    case WIFI_PREAMBLE_UHR_MU:
        return WIFI_PPDU_TYPE_DL_MU;
    case WIFI_PREAMBLE_UHR_TB:
        return WIFI_PPDU_TYPE_UL_MU;
    default:
        NS_ASSERT_MSG(false, "invalid preamble " << m_preamble);
        return WIFI_PPDU_TYPE_SU;
    }
}

bool
UhrPpdu::IsDlMu() const
{
    return (m_preamble == WIFI_PREAMBLE_UHR_MU) && !m_psdus.contains(SU_STA_ID);
}

bool
UhrPpdu::IsUlMu() const
{
    return (m_preamble == WIFI_PREAMBLE_UHR_TB) && !m_psdus.contains(SU_STA_ID);
}

void
UhrPpdu::SetTxVectorFromPhyHeaders(WifiTxVector& txVector) const
{
    txVector.SetAggregation(m_psdus.size() > 1 || m_psdus.begin()->second->IsAggregate());
    if (ns3::IsDlMu(m_preamble))
    {
        txVector.SetLength(m_lSig.GetLength());
        auto uhrPhyHeader = std::get_if<UhrMuPhyHeader>(&m_uhrPhyHeader);
        NS_ASSERT(uhrPhyHeader);
        const auto bw = GetChannelWidthMhzFromEncoding(uhrPhyHeader->m_bandwidth);
        txVector.SetChannelWidth(bw);
        txVector.SetBssColor(uhrPhyHeader->m_bssColor);
        txVector.SetEhtPpduType(uhrPhyHeader->m_ppduType);
        if (bw > MHz_u{80})
        {
            // TODO: use punctured channel information
        }
        txVector.SetSigBMode(UhrPhy::GetUhrMcs(uhrPhyHeader->m_uhrSigMcs));
        txVector.SetGuardInterval(GetGuardIntervalFromEncoding(uhrPhyHeader->m_giLtfSize));
        const auto ruAllocation = uhrPhyHeader->m_ruAllocationA; // RU Allocation-B not supported
                                                                 // yet
        if (const auto p20Index = m_operatingChannel.GetPrimaryChannelIndex(MHz_u{20});
            ruAllocation.has_value())
        {
            const auto isMuMimo = (uhrPhyHeader->m_ppduType == 2);
            const auto muMimoUsers =
                isMuMimo
                    ? std::accumulate(uhrPhyHeader->m_contentChannels.cbegin(),
                                      uhrPhyHeader->m_contentChannels.cend(),
                                      0,
                                      [](uint8_t prev, const auto& cc) { return prev + cc.size(); })
                    : 0;
            SetHeMuUserInfos(txVector,
                             WIFI_MOD_CLASS_UHR,
                             p20Index,
                             ruAllocation.value(),
                             std::nullopt,
                             uhrPhyHeader->m_contentChannels,
                             uhrPhyHeader->m_ppduType == 2,
                             muMimoUsers);
            txVector.SetRuAllocation(ruAllocation.value(), p20Index);
        }
        else if (uhrPhyHeader->m_ppduType == 1) // UHR SU
        {
            NS_ASSERT(uhrPhyHeader->m_contentChannels.size() == 1 &&
                      uhrPhyHeader->m_contentChannels.front().size() == 1);
            txVector.SetMode(GetMcs(uhrPhyHeader->m_contentChannels.front().front().mcs));
            txVector.SetNss(uhrPhyHeader->m_contentChannels.front().front().nss);
        }
        else
        {
            const auto fullBwRu{EhtRu::RuSpec(WifiRu::GetRuType(bw), 1, true, true)};
            txVector.SetHeMuUserInfo(uhrPhyHeader->m_contentChannels.front().front().staId,
                                     {fullBwRu,
                                      uhrPhyHeader->m_contentChannels.front().front().mcs,
                                      uhrPhyHeader->m_contentChannels.front().front().nss});
        }
    }
    else if (ns3::IsUlMu(m_preamble))
    {
        txVector.SetLength(m_lSig.GetLength());
        auto uhrPhyHeader = std::get_if<UhrTbPhyHeader>(&m_uhrPhyHeader);
        NS_ASSERT(uhrPhyHeader);
        txVector.SetChannelWidth(GetChannelWidthMhzFromEncoding(uhrPhyHeader->m_bandwidth));
        txVector.SetBssColor(uhrPhyHeader->m_bssColor);
        txVector.SetEhtPpduType(uhrPhyHeader->m_ppduType);
    }
}

UhrPpdu::UhrSigContentChannels
UhrPpdu::GetUhrSigContentChannels(const WifiTxVector& txVector, uint8_t p20Index)
{
    UhrSigContentChannels contentChannels;
    // TODO: UEQM not supported yet, reuse EhtPpdu::GetEhtSigContentChannels
    const auto content = EhtPpdu::GetEhtSigContentChannels(txVector, p20Index);
    std::transform(content.cbegin(),
                   content.cend(),
                   std::back_inserter(contentChannels),
                   [](const auto& cc) {
                       UhrSigContentChannel contentChannel;
                       std::transform(cc.cbegin(),
                                      cc.cend(),
                                      std::back_inserter(contentChannel),
                                      [](const auto& userField) {
                                          return UhrSigUserSpecificField{
                                              .staId = userField.staId,
                                              .mcs = userField.mcs,
                                              .nss = userField.nss,
                                          };
                                      });
                       return contentChannel;
                   });
    return contentChannels;
}

void
UhrPpdu::SetHeMuUserInfos(WifiTxVector& txVector,
                          WifiModulationClass mc,
                          uint8_t p20Index,
                          const RuAllocation& ruAllocation,
                          std::optional<Center26ToneRuIndication> center26ToneRuIndication,
                          const UhrSigContentChannels& contentChannels,
                          bool sigBCompression,
                          uint8_t numMuMimoUsers) const
{
    HeSigBContentChannels eqmContentChannels;
    std::transform(contentChannels.cbegin(),
                   contentChannels.cend(),
                   std::back_inserter(eqmContentChannels),
                   [](const auto& cc) {
                       HeSigBContentChannel contentChannel;
                       std::transform(cc.cbegin(),
                                      cc.cend(),
                                      std::back_inserter(contentChannel),
                                      [](const auto& userField) {
                                          return HeSigBUserSpecificField{
                                              .staId = userField.staId,
                                              .nss = userField.nss,
                                              .mcs = userField.mcs,
                                          };
                                      });
                       return contentChannel;
                   });
    // TODO: UEQM not supported yet, reuse EhtPpdu::SetHeMuUserInfos
    EhtPpdu::SetHeMuUserInfos(txVector,
                              mc,
                              ruAllocation,
                              center26ToneRuIndication,
                              eqmContentChannels,
                              sigBCompression,
                              numMuMimoUsers);
}

uint8_t
UhrPpdu::GetBssColor() const
{
    if (IsUlMu())
    {
        auto uhrSigHeader = std::get_if<UhrTbPhyHeader>(&m_uhrPhyHeader);
        NS_ASSERT(uhrSigHeader);
        return uhrSigHeader->m_bssColor;
    }

    auto uhrSigHeader = std::get_if<UhrMuPhyHeader>(&m_uhrPhyHeader);
    NS_ASSERT(uhrSigHeader);
    return uhrSigHeader->m_bssColor;
}

WifiMode
UhrPpdu::GetMcs(uint8_t mcs) const
{
    return UhrPhy::GetUhrMcs(mcs);
}

uint8_t
UhrPpdu::GetChannelWidthEncodingFromMhz(MHz_u channelWidth, const WifiPhyOperatingChannel& channel)
{
    if (channelWidth == MHz_u{320})
    {
        switch (channel.GetNumber())
        {
        case 31:
        case 95:
        case 159:
        default:
            return 4;
        case 63:
        case 127:
        case 191:
            return 5;
        }
    }
    return HePpdu::GetChannelWidthEncodingFromMhz(channelWidth);
}

Ptr<WifiPpdu>
UhrPpdu::Copy() const
{
    return Ptr<WifiPpdu>(new UhrPpdu(*this), false);
}

} // namespace ns3
