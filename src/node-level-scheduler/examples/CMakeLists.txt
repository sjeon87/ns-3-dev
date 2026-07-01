build_lib_example(
  NAME contikimac-example
  SOURCE_FILES contikimac-example.cc
  LIBRARIES_TO_LINK ${libcore}
                    ${libnetwork}
)

build_lib_example(
  NAME ntp-example
  SOURCE_FILES
    ntp-example.cc
    ntp-header.cc
    ntp-server.cc
    ntp-client.cc
  HEADER_FILES
    ntp-header.h
    ntp-server.h
    ntp-client.h
  LIBRARIES_TO_LINK
    ${libcore}
    ${libnetwork}
    ${libinternet}
    ${libpoint-to-point}
)
