build_lib_example(
  NAME bundle-protocol-simple
  SOURCE_FILES bundle-protocol-simple.cc
  LIBRARIES_TO_LINK
    ${libbundle-protocol}
    ${libpoint-to-point}
)

build_lib_example(
  NAME bundle-protocol-nocla
  SOURCE_FILES bundle-protocol-nocla.cc
  LIBRARIES_TO_LINK
    ${libbundle-protocol}
    ${libpoint-to-point}
)

build_lib_example(
  NAME ltp-protocol-example
  SOURCE_FILES ltp-protocol-example.cc
  LIBRARIES_TO_LINK
    ${libbundle-protocol}
    ${libcore}
    ${libinternet}
    ${libnetwork}
    ${libpoint-to-point}
)
