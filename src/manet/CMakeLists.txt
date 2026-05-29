build_lib(
  LIBNAME manet
  SOURCE_FILES
    helper/nhdp-helper.cc
    model/nhdp-info-base.cc
    model/nhdp-client.cc
    model/nhdp-hello-tag.cc
    model/time-tlv.cc
  HEADER_FILES
    helper/nhdp-helper.h
    model/nhdp-info-base.h
    model/nhdp-client.h
    model/nhdp-hello-tag.h
    model/time-tlv.h
  LIBRARIES_TO_LINK ${libinternet}
                    ${libnetwork}
                    ${libcore}
)
