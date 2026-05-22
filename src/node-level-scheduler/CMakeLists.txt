set(source_files
    model/local-clock.cc
    model/scheduler-clock.cc
    model/node-level-scheduler.cc
    helper/local-clock-helper.cc
)

set(header_files
    model/local-clock.h
    model/scheduler-clock.h
    model/node-level-scheduler.h
    helper/local-clock-helper.h
)

build_lib(
  LIBNAME node-level-scheduler
  SOURCE_FILES ${source_files}
  HEADER_FILES ${header_files}
  LIBRARIES_TO_LINK ${libcore}
                    ${libnetwork}
  TEST_SOURCES test/node-level-scheduler-test-suite.cc
)
