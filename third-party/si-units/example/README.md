# si-units Example

This example demonstrates how to use the si-units library in your own CMake project using `add_subdirectory()`.

## Building and Running

```bash
./build.sh
```

This will:

1. Configure the project with CMake
1. Build both the si-units library and the example
1. Run the example program

## Usage in Your Own Project

To use si-units in your own CMake project:

### 1. Add si-units as a subdirectory

In your `CMakeLists.txt`:

```cmake
cmake_minimum_required(VERSION 3.14)
project(your-project CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add si-units library
add_subdirectory(path/to/si-units si-units-build)

# Create your executable
add_executable(your-app main.cpp)

# Link against si-units
target_link_libraries(your-app PRIVATE si-units)
```

### 2. Include and use in your code

```cpp
#include <si-units/si-units.h>

int main() {
    auto power = 20_dBm;
    auto freq = 2.4_GHz;

    std::cout << "Power: " << power.str() << std::endl;
    std::cout << "Frequency: " << freq.str() << std::endl;

    return 0;
}
```

## What the Example Demonstrates

The example (`main.cpp`) shows:

- **Frequency units**: Creating and converting frequency values
- **Power units**: Working with both logarithmic (dBm, dB) and linear (mWatt) power units
- **Angle units**: Converting between degrees and radians
- **Time units**: Using nanosecond-based time with automatic formatting
- **Ratio units**: Working with percentages
- **Type safety**: How the library prevents invalid operations at compile time

## Key Features Demonstrated

1. **User-defined literals**: Natural syntax like `2.4_GHz`, `20_dBm`
1. **Unit conversion**: Methods like `to_dBm()` and `in_mWatt()`
1. **Type safety**: Compile-time prevention of incompatible unit operations
1. **Arithmetic operations**: Valid operations between compatible units
