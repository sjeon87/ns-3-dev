cmake_minimum_required(VERSION 3.14)
project(my-project-using-si-units CXX)

# Set C++20 standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add si-units library using add_subdirectory
# Note: binary_dir (2nd parameter) is REQUIRED because the source directory (..)
# is not a subdirectory of the current directory (out-of-tree source)
add_subdirectory(.. si-units-build)

# Create example executable
add_executable(my-project-using-si-units main.cpp)

# Link against si-units library
target_link_libraries(my-project-using-si-units PRIVATE si-units)

# The include path is automatically handled by si-units target
