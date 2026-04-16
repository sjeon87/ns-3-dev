build_lib_example(
  NAME mars-relay-network-example
  SOURCE_FILES mars-relay-network-example.cc
  LIBRARIES_TO_LINK
    ${libcore}
    ${libnetwork}
    ${libpoint-to-point}
    ${libinternet}
    ${libapplications}
    ${libcsma}
)
