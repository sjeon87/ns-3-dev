build_lib(
  LIBNAME aodvv2
  SOURCE_FILES
    helper/aodvv2-helper.cc
    model/aodvv2-dpd.cc
    model/aodvv2-id-cache.cc
    model/aodvv2-neighbor-set.cc
    model/aodvv2-packet.cc
    model/aodvv2-routing-protocol.cc
    model/aodvv2-rqueue.cc
    model/aodvv2-local-route-set.cc
    model/aodvv2-rerr-set.cc
    model/aodvv2-route-client-set.cc
  HEADER_FILES
    helper/aodvv2-helper.h
    model/aodvv2-dpd.h
    model/aodvv2-id-cache.h
    model/aodvv2-neighbor-set.h
    model/aodvv2-packet.h
    model/aodvv2-routing-protocol.h
    model/aodvv2-rqueue.h
    model/aodvv2-local-route-set.h
    model/aodvv2-rerr-set.h
    model/aodvv2-route-client-set.h
  LIBRARIES_TO_LINK ${libinternet}
  TEST_SOURCES test/aodvv2-test.cc
)
