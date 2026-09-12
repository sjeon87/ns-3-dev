set(base_examples
    three-gpp-http-example
    traffic-generator-example
)
foreach(
  example
  ${base_examples}
)
  build_lib_example(
    NAME ${example}
    SOURCE_FILES ${example}.cc
    LIBRARIES_TO_LINK
      ${libapplications}
      ${libpoint-to-point}
      ${libinternet}
      ${libnetwork}
      ${libinternet-apps}
  )
endforeach()

build_lib_example(
  NAME tgax-video-example
  SOURCE_FILES tgax-video-example.cc
  LIBRARIES_TO_LINK
    ${libapplications}
    ${libwifi}
    ${libmobility}
    ${libinternet}
    ${libnetwork}
)

build_lib_example(
  NAME tgax-voip-example
  SOURCE_FILES tgax-voip-example.cc
  LIBRARIES_TO_LINK
    ${libapplications}
    ${libwifi}
    ${libmobility}
    ${libinternet}
    ${libnetwork}
)

build_lib_example(
  NAME tgax-virtual-desktop-example
  SOURCE_FILES tgax-virtual-desktop-example.cc
  LIBRARIES_TO_LINK
    ${libapplications}
    ${libwifi}
    ${libmobility}
    ${libinternet}
    ${libnetwork}
)

build_lib_example(
  NAME rta-tig-mobile-gaming-example
  SOURCE_FILES rta-tig-mobile-gaming-example.cc
  LIBRARIES_TO_LINK
    ${libapplications}
    ${libwifi}
    ${libmobility}
    ${libinternet}
    ${libnetwork}
)
