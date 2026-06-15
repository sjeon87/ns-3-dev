set(source_files
    model/epoch-table.cc
    model/local-clock.cc
    model/scheduler-clock.cc
    model/static-skew-scheduler.cc
    model/dynamic-skew-scheduler.cc
    helper/local-clock-helper.cc
)

set(header_files
    model/epoch-table.h
    model/local-clock.h
    model/scheduler-clock.h
    model/static-skew-scheduler.h
    model/dynamic-skew-scheduler.h
    helper/local-clock-helper.h
)

build_lib(
  LIBNAME node-level-scheduler
  SOURCE_FILES ${source_files}
  HEADER_FILES ${header_files}
  LIBRARIES_TO_LINK ${libcore}
                    ${libnetwork}
  TEST_SOURCES
    test/epoch-table-test-suite.cc
    test/static-skew-scheduler-test-suite.cc
    test/dynamic-skew-scheduler-test-suite.cc
)
