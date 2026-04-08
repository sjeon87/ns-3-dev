build_lib_example(
  NAME bundle-protocol-example
  SOURCE_FILES bundle-protocol-example.cc
  LIBRARIES_TO_LINK
    ${libcore}
    ${libnetwork}
    ${libpoint-to-point}
    ${libinternet}
    ${libapplications}
)
