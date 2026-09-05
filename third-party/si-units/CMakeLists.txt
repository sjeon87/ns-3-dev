cmake_minimum_required(VERSION 3.14)
project(si-units VERSION 1.0.0 LANGUAGES CXX)

# Set C++20 standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Create the main si-units library (header-only interface + parser implementation)
add_library(si-units
    model/si-units-parser.cc
)

# Set include directories
# For build tree: expose model/ directory directly so headers can be included as <si-units>
target_include_directories(si-units
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/model>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

# Set library properties
set_target_properties(si-units PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
    POSITION_INDEPENDENT_CODE ON
)

# Optional: Enable testing
option(SI_UNITS_BUILD_TESTS "Build si-units tests" ON)

if(SI_UNITS_BUILD_TESTS)
    enable_testing()

    add_executable(test-si-units
        test/si-units-doctest-main.cc
        model/si-units-parser.cc
    )

    target_include_directories(test-si-units PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/model
        ${CMAKE_CURRENT_SOURCE_DIR}/test
    )

    set_target_properties(test-si-units PROPERTIES
        CXX_STANDARD 20
        CXX_STANDARD_REQUIRED ON
    )

    add_test(NAME si-units-tests COMMAND test-si-units)
endif()

# Installation rules
include(GNUInstallDirs)

install(TARGETS si-units
    EXPORT si-units-targets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# Install headers from model/ directory
install(DIRECTORY model/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    FILES_MATCHING PATTERN "*.h"
)

# Install the main si-units header (no extension)
install(FILES include/si-units
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

# Install export targets
install(EXPORT si-units-targets
    FILE si-units-targets.cmake
    NAMESPACE si-units::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/si-units
)

# Create and install package config file
include(CMakePackageConfigHelpers)

configure_package_config_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/si-units-config.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/si-units-config.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/si-units
)

write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/si-units-config-version.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/si-units-config.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/si-units-config-version.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/si-units
)
