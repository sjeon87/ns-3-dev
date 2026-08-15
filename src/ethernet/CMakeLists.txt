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
)
