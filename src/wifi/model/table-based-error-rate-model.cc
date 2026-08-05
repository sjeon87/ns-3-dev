/*
 * Copyright (c) 2020 University of Washington
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Authors: Rohan Patidar <rpatidar@uw.edu>
 *          Sébastien Deronne <sebastien.deronne@gmail.com>
 */

#include "table-based-error-rate-model.h"

#include "wifi-phy-common.h"
#include "wifi-ru.h"
#include "wifi-tx-vector.h"
#include "wifi-utils.h"
#include "yans-error-rate-model.h"

#include "ns3/dsss-error-rate-model.h"
#include "ns3/log.h"
#include "ns3/pointer.h"
#include "ns3/string.h"
#include "ns3/uinteger.h"

#include <algorithm>
#include <cmath>

namespace ns3
{

static const double SNR_PRECISION = 2;                        //!< precision for SNR
static const double TABLE_BASED_ERROR_MODEL_PRECISION = 1e-5; //!< precision for PER

NS_OBJECT_ENSURE_REGISTERED(TableBasedErrorRateModel);

NS_LOG_COMPONENT_DEFINE("TableBasedErrorRateModel");

TypeId
TableBasedErrorRateModel::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::TableBasedErrorRateModel")
            .SetParent<ErrorRateModel>()
            .SetGroupName("Wifi")
            .AddConstructor<TableBasedErrorRateModel>()
            .AddAttribute("FallbackErrorRateModel",
                          "Ptr to the fallback error rate model to be used when no matching value "
                          "is found in a table",
                          PointerValue(CreateObject<YansErrorRateModel>()),
                          MakePointerAccessor(&TableBasedErrorRateModel::m_fallbackErrorModel),
                          MakePointerChecker<ErrorRateModel>())
            .AddAttribute("SizeThreshold",
                          "Threshold in bytes over which the table for large size frames is used",
                          UintegerValue(400),
                          MakeUintegerAccessor(&TableBasedErrorRateModel::m_threshold),
                          MakeUintegerChecker<uint64_t>());
    return tid;
}

TableBasedErrorRateModel::TableBasedErrorRateModel()
{
    NS_LOG_FUNCTION(this);
}

TableBasedErrorRateModel::~TableBasedErrorRateModel()
{
    NS_LOG_FUNCTION(this);
    m_fallbackErrorModel = nullptr;
}

dB_u
TableBasedErrorRateModel::RoundSnr(dB_u snr, double precision) const
{
    NS_LOG_FUNCTION(this << snr);
    const auto multiplier = std::round(std::pow(10.0, precision));
    return dB_u{std::floor(snr * multiplier + 0.5) / multiplier};
}

std::optional<uint8_t>
TableBasedErrorRateModel::GetMcsForMode(WifiMode mode)
{
    std::optional<uint8_t> mcs;
    WifiModulationClass modulationClass = mode.GetModulationClass();
    WifiCodeRate codeRate = mode.GetCodeRate();
    uint16_t constellationSize = mode.GetConstellationSize();

    if (modulationClass == WIFI_MOD_CLASS_OFDM || modulationClass == WIFI_MOD_CLASS_ERP_OFDM)
    {
        if (constellationSize == 2) // BPSK
        {
            if (codeRate == WIFI_CODE_RATE_1_2)
            {
                mcs = 0;
            }
            if (codeRate == WIFI_CODE_RATE_3_4)
            {
                // No MCS uses BPSK and a Coding Rate of 3/4
            }
        }
        else if (constellationSize == 4) // QPSK
        {
            if (codeRate == WIFI_CODE_RATE_1_2)
            {
                mcs = 1;
            }
            else if (codeRate == WIFI_CODE_RATE_3_4)
            {
                mcs = 2;
            }
        }
        else if (constellationSize == 16) // 16-QAM
        {
            if (codeRate == WIFI_CODE_RATE_1_2)
            {
                mcs = 3;
            }
            else if (codeRate == WIFI_CODE_RATE_3_4)
            {
                mcs = 4;
            }
        }
        else if (constellationSize == 64) // 64-QAM
        {
            if (codeRate == WIFI_CODE_RATE_2_3)
            {
                mcs = 5;
            }
            else if (codeRate == WIFI_CODE_RATE_3_4)
            {
                mcs = 6;
            }
        }
    }
    else if (modulationClass >= WIFI_MOD_CLASS_HT)
    {
        mcs = mode.GetMcsValue();
    }
    return mcs;
}

double
TableBasedErrorRateModel::DoGetChunkSuccessRate(WifiMode mode,
                                                const WifiTxVector& txVector,
                                                double snr,
                                                uint64_t nbits,
                                                uint8_t numRxAntennas,
                                                WifiPpduField field,
                                                uint16_t staId) const
{
    NS_LOG_FUNCTION(this << mode << txVector << snr << nbits << +numRxAntennas << field << staId);
    const auto size = std::max<uint64_t>(1, (nbits / 8)); // in bytes

    uint8_t mcs;
    if (auto ret = GetMcsForMode(mode); ret.has_value())
    {
        mcs = ret.value();
    }
    else
    {
        NS_LOG_DEBUG("No MCS found for mode " << mode << ": use fallback error rate model");
        return m_fallbackErrorModel
            ->GetChunkSuccessRate(mode, txVector, snr, nbits, numRxAntennas, field, staId);
    }

    const auto modClass = mode.GetModulationClass();
    const auto channelWidth = txVector.GetChannelWidth();

    // Non-HT duplicate PPDUs and the pre-HT fields of wide PPDUs replicate the 20 MHz
    // OFDM tone layout on each 20 MHz subchannel, so the 20 MHz tone plan applies
    auto lookupWidth = channelWidth;
    if ((modClass == WIFI_MOD_CLASS_OFDM || modClass == WIFI_MOD_CLASS_ERP_OFDM) &&
        (channelWidth > MHz_u{20}))
    {
        lookupWidth = MHz_u{20};
    }

    // Defaults cover queries with no defined tone plan (e.g. rate managers probing an
    // HT MCS with a channel width above 40 MHz when building SNR threshold tables)
    double fftLength = 64.0;
    double numTones = 52.0; // 48 data + 4 pilots
    if (const auto tonePlan = GetTonePlan(modClass, lookupWidth))
    {
        fftLength = tonePlan->fftLength;
        numTones = tonePlan->usedTones;
    }
    else
    {
        NS_LOG_DEBUG("No tone plan for " << modClass << " at " << channelWidth
                                         << " MHz; using non-HT OFDM 20 MHz parameters");
    }

    // For MU PPDUs, the SNR is computed over the band of the RU allocated to the receiving
    // STA rather than over the full channel, so derive the tone metrics from that RU.
    //
    // PHY header fields are received by all STAs and carry no valid MU station ID
    // (staId is SU_STA_ID), hence the RU-derived metrics only apply to the payload
    if (txVector.IsMu() && (staId != SU_STA_ID))
    {
        // The tone span is identical for all RUs of a given type, hence PHY index 1 is used
        // fftLength after the group computation is no longer the FFT size of the full channel.
        // It becomes the subcarrier span of the RU (first to last index inclusive), while
        // numTones is the count of actually used subcarriers. Their ratio captures any internal
        // gaps (DC subcarriers) within the RU. For example:
        // - 26-tone RU, 20 MHz HE:
        //     - Initial from tonePlan: fftLength=256, numTones=242
        //     - Group (phyIndex=1): {{-121, -96}}
        //     - Result: fftLength = -96 - (-121) + 1 = 26,
        //               numTones = 26 (no internal DC gap)
        // - 996-tone RU, 80 MHz HE:
        //     - Initial from tonePlan: fftLength=1024, numTones=996
        //     - Group (phyIndex=1): {{-500, -3}, {3, 500}}
        //     - Result: fftLength = 500 - (-500) + 1 = 1001,
        //               numTones = 498 + 498 = 996 (5 DC/guard subcarriers in the center)
        const auto group = WifiRu::GetSubcarrierGroup(channelWidth,
                                                      WifiRu::GetRuType(txVector.GetRu(staId)),
                                                      1,
                                                      modClass);
        if (!group.empty())
        {
            fftLength = group.back().second - group.front().first + 1;
            numTones = 0.0;
            for (const auto& range : group)
            {
                numTones += range.second - range.first + 1;
            }
        }
    }

    // Compute calibration factor and shift lookup index to track table generation criteria
    // The SNR is adjusted to account for the difference in subcarrier density between the
    // reference table and the actual transmission. The adjustment is based on the ratio of
    // the FFT length to the number of tones used for data transmission. The adjustment is
    // calculated as 10 * log10(fftLength / numTones), which represents the difference in power
    // per subcarrier due to the different number of tones.
    //
    // See, for example, Eq. (2) in p. 32 of R. Patidar, S. Roy, T. R. Henderson, and
    // A. Chandramohan, "Link-to-System Mapping for ns-3 Wi-Fi OFDMA Error Models," in Proc. of
    // WNS3 2017, Porto, Portugal - June 13-14, 2017, ISBN: 978-1-4503-5219-2.
    const auto subcarrierAdjustment = 10.0 * std::log10((double)fftLength / numTones);
    const auto roundedSnr = RoundSnr(RatioToDb(snr) + subcarrierAdjustment, SNR_PRECISION);

    bool ldpc = txVector.IsLdpc();
    NS_LOG_FUNCTION(this << +mcs << roundedSnr << size << ldpc);

    // HT: for MCS greater than 7, use 0 - 7 curves for data rate
    if (mode.GetModulationClass() == WIFI_MOD_CLASS_HT)
    {
        mcs = mcs % 8;
    }

    if (mcs >= (ldpc ? ERROR_TABLE_LDPC_MAX_NUM_MCS : ERROR_TABLE_BCC_MAX_NUM_MCS))
    {
        NS_LOG_WARN("Table missing for MCS: "
                    << +mcs << " in TableBasedErrorRateModel: use fallback error rate model");
        return m_fallbackErrorModel
            ->GetChunkSuccessRate(mode, txVector, snr, nbits, numRxAntennas, field, staId);
    }

    auto errorTable = (ldpc ? AwgnErrorTableLdpc1458
                            : (size < m_threshold ? AwgnErrorTableBcc32 : AwgnErrorTableBcc1458));
    const auto& itVector = errorTable[mcs];
    auto itTable =
        std::find_if(itVector.cbegin(), itVector.cend(), [&roundedSnr](const auto& element) {
            return element.first == roundedSnr;
        });
    const auto minSnr = itVector.cbegin()->first;
    const auto maxSnr = (--itVector.cend())->first;
    double per;
    if (itTable == itVector.cend())
    {
        if (roundedSnr < minSnr)
        {
            per = 1.0;
        }
        else if (roundedSnr > maxSnr)
        {
            per = 0.0;
        }
        else
        {
            double a = 0.0;
            double b = 0.0;
            dB_u previousSnr{0.0};
            dB_u nextSnr{0.0};
            for (auto i = itVector.cbegin(); i != itVector.cend(); ++i)
            {
                if (i->first < roundedSnr)
                {
                    previousSnr = i->first;
                    a = i->second;
                }
                else
                {
                    nextSnr = i->first;
                    b = i->second;
                    break;
                }
            }
            per = a + (roundedSnr - previousSnr) * (b - a) / (nextSnr - previousSnr);
        }
    }
    else
    {
        per = itTable->second;
    }

    uint16_t tableSize = (ldpc ? ERROR_TABLE_LDPC_FRAME_SIZE
                               : (size < m_threshold ? ERROR_TABLE_BCC_SMALL_FRAME_SIZE
                                                     : ERROR_TABLE_BCC_LARGE_FRAME_SIZE));
    if (size != tableSize)
    {
        // From IEEE document 11-14/0803r1 (Packet Length for Box 0 Calibration)
        per = (1.0 - std::pow((1 - per), (static_cast<double>(size) / tableSize)));
    }

    if (per < TABLE_BASED_ERROR_MODEL_PRECISION)
    {
        per = 0.0;
    }

    return 1.0 - per;
}

} // namespace ns3
