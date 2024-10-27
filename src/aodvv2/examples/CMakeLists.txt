build_lib_example(
  NAME aodvv2-example
  SOURCE_FILES aodvv2-example.cc
  LIBRARIES_TO_LINK
    ${libwifi}
    ${libinternet}
    ${libaodvv2}
    ${libinternet-apps}
)

build_lib_example(
  NAME aodvv2-multi-example
  SOURCE_FILES aodvv2-multi-example.cc
  LIBRARIES_TO_LINK
    ${libwifi}
    ${libinternet}
    ${libaodvv2}
    ${libinternet-apps}
)
