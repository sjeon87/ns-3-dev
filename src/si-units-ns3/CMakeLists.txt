# si-units-ns3 module: ns-3 integration for third-party/si-units. Provides
# attribute system support and Time integration

set(source_files
    model/si-units-attributes.cc
)

set(header_files
    model/si-units-ns3.h
    model/si-units-attributes.h
    model/units-frequency-nstime.h
)

build_lib(
  LIBNAME si-units-ns3
  SOURCE_FILES ${source_files}
  HEADER_FILES ${header_files}
  LIBRARIES_TO_LINK ${libcore}
                    si-units # Link to third-party si-units library
  TEST_SOURCES test/si-units-attributes-test-suite.cc
               test/si-units-nstime-test-suite.cc
)
