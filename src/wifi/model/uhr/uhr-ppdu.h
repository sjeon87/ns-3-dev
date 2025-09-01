/*
 * Copyright (c) 2024 DERONNE SOFTWARE ENGINEERING
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Sébastien Deronne <sebastien.deronne@gmail.com>
 */

#ifndef UHR_PPDU_H
#define UHR_PPDU_H

#include "ns3/eht-ppdu.h"

/**
 * @file
 * @ingroup wifi
 * Declaration of ns3::UhrPpdu class.
 */

namespace ns3
{

/**
 * @brief UHR PPDU (11bn)
 * @ingroup wifi
 */
class UhrPpdu : public EhtPpdu
{
  public:
    /// User Specific Fields in HE-SIG-Bs.
    struct UhrSigUserSpecificField
    {
        uint16_t staId : 11 {NO_USER_STA_ID};     ///< STA-ID
        uint8_t mcs : 5 {0};                      ///< MCS index
        uint8_t nss : 3 {1};                      ///< number of spatial streams
        uint8_t ueqm : 1 {0};                     ///< flag whether EQM or UEQM is used
        uint8_t bfAndCodingOrUeqmPattern : 2 {0}; ///< Beamformed And Coding / UEQM Pattern

        /**
         * @brief Three-way comparison
         * @param rhs right hand side
         * @return deduced comparison type
         */
        auto operator<=>(const UhrSigUserSpecificField& rhs) const = default;
    };

    /// UHR SIG content Channels
    using UhrSigContentChannel = std::vector<UhrSigUserSpecificField>;
    /// UHR SIG Content Channels
    using UhrSigContentChannels = std::vector<UhrSigContentChannel>;

    /**
     * PHY header for UHR TB PPDUs
     */
    struct UhrTbPhyHeader
    {
        // U-SIG fields
        uint8_t m_phyVersionId : 3 {1}; ///< PHY Version Identifier field
        uint8_t m_bandwidth : 3 {0};    ///< Bandwidth field
        uint8_t m_bssColor : 6 {0};     ///< BSS color field
        uint8_t m_ppduType : 2 {0};     ///< PPDU Type And Compressed Mode field
    };

    /**
     * PHY header for UHR MU PPDUs
     */
    struct UhrMuPhyHeader
    {
        // U-SIG fields
        uint8_t m_phyVersionId : 3 {1};         ///< PHY Version Identifier field
        uint8_t m_bandwidth : 3 {0};            ///< Bandwidth field
        uint8_t m_bssColor : 6 {0};             ///< BSS color field
        uint8_t m_ppduType : 2 {0};             ///< PPDU Type And Compressed Mode field
        uint8_t m_puncturedChannelInfo : 5 {0}; ///< Punctured Channel Information field
        uint8_t m_uhrSigMcs : 2 {0};            ///< UHR-SIG MCS

        // UHR-SIG fields
        uint8_t m_giLtfSize : 2 {0}; ///< GI+LTF Size field

        std::optional<RuAllocation> m_ruAllocationA; //!< RU Allocation-A that are going to be
                                                     //!< carried in UHR-SIG common subfields
        std::optional<RuAllocation> m_ruAllocationB; //!< RU Allocation-B that are going to be
                                                     //!< carried in UHR-SIG common subfields

        UhrSigContentChannels m_contentChannels; //!< UHR-SIG Content Channels
    };

    /**
     * PHY header for UHR ELR PPDUs
     */
    struct UhrElrPhyHeader
    {
        // U-SIG fields
        uint8_t m_phyVersionId : 3 {1}; ///< PHY Version Identifier field
        uint8_t m_bandwidth : 3 {0};    ///< Bandwidth field
        uint8_t m_bssColor : 6 {0};     ///< BSS color field
        uint8_t m_ppduType : 2 {3};     ///< PPDU Type And Compressed Mode field
        uint16_t m_staId : 11 {3};      ///< STA-ID field

        // ELR-SIG fields
        uint8_t m_elrVersionId : 1 {0}; ///< ELR Version Identifier field
        uint8_t m_mcs : 1 {0};          ///< MCS field
        uint8_t m_coding : 1 {0};       ///< Coding field
        uint16_t m_length : 9 {0};      ///< Length field
        // TODO: STA-ID also defined as an ELR-SIG field, but let's ignore it for now since it is
        // duplicated information
    };

    /// type of the EHT PHY header
    using UhrPhyHeader =
        std::variant<std::monostate, UhrTbPhyHeader, UhrMuPhyHeader, UhrElrPhyHeader>;

    /**
     * Create an UHR PPDU, storing a map of PSDUs.
     *
     * @param psdus the PHY payloads (PSDUs)
     * @param txVector the TXVECTOR that was used for this PPDU
     * @param channel the operating channel of the PHY used to transmit this PPDU
     * @param ppduDuration the transmission duration of this PPDU
     * @param uid the unique ID of this PPDU or of the triggering PPDU if this is an EHT TB PPDU
     * @param flag the flag indicating the type of Tx PSD to build
     * @param instantiateHeaders flag used to instantiate EHT header, should be disabled by child
     */
    UhrPpdu(const WifiConstPsduMap& psdus,
            const WifiTxVector& txVector,
            const WifiPhyOperatingChannel& channel,
            Time ppduDuration,
            uint64_t uid,
            TxPsdFlag flag,
            bool instantiateHeaders = true);

    WifiPpduType GetType() const override;
    Ptr<WifiPpdu> Copy() const override;

  protected:
    uint8_t GetBssColor() const override;

    /**
     * Get the UHR-SIG content channels for a given PPDU
     * IEEE 802.11bn-D1.0 38.3.15.9.2 UHR-SIG content channels
     *
     * @param txVector the TXVECTOR used for the PPDU
     * @param p20Index the index of the primary20 channel
     * @return EHT-SIG content channels
     */
    static UhrSigContentChannels GetUhrSigContentChannels(const WifiTxVector& txVector,
                                                          uint8_t p20Index);

    /**
     * @copydoc HePpdu::SetHeMuUserInfos
     */
    void SetHeMuUserInfos(WifiTxVector& txVector,
                          WifiModulationClass mc,
                          uint8_t p20Index,
                          const RuAllocation& ruAllocation,
                          std::optional<Center26ToneRuIndication> center26ToneRuIndication,
                          const UhrSigContentChannels& contentChannels,
                          bool sigBCompression,
                          uint8_t numMuMimoUsers) const;

    /**
     * @copydoc EhtPpdu::GetChannelWidthEncodingFromMhz
     */
    static uint8_t GetChannelWidthEncodingFromMhz(MHz_u channelWidth,
                                                  const WifiPhyOperatingChannel& channel);

  private:
    bool IsDlMu() const override;
    bool IsUlMu() const override;
    WifiMode GetMcs(uint8_t mcs) const override;
    void SetTxVectorFromPhyHeaders(WifiTxVector& txVector) const override;

    /**
     * Fill in the PHY headers.
     *
     * @param txVector the TXVECTOR that was used for this PPDU
     * @param ppduDuration the transmission duration of this PPDU
     */
    void SetPhyHeaders(const WifiTxVector& txVector, Time ppduDuration) override;

    /**
     * Fill in the UHR PHY header.
     *
     * @param txVector the TXVECTOR that was used for this PPDU
     */
    void SetUhrPhyHeader(const WifiTxVector& txVector);

    UhrPhyHeader m_uhrPhyHeader; //!< the UHR PHY header
}; // class UhrPpdu

} // namespace ns3

#endif /* UHR_PPDU_H */
