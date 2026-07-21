/*
 * Copyright (c) 2016 University of Washington
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Authors: Tom Henderson (tomhend@u.washington.edu)
 *          Sébastien Deronne (sebastien.deronne@gmail.com)
 */

#ifdef HAVE_GSL
#include <gsl/gsl_cdf.h>
#include <gsl/gsl_integration.h>
#include <gsl/gsl_math.h>
#include <gsl/gsl_sf_bessel.h>
#endif

#include "ns3/dsss-error-rate-model.h"
#include "ns3/he-phy.h" //includes HT and VHT
#include "ns3/interference-helper.h"
#include "ns3/log.h"
#include "ns3/nist-error-rate-model.h"
#include "ns3/table-based-error-rate-model.h"
#include "ns3/test.h"
#include "ns3/wifi-phy.h"
#include "ns3/wifi-utils.h"
#include "ns3/yans-error-rate-model.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("WifiErrorRateModelsTest");

static double
FromRss(dBm_u rss)
{
    // SINR is based on receiver noise figure of 7 dB and thermal noise
    // of -100.5522786 dBm in this 22 MHz bandwidth at 290K
    dBm_u noisePower = -100.5522786 + 7;

    dB_u sinr = rss - noisePower;
    // return SINR expressed as ratio
    return pow(10.0, sinr / 10.0);
}

/**
 * @ingroup wifi-test
 * @ingroup tests
 *
 * @brief Wifi Error Rate Models Test Case Dsss
 */
class WifiErrorRateModelsTestCaseDsss : public TestCase
{
  public:
    WifiErrorRateModelsTestCaseDsss();
    ~WifiErrorRateModelsTestCaseDsss() override;

  private:
    void DoRun() override;
};

WifiErrorRateModelsTestCaseDsss::WifiErrorRateModelsTestCaseDsss()
    : TestCase("WifiErrorRateModel test case DSSS")
{
}

WifiErrorRateModelsTestCaseDsss::~WifiErrorRateModelsTestCaseDsss()
{
}

void
WifiErrorRateModelsTestCaseDsss::DoRun()
{
    // 1024 bytes plus headers
    uint64_t size = (1024 + 40 + 14) * 8;
    // Spot test some values returned from DsssErrorRateModel
    // Values taken from sample 80211b.c program used in validation paper
    double value;
    // DBPSK
    value = DsssErrorRateModel::GetDsssDbpskSuccessRate(FromRss(-105.0), size);
    NS_TEST_ASSERT_MSG_EQ_TOL(value, 0, 1e-13, "Not equal within tolerance");
    value = DsssErrorRateModel::GetDsssDbpskSuccessRate(FromRss(-100.0), size);
    NS_TEST_ASSERT_MSG_EQ_TOL(value, 1.5e-13, 1e-13, "Not equal within tolerance");
    value = DsssErrorRateModel::GetDsssDbpskSuccessRate(FromRss(-99.0), size);
    NS_TEST_ASSERT_MSG_EQ_TOL(value, 0.0003, 0.0001, "Not equal within tolerance");
    value = DsssErrorRateModel::GetDsssDbpskSuccessRate(FromRss(-98.0), size);
    NS_TEST_ASSERT_MSG_EQ_TOL(value, 0.202, 0.005, "Not equal within tolerance");
    value = DsssErrorRateModel::GetDsssDbpskSuccessRate(FromRss(-97.0), size);
    NS_TEST_ASSERT_MSG_EQ_TOL(value, 0.813, 0.005, "Not equal within tolerance");
    value = DsssErrorRateModel::GetDsssDbpskSuccessRate(FromRss(-96.0), size);
    NS_TEST_ASSERT_MSG_EQ_TOL(value, 0.984, 0.005, "Not equal within tolerance");
    value = DsssErrorRateModel::GetDsssDbpskSuccessRate(FromRss(-95.0), size);
    NS_TEST_ASSERT_MSG_EQ_TOL(value, 0.999, 0.001, "Not equal within tolerance");
    value = DsssErrorRateModel::GetDsssDbpskSuccessRate(FromRss(-90.0), size);
    NS_TEST_ASSERT_MSG_EQ_TOL(value, 1, 0.001, "Not equal within tolerance");

    // DQPSK
    //
    value = DsssErrorRateModel::GetDsssDqpskSuccessRate(FromRss(-96.0), size);
    NS_TEST_ASSERT_MSG_EQ_TOL(value, 0, 1e-13, "Not equal within tolerance");
    value = DsssErrorRateModel::GetDsssDqpskSuccessRate(FromRss(-95.0), size);
    NS_TEST_ASSERT_MSG_EQ_TOL(value, 4.5e-6, 1e-6, "Not equal within tolerance");
    value = DsssErrorRateModel::GetDsssDqpskSuccessRate(FromRss(-94.0), size);
    NS_TEST_ASSERT_MSG_EQ_TOL(value, 0.036, 0.005, "Not equal within tolerance");
    value = DsssErrorRateModel::GetDsssDqpskSuccessRate(FromRss(-93.0), size);
    NS_TEST_ASSERT_MSG_EQ_TOL(value, 0.519, 0.005, "Not equal within tolerance");
    value = DsssErrorRateModel::GetDsssDqpskSuccessRate(FromRss(-92.0), size);
    NS_TEST_ASSERT_MSG_EQ_TOL(value, 0.915, 0.005, "Not equal within tolerance");
    value = DsssErrorRateModel::GetDsssDqpskSuccessRate(FromRss(-91.0), size);
    NS_TEST_ASSERT_MSG_EQ_TOL(value, 0.993, 0.005, "Not equal within tolerance");
    value = DsssErrorRateModel::GetDsssDqpskSuccessRate(FromRss(-90.0), size);
    NS_TEST_ASSERT_MSG_EQ_TOL(value, 0.999, 0.001, "Not equal within tolerance");
    value = DsssErrorRateModel::GetDsssDqpskSuccessRate(FromRss(-89.0), size);
    NS_TEST_ASSERT_MSG_EQ_TOL(value, 1, 0.001, "Not equal within tolerance");

#ifdef HAVE_GSL
    // DQPSK_CCK5.5
    value = DsssErrorRateModel::GetDsssDqpskCck5_5SuccessRate(FromRss(-94.0), size);
    NS_TEST_ASSERT_MSG_EQ_TOL(value, 0, 1e-13, "Not equal within tolerance");
    value = DsssErrorRateModel::GetDsssDqpskCck5_5SuccessRate(FromRss(-93.0), size);
    NS_TEST_ASSERT_MSG_EQ_TOL(value, 6.6e-14, 5e-14, "Not equal within tolerance");
    value = DsssErrorRateModel::GetDsssDqpskCck5_5SuccessRate(FromRss(-92.0), size);
    NS_TEST_ASSERT_MSG_EQ_TOL(value, 0.0001, 0.00005, "Not equal within tolerance");
    value = DsssErrorRateModel::GetDsssDqpskCck5_5SuccessRate(FromRss(-91.0), size);
    NS_TEST_ASSERT_MSG_EQ_TOL(value, 0.132, 0.005, "Not equal within tolerance");
    value = DsssErrorRateModel::GetDsssDqpskCck5_5SuccessRate(FromRss(-90.0), size);
    NS_TEST_ASSERT_MSG_EQ_TOL(value, 0.744, 0.005, "Not equal within tolerance");
    value = DsssErrorRateModel::GetDsssDqpskCck5_5SuccessRate(FromRss(-89.0), size);
    NS_TEST_ASSERT_MSG_EQ_TOL(value, 0.974, 0.005, "Not equal within tolerance");
    value = DsssErrorRateModel::GetDsssDqpskCck5_5SuccessRate(FromRss(-88.0), size);
    NS_TEST_ASSERT_MSG_EQ_TOL(value, 0.999, 0.001, "Not equal within tolerance");
    value = DsssErrorRateModel::GetDsssDqpskCck5_5SuccessRate(FromRss(-87.0), size);
    NS_TEST_ASSERT_MSG_EQ_TOL(value, 1, 0.001, "Not equal within tolerance");

    // DQPSK_CCK11
    value = DsssErrorRateModel::GetDsssDqpskCck11SuccessRate(FromRss(-91.0), size);
    NS_TEST_ASSERT_MSG_EQ_TOL(value, 0, 1e-14, "Not equal within tolerance");
    value = DsssErrorRateModel::GetDsssDqpskCck11SuccessRate(FromRss(-90.0), size);
    NS_TEST_ASSERT_MSG_EQ_TOL(value, 4.7e-14, 1e-14, "Not equal within tolerance");
    value = DsssErrorRateModel::GetDsssDqpskCck11SuccessRate(FromRss(-89.0), size);
    NS_TEST_ASSERT_MSG_EQ_TOL(value, 8.85e-5, 1e-5, "Not equal within tolerance");
    value = DsssErrorRateModel::GetDsssDqpskCck11SuccessRate(FromRss(-88.0), size);
    NS_TEST_ASSERT_MSG_EQ_TOL(value, 0.128, 0.005, "Not equal within tolerance");
    value = DsssErrorRateModel::GetDsssDqpskCck11SuccessRate(FromRss(-87.0), size);
    NS_TEST_ASSERT_MSG_EQ_TOL(value, 0.739, 0.005, "Not equal within tolerance");
    value = DsssErrorRateModel::GetDsssDqpskCck11SuccessRate(FromRss(-86.0), size);
    NS_TEST_ASSERT_MSG_EQ_TOL(value, 0.973, 0.005, "Not equal within tolerance");
    value = DsssErrorRateModel::GetDsssDqpskCck11SuccessRate(FromRss(-85.0), size);
    NS_TEST_ASSERT_MSG_EQ_TOL(value, 0.999, 0.001, "Not equal within tolerance");
    value = DsssErrorRateModel::GetDsssDqpskCck11SuccessRate(FromRss(-84.0), size);
    NS_TEST_ASSERT_MSG_EQ_TOL(value, 1, 0.001, "Not equal within tolerance");
#endif
}

/**
 * @ingroup wifi-test
 * @ingroup tests
 *
 * @brief Wifi Error Rate Models Test Case Nist
 */
class WifiErrorRateModelsTestCaseNist : public TestCase
{
  public:
    WifiErrorRateModelsTestCaseNist();
    ~WifiErrorRateModelsTestCaseNist() override;

  private:
    void DoRun() override;
};

WifiErrorRateModelsTestCaseNist::WifiErrorRateModelsTestCaseNist()
    : TestCase("WifiErrorRateModel test case NIST")
{
}

WifiErrorRateModelsTestCaseNist::~WifiErrorRateModelsTestCaseNist()
{
}

void
WifiErrorRateModelsTestCaseNist::DoRun()
{
    uint32_t frameSize = 2000;
    WifiTxVector txVector;
    Ptr<NistErrorRateModel> nist = CreateObject<NistErrorRateModel>();

    // Spot test some values returned from NistErrorRateModel
    // values can be generated by the example program ofdm-validation.cc
    dB_u snr{2.5};
    auto ps = nist->GetChunkSuccessRate(WifiMode("OfdmRate6Mbps"),
                                        txVector,
                                        std::pow(10.0, snr / 10.0),
                                        frameSize * 8);
    NS_TEST_ASSERT_MSG_EQ_TOL(ps, 2.04e-10, 1e-10, "Not equal within tolerance");
    snr = dB_u{3.0};
    ps = nist->GetChunkSuccessRate(WifiMode("OfdmRate6Mbps"),
                                   txVector,
                                   std::pow(10.0, snr / 10.0),
                                   frameSize * 8);
    NS_TEST_ASSERT_MSG_EQ_TOL(ps, 0.020, 0.001, "Not equal within tolerance");
    snr = dB_u{4.0};
    ps = nist->GetChunkSuccessRate(WifiMode("OfdmRate6Mbps"),
                                   txVector,
                                   std::pow(10.0, snr / 10.0),
                                   frameSize * 8);
    NS_TEST_ASSERT_MSG_EQ_TOL(ps, 0.885, 0.001, "Not equal within tolerance");
    snr = dB_u{5.0};
    ps = nist->GetChunkSuccessRate(WifiMode("OfdmRate6Mbps"),
                                   txVector,
                                   std::pow(10.0, snr / 10.0),
                                   frameSize * 8);
    NS_TEST_ASSERT_MSG_EQ_TOL(ps, 0.997, 0.001, "Not equal within tolerance");

    snr = dB_u{6.0};
    ps = nist->GetChunkSuccessRate(WifiMode("OfdmRate9Mbps"),
                                   txVector,
                                   std::pow(10.0, snr / 10.0),
                                   frameSize * 8);
    NS_TEST_ASSERT_MSG_EQ_TOL(ps, 0.097, 0.001, "Not equal within tolerance");
    snr = dB_u{7.0};
    ps = nist->GetChunkSuccessRate(WifiMode("OfdmRate9Mbps"),
                                   txVector,
                                   std::pow(10.0, snr / 10.0),
                                   frameSize * 8);
    NS_TEST_ASSERT_MSG_EQ_TOL(ps, 0.918, 0.001, "Not equal within tolerance");
    snr = dB_u{8.0};
    ps = nist->GetChunkSuccessRate(WifiMode("OfdmRate9Mbps"),
                                   txVector,
                                   std::pow(10.0, snr / 10.0),
                                   frameSize * 8);
    NS_TEST_ASSERT_MSG_EQ_TOL(ps, 0.998, 0.001, "Not equal within tolerance");
    snr = dB_u{9.0};
    ps = nist->GetChunkSuccessRate(WifiMode("OfdmRate9Mbps"),
                                   txVector,
                                   std::pow(10.0, snr / 10.0),
                                   frameSize * 8);
    NS_TEST_ASSERT_MSG_EQ_TOL(ps, 0.999, 0.001, "Not equal within tolerance");

    snr = dB_u{6.0};
    ps = nist->GetChunkSuccessRate(WifiMode("OfdmRate12Mbps"),
                                   txVector,
                                   std::pow(10.0, snr / 10.0),
                                   frameSize * 8);
    NS_TEST_ASSERT_MSG_EQ_TOL(ps, 0.0174, 0.001, "Not equal within tolerance");
    snr = dB_u{7.0};
    ps = nist->GetChunkSuccessRate(WifiMode("OfdmRate12Mbps"),
                                   txVector,
                                   std::pow(10.0, snr / 10.0),
                                   frameSize * 8);
    NS_TEST_ASSERT_MSG_EQ_TOL(ps, 0.881, 0.001, "Not equal within tolerance");
    snr = dB_u{8.0};
    ps = nist->GetChunkSuccessRate(WifiMode("OfdmRate12Mbps"),
                                   txVector,
                                   std::pow(10.0, snr / 10.0),
                                   frameSize * 8);
    NS_TEST_ASSERT_MSG_EQ_TOL(ps, 0.997, 0.001, "Not equal within tolerance");
    snr = dB_u{9.0};
    ps = nist->GetChunkSuccessRate(WifiMode("OfdmRate12Mbps"),
                                   txVector,
                                   std::pow(10.0, snr / 10.0),
                                   frameSize * 8);
    NS_TEST_ASSERT_MSG_EQ_TOL(ps, 0.999, 0.001, "Not equal within tolerance");

    snr = dB_u{8.5};
    ps = nist->GetChunkSuccessRate(WifiMode("OfdmRate18Mbps"),
                                   txVector,
                                   std::pow(10.0, snr / 10.0),
                                   frameSize * 8);
    NS_TEST_ASSERT_MSG_EQ_TOL(ps, 2.85e-6, 1e-6, "Not equal within tolerance");
    snr = dB_u{9.5};
    ps = nist->GetChunkSuccessRate(WifiMode("OfdmRate18Mbps"),
                                   txVector,
                                   std::pow(10.0, snr / 10.0),
                                   frameSize * 8);
    NS_TEST_ASSERT_MSG_EQ_TOL(ps, 0.623, 0.001, "Not equal within tolerance");
    snr = dB_u{10.5};
    ps = nist->GetChunkSuccessRate(WifiMode("OfdmRate18Mbps"),
                                   txVector,
                                   std::pow(10.0, snr / 10.0),
                                   frameSize * 8);
    NS_TEST_ASSERT_MSG_EQ_TOL(ps, 0.985, 0.001, "Not equal within tolerance");
    snr = dB_u{11.5};
    ps = nist->GetChunkSuccessRate(WifiMode("OfdmRate18Mbps"),
                                   txVector,
                                   std::pow(10.0, snr / 10.0),
                                   frameSize * 8);
    NS_TEST_ASSERT_MSG_EQ_TOL(ps, 0.999, 0.001, "Not equal within tolerance");

    snr = dB_u{12.0};
    ps = nist->GetChunkSuccessRate(WifiMode("OfdmRate24Mbps"),
                                   txVector,
                                   std::pow(10.0, snr / 10.0),
                                   frameSize * 8);
    NS_TEST_ASSERT_MSG_EQ_TOL(ps, 2.22e-7, 1e-7, "Not equal within tolerance");
    snr = dB_u{13.0};
    ps = nist->GetChunkSuccessRate(WifiMode("OfdmRate24Mbps"),
                                   txVector,
                                   std::pow(10.0, snr / 10.0),
                                   frameSize * 8);
    NS_TEST_ASSERT_MSG_EQ_TOL(ps, 0.495, 0.001, "Not equal within tolerance");
    snr = dB_u{14.0};
    ps = nist->GetChunkSuccessRate(WifiMode("OfdmRate24Mbps"),
                                   txVector,
                                   std::pow(10.0, snr / 10.0),
                                   frameSize * 8);
    NS_TEST_ASSERT_MSG_EQ_TOL(ps, 0.974, 0.001, "Not equal within tolerance");
    snr = dB_u{15.0};
    ps = nist->GetChunkSuccessRate(WifiMode("OfdmRate24Mbps"),
                                   txVector,
                                   std::pow(10.0, snr / 10.0),
                                   frameSize * 8);
    NS_TEST_ASSERT_MSG_EQ_TOL(ps, 0.999, 0.001, "Not equal within tolerance");

    snr = dB_u{15.5};
    ps = nist->GetChunkSuccessRate(WifiMode("OfdmRate36Mbps"),
                                   txVector,
                                   std::pow(10.0, snr / 10.0),
                                   frameSize * 8);
    NS_TEST_ASSERT_MSG_EQ_TOL(ps, 0.012, 0.001, "Not equal within tolerance");
    snr = dB_u{16.5};
    ps = nist->GetChunkSuccessRate(WifiMode("OfdmRate36Mbps"),
                                   txVector,
                                   std::pow(10.0, snr / 10.0),
                                   frameSize * 8);
    NS_TEST_ASSERT_MSG_EQ_TOL(ps, 0.818, 0.001, "Not equal within tolerance");
    snr = dB_u{17.5};
    ps = nist->GetChunkSuccessRate(WifiMode("OfdmRate36Mbps"),
                                   txVector,
                                   std::pow(10.0, snr / 10.0),
                                   frameSize * 8);
    NS_TEST_ASSERT_MSG_EQ_TOL(ps, 0.993, 0.001, "Not equal within tolerance");
    snr = dB_u{18.5};
    ps = nist->GetChunkSuccessRate(WifiMode("OfdmRate36Mbps"),
                                   txVector,
                                   std::pow(10.0, snr / 10.0),
                                   frameSize * 8);
    NS_TEST_ASSERT_MSG_EQ_TOL(ps, 0.999, 0.001, "Not equal within tolerance");

    snr = dB_u{20.0};
    ps = nist->GetChunkSuccessRate(WifiMode("OfdmRate48Mbps"),
                                   txVector,
                                   std::pow(10.0, snr / 10.0),
                                   frameSize * 8);
    NS_TEST_ASSERT_MSG_EQ_TOL(ps, 1.3e-4, 1e-4, "Not equal within tolerance");
    snr = dB_u{21.0};
    ps = nist->GetChunkSuccessRate(WifiMode("OfdmRate48Mbps"),
                                   txVector,
                                   std::pow(10.0, snr / 10.0),
                                   frameSize * 8);
    NS_TEST_ASSERT_MSG_EQ_TOL(ps, 0.649, 0.001, "Not equal within tolerance");
    snr = dB_u{22.0};
    ps = nist->GetChunkSuccessRate(WifiMode("OfdmRate48Mbps"),
                                   txVector,
                                   std::pow(10.0, snr / 10.0),
                                   frameSize * 8);
    NS_TEST_ASSERT_MSG_EQ_TOL(ps, 0.983, 0.001, "Not equal within tolerance");
    snr = dB_u{23.0};
    ps = nist->GetChunkSuccessRate(WifiMode("OfdmRate48Mbps"),
                                   txVector,
                                   std::pow(10.0, snr / 10.0),
                                   frameSize * 8);
    NS_TEST_ASSERT_MSG_EQ_TOL(ps, 0.999, 0.001, "Not equal within tolerance");

    snr = dB_u{21.0};
    ps = nist->GetChunkSuccessRate(WifiMode("OfdmRate54Mbps"),
                                   txVector,
                                   std::pow(10.0, snr / 10.0),
                                   frameSize * 8);
    NS_TEST_ASSERT_MSG_EQ_TOL(ps, 5.44e-8, 1e-8, "Not equal within tolerance");
    snr = dB_u{22.0};
    ps = nist->GetChunkSuccessRate(WifiMode("OfdmRate54Mbps"),
                                   txVector,
                                   std::pow(10.0, snr / 10.0),
                                   frameSize * 8);
    NS_TEST_ASSERT_MSG_EQ_TOL(ps, 0.410, 0.001, "Not equal within tolerance");
    snr = dB_u{23.0};
    ps = nist->GetChunkSuccessRate(WifiMode("OfdmRate54Mbps"),
                                   txVector,
                                   std::pow(10.0, snr / 10.0),
                                   frameSize * 8);
    NS_TEST_ASSERT_MSG_EQ_TOL(ps, 0.958, 0.001, "Not equal within tolerance");
    snr = dB_u{24.0};
    ps = nist->GetChunkSuccessRate(WifiMode("OfdmRate54Mbps"),
                                   txVector,
                                   std::pow(10.0, snr / 10.0),
                                   frameSize * 8);
    NS_TEST_ASSERT_MSG_EQ_TOL(ps, 0.999, 0.001, "Not equal within tolerance");
}

class TestInterferenceHelper : public InterferenceHelper
{
  public:
    using InterferenceHelper::CalculatePayloadChunkSuccessRate;
    using InterferenceHelper::CalculateSnr;
};

/**
 * @ingroup wifi-test
 * @ingroup tests
 *
 * @brief Wifi Error Rate Models Test Case MIMO
 */
class WifiErrorRateModelsTestCaseMimo : public TestCase
{
  public:
    WifiErrorRateModelsTestCaseMimo();
    ~WifiErrorRateModelsTestCaseMimo() override;

  private:
    void DoRun() override;
};

WifiErrorRateModelsTestCaseMimo::WifiErrorRateModelsTestCaseMimo()
    : TestCase("WifiErrorRateModel test case MIMO")
{
}

WifiErrorRateModelsTestCaseMimo::~WifiErrorRateModelsTestCaseMimo()
{
}

void
WifiErrorRateModelsTestCaseMimo::DoRun()
{
    TestInterferenceHelper interference;
    interference.SetNoiseFigure(0);
    WifiMode mode = HtPhy::GetHtMcs0();
    WifiTxVector txVector;

    txVector.SetMode(mode);
    txVector.SetTxPowerLevel(WIFI_MIN_TX_PWR_LEVEL);
    txVector.SetChannelWidth(MHz_u{20});
    txVector.SetNss(1);
    txVector.SetNTx(1);

    interference.SetNumberOfReceiveAntennas(1);
    Ptr<NistErrorRateModel> nist = CreateObject<NistErrorRateModel>();
    interference.SetErrorRateModel(nist);

    // SISO: initial SNR set to 4dB
    dB_u initialSnr{4.0};
    dB_u tol{0.1};
    auto snr = interference.CalculateSnr(Watt_u{0.001},
                                         Watt_u{0.001} / DbToRatio(initialSnr),
                                         txVector.GetChannelWidth(),
                                         txVector.GetNss());
    NS_TEST_ASSERT_MSG_EQ_TOL(RatioToDb(snr),
                              initialSnr,
                              tol,
                              "Attempt to set initial SNR to known value failed");
    Time duration = MilliSeconds(2);
    auto chunkSuccess = interference.CalculatePayloadChunkSuccessRate(snr, duration, txVector);
    NS_TEST_ASSERT_MSG_EQ_TOL(chunkSuccess,
                              0.905685,
                              0.000001,
                              "CSR not within tolerance for SISO");
    auto sisoChunkSuccess = chunkSuccess;

    // MIMO 2x1:2: expect no SNR gain in AWGN channel
    txVector.SetNss(2);
    txVector.SetNTx(2);
    snr = interference.CalculateSnr(Watt_u{0.001},
                                    Watt_u{0.001} / DbToRatio(initialSnr),
                                    txVector.GetChannelWidth(),
                                    txVector.GetNss());
    NS_TEST_ASSERT_MSG_EQ_TOL(RatioToDb(snr),
                              initialSnr,
                              tol,
                              "SNR not within tolerance for 2x1:2 MIMO");
    chunkSuccess = interference.CalculatePayloadChunkSuccessRate(snr, duration, txVector);
    NS_TEST_ASSERT_MSG_EQ_TOL(chunkSuccess,
                              0.905685,
                              0.000001,
                              "CSR not within tolerance for SISO");

    // MIMO 1x2:1: expect that SNR is increased by a factor of 3 dB (10 log 2/1) compared to SISO
    // thanks to RX diversity
    txVector.SetNss(1);
    txVector.SetNTx(1);
    interference.SetNumberOfReceiveAntennas(2);
    snr = interference.CalculateSnr(Watt_u{0.001},
                                    Watt_u{0.001} / DbToRatio(initialSnr),
                                    txVector.GetChannelWidth(),
                                    txVector.GetNss());
    NS_TEST_ASSERT_MSG_EQ_TOL(RatioToDb(snr),
                              initialSnr + dB_u{3.0},
                              tol,
                              "SNR not within tolerance for 1x2:1 MIMO");
    chunkSuccess = interference.CalculatePayloadChunkSuccessRate(snr, duration, txVector);
    NS_TEST_ASSERT_MSG_GT(chunkSuccess,
                          sisoChunkSuccess,
                          "CSR not within tolerance for 1x2:1 MIMO");

    // MIMO 2x2:1: expect that SNR is increased by a factor of 3 dB (10 log 2/1) compared to SISO
    // thanks to RX diversity
    txVector.SetNss(1);
    txVector.SetNTx(2);
    interference.SetNumberOfReceiveAntennas(2);
    snr = interference.CalculateSnr(Watt_u{0.001},
                                    Watt_u{0.001} / DbToRatio(initialSnr),
                                    txVector.GetChannelWidth(),
                                    txVector.GetNss());
    NS_TEST_ASSERT_MSG_EQ_TOL(RatioToDb(snr),
                              initialSnr + dB_u{3.0},
                              tol,
                              "SNR not equal within tolerance for 2x2:1 MIMO");
    chunkSuccess = interference.CalculatePayloadChunkSuccessRate(snr, duration, txVector);
    NS_TEST_ASSERT_MSG_GT(chunkSuccess,
                          sisoChunkSuccess,
                          "CSR not within tolerance for 2x2:1 MIMO");

    // MIMO 2x2:2: expect no SNR gain in AWGN channel
    txVector.SetNss(2);
    txVector.SetNTx(2);
    interference.SetNumberOfReceiveAntennas(2);
    snr = interference.CalculateSnr(Watt_u{0.001},
                                    Watt_u{0.001} / DbToRatio(initialSnr),
                                    txVector.GetChannelWidth(),
                                    txVector.GetNss());
    NS_TEST_ASSERT_MSG_EQ_TOL(RatioToDb(snr),
                              initialSnr,
                              tol,
                              "SNR not equal within tolerance for 2x2:2 MIMO");
    chunkSuccess = interference.CalculatePayloadChunkSuccessRate(snr, duration, txVector);
    NS_TEST_ASSERT_MSG_EQ_TOL(chunkSuccess,
                              sisoChunkSuccess,
                              0.000001,
                              "CSR not within tolerance for 2x2:2 MIMO");

    // MIMO 3x3:1: expect that SNR is increased by a factor of 4.8 dB (10 log 3/1) compared to SISO
    // thanks to RX diversity
    txVector.SetNss(1);
    txVector.SetNTx(3);
    interference.SetNumberOfReceiveAntennas(3);
    snr = interference.CalculateSnr(Watt_u{0.001},
                                    Watt_u{0.001} / DbToRatio(initialSnr),
                                    txVector.GetChannelWidth(),
                                    txVector.GetNss());
    NS_TEST_ASSERT_MSG_EQ_TOL(RatioToDb(snr),
                              initialSnr + dB_u{4.8},
                              tol,
                              "SNR not within tolerance for 3x3:1 MIMO");
    chunkSuccess = interference.CalculatePayloadChunkSuccessRate(snr, duration, txVector);
    NS_TEST_ASSERT_MSG_GT(chunkSuccess,
                          sisoChunkSuccess,
                          "CSR not within tolerance for 3x3:1 MIMO");

    // MIMO 3x3:2: expect that SNR is increased by a factor of 1.8 dB (10 log 3/2) compared to SISO
    // thanks to RX diversity
    txVector.SetNss(2);
    txVector.SetNTx(3);
    interference.SetNumberOfReceiveAntennas(3);
    snr = interference.CalculateSnr(Watt_u{0.001},
                                    Watt_u{0.001} / DbToRatio(initialSnr),
                                    txVector.GetChannelWidth(),
                                    txVector.GetNss());
    NS_TEST_ASSERT_MSG_EQ_TOL(RatioToDb(snr),
                              initialSnr + dB_u{1.8},
                              tol,
                              "SNR not within tolerance for 3x3:2 MIMO");
    chunkSuccess = interference.CalculatePayloadChunkSuccessRate(snr, duration, txVector);
    NS_TEST_ASSERT_MSG_GT(chunkSuccess,
                          sisoChunkSuccess,
                          "CSR not within tolerance for 3x3:2 MIMO");

    // MIMO 3x3:3: expect no SNR gain in AWGN channel
    txVector.SetNss(3);
    txVector.SetNTx(3);
    interference.SetNumberOfReceiveAntennas(3);
    snr = interference.CalculateSnr(Watt_u{0.001},
                                    Watt_u{0.001} / DbToRatio(initialSnr),
                                    txVector.GetChannelWidth(),
                                    txVector.GetNss());
    NS_TEST_ASSERT_MSG_EQ_TOL(RatioToDb(snr),
                              initialSnr,
                              tol,
                              "SNR not within tolerance for 3x3:3 MIMO");
    chunkSuccess = interference.CalculatePayloadChunkSuccessRate(snr, duration, txVector);
    NS_TEST_ASSERT_MSG_EQ_TOL(chunkSuccess,
                              sisoChunkSuccess,
                              0.000001,
                              "CSR not equal within tolerance for 3x3:3 MIMO");

    // MIMO 4x4:1: expect that SNR is increased by a factor of 6 dB (10 log 4/1) compared to SISO
    // thanks to RX diversity
    txVector.SetNss(1);
    txVector.SetNTx(4);
    interference.SetNumberOfReceiveAntennas(4);
    snr = interference.CalculateSnr(Watt_u{0.001},
                                    Watt_u{0.001} / DbToRatio(initialSnr),
                                    txVector.GetChannelWidth(),
                                    txVector.GetNss());
    NS_TEST_ASSERT_MSG_EQ_TOL(RatioToDb(snr),
                              initialSnr + dB_u{6.0},
                              tol,
                              "SNR not within tolerance for 4x4:1 MIMO");
    chunkSuccess = interference.CalculatePayloadChunkSuccessRate(snr, duration, txVector);
    NS_TEST_ASSERT_MSG_GT(chunkSuccess,
                          sisoChunkSuccess,
                          "CSR not within tolerance for 4x4:1 MIMO");

    // MIMO 4x4:2: expect that SNR is increased by a factor of 3 dB (10 log 4/2) compared to SISO
    // thanks to RX diversity
    txVector.SetNss(2);
    txVector.SetNTx(4);
    interference.SetNumberOfReceiveAntennas(4);
    snr = interference.CalculateSnr(Watt_u{0.001},
                                    Watt_u{0.001} / DbToRatio(initialSnr),
                                    txVector.GetChannelWidth(),
                                    txVector.GetNss());
    NS_TEST_ASSERT_MSG_EQ_TOL(RatioToDb(snr),
                              initialSnr + dB_u{3.0},
                              tol,
                              "SNR not within tolerance for 4x4:2 MIMO");
    chunkSuccess = interference.CalculatePayloadChunkSuccessRate(snr, duration, txVector);
    NS_TEST_ASSERT_MSG_GT(chunkSuccess,
                          sisoChunkSuccess,
                          "CSR not within tolerance for 4x4:2 MIMO");

    // MIMO 4x4:3: expect that SNR is increased by a factor of 1.2 dB (10 log 4/3) compared to SISO
    // thanks to RX diversity
    txVector.SetNss(3);
    txVector.SetNTx(4);
    interference.SetNumberOfReceiveAntennas(4);
    snr = interference.CalculateSnr(Watt_u{0.001},
                                    Watt_u{0.001} / DbToRatio(initialSnr),
                                    txVector.GetChannelWidth(),
                                    txVector.GetNss());
    NS_TEST_ASSERT_MSG_EQ_TOL(RatioToDb(snr),
                              initialSnr + dB_u{1.2},
                              tol,
                              "SNR not within tolerance for 4x4:3 MIMO");
    chunkSuccess = interference.CalculatePayloadChunkSuccessRate(snr, duration, txVector);
    NS_TEST_ASSERT_MSG_GT(chunkSuccess,
                          sisoChunkSuccess,
                          "CSR not within tolerance for 4x4:1 MIMO");

    // MIMO 4x4:4: expect no SNR gain in AWGN channel
    txVector.SetNss(4);
    txVector.SetNTx(4);
    interference.SetNumberOfReceiveAntennas(4);
    snr = interference.CalculateSnr(Watt_u{0.001},
                                    Watt_u{0.001} / DbToRatio(initialSnr),
                                    txVector.GetChannelWidth(),
                                    txVector.GetNss());
    NS_TEST_ASSERT_MSG_EQ_TOL(RatioToDb(snr),
                              initialSnr,
                              tol,
                              "SNR not within tolerance for 4x4:4 MIMO");
    chunkSuccess = interference.CalculatePayloadChunkSuccessRate(snr, duration, txVector);
    NS_TEST_ASSERT_MSG_EQ_TOL(chunkSuccess,
                              sisoChunkSuccess,
                              0.000001,
                              "CSR not within tolerance for 4x4:4 MIMO");
}

/**
 * map of PER values that have been manually computed for a given MCS, size (in bytes) and SNR (in
 * dB) in order to verify against the PER calculated by the model
 */
const std::map<std::pair<uint8_t /* mcs */, uint32_t /* size */>,
               std::map<dB_u /* snr */, double /* per */>>
    expectedTableValues = {
        /* MCS 0 - 1458 bytes */
        {std::make_pair(0, 1458),
         {
             {dB_u{-4.00}, 1.00000}, {dB_u{-3.75}, 1.00000}, {dB_u{-3.50}, 1.00000},
             {dB_u{-3.25}, 1.00000}, {dB_u{-3.00}, 1.00000}, {dB_u{-2.75}, 1.00000},
             {dB_u{-2.50}, 1.00000}, {dB_u{-2.25}, 1.00000}, {dB_u{-2.00}, 1.00000},
             {dB_u{-1.75}, 1.00000}, {dB_u{-1.50}, 0.99904}, {dB_u{-1.25}, 0.99604},
             {dB_u{-1.00}, 0.96592}, {dB_u{-0.75}, 0.87817}, {dB_u{-0.50}, 0.73407},
             {dB_u{-0.25}, 0.47022}, {dB_u{0.00}, 0.25488},  {dB_u{0.25}, 0.14263},
             {dB_u{0.50}, 0.05748},  {dB_u{0.75}, 0.02993},  {dB_u{1.00}, 0.00965},
             {dB_u{1.25}, 0.00480},  {dB_u{1.50}, 0.00128},  {dB_u{1.75}, 0.00061},
             {dB_u{2.00}, 0.00013},  {dB_u{2.25}, 0.00006},  {dB_u{2.50}, 0.00000},
             {dB_u{2.75}, 0.00000},  {dB_u{3.00}, 0.00000},  {dB_u{3.25}, 0.00000},
             {dB_u{3.50}, 0.00000},  {dB_u{3.75}, 0.00000},  {dB_u{4.00}, 0.00000},
             {dB_u{4.25}, 0.00000},  {dB_u{4.50}, 0.00000},  {dB_u{4.75}, 0.00000},
             {dB_u{5.00}, 0.00000},  {dB_u{5.25}, 0.00000},  {dB_u{5.50}, 0.00000},
             {dB_u{5.75}, 0.00000},  {dB_u{6.00}, 0.00000},  {dB_u{6.25}, 0.00000},
             {dB_u{6.50}, 0.00000},  {dB_u{6.75}, 0.00000},  {dB_u{7.00}, 0.00000},
             {dB_u{7.25}, 0.00000},  {dB_u{7.50}, 0.00000},  {dB_u{7.75}, 0.00000},
             {dB_u{8.00}, 0.00000},  {dB_u{8.25}, 0.00000},  {dB_u{8.50}, 0.00000},
             {dB_u{8.75}, 0.00000},  {dB_u{9.00}, 0.00000},  {dB_u{9.25}, 0.00000},
             {dB_u{9.50}, 0.00000},  {dB_u{9.75}, 0.00000},  {dB_u{10.00}, 0.00000},
             {dB_u{10.25}, 0.00000}, {dB_u{10.50}, 0.00000}, {dB_u{10.75}, 0.00000},
             {dB_u{11.00}, 0.00000}, {dB_u{11.25}, 0.00000}, {dB_u{11.50}, 0.00000},
             {dB_u{11.75}, 0.00000}, {dB_u{12.00}, 0.00000}, {dB_u{12.25}, 0.00000},
             {dB_u{12.50}, 0.00000}, {dB_u{12.75}, 0.00000}, {dB_u{13.00}, 0.00000},
             {dB_u{13.25}, 0.00000}, {dB_u{13.50}, 0.00000}, {dB_u{13.75}, 0.00000},
             {dB_u{14.00}, 0.00000}, {dB_u{14.25}, 0.00000}, {dB_u{14.50}, 0.00000},
             {dB_u{14.75}, 0.00000}, {dB_u{15.00}, 0.00000}, {dB_u{15.25}, 0.00000},
             {dB_u{15.50}, 0.00000}, {dB_u{15.75}, 0.00000}, {dB_u{16.00}, 0.00000},
             {dB_u{16.25}, 0.00000}, {dB_u{16.50}, 0.00000}, {dB_u{16.75}, 0.00000},
             {dB_u{17.00}, 0.00000}, {dB_u{17.25}, 0.00000}, {dB_u{17.50}, 0.00000},
             {dB_u{17.75}, 0.00000}, {dB_u{18.00}, 0.00000}, {dB_u{18.25}, 0.00000},
             {dB_u{18.50}, 0.00000}, {dB_u{18.75}, 0.00000}, {dB_u{19.00}, 0.00000},
             {dB_u{19.25}, 0.00000}, {dB_u{19.50}, 0.00000}, {dB_u{19.75}, 0.00000},
             {dB_u{20.00}, 0.00000}, {dB_u{20.25}, 0.00000}, {dB_u{20.50}, 0.00000},
             {dB_u{20.75}, 0.00000}, {dB_u{21.00}, 0.00000}, {dB_u{21.25}, 0.00000},
             {dB_u{21.50}, 0.00000}, {dB_u{21.75}, 0.00000}, {dB_u{22.00}, 0.00000},
             {dB_u{22.25}, 0.00000}, {dB_u{22.50}, 0.00000}, {dB_u{22.75}, 0.00000},
             {dB_u{23.00}, 0.00000}, {dB_u{23.25}, 0.00000}, {dB_u{23.50}, 0.00000},
             {dB_u{23.75}, 0.00000}, {dB_u{24.00}, 0.00000}, {dB_u{24.25}, 0.00000},
             {dB_u{24.50}, 0.00000}, {dB_u{24.75}, 0.00000}, {dB_u{25.00}, 0.00000},
             {dB_u{25.25}, 0.00000}, {dB_u{25.50}, 0.00000}, {dB_u{25.75}, 0.00000},
             {dB_u{26.00}, 0.00000}, {dB_u{26.25}, 0.00000}, {dB_u{26.50}, 0.00000},
             {dB_u{26.75}, 0.00000}, {dB_u{27.00}, 0.00000}, {dB_u{27.25}, 0.00000},
             {dB_u{27.50}, 0.00000}, {dB_u{27.75}, 0.00000}, {dB_u{28.00}, 0.00000},
             {dB_u{28.25}, 0.00000}, {dB_u{28.50}, 0.00000}, {dB_u{28.75}, 0.00000},
             {dB_u{29.00}, 0.00000}, {dB_u{29.25}, 0.00000}, {dB_u{29.50}, 0.00000},
             {dB_u{29.75}, 0.00000}, {dB_u{30.00}, 0.00000},
         }},
        /* MCS 0 - 32 bytes */
        {std::make_pair(0, 32),
         {
             {dB_u{-4.00}, 0.99920}, {dB_u{-3.75}, 0.99670}, {dB_u{-3.50}, 0.98633},
             {dB_u{-3.25}, 0.95923}, {dB_u{-3.00}, 0.92242}, {dB_u{-2.75}, 0.86497},
             {dB_u{-2.50}, 0.78808}, {dB_u{-2.25}, 0.66988}, {dB_u{-2.00}, 0.54451},
             {dB_u{-1.75}, 0.40391}, {dB_u{-1.50}, 0.27904}, {dB_u{-1.25}, 0.18759},
             {dB_u{-1.00}, 0.11084}, {dB_u{-0.75}, 0.06534}, {dB_u{-0.50}, 0.03026},
             {dB_u{-0.25}, 0.01731}, {dB_u{0.00}, 0.00738},  {dB_u{0.25}, 0.00388},
             {dB_u{0.50}, 0.00130},  {dB_u{0.75}, 0.00067},  {dB_u{1.00}, 0.00022},
             {dB_u{1.25}, 0.00014},  {dB_u{1.50}, 0.00008},  {dB_u{1.75}, 0.00003},
             {dB_u{2.00}, 0.00000},  {dB_u{2.25}, 0.00000},  {dB_u{2.50}, 0.00000},
             {dB_u{2.75}, 0.00000},  {dB_u{3.00}, 0.00000},  {dB_u{3.25}, 0.00000},
             {dB_u{3.50}, 0.00000},  {dB_u{3.75}, 0.00000},  {dB_u{4.00}, 0.00000},
             {dB_u{4.25}, 0.00000},  {dB_u{4.50}, 0.00000},  {dB_u{4.75}, 0.00000},
             {dB_u{5.00}, 0.00000},  {dB_u{5.25}, 0.00000},  {dB_u{5.50}, 0.00000},
             {dB_u{5.75}, 0.00000},  {dB_u{6.00}, 0.00000},  {dB_u{6.25}, 0.00000},
             {dB_u{6.50}, 0.00000},  {dB_u{6.75}, 0.00000},  {dB_u{7.00}, 0.00000},
             {dB_u{7.25}, 0.00000},  {dB_u{7.50}, 0.00000},  {dB_u{7.75}, 0.00000},
             {dB_u{8.00}, 0.00000},  {dB_u{8.25}, 0.00000},  {dB_u{8.50}, 0.00000},
             {dB_u{8.75}, 0.00000},  {dB_u{9.00}, 0.00000},  {dB_u{9.25}, 0.00000},
             {dB_u{9.50}, 0.00000},  {dB_u{9.75}, 0.00000},  {dB_u{10.00}, 0.00000},
             {dB_u{10.25}, 0.00000}, {dB_u{10.50}, 0.00000}, {dB_u{10.75}, 0.00000},
             {dB_u{11.00}, 0.00000}, {dB_u{11.25}, 0.00000}, {dB_u{11.50}, 0.00000},
             {dB_u{11.75}, 0.00000}, {dB_u{12.00}, 0.00000}, {dB_u{12.25}, 0.00000},
             {dB_u{12.50}, 0.00000}, {dB_u{12.75}, 0.00000}, {dB_u{13.00}, 0.00000},
             {dB_u{13.25}, 0.00000}, {dB_u{13.50}, 0.00000}, {dB_u{13.75}, 0.00000},
             {dB_u{14.00}, 0.00000}, {dB_u{14.25}, 0.00000}, {dB_u{14.50}, 0.00000},
             {dB_u{14.75}, 0.00000}, {dB_u{15.00}, 0.00000}, {dB_u{15.25}, 0.00000},
             {dB_u{15.50}, 0.00000}, {dB_u{15.75}, 0.00000}, {dB_u{16.00}, 0.00000},
             {dB_u{16.25}, 0.00000}, {dB_u{16.50}, 0.00000}, {dB_u{16.75}, 0.00000},
             {dB_u{17.00}, 0.00000}, {dB_u{17.25}, 0.00000}, {dB_u{17.50}, 0.00000},
             {dB_u{17.75}, 0.00000}, {dB_u{18.00}, 0.00000}, {dB_u{18.25}, 0.00000},
             {dB_u{18.50}, 0.00000}, {dB_u{18.75}, 0.00000}, {dB_u{19.00}, 0.00000},
             {dB_u{19.25}, 0.00000}, {dB_u{19.50}, 0.00000}, {dB_u{19.75}, 0.00000},
             {dB_u{20.00}, 0.00000}, {dB_u{20.25}, 0.00000}, {dB_u{20.50}, 0.00000},
             {dB_u{20.75}, 0.00000}, {dB_u{21.00}, 0.00000}, {dB_u{21.25}, 0.00000},
             {dB_u{21.50}, 0.00000}, {dB_u{21.75}, 0.00000}, {dB_u{22.00}, 0.00000},
             {dB_u{22.25}, 0.00000}, {dB_u{22.50}, 0.00000}, {dB_u{22.75}, 0.00000},
             {dB_u{23.00}, 0.00000}, {dB_u{23.25}, 0.00000}, {dB_u{23.50}, 0.00000},
             {dB_u{23.75}, 0.00000}, {dB_u{24.00}, 0.00000}, {dB_u{24.25}, 0.00000},
             {dB_u{24.50}, 0.00000}, {dB_u{24.75}, 0.00000}, {dB_u{25.00}, 0.00000},
             {dB_u{25.25}, 0.00000}, {dB_u{25.50}, 0.00000}, {dB_u{25.75}, 0.00000},
             {dB_u{26.00}, 0.00000}, {dB_u{26.25}, 0.00000}, {dB_u{26.50}, 0.00000},
             {dB_u{26.75}, 0.00000}, {dB_u{27.00}, 0.00000}, {dB_u{27.25}, 0.00000},
             {dB_u{27.50}, 0.00000}, {dB_u{27.75}, 0.00000}, {dB_u{28.00}, 0.00000},
             {dB_u{28.25}, 0.00000}, {dB_u{28.50}, 0.00000}, {dB_u{28.75}, 0.00000},
             {dB_u{29.00}, 0.00000}, {dB_u{29.25}, 0.00000}, {dB_u{29.50}, 0.00000},
             {dB_u{29.75}, 0.00000}, {dB_u{30.00}, 0.00000},
         }},
        /* MCS 0 - 1000 bytes */
        {std::make_pair(0, 1000),
         {
             {dB_u{-4.00}, 1.00000}, {dB_u{-3.75}, 1.00000}, {dB_u{-3.50}, 1.00000},
             {dB_u{-3.25}, 1.00000}, {dB_u{-3.00}, 1.00000}, {dB_u{-2.75}, 1.00000},
             {dB_u{-2.50}, 1.00000}, {dB_u{-2.25}, 1.00000}, {dB_u{-2.00}, 1.00000},
             {dB_u{-1.75}, 1.00000}, {dB_u{-1.50}, 0.99148}, {dB_u{-1.25}, 0.97749},
             {dB_u{-1.00}, 0.90149}, {dB_u{-0.75}, 0.76398}, {dB_u{-0.50}, 0.59685},
             {dB_u{-0.25}, 0.35321}, {dB_u{0.00}, 0.18273},  {dB_u{0.25}, 0.10017},
             {dB_u{0.50}, 0.03979},  {dB_u{0.75}, 0.02063},  {dB_u{1.00}, 0.00663},
             {dB_u{1.25}, 0.00329},  {dB_u{1.50}, 0.00088},  {dB_u{1.75}, 0.00042},
             {dB_u{2.00}, 0.00009},  {dB_u{2.25}, 0.00004},  {dB_u{2.50}, 0.00000},
             {dB_u{2.75}, 0.00000},  {dB_u{3.00}, 0.00000},  {dB_u{3.25}, 0.00000},
             {dB_u{3.50}, 0.00000},  {dB_u{3.75}, 0.00000},  {dB_u{4.00}, 0.00000},
             {dB_u{4.25}, 0.00000},  {dB_u{4.50}, 0.00000},  {dB_u{4.75}, 0.00000},
             {dB_u{5.00}, 0.00000},  {dB_u{5.25}, 0.00000},  {dB_u{5.50}, 0.00000},
             {dB_u{5.75}, 0.00000},  {dB_u{6.00}, 0.00000},  {dB_u{6.25}, 0.00000},
             {dB_u{6.50}, 0.00000},  {dB_u{6.75}, 0.00000},  {dB_u{7.00}, 0.00000},
             {dB_u{7.25}, 0.00000},  {dB_u{7.50}, 0.00000},  {dB_u{7.75}, 0.00000},
             {dB_u{8.00}, 0.00000},  {dB_u{8.25}, 0.00000},  {dB_u{8.50}, 0.00000},
             {dB_u{8.75}, 0.00000},  {dB_u{9.00}, 0.00000},  {dB_u{9.25}, 0.00000},
             {dB_u{9.50}, 0.00000},  {dB_u{9.75}, 0.00000},  {dB_u{10.00}, 0.00000},
             {dB_u{10.25}, 0.00000}, {dB_u{10.50}, 0.00000}, {dB_u{10.75}, 0.00000},
             {dB_u{11.00}, 0.00000}, {dB_u{11.25}, 0.00000}, {dB_u{11.50}, 0.00000},
             {dB_u{11.75}, 0.00000}, {dB_u{12.00}, 0.00000}, {dB_u{12.25}, 0.00000},
             {dB_u{12.50}, 0.00000}, {dB_u{12.75}, 0.00000}, {dB_u{13.00}, 0.00000},
             {dB_u{13.25}, 0.00000}, {dB_u{13.50}, 0.00000}, {dB_u{13.75}, 0.00000},
             {dB_u{14.00}, 0.00000}, {dB_u{14.25}, 0.00000}, {dB_u{14.50}, 0.00000},
             {dB_u{14.75}, 0.00000}, {dB_u{15.00}, 0.00000}, {dB_u{15.25}, 0.00000},
             {dB_u{15.50}, 0.00000}, {dB_u{15.75}, 0.00000}, {dB_u{16.00}, 0.00000},
             {dB_u{16.25}, 0.00000}, {dB_u{16.50}, 0.00000}, {dB_u{16.75}, 0.00000},
             {dB_u{17.00}, 0.00000}, {dB_u{17.25}, 0.00000}, {dB_u{17.50}, 0.00000},
             {dB_u{17.75}, 0.00000}, {dB_u{18.00}, 0.00000}, {dB_u{18.25}, 0.00000},
             {dB_u{18.50}, 0.00000}, {dB_u{18.75}, 0.00000}, {dB_u{19.00}, 0.00000},
             {dB_u{19.25}, 0.00000}, {dB_u{19.50}, 0.00000}, {dB_u{19.75}, 0.00000},
             {dB_u{20.00}, 0.00000}, {dB_u{20.25}, 0.00000}, {dB_u{20.50}, 0.00000},
             {dB_u{20.75}, 0.00000}, {dB_u{21.00}, 0.00000}, {dB_u{21.25}, 0.00000},
             {dB_u{21.50}, 0.00000}, {dB_u{21.75}, 0.00000}, {dB_u{22.00}, 0.00000},
             {dB_u{22.25}, 0.00000}, {dB_u{22.50}, 0.00000}, {dB_u{22.75}, 0.00000},
             {dB_u{23.00}, 0.00000}, {dB_u{23.25}, 0.00000}, {dB_u{23.50}, 0.00000},
             {dB_u{23.75}, 0.00000}, {dB_u{24.00}, 0.00000}, {dB_u{24.25}, 0.00000},
             {dB_u{24.50}, 0.00000}, {dB_u{24.75}, 0.00000}, {dB_u{25.00}, 0.00000},
             {dB_u{25.25}, 0.00000}, {dB_u{25.50}, 0.00000}, {dB_u{25.75}, 0.00000},
             {dB_u{26.00}, 0.00000}, {dB_u{26.25}, 0.00000}, {dB_u{26.50}, 0.00000},
             {dB_u{26.75}, 0.00000}, {dB_u{27.00}, 0.00000}, {dB_u{27.25}, 0.00000},
             {dB_u{27.50}, 0.00000}, {dB_u{27.75}, 0.00000}, {dB_u{28.00}, 0.00000},
             {dB_u{28.25}, 0.00000}, {dB_u{28.50}, 0.00000}, {dB_u{28.75}, 0.00000},
             {dB_u{29.00}, 0.00000}, {dB_u{29.25}, 0.00000}, {dB_u{29.50}, 0.00000},
             {dB_u{29.75}, 0.00000}, {dB_u{30.00}, 0.00000},
         }},
        /* MCS 0 - 1 byte */
        {std::make_pair(0, 1),
         {
             {dB_u{-4.00}, 0.19976}, {dB_u{-3.75}, 0.16352}, {dB_u{-3.50}, 0.12553},
             {dB_u{-3.25}, 0.09516}, {dB_u{-3.00}, 0.07678}, {dB_u{-2.75}, 0.06065},
             {dB_u{-2.50}, 0.04733}, {dB_u{-2.25}, 0.03404}, {dB_u{-2.00}, 0.02427},
             {dB_u{-1.75}, 0.01604}, {dB_u{-1.50}, 0.01017}, {dB_u{-1.25}, 0.00647},
             {dB_u{-1.00}, 0.00366}, {dB_u{-0.75}, 0.00211}, {dB_u{-0.50}, 0.00096},
             {dB_u{-0.25}, 0.00055}, {dB_u{0.00}, 0.00023},  {dB_u{0.25}, 0.00012},
             {dB_u{0.50}, 0.00004},  {dB_u{0.75}, 0.00002},  {dB_u{1.00}, 0.00000},
             {dB_u{1.25}, 0.00000},  {dB_u{1.50}, 0.00000},  {dB_u{1.75}, 0.00000},
             {dB_u{2.00}, 0.00000},  {dB_u{2.25}, 0.00000},  {dB_u{2.50}, 0.00000},
             {dB_u{2.75}, 0.00000},  {dB_u{3.00}, 0.00000},  {dB_u{3.25}, 0.00000},
             {dB_u{3.50}, 0.00000},  {dB_u{3.75}, 0.00000},  {dB_u{4.00}, 0.00000},
             {dB_u{4.25}, 0.00000},  {dB_u{4.50}, 0.00000},  {dB_u{4.75}, 0.00000},
             {dB_u{5.00}, 0.00000},  {dB_u{5.25}, 0.00000},  {dB_u{5.50}, 0.00000},
             {dB_u{5.75}, 0.00000},  {dB_u{6.00}, 0.00000},  {dB_u{6.25}, 0.00000},
             {dB_u{6.50}, 0.00000},  {dB_u{6.75}, 0.00000},  {dB_u{7.00}, 0.00000},
             {dB_u{7.25}, 0.00000},  {dB_u{7.50}, 0.00000},  {dB_u{7.75}, 0.00000},
             {dB_u{8.00}, 0.00000},  {dB_u{8.25}, 0.00000},  {dB_u{8.50}, 0.00000},
             {dB_u{8.75}, 0.00000},  {dB_u{9.00}, 0.00000},  {dB_u{9.25}, 0.00000},
             {dB_u{9.50}, 0.00000},  {dB_u{9.75}, 0.00000},  {dB_u{10.00}, 0.00000},
             {dB_u{10.25}, 0.00000}, {dB_u{10.50}, 0.00000}, {dB_u{10.75}, 0.00000},
             {dB_u{11.00}, 0.00000}, {dB_u{11.25}, 0.00000}, {dB_u{11.50}, 0.00000},
             {dB_u{11.75}, 0.00000}, {dB_u{12.00}, 0.00000}, {dB_u{12.25}, 0.00000},
             {dB_u{12.50}, 0.00000}, {dB_u{12.75}, 0.00000}, {dB_u{13.00}, 0.00000},
             {dB_u{13.25}, 0.00000}, {dB_u{13.50}, 0.00000}, {dB_u{13.75}, 0.00000},
             {dB_u{14.00}, 0.00000}, {dB_u{14.25}, 0.00000}, {dB_u{14.50}, 0.00000},
             {dB_u{14.75}, 0.00000}, {dB_u{15.00}, 0.00000}, {dB_u{15.25}, 0.00000},
             {dB_u{15.50}, 0.00000}, {dB_u{15.75}, 0.00000}, {dB_u{16.00}, 0.00000},
             {dB_u{16.25}, 0.00000}, {dB_u{16.50}, 0.00000}, {dB_u{16.75}, 0.00000},
             {dB_u{17.00}, 0.00000}, {dB_u{17.25}, 0.00000}, {dB_u{17.50}, 0.00000},
             {dB_u{17.75}, 0.00000}, {dB_u{18.00}, 0.00000}, {dB_u{18.25}, 0.00000},
             {dB_u{18.50}, 0.00000}, {dB_u{18.75}, 0.00000}, {dB_u{19.00}, 0.00000},
             {dB_u{19.25}, 0.00000}, {dB_u{19.50}, 0.00000}, {dB_u{19.75}, 0.00000},
             {dB_u{20.00}, 0.00000}, {dB_u{20.25}, 0.00000}, {dB_u{20.50}, 0.00000},
             {dB_u{20.75}, 0.00000}, {dB_u{21.00}, 0.00000}, {dB_u{21.25}, 0.00000},
             {dB_u{21.50}, 0.00000}, {dB_u{21.75}, 0.00000}, {dB_u{22.00}, 0.00000},
             {dB_u{22.25}, 0.00000}, {dB_u{22.50}, 0.00000}, {dB_u{22.75}, 0.00000},
             {dB_u{23.00}, 0.00000}, {dB_u{23.25}, 0.00000}, {dB_u{23.50}, 0.00000},
             {dB_u{23.75}, 0.00000}, {dB_u{24.00}, 0.00000}, {dB_u{24.25}, 0.00000},
             {dB_u{24.50}, 0.00000}, {dB_u{24.75}, 0.00000}, {dB_u{25.00}, 0.00000},
             {dB_u{25.25}, 0.00000}, {dB_u{25.50}, 0.00000}, {dB_u{25.75}, 0.00000},
             {dB_u{26.00}, 0.00000}, {dB_u{26.25}, 0.00000}, {dB_u{26.50}, 0.00000},
             {dB_u{26.75}, 0.00000}, {dB_u{27.00}, 0.00000}, {dB_u{27.25}, 0.00000},
             {dB_u{27.50}, 0.00000}, {dB_u{27.75}, 0.00000}, {dB_u{28.00}, 0.00000},
             {dB_u{28.25}, 0.00000}, {dB_u{28.50}, 0.00000}, {dB_u{28.75}, 0.00000},
             {dB_u{29.00}, 0.00000}, {dB_u{29.25}, 0.00000}, {dB_u{29.50}, 0.00000},
             {dB_u{29.75}, 0.00000}, {dB_u{30.00}, 0.00000},
         }},
        /* MCS 0 - 2000 bytes */
        {std::make_pair(0, 2000),
         {
             {dB_u{-4.00}, 1.00000}, {dB_u{-3.75}, 1.00000}, {dB_u{-3.50}, 1.00000},
             {dB_u{-3.25}, 1.00000}, {dB_u{-3.00}, 1.00000}, {dB_u{-2.75}, 1.00000},
             {dB_u{-2.50}, 1.00000}, {dB_u{-2.25}, 1.00000}, {dB_u{-2.00}, 1.00000},
             {dB_u{-1.75}, 1.00000}, {dB_u{-1.50}, 0.99993}, {dB_u{-1.25}, 0.99949},
             {dB_u{-1.00}, 0.99030}, {dB_u{-0.75}, 0.94430}, {dB_u{-0.50}, 0.83747},
             {dB_u{-0.25}, 0.58166}, {dB_u{0.00}, 0.33208},  {dB_u{0.25}, 0.19030},
             {dB_u{0.50}, 0.07800},  {dB_u{0.75}, 0.04083},  {dB_u{1.00}, 0.01321},
             {dB_u{1.25}, 0.00658},  {dB_u{1.50}, 0.00176},  {dB_u{1.75}, 0.00084},
             {dB_u{2.00}, 0.00018},  {dB_u{2.25}, 0.00008},  {dB_u{2.50}, 0.00001},
             {dB_u{2.75}, 0.00000},  {dB_u{3.00}, 0.00000},  {dB_u{3.25}, 0.00000},
             {dB_u{3.50}, 0.00000},  {dB_u{3.75}, 0.00000},  {dB_u{4.00}, 0.00000},
             {dB_u{4.25}, 0.00000},  {dB_u{4.50}, 0.00000},  {dB_u{4.75}, 0.00000},
             {dB_u{5.00}, 0.00000},  {dB_u{5.25}, 0.00000},  {dB_u{5.50}, 0.00000},
             {dB_u{5.75}, 0.00000},  {dB_u{6.00}, 0.00000},  {dB_u{6.25}, 0.00000},
             {dB_u{6.50}, 0.00000},  {dB_u{6.75}, 0.00000},  {dB_u{7.00}, 0.00000},
             {dB_u{7.25}, 0.00000},  {dB_u{7.50}, 0.00000},  {dB_u{7.75}, 0.00000},
             {dB_u{8.00}, 0.00000},  {dB_u{8.25}, 0.00000},  {dB_u{8.50}, 0.00000},
             {dB_u{8.75}, 0.00000},  {dB_u{9.00}, 0.00000},  {dB_u{9.25}, 0.00000},
             {dB_u{9.50}, 0.00000},  {dB_u{9.75}, 0.00000},  {dB_u{10.00}, 0.00000},
             {dB_u{10.25}, 0.00000}, {dB_u{10.50}, 0.00000}, {dB_u{10.75}, 0.00000},
             {dB_u{11.00}, 0.00000}, {dB_u{11.25}, 0.00000}, {dB_u{11.50}, 0.00000},
             {dB_u{11.75}, 0.00000}, {dB_u{12.00}, 0.00000}, {dB_u{12.25}, 0.00000},
             {dB_u{12.50}, 0.00000}, {dB_u{12.75}, 0.00000}, {dB_u{13.00}, 0.00000},
             {dB_u{13.25}, 0.00000}, {dB_u{13.50}, 0.00000}, {dB_u{13.75}, 0.00000},
             {dB_u{14.00}, 0.00000}, {dB_u{14.25}, 0.00000}, {dB_u{14.50}, 0.00000},
             {dB_u{14.75}, 0.00000}, {dB_u{15.00}, 0.00000}, {dB_u{15.25}, 0.00000},
             {dB_u{15.50}, 0.00000}, {dB_u{15.75}, 0.00000}, {dB_u{16.00}, 0.00000},
             {dB_u{16.25}, 0.00000}, {dB_u{16.50}, 0.00000}, {dB_u{16.75}, 0.00000},
             {dB_u{17.00}, 0.00000}, {dB_u{17.25}, 0.00000}, {dB_u{17.50}, 0.00000},
             {dB_u{17.75}, 0.00000}, {dB_u{18.00}, 0.00000}, {dB_u{18.25}, 0.00000},
             {dB_u{18.50}, 0.00000}, {dB_u{18.75}, 0.00000}, {dB_u{19.00}, 0.00000},
             {dB_u{19.25}, 0.00000}, {dB_u{19.50}, 0.00000}, {dB_u{19.75}, 0.00000},
             {dB_u{20.00}, 0.00000}, {dB_u{20.25}, 0.00000}, {dB_u{20.50}, 0.00000},
             {dB_u{20.75}, 0.00000}, {dB_u{21.00}, 0.00000}, {dB_u{21.25}, 0.00000},
             {dB_u{21.50}, 0.00000}, {dB_u{21.75}, 0.00000}, {dB_u{22.00}, 0.00000},
             {dB_u{22.25}, 0.00000}, {dB_u{22.50}, 0.00000}, {dB_u{22.75}, 0.00000},
             {dB_u{23.00}, 0.00000}, {dB_u{23.25}, 0.00000}, {dB_u{23.50}, 0.00000},
             {dB_u{23.75}, 0.00000}, {dB_u{24.00}, 0.00000}, {dB_u{24.25}, 0.00000},
             {dB_u{24.50}, 0.00000}, {dB_u{24.75}, 0.00000}, {dB_u{25.00}, 0.00000},
             {dB_u{25.25}, 0.00000}, {dB_u{25.50}, 0.00000}, {dB_u{25.75}, 0.00000},
             {dB_u{26.00}, 0.00000}, {dB_u{26.25}, 0.00000}, {dB_u{26.50}, 0.00000},
             {dB_u{26.75}, 0.00000}, {dB_u{27.00}, 0.00000}, {dB_u{27.25}, 0.00000},
             {dB_u{27.50}, 0.00000}, {dB_u{27.75}, 0.00000}, {dB_u{28.00}, 0.00000},
             {dB_u{28.25}, 0.00000}, {dB_u{28.50}, 0.00000}, {dB_u{28.75}, 0.00000},
             {dB_u{29.00}, 0.00000}, {dB_u{29.25}, 0.00000}, {dB_u{29.50}, 0.00000},
             {dB_u{29.75}, 0.00000}, {dB_u{30.00}, 0.00000},
         }},
        /* MCS 7 - 1500 bytes */
        {std::make_pair(7, 1500),
         {
             {dB_u{-4.00}, 1.00000}, {dB_u{-3.75}, 1.00000}, {dB_u{-3.50}, 1.00000},
             {dB_u{-3.25}, 1.00000}, {dB_u{-3.00}, 1.00000}, {dB_u{-2.75}, 1.00000},
             {dB_u{-2.50}, 1.00000}, {dB_u{-2.25}, 1.00000}, {dB_u{-2.00}, 1.00000},
             {dB_u{-1.75}, 1.00000}, {dB_u{-1.50}, 1.00000}, {dB_u{-1.25}, 1.00000},
             {dB_u{-1.00}, 1.00000}, {dB_u{-0.75}, 1.00000}, {dB_u{-0.50}, 1.00000},
             {dB_u{-0.25}, 1.00000}, {dB_u{0.00}, 1.00000},  {dB_u{0.25}, 1.00000},
             {dB_u{0.50}, 1.00000},  {dB_u{0.75}, 1.00000},  {dB_u{1.00}, 1.00000},
             {dB_u{1.25}, 1.00000},  {dB_u{1.50}, 1.00000},  {dB_u{1.75}, 1.00000},
             {dB_u{2.00}, 1.00000},  {dB_u{2.25}, 1.00000},  {dB_u{2.50}, 1.00000},
             {dB_u{2.75}, 1.00000},  {dB_u{3.00}, 1.00000},  {dB_u{3.25}, 1.00000},
             {dB_u{3.50}, 1.00000},  {dB_u{3.75}, 1.00000},  {dB_u{4.00}, 1.00000},
             {dB_u{4.25}, 1.00000},  {dB_u{4.50}, 1.00000},  {dB_u{4.75}, 1.00000},
             {dB_u{5.00}, 1.00000},  {dB_u{5.25}, 1.00000},  {dB_u{5.50}, 1.00000},
             {dB_u{5.75}, 1.00000},  {dB_u{6.00}, 1.00000},  {dB_u{6.25}, 1.00000},
             {dB_u{6.50}, 1.00000},  {dB_u{6.75}, 1.00000},  {dB_u{7.00}, 1.00000},
             {dB_u{7.25}, 1.00000},  {dB_u{7.50}, 1.00000},  {dB_u{7.75}, 1.00000},
             {dB_u{8.00}, 1.00000},  {dB_u{8.25}, 1.00000},  {dB_u{8.50}, 1.00000},
             {dB_u{8.75}, 1.00000},  {dB_u{9.00}, 1.00000},  {dB_u{9.25}, 1.00000},
             {dB_u{9.50}, 1.00000},  {dB_u{9.75}, 1.00000},  {dB_u{10.00}, 1.00000},
             {dB_u{10.25}, 1.00000}, {dB_u{10.50}, 1.00000}, {dB_u{10.75}, 1.00000},
             {dB_u{11.00}, 1.00000}, {dB_u{11.25}, 1.00000}, {dB_u{11.50}, 1.00000},
             {dB_u{11.75}, 1.00000}, {dB_u{12.00}, 1.00000}, {dB_u{12.25}, 1.00000},
             {dB_u{12.50}, 1.00000}, {dB_u{12.75}, 1.00000}, {dB_u{13.00}, 1.00000},
             {dB_u{13.25}, 1.00000}, {dB_u{13.50}, 1.00000}, {dB_u{13.75}, 1.00000},
             {dB_u{14.00}, 1.00000}, {dB_u{14.25}, 1.00000}, {dB_u{14.50}, 1.00000},
             {dB_u{14.75}, 1.00000}, {dB_u{15.00}, 1.00000}, {dB_u{15.25}, 1.00000},
             {dB_u{15.50}, 1.00000}, {dB_u{15.75}, 1.00000}, {dB_u{16.00}, 1.00000},
             {dB_u{16.25}, 1.00000}, {dB_u{16.50}, 1.00000}, {dB_u{16.75}, 1.00000},
             {dB_u{17.00}, 0.99708}, {dB_u{17.25}, 0.98745}, {dB_u{17.50}, 0.94489},
             {dB_u{17.75}, 0.82929}, {dB_u{18.00}, 0.68537}, {dB_u{18.25}, 0.48376},
             {dB_u{18.50}, 0.31046}, {dB_u{18.75}, 0.20124}, {dB_u{19.00}, 0.11230},
             {dB_u{19.25}, 0.06720}, {dB_u{19.50}, 0.03231}, {dB_u{19.75}, 0.01920},
             {dB_u{20.00}, 0.00909}, {dB_u{20.25}, 0.00533}, {dB_u{20.50}, 0.00242},
             {dB_u{20.75}, 0.00128}, {dB_u{21.00}, 0.00045}, {dB_u{21.25}, 0.00024},
             {dB_u{21.50}, 0.00008}, {dB_u{21.75}, 0.00004}, {dB_u{22.00}, 0.00000},
             {dB_u{22.25}, 0.00000}, {dB_u{22.50}, 0.00000}, {dB_u{22.75}, 0.00000},
             {dB_u{23.00}, 0.00000}, {dB_u{23.25}, 0.00000}, {dB_u{23.50}, 0.00000},
             {dB_u{23.75}, 0.00000}, {dB_u{24.00}, 0.00000}, {dB_u{24.25}, 0.00000},
             {dB_u{24.50}, 0.00000}, {dB_u{24.75}, 0.00000}, {dB_u{25.00}, 0.00000},
             {dB_u{25.25}, 0.00000}, {dB_u{25.50}, 0.00000}, {dB_u{25.75}, 0.00000},
             {dB_u{26.00}, 0.00000}, {dB_u{26.25}, 0.00000}, {dB_u{26.50}, 0.00000},
             {dB_u{26.75}, 0.00000}, {dB_u{27.00}, 0.00000}, {dB_u{27.25}, 0.00000},
             {dB_u{27.50}, 0.00000}, {dB_u{27.75}, 0.00000}, {dB_u{28.00}, 0.00000},
             {dB_u{28.25}, 0.00000}, {dB_u{28.50}, 0.00000}, {dB_u{28.75}, 0.00000},
             {dB_u{29.00}, 0.00000}, {dB_u{29.25}, 0.00000}, {dB_u{29.50}, 0.00000},
             {dB_u{29.75}, 0.00000}, {dB_u{30.00}, 0.00000},
         }},
        /* MCS 8 - 1500 bytes */
        {std::make_pair(8, 1500),
         {
             {dB_u{-4.00}, 1.00000}, {dB_u{-3.75}, 1.00000}, {dB_u{-3.50}, 1.00000},
             {dB_u{-3.25}, 1.00000}, {dB_u{-3.00}, 1.00000}, {dB_u{-2.75}, 1.00000},
             {dB_u{-2.50}, 1.00000}, {dB_u{-2.25}, 1.00000}, {dB_u{-2.00}, 1.00000},
             {dB_u{-1.75}, 1.00000}, {dB_u{-1.50}, 1.00000}, {dB_u{-1.25}, 1.00000},
             {dB_u{-1.00}, 1.00000}, {dB_u{-0.75}, 1.00000}, {dB_u{-0.50}, 1.00000},
             {dB_u{-0.25}, 1.00000}, {dB_u{0.00}, 1.00000},  {dB_u{0.25}, 1.00000},
             {dB_u{0.50}, 1.00000},  {dB_u{0.75}, 1.00000},  {dB_u{1.00}, 1.00000},
             {dB_u{1.25}, 1.00000},  {dB_u{1.50}, 1.00000},  {dB_u{1.75}, 1.00000},
             {dB_u{2.00}, 1.00000},  {dB_u{2.25}, 1.00000},  {dB_u{2.50}, 1.00000},
             {dB_u{2.75}, 1.00000},  {dB_u{3.00}, 1.00000},  {dB_u{3.25}, 1.00000},
             {dB_u{3.50}, 1.00000},  {dB_u{3.75}, 1.00000},  {dB_u{4.00}, 1.00000},
             {dB_u{4.25}, 1.00000},  {dB_u{4.50}, 1.00000},  {dB_u{4.75}, 1.00000},
             {dB_u{5.00}, 1.00000},  {dB_u{5.25}, 1.00000},  {dB_u{5.50}, 1.00000},
             {dB_u{5.75}, 1.00000},  {dB_u{6.00}, 1.00000},  {dB_u{6.25}, 1.00000},
             {dB_u{6.50}, 1.00000},  {dB_u{6.75}, 1.00000},  {dB_u{7.00}, 1.00000},
             {dB_u{7.25}, 1.00000},  {dB_u{7.50}, 1.00000},  {dB_u{7.75}, 1.00000},
             {dB_u{8.00}, 1.00000},  {dB_u{8.25}, 1.00000},  {dB_u{8.50}, 1.00000},
             {dB_u{8.75}, 1.00000},  {dB_u{9.00}, 1.00000},  {dB_u{9.25}, 1.00000},
             {dB_u{9.50}, 1.00000},  {dB_u{9.75}, 1.00000},  {dB_u{10.00}, 1.00000},
             {dB_u{10.25}, 1.00000}, {dB_u{10.50}, 1.00000}, {dB_u{10.75}, 1.00000},
             {dB_u{11.00}, 1.00000}, {dB_u{11.25}, 1.00000}, {dB_u{11.50}, 1.00000},
             {dB_u{11.75}, 1.00000}, {dB_u{12.00}, 1.00000}, {dB_u{12.25}, 1.00000},
             {dB_u{12.50}, 1.00000}, {dB_u{12.75}, 1.00000}, {dB_u{13.00}, 1.00000},
             {dB_u{13.25}, 1.00000}, {dB_u{13.50}, 1.00000}, {dB_u{13.75}, 1.00000},
             {dB_u{14.00}, 1.00000}, {dB_u{14.25}, 1.00000}, {dB_u{14.50}, 1.00000},
             {dB_u{14.75}, 1.00000}, {dB_u{15.00}, 1.00000}, {dB_u{15.25}, 1.00000},
             {dB_u{15.50}, 1.00000}, {dB_u{15.75}, 1.00000}, {dB_u{16.00}, 1.00000},
             {dB_u{16.25}, 1.00000}, {dB_u{16.50}, 1.00000}, {dB_u{16.75}, 1.00000},
             {dB_u{17.00}, 1.00000}, {dB_u{17.25}, 1.00000}, {dB_u{17.50}, 1.00000},
             {dB_u{17.75}, 1.00000}, {dB_u{18.00}, 1.00000}, {dB_u{18.25}, 1.00000},
             {dB_u{18.50}, 1.00000}, {dB_u{18.75}, 1.00000}, {dB_u{19.00}, 1.00000},
             {dB_u{19.25}, 1.00000}, {dB_u{19.50}, 1.00000}, {dB_u{19.75}, 1.00000},
             {dB_u{20.00}, 1.00000}, {dB_u{20.25}, 1.00000}, {dB_u{20.50}, 0.99975},
             {dB_u{20.75}, 0.99891}, {dB_u{21.00}, 0.99007}, {dB_u{21.25}, 0.96322},
             {dB_u{21.50}, 0.90339}, {dB_u{21.75}, 0.77200}, {dB_u{22.00}, 0.63040},
             {dB_u{22.25}, 0.47026}, {dB_u{22.50}, 0.32560}, {dB_u{22.75}, 0.21603},
             {dB_u{23.00}, 0.12650}, {dB_u{23.25}, 0.08029}, {dB_u{23.50}, 0.04356},
             {dB_u{23.75}, 0.02706}, {dB_u{24.00}, 0.01416}, {dB_u{24.25}, 0.00892},
             {dB_u{24.50}, 0.00474}, {dB_u{24.75}, 0.00284}, {dB_u{25.00}, 0.00136},
             {dB_u{25.25}, 0.00077}, {dB_u{25.50}, 0.00032}, {dB_u{25.75}, 0.00017},
             {dB_u{26.00}, 0.00006}, {dB_u{26.25}, 0.00002}, {dB_u{26.50}, 0.00000},
             {dB_u{26.75}, 0.00000}, {dB_u{27.00}, 0.00000}, {dB_u{27.25}, 0.00000},
             {dB_u{27.50}, 0.00000}, {dB_u{27.75}, 0.00000}, {dB_u{28.00}, 0.00000},
             {dB_u{28.25}, 0.00000}, {dB_u{28.50}, 0.00000}, {dB_u{28.75}, 0.00000},
             {dB_u{29.00}, 0.00000}, {dB_u{29.25}, 0.00000}, {dB_u{29.50}, 0.00000},
             {dB_u{29.75}, 0.00000}, {dB_u{30.00}, 0.00000},
         }},
};

/**
 * @ingroup wifi-test
 * @ingroup tests
 *
 * @brief Wifi Table-based Error Rate Models Test Case
 */
class TableBasedErrorRateTestCase : public TestCase
{
  public:
    /**
     * Constructor
     *
     * @param testName the test name
     * @param mode the WifiMode to use for the test
     * @param size the number of bytes to use for the test
     */
    TableBasedErrorRateTestCase(const std::string& testName, WifiMode mode, uint32_t size);
    ~TableBasedErrorRateTestCase() override;

  private:
    void DoRun() override;

    std::string m_testName; ///< The name of the test to run
    WifiMode m_mode;        ///< The WifiMode to test
    uint32_t m_size;        ///< The size (in bytes) to test
};

TableBasedErrorRateTestCase::TableBasedErrorRateTestCase(const std::string& testName,
                                                         WifiMode mode,
                                                         uint32_t size)
    : TestCase(testName),
      m_testName(testName),
      m_mode(mode),
      m_size(size)
{
}

TableBasedErrorRateTestCase::~TableBasedErrorRateTestCase()
{
}

void
TableBasedErrorRateTestCase::DoRun()
{
    // LogComponentEnable ("WifiErrorRateModelsTest", LOG_LEVEL_ALL);
    // LogComponentEnable ("TableBasedErrorRateModel", LOG_LEVEL_ALL);
    // LogComponentEnable ("YansErrorRateModel", LOG_LEVEL_ALL);

    Ptr<TableBasedErrorRateModel> table = CreateObject<TableBasedErrorRateModel>();
    WifiTxVector txVector;
    txVector.SetMode(m_mode);

    // Spot test some values returned from TableBasedErrorRateModel
    for (dB_u snr = -4; snr <= dB_u{30}; snr += dB_u{0.25})
    {
        double expectedPer = 0;
        if (m_mode.GetMcsValue() > ERROR_TABLE_BCC_MAX_NUM_MCS)
        {
            Ptr<YansErrorRateModel> yans = CreateObject<YansErrorRateModel>();
            expectedPer =
                1 - yans->GetChunkSuccessRate(m_mode, txVector, std::pow(10, snr / 10), m_size * 8);
        }
        else
        {
            const auto cit = expectedTableValues.find(std::make_pair(m_mode.GetMcsValue(), m_size));
            if (cit != expectedTableValues.cend())
            {
                // Lambda to find map key within tolerance (avoids floating-point comparison issues)
                const auto& snrPerMap = cit->second;
                const auto expectedIt = [&]() {
                    constexpr dB_u tolerance{1e-6};
                    const auto iter = snrPerMap.lower_bound(snr - tolerance);
                    return (iter != snrPerMap.cend() && iter->first <= snr + tolerance)
                               ? iter
                               : snrPerMap.cend();
                }();
                if (expectedIt != snrPerMap.cend())
                {
                    expectedPer = expectedIt->second;
                }
                else
                {
                    NS_FATAL_ERROR("SNR value " << snr << " dB not found!");
                }
            }
            else
            {
                NS_FATAL_ERROR("No expected PER found for the combination MCS "
                               << +m_mode.GetMcsValue() << " and size " << m_size << " bytes");
            }
        }
        const auto per =
            1 - table->GetChunkSuccessRate(m_mode, txVector, std::pow(10, snr / 10), m_size * 8);
        NS_LOG_INFO(m_testName << ": snr=" << snr << "dB per=" << per
                               << " expectedPER=" << expectedPer);
        NS_TEST_ASSERT_MSG_EQ_TOL(per, expectedPer, 1e-5, "Not equal within tolerance");
    }
}

/**
 * @ingroup wifi-test
 * @ingroup tests
 *
 * @brief Wifi Error Rate Models Test Suite
 */
class WifiErrorRateModelsTestSuite : public TestSuite
{
  public:
    WifiErrorRateModelsTestSuite();
};

WifiErrorRateModelsTestSuite::WifiErrorRateModelsTestSuite()
    : TestSuite("wifi-error-rate-models", Type::UNIT)
{
    AddTestCase(new WifiErrorRateModelsTestCaseDsss, TestCase::Duration::QUICK);
    AddTestCase(new WifiErrorRateModelsTestCaseNist, TestCase::Duration::QUICK);
    AddTestCase(new WifiErrorRateModelsTestCaseMimo, TestCase::Duration::QUICK);
    AddTestCase(new TableBasedErrorRateTestCase("DefaultTableBasedHtMcs0-1458bytes",
                                                HtPhy::GetHtMcs0(),
                                                1458),
                TestCase::Duration::QUICK);
    AddTestCase(
        new TableBasedErrorRateTestCase("DefaultTableBasedHtMcs0-32bytes", HtPhy::GetHtMcs0(), 32),
        TestCase::Duration::QUICK);
    AddTestCase(new TableBasedErrorRateTestCase("DefaultTableBasedHtMcs0-1000bytes",
                                                HtPhy::GetHtMcs0(),
                                                1000),
                TestCase::Duration::QUICK);
    AddTestCase(
        new TableBasedErrorRateTestCase("DefaultTableBasedHtMcs0-1byte", HtPhy::GetHtMcs0(), 1),
        TestCase::Duration::QUICK);
    AddTestCase(new TableBasedErrorRateTestCase("DefaultTableBasedHtMcs0-2000bytes",
                                                HtPhy::GetHtMcs0(),
                                                2000),
                TestCase::Duration::QUICK);
    AddTestCase(new TableBasedErrorRateTestCase("DefaultTableBasedHtMcs7-1500bytes",
                                                HtPhy::GetHtMcs7(),
                                                1500),
                TestCase::Duration::QUICK);
    AddTestCase(new TableBasedErrorRateTestCase("DefaultTableBasedVhtMcs0-1458bytes",
                                                VhtPhy::GetVhtMcs0(),
                                                1458),
                TestCase::Duration::QUICK);
    AddTestCase(new TableBasedErrorRateTestCase("DefaultTableBasedVhtMcs0-32bytes",
                                                VhtPhy::GetVhtMcs0(),
                                                32),
                TestCase::Duration::QUICK);
    AddTestCase(new TableBasedErrorRateTestCase("DefaultTableBasedVhtMcs0-1000bytes",
                                                VhtPhy::GetVhtMcs0(),
                                                1000),
                TestCase::Duration::QUICK);
    AddTestCase(
        new TableBasedErrorRateTestCase("DefaultTableBasedVhtMcs0-1byte", VhtPhy::GetVhtMcs0(), 1),
        TestCase::Duration::QUICK);
    AddTestCase(new TableBasedErrorRateTestCase("DefaultTableBasedVhtMcs0-2000bytes",
                                                VhtPhy::GetVhtMcs0(),
                                                2000),
                TestCase::Duration::QUICK);
    AddTestCase(new TableBasedErrorRateTestCase("DefaultTableBasedVhtMcs8-1500bytes",
                                                VhtPhy::GetVhtMcs8(),
                                                1500),
                TestCase::Duration::QUICK);
    AddTestCase(new TableBasedErrorRateTestCase("FallbackTableBasedHeMcs11-1458bytes",
                                                HePhy::GetHeMcs11(),
                                                1458),
                TestCase::Duration::QUICK);
}

static WifiErrorRateModelsTestSuite wifiErrorRateModelsTestSuite; ///< the test suite
