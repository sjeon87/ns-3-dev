build_lib_example(
  NAME aodvv2
  SOURCE_FILES aodvv2.cc
  LIBRARIES_TO_LINK
    ${libwifi}
    ${libinternet}
    ${libaodvv2}
    ${libinternet-apps}
)
