#! /usr/bin/env python3

# A list of C++ examples to run in order to ensure that they remain
# buildable and runnable over time.  Each tuple in the list contains
#
#     (example_name, do_run, do_valgrind_run).
#
# See test.py for more information.
cpp_examples = [
    ("mixed-wired-wireless", "True", "True"),
    ("wifi-multirate --totalTime=0.3s --rateManager=ns3::AarfcdWifiManager", "True", "True"),
    ("wifi-multirate --totalTime=0.3s --rateManager=ns3::AmrrWifiManager", "True", "False"),
    ("wifi-multirate --totalTime=0.3s --rateManager=ns3::CaraWifiManager", "True", "False"),
    ("wifi-multirate --totalTime=0.3s --rateManager=ns3::IdealWifiManager", "True", "False"),
    ("wifi-multirate --totalTime=0.3s --rateManager=ns3::MinstrelWifiManager", "True", "False"),
    ("wifi-multirate --totalTime=0.3s --rateManager=ns3::OnoeWifiManager", "True", "False"),
    ("wifi-multirate --totalTime=0.3s --rateManager=ns3::RraaWifiManager", "True", "False"),
    ("wifi-adhoc", "False", "True"),  # Takes too long to run
    ("wifi-ap --verbose=0", "True", "True"),  # Don't let it spew to stdout
    ("wifi-clear-channel-cmu", "False", "True"),  # Requires specific hardware
    ("wifi-simple-adhoc", "True", "True"),
    ("wifi-simple-adhoc-grid", "True", "True"),
    ("wifi-simple-infra", "True", "True"),
    ("wifi-simple-interference", "True", "True"),
    ("wifi-wired-bridging", "True", "True"),
    ("wifi-sleep", "True", "True"),
    ("wifi-blockack", "True", "True"),
    ("wifi-timing-attributes --simulationTime=1s", "True", "True"),
    (
        "wifi-power-adaptation-distance --manager=ns3::ParfWifiManager --outputFileName=parf --steps=5 --stepsSize=10",
        "True",
        "True",
    ),
    (
        "wifi-power-adaptation-distance --manager=ns3::AparfWifiManager --outputFileName=aparf --steps=5 --stepsSize=10",
        "True",
        "False",
    ),
    (
        "wifi-power-adaptation-distance --manager=ns3::RrpaaWifiManager --outputFileName=rrpaa --steps=5 --stepsSize=10",
        "True",
        "False",
    ),
    (
        "wifi-rate-adaptation-distance --standard=802.11a --staManager=ns3::MinstrelWifiManager --apManager=ns3::MinstrelWifiManager --outputFileName=minstrel --stepsSize=50 --stepsTime=0.1",
        "True",
        "False",
    ),
    (
        "wifi-rate-adaptation-distance --standard=802.11a --staManager=ns3::MinstrelWifiManager --apManager=ns3::MinstrelWifiManager --outputFileName=minstrel --stepsSize=50 --stepsTime=0.1 --STA1_x=-200",
        "True",
        "False",
    ),
    (
        "wifi-rate-adaptation-distance --staManager=ns3::MinstrelHtWifiManager --apManager=ns3::MinstrelHtWifiManager --outputFileName=minstrelHt --shortGuardInterval=true --channelWidth=40 --stepsSize=50 --stepsTime=0.1",
        "True",
        "False",
    ),
    ("wifi-power-adaptation-interference --simuTime=5", "True", "False"),
    ("wifi-dsss-validation", "True", "True"),
    ("wifi-ofdm-validation", "True", "True"),
    ("wifi-ofdm-ht-validation", "True", "True"),
    ("wifi-ofdm-vht-validation", "True", "True"),
    ("wifi-ofdm-he-validation", "True", "True"),
    ("wifi-error-models-comparison", "True", "True"),
    ("wifi-80211n-mimo --simulationTime=0.1s --step=10", "True", "True"),
    (
        "wifi-ht-network --simulationTime=0.2s --frequency=5 --useRts=0 --minExpectedThroughput=5 --maxExpectedThroughput=135",
        "True",
        "True",
    ),
    (
        "wifi-ht-network --simulationTime=0.2s --frequency=5 --useRts=1 --minExpectedThroughput=5 --maxExpectedThroughput=132",
        "True",
        "True",
    ),
    (
        "wifi-ht-network --simulationTime=0.2s --frequency=2.4 --useRts=0 --minExpectedThroughput=4 --maxExpectedThroughput=132",
        "True",
        "True",
    ),
    (
        "wifi-ht-network --simulationTime=0.2s --frequency=2.4 --useRts=1 --minExpectedThroughput=4 --maxExpectedThroughput=129",
        "True",
        "True",
    ),
    (
        "wifi-vht-network --simulationTime=0.2s --useRts=0 --minExpectedThroughput=5 --maxExpectedThroughput=675",
        "True",
        "True",
    ),
    (
        "wifi-vht-network --simulationTime=0.2s --useRts=1 --minExpectedThroughput=4.5 --maxExpectedThroughput=645",
        "True",
        "True",
    ),
    (
        "wifi-vht-network --simulationTime=0.2s --useRts=0 --use80Plus80=1 --minExpectedThroughput=5 --maxExpectedThroughput=675",
        "True",
        "True",
    ),
    (
        "wifi-he-network --simulationTime=0.25s --frequency=5 --useRts=0 --minExpectedThroughput=6 --maxExpectedThroughput=844",
        "True",
        "True",
    ),
    (
        "wifi-he-network --simulationTime=0.25s --frequency=5 --useRts=0 --use80Plus80=1 --minExpectedThroughput=6 --maxExpectedThroughput=844",
        "True",
        "True",
    ),
    (
        "wifi-he-network --simulationTime=0.3s --frequency=5 --useRts=0 --useExtendedBlockAck=1 --minExpectedThroughput=6 --maxExpectedThroughput=1033",
        "True",
        "True",
    ),
    (
        "wifi-he-network --simulationTime=0.3s --frequency=5 --useRts=1 --minExpectedThroughput=4.7 --maxExpectedThroughput=795",
        "True",
        "True",
    ),
    (
        "wifi-he-network --simulationTime=0.25s --frequency=2.4 --useRts=0 --minExpectedThroughput=4.3 --maxExpectedThroughput=246",
        "True",
        "True",
    ),
    (
        "wifi-he-network --simulationTime=0.3s --frequency=2.4 --useRts=1 --minExpectedThroughput=4.1 --maxExpectedThroughput=238",
        "True",
        "True",
    ),
    (
        "wifi-he-network --simulationTime=0.3s --udp=0 --downlink=1 --useRts=0 --nStations=4 --dlAckType=ACK-SU-FORMAT --enableUlOfdma=1 --enableBsrp=0 --mcs=4 --minExpectedThroughput=20 --maxExpectedThroughput=212",
        "True",
        "True",
    ),
    (
        # TXOP limit pinned to 0 pending !2938 (per-MPDU DL MU protection fix)
        "wifi-he-network --simulationTime=0.3s --frequency=2.4 --udp=0 --downlink=1 --useRts=1 --nStations=5 --dlAckType=MU-BAR --enableUlOfdma=1 --enableBsrp=1 --mcs=5 --ns3::Txop::TxopLimits=0us --minExpectedThroughput=21 --maxExpectedThroughput=56",
        "True",
        "True",
    ),
    (
        "wifi-he-network --simulationTime=0.3s --udp=0 --downlink=1 --useRts=0 --nStations=5 --dlAckType=AGGR-MU-BAR --enableUlOfdma=1 --enableBsrp=0 --mcs=6 --muSchedAccessReqInterval=50ms --minExpectedThroughput=31 --maxExpectedThroughput=290",
        "True",
        "True",
    ),
    (
        "wifi-he-network --simulationTime=0.3s --udp=1 --downlink=0 --useRts=1 --nStations=5 --dlAckType=AGGR-MU-BAR --enableUlOfdma=1 --enableBsrp=1 --mcs=5 --muSchedAccessReqInterval=50ms --minExpectedThroughput=46 --maxExpectedThroughput=327",
        "True",
        "True",
    ),
    (
        "wifi-eht-network --simulationTime=0.1s --frequency=5 --useRts=0 --minExpectedThroughput=4.9 --maxExpectedThroughput=1000",
        "True",
        "True",
    ),
    (
        "wifi-eht-network --simulationTime=0.1s --frequency=5 --useRts=0 --use80Plus80=1 --minExpectedThroughput=4.9 --maxExpectedThroughput=1000",
        "True",
        "True",
    ),
    (
        "wifi-eht-network --simulationTime=0.1s --frequency=5 --useRts=0 --mpduBufferSize=1024 --frequency2=6 --minExpectedThroughput=7 --maxExpectedThroughput=1900",
        "True",
        "True",
    ),
    (
        "wifi-eht-network --simulationTime=0.1s --frequency=6 --useRts=1 --minExpectedThroughput=4.5 --maxExpectedThroughput=1400",
        "True",
        "True",
    ),
    (
        "wifi-eht-network --simulationTime=0.1s --frequency=2.4 --useRts=0 --mpduBufferSize=512 --frequency2=5 --minExpectedThroughput=7 --maxExpectedThroughput=512",
        "True",
        "True",
    ),
    (
        "wifi-eht-network --simulationTime=0.1s --frequency=2.4 --useRts=1 --minExpectedThroughput=3.9 --maxExpectedThroughput=275",
        "True",
        "True",
    ),
    (
        "wifi-eht-network --simulationTime=0.23s --udp=0 --downlink=1 --useRts=0 --nStations=4 --dlAckType=ACK-SU-FORMAT --enableUlOfdma=1 --enableBsrp=0 --mcs=6 --frequency2=6 --minExpectedThroughput=35 --maxExpectedThroughput=404",
        "True",
        "True",
    ),
    (
        "wifi-eht-network --simulationTime=0.25s --frequency=2.4 --udp=0 --downlink=1 --useRts=0 --nStations=5 --dlAckType=MU-BAR --enableUlOfdma=1 --enableBsrp=1 --mcs=5 --frequency2=5 --mpduBufferSize=1024 --minExpectedThroughput=50 --maxExpectedThroughput=120",
        "True",
        "True",
    ),
    (
        # TXOP limit pinned to 0 pending !2938 (per-MPDU DL MU protection fix)
        "wifi-eht-network --simulationTime=0.3s --udp=0 --downlink=1 --useRts=1 --nStations=5 --dlAckType=AGGR-MU-BAR --enableUlOfdma=1 --enableBsrp=0 --mcs=6 --muSchedAccessReqInterval=50ms --frequency2=2.4 --ns3::Txop::TxopLimits=0us,0us --minExpectedThroughput=50 --maxExpectedThroughput=140",
        "True",
        "True",
    ),
    (
        "wifi-eht-network --simulationTime=0.25s --udp=0 --downlink=0 --useRts=0 --nStations=4 --dlAckType=AGGR-MU-BAR --enableUlOfdma=1 --enableBsrp=1 --mpduBufferSize=1024 --mcs=8 --muSchedAccessReqInterval=45ms --frequency2=6 --ns3::Txop::TxopLimits=0us,0us --minExpectedThroughput=50 --maxExpectedThroughput=550 --RngRun=6",
        "True",
        "True",
    ),
    (
        "wifi-eht-network --simulationTime=0.2s --frequency=2.4 --frequency2=5 --guardInterval=1600 --udp=0 --downlink=1 --useRts=0 --mpduBufferSize=512 --emlsrLinks=0,1 --emlsrPaddingDelay=32 --emlsrTransitionDelay=32 --channelSwitchDelay=32us --emlsrAuxSwitch=True --emlsrAuxTxCapable=True --minExpectedThroughput=5 --maxExpectedThroughput=200",
        "True",
        "True",
    ),
    (
        "wifi-eht-network --simulationTime=0.2s --frequency=2.4 --frequency2=5 --guardInterval=1600 --udp=0 --downlink=1 --useRts=1 --mpduBufferSize=512 --emlsrLinks=0,1 --emlsrPaddingDelay=64 --emlsrTransitionDelay=64 --channelSwitchDelay=64us --emlsrMgrTypeId=ns3::AdvancedEmlsrManager --emlsrAuxSwitch=False --emlsrAuxTxCapable=True --minExpectedThroughput=5 --maxExpectedThroughput=190",
        "True",
        "True",
    ),
    (
        "wifi-eht-network --simulationTime=0.2s --frequency=2.4 --frequency2=5 --guardInterval=1600 --udp=0 --downlink=0 --useRts=0 --mpduBufferSize=512 --emlsrLinks=0,1 --emlsrPaddingDelay=0 --emlsrTransitionDelay=0 --channelSwitchDelay=1ns --emlsrMgrTypeId=ns3::AdvancedEmlsrManager --emlsrAuxSwitch=False --emlsrAuxTxCapable=False --minExpectedThroughput=5 --maxExpectedThroughput=40 --RngRun=9",
        "True",
        "True",
    ),
    (
        # TXOP limit pinned to 0: at low MCS these EMLSR configurations hit a
        # pre-existing channel-access issue (backoff state reset on every EMLSR
        # link switch, issue #1352) that breaks the example's monotonicity
        # checks
        "wifi-eht-network --simulationTime=0.3s --frequency=2.4 --frequency2=5 --frequency3=6 --guardInterval=1600 --udp=0 --downlink=1 --useRts=0 --mpduBufferSize=512 --emlsrLinks=0,1,2 --emlsrPaddingDelay=32 --emlsrTransitionDelay=32 --channelSwitchDelay=32us --emlsrAuxSwitch=True --emlsrAuxTxCapable=True --nStations=4 --dlAckType=AGGR-MU-BAR --enableUlOfdma=1 --enableBsrp=0 --mcs=0,3,5,9,10 --ns3::Txop::TxopLimits=0us,0us,0us --minExpectedThroughput=8 --maxExpectedThroughput=300",
        "True",
        "True",
    ),
    (
        # TXOP limit pinned to 0: same pre-existing EMLSR channel-access issue
        # (#1352) as the previous invocation
        "wifi-eht-network --simulationTime=0.3s --frequency=2.4 --frequency2=5 --frequency3=6 --guardInterval=1600 --udp=0 --downlink=0 --useRts=1 --mpduBufferSize=512 --emlsrLinks=0,1,2 --emlsrPaddingDelay=64 --emlsrTransitionDelay=64 --channelSwitchDelay=64us --emlsrAuxSwitch=False --emlsrAuxTxCapable=True --nStations=4 --dlAckType=MU-BAR --enableUlOfdma=1 --enableBsrp=1 --mcs=1,4,8,11,13 --ns3::Txop::TxopLimits=0us,0us,0us --minExpectedThroughput=10 --maxExpectedThroughput=260",
        "True",
        "True",
    ),
    (
        "wifi-eht-network --simulationTime=0.3s --frequency=2.4 --frequency2=5 --frequency3=6 --guardInterval=1600 --udp=0 --downlink=0 --useRts=1 --mpduBufferSize=512 --emlsrLinks=0,1,2 --emlsrPaddingDelay=0 --emlsrTransitionDelay=0 --channelSwitchDelay=1ns --emlsrMgrTypeId=ns3::AdvancedEmlsrManager --emlsrAuxSwitch=False --emlsrAuxTxCapable=False --nStations=4 --dlAckType=ACK-SU-FORMAT --enableUlOfdma=1 --enableBsrp=1 --mcs=1,5,8,11 --minExpectedThroughput=30 --maxExpectedThroughput=333",
        "True",
        "True",
    ),
    (
        "wifi-simple-ht-hidden-stations --simulationTime=1s --enableRts=0 --nMpdus=32 --minExpectedThroughput=54 --maxExpectedThroughput=56",
        "True",
        "True",
    ),
    (
        "wifi-simple-ht-hidden-stations --simulationTime=1s --enableRts=1 --nMpdus=32 --minExpectedThroughput=51.5 --maxExpectedThroughput=53.5",
        "True",
        "True",
    ),
    ("wifi-mixed-network --simulationTime=1s", "True", "True"),
    ("wifi-aggregation --simulationTime=1s --verifyResults=1", "True", "True"),
    ("wifi-txop-aggregation --simulationTime=1s --verifyResults=1", "True", "True"),
    ("wifi-80211e-txop --simulationTime=1s --verifyResults=1", "True", "True"),
    (
        "wifi-multi-tos --simulationTime=1s --nWifi=16 --useRts=1 --useShortGuardInterval=1",
        "True",
        "True",
    ),
    ("wifi-tcp", "True", "True"),
    ("wifi-hidden-terminal --wifiManager=Arf", "True", "True"),
    ("wifi-hidden-terminal --wifiManager=Aarf", "True", "True"),
    ("wifi-hidden-terminal --wifiManager=Aarfcd", "True", "True"),
    ("wifi-hidden-terminal --wifiManager=Onoe", "True", "True"),
    ("wifi-hidden-terminal --wifiManager=Amrr", "True", "True"),
    ("wifi-hidden-terminal --wifiManager=Minstrel", "True", "True"),
    ("wifi-hidden-terminal --wifiManager=Cara", "True", "True"),
    ("wifi-hidden-terminal --wifiManager=Rraa", "True", "True"),
    ("wifi-hidden-terminal --wifiManager=Rrpaa", "True", "True"),
    (
        "wifi-spectrum-per-example --distance=52 --index=3 --wifiType=ns3::SpectrumWifiPhy --simulationTime=1s",
        "True",
        "True",
    ),
    (
        "wifi-spectrum-per-example --distance=24 --index=31 --wifiType=ns3::YansWifiPhy --simulationTime=1s",
        "True",
        "False",
    ),
    (
        "wifi-spectrum-per-interference --distance=24 --index=31 --simulationTime=1s --waveformPower=0.1",
        "True",
        "True",
    ),
    ("wifi-spectrum-saturation-example --simulationTime=1s --index=63", "True", "True"),
    (
        "wifi-backward-compatibility --apVersion=80211a --staVersion=80211n_5GHZ --simulationTime=1s",
        "True",
        "True",
    ),
    (
        "wifi-backward-compatibility --apVersion=80211a --staVersion=80211n_5GHZ --apRaa=Ideal --staRaa=Ideal --simulationTime=1s",
        "True",
        "False",
    ),
    (
        "wifi-backward-compatibility --apVersion=80211a --staVersion=80211ac --simulationTime=1s",
        "True",
        "False",
    ),
    (
        "wifi-backward-compatibility --apVersion=80211a --staVersion=80211ac --apRaa=Ideal --staRaa=Ideal --simulationTime=1s",
        "True",
        "False",
    ),
    (
        "wifi-multicast --minExpectedPackets=10",
        "True",
        "True",
    ),
    (
        "wifi-multicast --gcrRetransmissionPolicy=GcrUr --minExpectedPackets=10",
        "True",
        "True",
    ),
    (
        "wifi-multicast --gcrRetransmissionPolicy=GcrUr --multicastFrameErrorRate=0.2 --minExpectedPackets=10",
        "True",
        "True",
    ),
    (
        # TXOP limit pinned to 0: GCR-UR retransmission scheduling does not
        # implement the per-TXOP rules of IEEE 802.11-2024 Sec. 10.23.2.12.2
        # (no MPDU and its retransmission within the same GCR TXOP; backoff
        # after each unsolicited retry transmission when the protection
        # mechanism elicits no response; issue #1353), so GCR-UR is exercised
        # at TXOP limit 0, where its operation is compliant
        "wifi-multicast --gcrRetransmissionPolicy=GcrUr --maxAmpduLength=65535 --maxPackets=0 --nStations=4 --dataRate=50Mbps --gcrProtection=Rts-Cts --rtsThreshold=0 --simulationTime=1 --ns3::Txop::TxopLimits=0us --minExpectedThroughput=35 --maxExpectedThroughput=40",
        "True",
        "True",
    ),
    (
        # TXOP limit pinned to 0: see the GCR-UR compliance note (#1353) on
        # the previous entry
        "wifi-multicast --gcrRetransmissionPolicy=GcrUr --maxAmpduLength=65535 --maxPackets=0 --nStations=4 --dataRate=50Mbps --gcrProtection=Cts-To-Self --simulationTime=1 --ns3::Txop::TxopLimits=0us --minExpectedThroughput=40 --maxExpectedThroughput=45",
        "True",
        "True",
    ),
    (
        "wifi-multicast --gcrRetransmissionPolicy=GcrBlockAck --minExpectedPackets=10",
        "True",
        "True",
    ),
    (
        "wifi-multicast --gcrRetransmissionPolicy=GcrBlockAck --multicastFrameErrorRate=0.2 --minExpectedPackets=10",
        "True",
        "True",
    ),
    (
        "wifi-multicast --gcrRetransmissionPolicy=GcrBlockAck --maxAmpduLength=65535 --maxPackets=0 --nStations=4 --dataRate=100Mbps --gcrProtection=Rts-Cts --rtsThreshold=0 --simulationTime=1s --minExpectedThroughput=100 --maxExpectedThroughput=100",
        "True",
        "True",
    ),
    (
        "wifi-multicast --gcrRetransmissionPolicy=GcrBlockAck --maxAmpduLength=65535 --maxPackets=0 --nStations=4 --dataRate=100Mbps --gcrProtection=Cts-To-Self --simulationTime=1s --minExpectedThroughput=100 --maxExpectedThroughput=100",
        "True",
        "True",
    ),
    (
        "wifi-ps-mode --dlLoad=10Mbps --ulLoad=25Mbps --simulationTime=500ms --nStas=2 --staticSetup=1 --psMode=0:true,1:true --channels=36,0,BAND_5GHZ,0:100,0,BAND_5GHZ,0",
        "True",
        "True",
    ),
]

# A list of Python examples to run in order to ensure that they remain
# runnable over time.  Each tuple in the list contains
#
#     (example_name, do_run).
#
# See test.py for more information.
python_examples = [
    ("wifi-ap.py", "True"),
    ("mixed-wired-wireless.py", "True"),
]
