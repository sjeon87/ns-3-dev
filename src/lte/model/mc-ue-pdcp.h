/*
 * Copyright (c) 2011-2012 Centre Tecnologic de Telecomunicacions de Catalunya (CTTC)
 * Copyright (c) 2016, University of Padova, Dep. of Information Engineering, SIGNET lab.
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author of base class: Manuel Requena <manuel.requena@cttc.es>
 * Extension to DC devices by Michele Polese <michele.polese@gmail.com>
 */

#ifndef MC_UE_PDCP_H
#define MC_UE_PDCP_H

#include "ns3/object.h"
#include "ns3/trace-source-accessor.h"
#include "ns3/traced-value.h"
#include <ns3/lte-pdcp-sap.h>
#include <ns3/lte-pdcp.h>
#include <ns3/lte-rlc-sap.h>

namespace ns3
{

/**
 * MC UE PDCP entity. It has 2 interfaces to the 2 RLC layers
 * Note: there is a single IMSI and lcid (no problem in having the same
 * in the 2 eNBs), but 2 rnti.
 */
class McUePdcp : public LtePdcp
{
    friend class McUePdcpSpecificLteRlcSapUser;
    friend class LtePdcpSpecificLtePdcpSapProvider<McUePdcp>;

  public:
    McUePdcp();
    ~McUePdcp() override;
    static TypeId GetTypeId();
    void DoDispose() override;

    /**
     * Set the RNTI of the UE in the MmWave eNB
     *
     * \param rnti
     */
    void SetMmWaveRnti(uint16_t rnti);

    /**
     * Set the RNTI of the UE in the LTE eNB
     *
     * \param rnti
     */
    void SetRnti(uint16_t rnti);

    /**
     * Set the ldid
     *
     * \param lcId
     */
    void SetLcId(uint8_t lcId);

    /**
     *
     *
     * \param s the PDCP SAP user to be used by this MC_PDCP
     */
    void SetLtePdcpSapUser(LtePdcpSapUser* s);

    /**
     *
     *
     * \return the PDCP SAP Provider interface offered to the RRC by this MC_PDCP
     */
    LtePdcpSapProvider* GetLtePdcpSapProvider();

    /**
     *
     *
     * \param s the Lte RLC SAP Provider to be used by this MC_PDCP
     */
    void SetLteRlcSapProvider(LteRlcSapProvider* s);

    /**
     *
     *
     * \return the RLC SAP User interface offered to the RLC by this MC_PDCP
     */
    LteRlcSapUser* GetLteRlcSapUser();

    /**
     *
     *
     * \param s the MmWave RLC SAP Provider to be used by this MC_PDCP
     */
    void SetMmWaveRlcSapProvider(LteRlcSapProvider* s);

    static const uint16_t MAX_PDCP_SN = 4096;

    /**
     * Status variables of the PDCP
     *
     */
    struct Status
    {
        uint16_t txSn; ///< TX sequence number
        uint16_t rxSn; ///< RX sequence number
    };

    /**
     *
     * \return the current status of the PDCP
     */
    Status GetStatus();

    /**
     * Set the status of the PDCP
     *
     * \param s
     */
    void SetStatus(Status s);

    /**
     * TracedCallback for PDU transmission event.
     *
     * \param [in] rnti The C-RNTI identifying the UE.
     * \param [in] lcid The logical channel id corresponding to
     *             the sending RLC instance.
     * \param [in] size Packet size.
     */
    typedef void (*PduTxTracedCallback)(uint16_t rnti, uint8_t lcid, uint32_t size);

    /**
     * TracedCallback signature for PDU receive event.
     *
     * \param [in] rnti The C-RNTI identifying the UE.
     * \param [in] lcid The logical channel id corresponding to
     *             the sending RLC instance.
     * \param [in] size Packet size.
     * \param [in] delay Delay since packet sent, in ns..
     */
    typedef void (*PduRxTracedCallback)(const uint16_t rnti,
                                        const uint8_t lcid,
                                        const uint32_t size,
                                        const uint64_t delay);

    /**
     * Switch between LTE and MmWave
     */
    void SwitchConnection(bool useMmWaveConnection);

  protected:
    // Interface provided to upper RRC entity
    void DoTransmitPdcpSdu(LtePdcpSapProvider::TransmitPdcpSduParameters params) override;
    void DoTransmitPdcpSdu(Ptr<Packet> p);

    LtePdcpSapUser* m_pdcpSapUser;
    LtePdcpSapProvider* m_pdcpSapProvider;

    // Interface provided to lower RLC entity
    void DoReceivePdu(Ptr<Packet> p) override;

    LteRlcSapUser* m_rlcSapUser;
    LteRlcSapProvider* m_rlcSapProvider;
    LteRlcSapProvider* m_mmWaveRlcSapProvider;

    uint16_t m_rnti;
    uint8_t m_lcid;
    uint16_t m_mmWaveRnti;

    /**
     * Used to inform of a PDU delivery to the RLC SAP provider.
     * The parameters are RNTI, LCID and bytes delivered
     */
    TracedCallback<uint16_t, uint8_t, uint32_t> m_txPdu;
    /**
     * Used to inform of a PDU reception from the RLC SAP user.
     * The parameters are RNTI, LCID, bytes delivered and delivery delay in nanoseconds.
     */
    TracedCallback<uint16_t, uint8_t, uint32_t, uint64_t> m_rxPdu;

  private:
    /**
     * State variables. See section 7.1 in TS 36.323
     */
    uint16_t m_txSequenceNumber;
    uint16_t m_rxSequenceNumber;

    /**
     * Constants. See section 7.2 in TS 36.323
     */
    static const uint16_t m_maxPdcpSn = 4095;

    bool m_useMmWaveConnection;
    bool m_alwaysLteUplink;
};

} // namespace ns3

#endif // MC_UE_PDCP_H
