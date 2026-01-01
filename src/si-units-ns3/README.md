# si-units-ns3: ns-3 Integration Module

This module provides ns-3-specific extensions for the SI Units library located in `third-party/si-units/`.

## Purpose

The si-units-ns3 module acts as a bridge between the standalone SI Units library and ns-3's infrastructure:

1. **Attribute System Integration** (`si-units-attributes.h/cc`)
   - Provides ATTRIBUTE_VALUE, ATTRIBUTE_ACCESSOR, and ATTRIBUTE_CHECKER macros
   - Enables SI units to be used as ns-3 object attributes
   - Supports all unit types: angle, frequency, power, ratio, time

1. **ns-3 Time Integration** (`units-frequency-nstime.h`)
   - Provides operators for multiplying frequency units with ns-3 Time objects
   - Example: `auto samples = 2.4_GHz * MilliSeconds(10)`

## Usage

### Basic SI Units (no ns-3 dependencies)

```cpp
#include <si-units>

auto freq = 2.4_GHz;
auto power = 20_dBm;
```

### With ns-3 Integration

```cpp
#include "ns3/si-units-ns3.h"

// Use in attribute system
.AddAttribute("TxPower",
              "Transmission power",
              dBmValue(20_dBm),
              MakedBmAccessor(&MyClass::m_txPower),
              MakedBmChecker())

// Use with ns-3 Time
auto samples = freq * MilliSeconds(100);
```

## Architecture

- **third-party/si-units/**: Core SI units library (header-only, no ns-3 dependencies)
- **src/si-units-ns3/**: ns-3 integration layer (depends on libcore and third-party/si-units)

This separation ensures:

- The core SI units library remains portable and testable independently
- ns-3 modules can use basic SI units without pulling in ns-3 dependencies
- ns-3-specific features are available when needed via si-units-ns3 module

## Testing

The module includes regression tests for:

- Attribute system integration (`si-units-attributes-test-suite.cc`)
- ns-3 Time interoperability (`si-units-nstime-test-suite.cc`)

Basic SI units functionality is tested in `third-party/si-units/` via its own test suite.
