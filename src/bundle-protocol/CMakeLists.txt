build_lib(
  LIBNAME bundle-protocol
  SOURCE_FILES
    model/bp-cla-protocol.cc
    model/bp-tcp-cla-protocol.cc
    model/bp-endpoint-id.cc
    model/bp-header.cc
    model/bp-payload-header.cc
    model/bundle-protocol.cc
    model/bp-routing-protocol.cc
    model/bp-static-routing-protocol.cc
    model/sdnv.cc
    helper/bundle-protocol-helper.cc
    helper/bundle-protocol-container.cc
    model/ltp-header.cc
    model/ltp-convergence-layer-adapter.cc

  HEADER_FILES
    model/bp-cla-protocol.h
    model/bp-tcp-cla-protocol.h
    model/bp-endpoint-id.h
    model/bp-header.h
    model/bp-payload-header.h
    model/bundle-protocol.h
    model/bp-routing-protocol.h
    model/bp-static-routing-protocol.h
    model/sdnv.h
    helper/bundle-protocol-helper.h
    helper/bundle-protocol-container.h
    model/ltp-header.h
    model/ltp-convergence-layer-adapter.h

  LIBRARIES_TO_LINK
    ${libcore}
    ${libnetwork}
    ${libinternet}

  TEST_SOURCES
    test/bundle-protocol-test-suite.cc
    test/ltp-protocol-test-suite.cc
    test/ltp-protocol-channel-loss-test-suite.cc
)
