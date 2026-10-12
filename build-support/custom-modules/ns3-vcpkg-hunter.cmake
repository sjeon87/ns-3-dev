# SPDX-License-Identifier: GPL-2.0-only

# Compatibility entry point for modules using the former package-manager API.
# Manifest dependencies are installed by the toolchain before project().
function(add_package package_name)
  if(NOT NS3_VCPKG)
    message(STATUS "vcpkg support is disabled: ${package_name}")
    return()
  endif()
  string(TOLOWER "${package_name}" package_name)
  if(NOT EXISTS
     "${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/share/${package_name}"
  )
    message(
      FATAL_ERROR
        "vcpkg: ${package_name} is not installed. Add it to vcpkg.json or select "
        "the corresponding VCPKG_MANIFEST_FEATURES and reconfigure."
    )
  endif()
endfunction()
