.. include:: replace.txt
.. highlight:: cpp

SI Units ns-3 Integration
--------------------------

Overview
********

The ``si-units-ns3`` module provides ns-3-specific extensions for the SI Units library.
For comprehensive documentation about the SI Units library itself, including design philosophy,
implementation patterns, and migration strategies, please refer to the primary documentation
in ``third-party/si-units/doc/si-units.rst``.

This document focuses solely on the ns-3-specific integration features:

* Integration with the |ns3| attribute system
* Interoperability with |ns3| ``Time`` class

Architecture
************

The SI Units implementation in |ns3| is split into two components:

**third-party/si-units/**
  The core SI Units library - a standalone, portable implementation with no |ns3| dependencies.
  This library can be used independently for basic SI units functionality.

**src/si-units-ns3/**
  The |ns3| integration layer that depends on both the core SI Units library and |ns3|'s
  ``libcore`` module. This module provides attribute system support and Time interoperability.

This separation ensures:

* The core library remains testable independently
* Modules can use basic SI units without pulling in |ns3| dependencies
* ns-3-specific features are available when needed

Usage
*****

**Basic SI Units (no ns-3 dependencies):**

::

  #include <si-units>

  auto freq = 2.4_GHz;
  auto power = 20_dBm;

**With ns-3 Integration:**

::

  #include "ns3/si-units-ns3.h"

  // Now you have access to attribute system support and Time integration

Attribute System Integration
*****************************

The ``si-units-ns3`` module provides macros to integrate SI units with the |ns3| attribute system.
For each unit type, the following are available:

* ``<Unit>Value(value)`` - Creates an AttributeValue wrapper
* ``Make<Unit>Accessor(...)`` - Creates an AttributeAccessor
* ``Make<Unit>Checker()`` - Creates an AttributeChecker

Example: Adding a Power Attribute
==================================

::

  class WifiPhy : public Object
  {
  public:
    static TypeId GetTypeId()
    {
      static TypeId tid = TypeId("ns3::WifiPhy")
        .SetParent<Object>()
        .AddAttribute("TxPower",
                      "Transmission power in dBm",
                      dBmValue(20_dBm),
                      MakedBmAccessor(&WifiPhy::m_txPower),
                      MakedBmChecker())
        .AddAttribute("Frequency",
                      "Operating frequency",
                      MHzValue(2400_MHz),
                      MakeMHzAccessor(&WifiPhy::m_frequency),
                      MakeMHzChecker());
      return tid;
    }

  private:
    dBm_t m_txPower{20_dBm};
    MHz_t m_frequency{2400_MHz};
  };

Supported Unit Types for Attributes
====================================

The following SI unit types have attribute system support:

**Angle:**
  * ``degree_t`` - Degrees
  * ``radian_t`` - Radians

**Frequency:**
  * ``Hz_t`` - Hertz
  * ``kHz_t`` - Kilohertz
  * ``MHz_t`` - Megahertz
  * ``GHz_t`` - Gigahertz
  * ``THz_t`` - Terahertz

**Power:**
  * ``dB_t`` - Decibels (relative power)
  * ``dBr_t`` - Decibel ratio
  * ``dBm_t`` - Decibel-milliwatts
  * ``mWatt_t`` - Milliwatts
  * ``Watt_t`` - Watts
  * ``dBm_per_Hz_t`` - Spectral density (dBm per Hz)
  * ``dBm_per_MHz_t`` - Spectral density (dBm per MHz)

**Ratio:**
  * ``percent_t`` - Percentage

**Time:**
  * ``nSEC_t`` - Nanoseconds (with support for larger time scales)

Time Integration
****************

The ``si-units-ns3`` module provides operators for multiplying frequency units with |ns3| ``Time``
objects, enabling natural expressions of sample counts and similar operations.

Frequency × Time Operators
===========================

::

  #include "ns3/si-units-ns3.h"
  #include "ns3/nstime.h"

  using namespace ns3;
  using namespace si_units;

  // Calculate number of samples in a time duration
  auto freq = 2.4_GHz;
  auto duration = MilliSeconds(10);
  double samples = freq * duration;  // Returns unitless double

  // Order doesn't matter
  double samples2 = duration * freq;  // Same result

The multiplication of ``Hz_t`` and ``Time`` returns a dimensionless ``double`` representing
the number of cycles or samples in the given time period.

Implementation Details
======================

The operator is defined in ``units-frequency-nstime.h``:

::

  inline double operator*(const Hz_t& lhs, ns3::Time nstime)
  {
      return (lhs.val * nstime.GetNanoSeconds()) / ONE_GIGA;
  }

  inline double operator*(ns3::Time nstime, const Hz_t& rhs)
  {
      return rhs * nstime;
  }

The implementation uses 64-bit double precision, which provides sufficient range for typical
|ns3| simulations (up to 1.8e289 Hz, far exceeding practical requirements).

Testing
*******

The ``si-units-ns3`` module includes regression tests for:

* Attribute system integration (``si-units-attributes-test-suite.cc``)
* Time interoperability (``si-units-nstime-test-suite.cc``)

Basic SI units functionality is tested in the core library at ``third-party/si-units/``.

To run the ns-3 integration tests:

::

  $ ./test.py -s si-units-attributes-test
  $ ./test.py -s si-units-nstime-test

Configuration Files and Command-Line Arguments
**********************************************

The attribute system integration enables SI units to be used consistently from configuration
files to command-line arguments:

::

  # In a configuration file
  default TxPower "23dBm"
  default Frequency "5800MHz"

  # From command line
  ./ns3 run "my-simulation --TxPower=23dBm --Frequency=5800MHz"

The attribute system automatically parses the string representations using the unit parsing
capabilities built into each SI unit type.

See Also
********

* Primary SI Units documentation: ``third-party/si-units/doc/si-units.rst``
* Core SI Units library: ``third-party/si-units/``
* SI Units header: ``<si-units>``
* ns-3 integration header: ``"ns3/si-units-ns3.h"``
