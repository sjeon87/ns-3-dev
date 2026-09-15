build_lib_example(
  NAME ethernet-ping
  SOURCE_FILES ethernet-ping.cc
  LIBRARIES_TO_LINK
    ${libethernet}
    ${libinternet}
    ${libinternet-apps}
    ${libapplications}
)

build_lib_example(
  NAME ethernet-switch-ping
  SOURCE_FILES ethernet-switch-ping.cc
  LIBRARIES_TO_LINK
    ${libethernet}
    ${libinternet}
    ${libinternet-apps}
    ${libapplications}
)
