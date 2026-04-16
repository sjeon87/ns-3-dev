/*
 * Copyright (c) 2023 SIGNET Lab, Department of Information Engineering,
 * University of Padova
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */

/**
 * @file
 * This example is a modified version of "three-gpp-channel-example", to include
 * the 3GPP NTN channel model.
 * Specifically, most changes (which are also highlighted throughout the code)
 * impact the main method, and comprise:
 * - the configuration of ad-hoc propagation and channel condition models;
 * - the use of GeocentricConstantPositionMobilityModel for the nodes' mobility.
 * The pre-configured parameters are the one provided by 3GPP in TR 38.821,
 * more specifically scenario 10 in down-link mode.
 * Two static nodes, one on the ground and one in orbit, communicate with each other.
 * The carrier frequency is set at 20GHz with 400MHz bandwidth.
 * The result is the SNR of the signal and the path loss, saved in the ntn-snr-trace.txt file.
 */

#include "ns3/antenna-model.h"
#include "ns3/core-module.h"
#include "ns3/geocentric-constant-position-mobility-model.h"
#include "ns3/isotropic-antenna-model.h"
#include "ns3/mobility-model.h"
#include "ns3/node-container.h"
#include "ns3/node.h"
#include "ns3/spectrum-signal-parameters.h"
#include "ns3/three-gpp-channel-model.h"
#include "ns3/three-gpp-propagation-loss-model.h"
#include "ns3/three-gpp-spectrum-propagation-loss-model.h"
#include "ns3/uniform-planar-array.h"

#include <fstream>

NS_LOG_COMPONENT_DEFINE("NTNChannelExample");

using namespace ns3;

static Ptr<ThreeGppPropagationLossModel>
    m_propagationLossModel; //!< the PropagationLossModel object
static Ptr<ThreeGppSpectrumPropagationLossModel>
    m_spectrumLossModel;          //!< the SpectrumPropagationLossModel object
static std::ofstream resultsFile; //!< The results file

/**
 * Frequency range definitions
 */
enum FrequencyRange
{
    FR1 = 1,    /**< Frequency range FR1 410MHz..7125MHz */
    FR2 = 2,    /**< Frequency range FR2 17300MHz..30000MHz */
    NON_NTN = 3 /**< Other non-NTN frequency range */
};

/**
 * @brief get frequency range accroding to TS 138 101-5 V18.5.0 Table 5.1-1 (NTN)
 *
 * @param fcHz the carrier frequency in Hz
 * @return FrequencyRange
 */
FrequencyRange
getFrequencyRange(double fcHz)
{
    if (fcHz > 4.1e8 && fcHz < 7.125e9)
    {
        return FrequencyRange::FR1;
    }

    if (fcHz > 17.3e9 && fcHz < 30.6e9)
    {
        return FrequencyRange::FR2;
    }

    return FrequencyRange::NON_NTN;
}

/**
 * @brief A structure that holds structure of UE channel BW
 */
struct UEChannelBandwidth
{
    double total; //!< the total bandwidth of channel in Hz
    double perRB; //!< the Resource Block bandwidth in Hz

    /**
     * @brief Create a UEChannelBandwidth
     */
    UEChannelBandwidth()
        : total(0.0),
          perRB(0.0)
    {
    }

    /**
     * @brief Destroy a UEChannelBandwidth
     */
    ~UEChannelBandwidth()
    {
    }

    /**
     * @brief Copy constructor.
     *
     * @param chBw Object to copy from
     */
    UEChannelBandwidth(const UEChannelBandwidth& chBw)
    {
        total = chBw.total;
        perRB = chBw.perRB;
    }

    /// Guardband holding a pair<SCR,BW> + actual guardband
    using Guardband = std::map<std::pair<double, double>, double>;

    /// Guardband records for FR1
    mutable Guardband guardsFR1 = {{{15.0, 5000.0}, 242.5},
                                   {{15.0, 10000.0}, 312.5},
                                   {{15.0, 15000.0}, 382.5},
                                   {{15.0, 20000.0}, 452.5},
                                   {{15.0, 30000.0}, 592.5},
                                   {{30.0, 5000.0}, 505.0},
                                   {{30.0, 10000.0}, 665.0},
                                   {{30.0, 15000.0}, 645.0},
                                   {{30.0, 20000.0}, 805.0},
                                   {{30.0, 30000.0}, 945.0},
                                   {{60.0, 10000.0}, 1010.0},
                                   {{60.0, 15000.0}, 990.0},
                                   {{60.0, 20000.0}, 1330.0},
                                   {{60.0, 30000.0}, 1290.0}};

    /// Guardband records for FR2
    mutable Guardband guardsFR2 = {{{60.0, 50000.0}, 1210.0},
                                   {{60.0, 100000.0}, 2450.0},
                                   {{60.0, 200000.0}, 4930.0},
                                   {{120.0, 50000.0}, 1900.0},
                                   {{120.0, 100000.0}, 2420.0},
                                   {{120.0, 200000.0}, 4900.0},
                                   {{120.0, 400000.0}, 9860.0}};

    /**
     * @brief getGuardband based on frequency, channel bandwidth and rb width
     *
     * @param fcHz the carrier frequency in Hz
     * @return guardband in hz
     */
    double getGuardband(double fcHz) const
    {
        FrequencyRange fr = getFrequencyRange(fcHz);

        Guardband* guardband;

        if (fr == FrequencyRange::FR1)
        {
            guardband = &guardsFR1;
        }
        else
        {
            guardband = &guardsFR2;
        }

        auto it = guardband->find(std::pair(perRB / 1000.0, total / 1000.0));
        if (it != guardband->end())
        {
            NS_LOG_DEBUG("Guardband value: " << it->second << "(SCS=" << perRB << ",BW=" << total
                                             << ")");
            return it->second;
        }

        return 0.0;
    }

    /**
     * @brief calculateNrb based on frequency, channel bandwidth and rb width
     * formula for calculating Nrbs: (1000*BW - SCS - (GbwR + GbwL))/12*SCS
     * please check TS 138 101-5 5.3.3 NOTE
     *
     * @param fcHz the carrier frequency in Hz
     * @return Nrb
     */
    unsigned int calculateNrb(double fcHz) const
    {
        if (perRB == 0.0)
        {
            NS_LOG_ERROR("Resource block width is not set, Nrb cannot be calculated!");
            return 0;
        }

        return std::floor((total / 1000 - perRB / 1000 - (getGuardband(fcHz) * 2)) /
                          (12 * (perRB / 1000)));
    }
};

/**
 * @brief Create the PSD for the TX
 *
 * @param fcHz the carrier frequency in Hz
 * @param pwrDbm the transmission power in dBm
 * @param bw the structure that holds UE bandwidth
 * @return the pointer to the PSD
 */
Ptr<SpectrumValue>
CreateTxPowerSpectralDensity(double fcHz, double pwrDbm, const UEChannelBandwidth& bw)
{
    unsigned int numRbs = bw.calculateNrb(fcHz);

    double f = fcHz - (numRbs * bw.perRB / 2.0);
    double powerTx = pwrDbm; // dBm power

    Bands rbs; // A vector representing each resource block
    for (uint32_t numrb = 0; numrb < numRbs; ++numrb)
    {
        BandInfo rb;
        rb.fl = f;
        f += bw.perRB / 2;
        rb.fc = f;
        f += bw.perRB / 2;
        rb.fh = f;

        rbs.push_back(rb);
    }
    Ptr<SpectrumModel> model = Create<SpectrumModel>(rbs);
    Ptr<SpectrumValue> txPsd = Create<SpectrumValue>(model);

    double powerTxW = std::pow(10., (powerTx - 30) / 10); // Get Tx power in Watts
    double txPowerDensity = (powerTxW / bw.total);

    for (auto psd = txPsd->ValuesBegin(); psd != txPsd->ValuesEnd(); ++psd)
    {
        *psd = txPowerDensity;
    }

    return txPsd; // [W/Hz]
}

/**
 * @brief Create the noise PSD for the
 *
 * @param fcHz the carrier frequency in Hz
 * @param noiseFigureDb the noise figure in dB
 * @param bw the structure that holds UE bandwidth
 *
 * @return the pointer to the noise PSD
 */
Ptr<SpectrumValue>
CreateNoisePowerSpectralDensity(double fcHz, double noiseFigureDb, const UEChannelBandwidth& bw)
{
    unsigned int numRbs = bw.calculateNrb(fcHz);

    double f = fcHz - (numRbs * bw.perRB / 2.0);

    Bands rbs;              // A vector representing each resource block
    std::vector<int> rbsId; // A vector representing the resource block IDs
    for (uint32_t numrb = 0; numrb < numRbs; ++numrb)
    {
        BandInfo rb;
        rb.fl = f;
        f += bw.perRB / 2;
        rb.fc = f;
        f += bw.perRB / 2;
        rb.fh = f;

        rbs.push_back(rb);
        rbsId.push_back(numrb);
    }
    Ptr<SpectrumModel> model = Create<SpectrumModel>(rbs);
    Ptr<SpectrumValue> txPsd = Create<SpectrumValue>(model);

    // see "LTE - From theory to practice"
    // Section 22.4.4.2 Thermal Noise and Receiver Noise Figure
    const double ktDbmHz = -174.0;                        // dBm/Hz
    double ktWHz = std::pow(10.0, (ktDbmHz - 30) / 10.0); // W/Hz
    double noiseFigureLinear = std::pow(10.0, noiseFigureDb / 10.0);

    double noisePowerSpectralDensity = ktWHz * noiseFigureLinear;

    for (auto rbId : rbsId)
    {
        (*txPsd)[rbId] = noisePowerSpectralDensity;
    }

    return txPsd; // W/Hz
}

/**
 * @brief A structure that holds the parameters for the
 * ComputeSnr function. In this way the problem with the limited
 * number of parameters of method Schedule is avoided.
 */
struct ComputeSnrParams
{
    Ptr<MobilityModel> txMob;        //!< the tx mobility model
    Ptr<MobilityModel> rxMob;        //!< the rx mobility model
    double txPow;                    //!< the tx power in dBm
    double noiseFigure;              //!< the noise figure in dB
    Ptr<PhasedArrayModel> txAntenna; //!< the tx antenna array
    Ptr<PhasedArrayModel> rxAntenna; //!< the rx antenna array
    double frequency;                //!< the carrier frequency in Hz
    UEChannelBandwidth bandwidth;    //!< UE channel bandwidth structure as per TS.138.101-5 5.3

    /**
     * @brief Constructor
     * @param pTxMob the tx mobility model
     * @param pRxMob the rx mobility model
     * @param pTxPow the tx power in dBm
     * @param pNoiseFigure the noise figure in dB
     * @param pTxAntenna the tx antenna array
     * @param pRxAntenna the rx antenna array
     * @param pFrequency the carrier frequency in Hz
     * @param chBandwidth UE channel bandwidth structure
     */
    ComputeSnrParams(Ptr<MobilityModel> pTxMob,
                     Ptr<MobilityModel> pRxMob,
                     double pTxPow,
                     double pNoiseFigure,
                     Ptr<PhasedArrayModel> pTxAntenna,
                     Ptr<PhasedArrayModel> pRxAntenna,
                     double pFrequency,
                     const UEChannelBandwidth& chBandwidth)
    {
        txMob = pTxMob;
        rxMob = pRxMob;
        txPow = pTxPow;
        noiseFigure = pNoiseFigure;
        txAntenna = pTxAntenna;
        rxAntenna = pRxAntenna;
        frequency = pFrequency;
        bandwidth = chBandwidth;
    }
};

/**
 * Perform the beamforming using the DFT beamforming method
 * @param txMob the mobility model of the node performing the beamforming
 * @param thisAntenna the antenna object associated to txMob
 * @param rxMob the mobility model of the node towards which will point the beam
 */
static void
DoBeamforming(Ptr<MobilityModel> rxMob, Ptr<PhasedArrayModel> thisAntenna, Ptr<MobilityModel> txMob)
{
    // compute the azimuth and the elevation angles
    Angles completeAngle(txMob->GetPosition(), rxMob->GetPosition());
    double hAngleRadian = completeAngle.GetAzimuth();
    double vAngleRadian = completeAngle.GetInclination(); // the elevation angle

    // retrieve the number of antenna elements and resize the vector
    uint64_t totNoArrayElements = thisAntenna->GetNumElems();
    PhasedArrayModel::ComplexVector antennaWeights(totNoArrayElements);

    // the total power is divided equally among the antenna elements
    double power = 1.0 / sqrt(totNoArrayElements);

    // compute the antenna weights
    const double sinVAngleRadian = sin(vAngleRadian);
    const double cosVAngleRadian = cos(vAngleRadian);
    const double sinHAngleRadian = sin(hAngleRadian);
    const double cosHAngleRadian = cos(hAngleRadian);

    for (uint64_t ind = 0; ind < totNoArrayElements; ind++)
    {
        Vector loc = thisAntenna->GetElementLocation(ind);
        double phase = -2 * M_PI *
                       (sinVAngleRadian * cosHAngleRadian * loc.x +
                        sinVAngleRadian * sinHAngleRadian * loc.y + cosVAngleRadian * loc.z);
        antennaWeights[ind] = exp(std::complex<double>(0, phase)) * power;
    }

    // store the antenna weights
    thisAntenna->SetBeamformingVector(antennaWeights);
}

/**
 * Compute the average SNR
 * @param params A structure that holds the parameters that are needed to perform calculations in
 * ComputeSnr
 */
static void
ComputeSnr(ComputeSnrParams& params)
{
    Ptr<SpectrumValue> txPsd =
        CreateTxPowerSpectralDensity(params.frequency, params.txPow, params.bandwidth);
    Ptr<SpectrumValue> rxPsd = txPsd->Copy();
    NS_LOG_DEBUG("Average tx power " << 10 * log10(Sum(*txPsd) * params.bandwidth.perRB) << " dB");

    // create the noise PSD
    Ptr<SpectrumValue> noisePsd =
        CreateNoisePowerSpectralDensity(params.frequency, params.noiseFigure, params.bandwidth);
    NS_LOG_DEBUG("Average noise power " << 10 * log10(Sum(*noisePsd) * params.bandwidth.perRB)
                                        << " dB");

    // apply the pathloss
    double propagationGainDb = m_propagationLossModel->CalcRxPower(0, params.txMob, params.rxMob);
    NS_LOG_DEBUG("Pathloss " << -propagationGainDb << " dB");
    double propagationGainLinear = std::pow(10.0, (propagationGainDb) / 10.0);
    *(rxPsd) *= propagationGainLinear;

    NS_ASSERT_MSG(params.txAntenna, "params.txAntenna is nullptr!");
    NS_ASSERT_MSG(params.rxAntenna, "params.rxAntenna is nullptr!");

    Ptr<SpectrumSignalParameters> rxSsp = Create<SpectrumSignalParameters>();
    rxSsp->psd = rxPsd;
    rxSsp->txAntenna =
        ConstCast<AntennaModel, const AntennaModel>(params.txAntenna->GetAntennaElement());

    // apply the fast fading and the beamforming gain
    rxSsp = m_spectrumLossModel->CalcRxPowerSpectralDensity(rxSsp,
                                                            params.txMob,
                                                            params.rxMob,
                                                            params.txAntenna,
                                                            params.rxAntenna);
    NS_LOG_DEBUG("Average rx power " << 10 * log10(Sum(*rxSsp->psd) * params.bandwidth.total)
                                     << " dB");
    // compute the SNR
    NS_LOG_DEBUG("Average SNR " << 10 * log10(Sum(*rxSsp->psd) / Sum(*noisePsd)) << " dB");

    // print the SNR and pathloss values in the output file
    resultsFile << Simulator::Now().GetSeconds() << " "
                << 10 * log10(Sum(*rxSsp->psd) / Sum(*noisePsd)) << " " << propagationGainDb
                << std::endl;
}

int
main(int argc, char* argv[])
{
    uint32_t simTimeMs = 1000; // simulation time in milliseconds
    uint32_t timeResMs = 10;   // time resolution in milliseconds

    // Start changes with respect to three-gpp-channel-example
    // SCENARIO 10 DL of TR 38.321
    // This parameters can be set accordingly to 3GPP TR 38.821 or arbitrarily modified
    std::string scenario = "NTN-Suburban"; // 3GPP propagation scenario
    // All available NTN scenarios: DenseUrban, Urban, Suburban, Rural.

    double frequencyHz = 20e9; // operating frequency in Hz

    UEChannelBandwidth ueChannelBandwidth;
    ueChannelBandwidth.total = 400e6; // Hz
    ueChannelBandwidth.perRB = 120e3; // Hz

    // Satellite parameters
    double satEIRPDensity = 40;     // dBW/MHz
    double satAntennaGainDb = 58.5; // dB

    // UE Parameters
    double vsatAntennaGainDb = 39.7;       // dB
    double vsatAntennaNoiseFigureDb = 1.2; // dB
    // End changes with respect to three-gpp-channel-example

    /* Command line argument parser setup. */
    CommandLine cmd(__FILE__);
    cmd.AddValue("scenario",
                 "The 3GPP NTN scenario to use. Valid options are: "
                 "NTN-DenseUrban, NTN-Urban, NTN-Suburban, and NTN-Rural",
                 scenario);
    cmd.AddValue("frequencyHz", "The carrier frequency in Hz", frequencyHz);
    cmd.AddValue("bandwidthHz", "The UE channel bandwidth in Hz", ueChannelBandwidth.total);
    cmd.AddValue("satEIRPDensity", "The satellite EIRP density in dBW/MHz", satEIRPDensity);
    cmd.AddValue("satAntennaGainDb", "The satellite antenna gain in dB", satAntennaGainDb);
    cmd.AddValue("vsatAntennaGainDb", "The UE VSAT antenna gain in dB", vsatAntennaGainDb);
    cmd.AddValue("vsatAntennaNoiseFigureDb",
                 "The UE VSAT antenna noise figure in dB",
                 vsatAntennaNoiseFigureDb);
    cmd.Parse(argc, argv);

    // Calculate transmission power in dBm using EIRPDensity + 10*log10(Bandwidth) - AntennaGain +
    // 30
    double txPowDbm =
        (satEIRPDensity + 10 * log10(ueChannelBandwidth.total / 1e6) - satAntennaGainDb) + 30;

    NS_LOG_DEBUG("Transmitting power: " << txPowDbm << "dBm, (" << pow(10., (txPowDbm - 30) / 10)
                                        << "W)");

    Config::SetDefault("ns3::ThreeGppChannelModel::UpdatePeriod",
                       TimeValue(MilliSeconds(10))); // update the channel at every 10 ms
    Config::SetDefault("ns3::ThreeGppChannelConditionModel::UpdatePeriod",
                       TimeValue(MilliSeconds(0))); // do not update the channel condition

    RngSeedManager::SetSeed(1);
    RngSeedManager::SetRun(1);

    // create and configure the factories for the channel condition and propagation loss models
    ObjectFactory propagationLossModelFactory;
    ObjectFactory channelConditionModelFactory;

    // Start changes with respect to three-gpp-channel-example
    if (scenario == "NTN-DenseUrban")
    {
        propagationLossModelFactory.SetTypeId(
            ThreeGppNTNDenseUrbanPropagationLossModel::GetTypeId());
        channelConditionModelFactory.SetTypeId(
            ThreeGppNTNDenseUrbanChannelConditionModel::GetTypeId());
    }
    else if (scenario == "NTN-Urban")
    {
        propagationLossModelFactory.SetTypeId(ThreeGppNTNUrbanPropagationLossModel::GetTypeId());
        channelConditionModelFactory.SetTypeId(ThreeGppNTNUrbanChannelConditionModel::GetTypeId());
    }
    else if (scenario == "NTN-Suburban")
    {
        propagationLossModelFactory.SetTypeId(ThreeGppNTNSuburbanPropagationLossModel::GetTypeId());
        channelConditionModelFactory.SetTypeId(
            ThreeGppNTNSuburbanChannelConditionModel::GetTypeId());
    }
    else if (scenario == "NTN-Rural")
    {
        propagationLossModelFactory.SetTypeId(ThreeGppNTNRuralPropagationLossModel::GetTypeId());
        channelConditionModelFactory.SetTypeId(ThreeGppNTNRuralChannelConditionModel::GetTypeId());
    }
    else
    {
        NS_FATAL_ERROR("Unknown NTN scenario");
    }
    // End changes with respect to three-gpp-channel-example

    // create the propagation loss model
    m_propagationLossModel = propagationLossModelFactory.Create<ThreeGppPropagationLossModel>();
    m_propagationLossModel->SetAttribute("Frequency", DoubleValue(frequencyHz));
    m_propagationLossModel->SetAttribute("ShadowingEnabled", BooleanValue(true));

    // create the spectrum propagation loss model
    m_spectrumLossModel = CreateObject<ThreeGppSpectrumPropagationLossModel>();
    m_spectrumLossModel->SetChannelModelAttribute("Frequency", DoubleValue(frequencyHz));
    m_spectrumLossModel->SetChannelModelAttribute("Scenario", StringValue(scenario));

    // create the channel condition model and associate it with the spectrum and
    // propagation loss model
    Ptr<ChannelConditionModel> condModel =
        channelConditionModelFactory.Create<ThreeGppChannelConditionModel>();
    m_spectrumLossModel->SetChannelModelAttribute("ChannelConditionModel", PointerValue(condModel));
    m_propagationLossModel->SetChannelConditionModel(condModel);

    // create the tx and rx nodes
    NodeContainer nodes;
    nodes.Create(2);

    // Start changes with respect to three-gpp-channel-example, here a mobility model
    // tailored to NTN scenarios is used (GeocentricConstantPositionMobilityModel)
    // create the tx and rx mobility models, set the positions
    Ptr<GeocentricConstantPositionMobilityModel> txMob =
        CreateObject<GeocentricConstantPositionMobilityModel>();
    Ptr<GeocentricConstantPositionMobilityModel> rxMob =
        CreateObject<GeocentricConstantPositionMobilityModel>();

    txMob->SetGeographicPosition(Vector(45.40869, 11.89448, 35786000)); // GEO over Padova
    rxMob->SetGeographicPosition(Vector(45.40869, 11.89448, 14.0));     // Padova Coordinates

    // This is not strictly necessary, but is useful to have "sensible" values when using
    // GetPosition()
    txMob->SetCoordinateTranslationReferencePoint(
        Vector(45.40869, 11.89448, 0.0)); // Padova Coordinates without altitude
    rxMob->SetCoordinateTranslationReferencePoint(
        Vector(45.40869, 11.89448, 0.0)); // Padova Coordinates without altitude
    // End changes with respect to three-gpp-channel-example,

    NS_LOG_DEBUG("TX Position: " << txMob->GetPosition());
    NS_LOG_DEBUG("RX Position: " << rxMob->GetPosition());

    // assign the mobility models to the nodes
    nodes.Get(0)->AggregateObject(txMob);
    nodes.Get(1)->AggregateObject(rxMob);

    // Start changes with respect to three-gpp-channel-example,
    // Here antenna models mimic the gain achieved by the CircularApertureAntennaModel,
    // which is not used to avoid inheriting the latter's dependence on either libstdc++
    // or Boost.
    Ptr<PhasedArrayModel> txAntenna = CreateObjectWithAttributes<UniformPlanarArray>(
        "NumColumns",
        UintegerValue(1),
        "NumRows",
        UintegerValue(1),
        "AntennaElement",
        PointerValue(
            CreateObjectWithAttributes<IsotropicAntennaModel>("Gain",
                                                              DoubleValue(satAntennaGainDb))));

    Ptr<PhasedArrayModel> rxAntenna = CreateObjectWithAttributes<UniformPlanarArray>(
        "NumColumns",
        UintegerValue(1),
        "NumRows",
        UintegerValue(1),
        "AntennaElement",
        PointerValue(
            CreateObjectWithAttributes<IsotropicAntennaModel>("Gain",
                                                              DoubleValue(vsatAntennaGainDb))));
    // End changes with respect to three-gpp-channel-example

    // set the beamforming vectors
    DoBeamforming(rxMob, rxAntenna, txMob);
    DoBeamforming(txMob, txAntenna, rxMob);

    // Open the output results file
    resultsFile.open("ntn-snr-trace.txt", std::ios::out);
    NS_ASSERT_MSG(resultsFile.is_open(), "Results file could not be created");

    for (int i = 0; i < floor(simTimeMs / timeResMs); i++)
    {
        Simulator::Schedule(MilliSeconds(timeResMs * i),
                            &ComputeSnr,
                            ComputeSnrParams(txMob,
                                             rxMob,
                                             txPowDbm,
                                             vsatAntennaNoiseFigureDb,
                                             txAntenna,
                                             rxAntenna,
                                             frequencyHz,
                                             ueChannelBandwidth));
    }

    Simulator::Run();
    Simulator::Destroy();

    resultsFile.close();

    return 0;
}
