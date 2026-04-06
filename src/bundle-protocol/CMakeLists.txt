build_lib(
  LIBNAME bp
  SOURCE_FILES
    model/bundle-header.cc
    model/bundle-agent.cc
  HEADER_FILES
    model/bundle-header.h
    model/bundle-agent.h
  LIBRARIES_TO_LINK
  TEST_SOURCES
)
