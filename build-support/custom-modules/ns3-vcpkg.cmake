# SPDX-License-Identifier: GPL-2.0-only

include_guard(GLOBAL)
option(NS3_VCPKG "Enable vcpkg manifest dependency management" OFF)

# An explicitly supplied vcpkg toolchain also enables the ns-3 integration.
if(DEFINED CMAKE_TOOLCHAIN_FILE AND CMAKE_TOOLCHAIN_FILE MATCHES
                                    "vcpkg\\.cmake$"
)
  set(NS3_VCPKG ON CACHE BOOL "Enable vcpkg manifest dependency management"
                         FORCE
  )
endif()
if(NOT NS3_VCPKG)
  return()
endif()

get_filename_component(
  NS3_VCPKG_SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}/../.." ABSOLUTE
)
if(NOT DEFINED VCPKG_MANIFEST_DIR)
  set(VCPKG_MANIFEST_DIR "${NS3_VCPKG_SOURCE_DIR}" CACHE PATH
                                                         "Shared ns-3 manifest"
  )
endif()

if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
  set(NS3_VCPKG_ROOT "$ENV{VCPKG_ROOT}" CACHE PATH "vcpkg SDK directory")
  if(NOT NS3_VCPKG_ROOT)
    set(NS3_VCPKG_ROOT "${NS3_VCPKG_SOURCE_DIR}/vcpkg"
        CACHE PATH "vcpkg SDK directory" FORCE
    )
  endif()
  find_package(Python3 REQUIRED COMPONENTS Interpreter)
  execute_process(
    COMMAND
      "${Python3_EXECUTABLE}" "${NS3_VCPKG_SOURCE_DIR}/utils/setup-vcpkg.py"
      --root "${NS3_VCPKG_ROOT}" --no-submodules
    RESULT_VARIABLE ns3_vcpkg_result
  )
  if(NOT ns3_vcpkg_result EQUAL 0)
    message(FATAL_ERROR "vcpkg setup failed (${ns3_vcpkg_result})")
  endif()
  set(CMAKE_TOOLCHAIN_FILE "${NS3_VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake"
      CACHE FILEPATH "vcpkg toolchain"
  )
elseif(NOT CMAKE_TOOLCHAIN_FILE MATCHES "vcpkg\\.cmake$")
  message(
    FATAL_ERROR
      "NS3_VCPKG requires the vcpkg toolchain. Set VCPKG_CHAINLOAD_TOOLCHAIN_FILE "
      "to your existing toolchain and CMAKE_TOOLCHAIN_FILE to vcpkg.cmake."
  )
endif()

# Let vcpkg choose the architecture, OS and ABI unless the caller specifies one.
if(MINGW OR (CMAKE_HOST_WIN32 AND "$ENV{MSYSTEM}" MATCHES "MINGW|CLANG|UCRT"))
  if(NOT DEFINED VCPKG_TARGET_TRIPLET)
    set(VCPKG_TARGET_TRIPLET x64-mingw-dynamic CACHE STRING
                                                     "vcpkg target triplet"
    )
  endif()
  if(NOT DEFINED VCPKG_HOST_TRIPLET)
    set(VCPKG_HOST_TRIPLET x64-mingw-dynamic CACHE STRING "vcpkg host triplet")
  endif()
endif()
if(TEST_PACKAGE_MANAGER STREQUAL "VCPKG")
  list(APPEND VCPKG_MANIFEST_FEATURES package-manager-test)
endif()
if(NS3_MPI)
  list(APPEND VCPKG_MANIFEST_FEATURES mpi)
endif()
if(NS3_DPDK)
  list(APPEND VCPKG_MANIFEST_FEATURES dpdk)
endif()
if(NS3_NETANIM)
  list(APPEND VCPKG_MANIFEST_FEATURES netanim)
endif()
