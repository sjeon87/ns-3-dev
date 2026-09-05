# si-units Library

A type-safe C++20 implementation of SI units for network simulation.

## Building and Testing

To build the library and run all unit tests:

```bash
./run.sh
```

This will:

1. Configure the project with CMake
1. Build the si-units static library
1. Build the test suite
1. Run all tests using the doctest framework

To clean all build directories:

```bash
./run.sh clean
```

## Using in Your Project

### With CMake's `add_subdirectory()`

See the `example/` directory for a complete working example.

In your `CMakeLists.txt`:

```cmake
add_subdirectory(path/to/si-units si-units-build)
target_link_libraries(your-target PRIVATE si-units)
```

In your code:

```cpp
#include <si-units>

auto power = 20_dBm;
auto freq = 2.4_GHz;
```

### Manual Compilation

```bash
clang++ -std=c++20 your_file.cpp si-units/model/si-units-parser.cc \
  -I si-units/model -I si-units/include -o your_program
```

## Running the Example

```bash
cd example
./run.sh
```

## Requirements

- C++20 compiler (clang++ or g++)
- CMake 3.14 or later (for building with CMake)
- No external dependencies

## Testing

The library uses the doctest testing framework. All tests are included in:

- `test/si-units-doctest.cc` - Test cases
- `test/si-units-doctest-main.cc` - Test runner

## Documentation

See `doc/si-units.rst` for comprehensive documentation.

## License

MIT License - See LICENSE.txt for details.
