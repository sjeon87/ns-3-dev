# SPDX-License-Identifier: MIT

# Based on the public vcpkg port at the repository builtin-baseline. Python
# stays in the host environment for ns-3 Python applications.
vcpkg_from_github(
  OUT_SOURCE_PATH
  SOURCE_PATH
  REPO
  pybind/pybind11
  REF
  "v${VERSION}"
  SHA512
  a79f7ed52d23c27325f37c50926bd0d0561aafefcf847219ed1378ba0a5a94479e15943e4da49117606695f40762b21ddcc8c32a6b8be61afc6de503c635af74
)
vcpkg_cmake_configure(
  SOURCE_PATH "${SOURCE_PATH}" OPTIONS -DPYBIND11_TEST=OFF
  -DPYBIND11_NOPYTHON=ON
)
vcpkg_cmake_install()
vcpkg_cmake_config_fixup(CONFIG_PATH "share/cmake/pybind11")
vcpkg_fixup_pkgconfig()
file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug")
vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/LICENSE")
