# performance-test.cc compares EthernetNetDevice against CsmaNetDevice and
# PointToPointNetDevice over an IPv4 internet stack, so it is built only when
# the modules providing the other devices, the stack and the traffic generators
# are enabled as well.
set(ethernet_performance_test_sources)
set(ethernet_performance_test_libs)
if((applications
    IN_LIST
    ns3-all-enabled-modules
   )
   AND (csma
        IN_LIST
        ns3-all-enabled-modules
       )
   AND (internet
        IN_LIST
        ns3-all-enabled-modules
       )
   AND (internet-apps
        IN_LIST
        ns3-all-enabled-modules
       )
   AND (point-to-point
        IN_LIST
        ns3-all-enabled-modules
       )
)
  set(ethernet_performance_test_sources
      test/performance-test.cc
  )
  set(ethernet_performance_test_libs
      ${libapplications}
      ${libcsma}
      ${libinternet}
      ${libinternet-apps}
      ${libpoint-to-point}
      ${libtraffic-control}
  )
endif()

build_lib(
  LIBNAME ethernet
  SOURCE_FILES
    model/ethernet-channel.cc
    model/ethernet-net-device.cc
    model/ethernet-phy.cc
    model/ethernet-mac.cc
    model/ethernet-flow-control-header.cc
    helper/ethernet-helper.cc
  HEADER_FILES
    model/ethernet-channel.h
    model/ethernet-net-device.h
    model/ethernet-phy.h
    model/ethernet-mac.h
    model/ethernet-flow-control-header.h
    helper/ethernet-helper.h
  LIBRARIES_TO_LINK ${libnetwork}
  TEST_SOURCES
    test/ethernet-full-duplex-test.cc
    test/ethernet-ifg-test.cc
    test/ethernet-link-speed-test.cc
    test/ethernet-flow-control-test.cc
    ${ethernet_performance_test_sources}
  TEST_LIBRARIES_TO_LINK ${ethernet_performance_test_libs}
)
