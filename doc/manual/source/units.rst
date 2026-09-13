.. include:: replace.txt
.. highlight:: cpp


.. heading hierarchy:
   ------------- Chapter
   ************* Section (#.#)
   ============= Subsection (#.#.#)
   ############# Paragraph (no number)

Units
-----

For physical quantities such as time, length, power, etc., |ns3| uses
unit types rather than built-in data types, so as to avoid errors that
may arise from arithmetic operations on incompatible values.

The ``ns3::Time`` class is summarized in the documentation on the
|ns3| Event class.  This chapter documents all other strongly-typed
physical units and quantities, based on the `mp-units
<https://mpusz.github.io/mp-units/>`_ C++20 quantities and units library.

The example program ``src/core/examples/units-example.cc`` demonstrates the
features described below.  To run it::

    ./ns3 run units-example

Linear Units
************

mp-units uses a multiplication syntax to construct quantities::

    auto distance = 8.0 * m;
    auto freq = 5.0 * MHz;
    auto power = 100.0 * mW;

    std::cout << "distance = " << distance << std::endl;
    std::cout << "freq = "     << freq     << std::endl;
    std::cout << "power = "    << power    << std::endl;

Running the above code produces:

.. code-block:: none

    distance = 8 m
    freq = 5 MHz
    power = 100 mW

The key design feature is visible in the expression ``8.0 * m``: this means
literally "8.0 times the unit meter," yielding a quantity of dimension length.
The ``*`` is standard arithmetic multiplication, overloaded so that one operand
is a unit object rather than a dimensionless scalar.

This contrasts with the nholthaus units library, which uses user-defined
literals (UDLs) such as ``8.0_m``.  UDLs require every unit to be registered
as a literal suffix at compile time and cannot express derived or scaled units
as cleanly.  With mp-units you can write ``9.8 * m / (s * s)`` for acceleration
using only standard arithmetic operators.

Type Aliases
************

The ``auto`` keyword deduces verbose mp-units types such as::

    mp_units::quantity<mp_units::si::metre, double>
    mp_units::quantity<mp_units::si::mega<mp_units::si::hertz>, double>
    mp_units::quantity<mp_units::si::milli<mp_units::si::watt>, double>

|ns3| provides type aliases in namespace ``ns3`` for commonly used quantities::

    MHz_t  frequency{2400.0 * MHz};
    Watt_t txPower{1.0 * W};
    meter_t range{100.0 * m};

    std::cout << "frequency = " << frequency << std::endl;
    std::cout << "txPower = "   << txPower   << std::endl;
    std::cout << "range = "     << range     << std::endl;

Running the above code produces:

.. code-block:: none

    frequency = 2400 MHz
    txPower = 1 W
    range = 100 m

All |ns3| aliases fix the representation type to ``double``, consistent with
ns-3 conventions.

Additional Type Aliases
=======================

|ns3| also provides these additional aliases::

    scalar_t     ratio{d1 / d2};                  // dimensionless quantity
    degree_t     beamwidth{60.0 * deg};           // angle in degrees
    dBr_t        relativeLevel = 3.0 * dB;         // alias for dB_t (relative dB)
    mW_per_Hz_t  noisePsd{1.0e-9 * mW / Hz};     // linear PSD in mW/Hz
    mW_per_MHz_t channelPsd{1.0e-3 * mW / MHz};  // linear PSD in mW/MHz

``degree_t`` demonstrates an important detail about mp-units stream output.
All |ns3| mp-units aliases except ``degree_t`` use ASCII-safe unit symbols
(MHz, mW, Hz, W, m, etc.).  ``degree_t`` is the sole exception: its symbol is
U+00B0 DEGREE SIGN, and mp-units defaults ``operator<<`` to UTF-8 output::

    degree_t beamwidth{60.0 * deg};
    std::cout << "beamwidth = " << beamwidth;

Running the above produces (UTF-8 degree sign, no preceding space):

.. code-block:: none

    beamwidth = 60°

mp-units specializes ``space_before_unit_symbol<non_si::degree> = false``,
so no space is inserted between the numeric value and the degree symbol.

For ASCII-only output, use ``std::format`` with the ``::U[P]`` sub-entity spec,
which selects ``character_set::portable`` for the unit symbol::

    std::format("{::U[P]}", beamwidth)  // portable: no UTF-8 degree sign

Portable output:

.. code-block:: none

    60deg

``::U[P]`` is an mp-units format extension, not standard C++.  The outer ``:``
enters the defaults-specs block; ``U[P]`` applies format spec ``P`` (portable)
to the unit sub-entity.  Since ``space_before_unit_symbol<degree> = false``,
the portable output is ``60deg`` with no space, matching the UTF-8 form.

``dBr_t`` is a straight alias for ``dB_t`` used as a naming convention for
relative levels (e.g., antenna gain relative to isotropic).  It carries no
additional type-safety over ``dB_t``.

Using Unit Symbols in Headers vs Implementation Files
=====================================================

mp-units places its unit symbols (``Hz``, ``MHz``, ``W``, ``mW``, etc.) in the
namespace ``mp_units::si::unit_symbols``.  The rules for importing them differ
between ``.cc`` and ``.h`` files.

In implementation files (``.cc``), a ``using namespace`` directive at file scope
is acceptable::

    using namespace mp_units::si::unit_symbols;

In header files (``.h``), a ``using namespace`` directive would leak the entire
symbol set into every file that includes the header, directly or transitively.
Instead, ``units.h`` exports the most commonly needed unit symbols via selective
``using`` declarations: ``Hz``, ``MHz``, ``GHz``, ``W``, and ``mW``.  Any
header that includes ``units.h`` (directly or transitively) can use these
symbols without further qualification::

    // default argument value -- concise because MHz is exported by units.h
    MHz_t GetTxBandwidth(WifiMode mode, MHz_t maxBw = 20.0 * MHz) const;

    // constexpr initializer
    constexpr FrequencyRange WIFI_SPECTRUM_5_GHZ = {5170.0 * MHz, 5915.0 * MHz};

If a needed SI unit symbol is not among those exported by ``units.h``, authors
may add a selective ``using`` declaration in their own header
(e.g., ``using mp_units::si::unit_symbols::kHz;``) or use the fully qualified
form.

The two practical cases where this matters are default argument values and
``constexpr`` initializers -- the only contexts in which a header must spell
out a unit-symbol expression.

See also the coding style guide's *Unit symbol imports* subsection for the
concise version of these rules.

Extracting Numeric Values
*************************

mp-units uses ``numerical_value_in()`` to extract the underlying value::

    auto distValue = distance.numerical_value_in(m);  // 8.0

Printing this out yields:

.. code-block:: none

    distValue = 8 (raw double)

Key points about extraction:

1. The representation type is controlled by the numeric literal used in
   construction.  ``operator*`` deduces the representation (``Rep``) from
   the left-hand operand::

       auto d1 = 8.0 * m;   // quantity<si::metre, double>
       auto d2 = 8.0f * m;  // quantity<si::metre, float>
       auto d3 = 8 * m;     // quantity<si::metre, int>

   The |ns3| aliases fix the representation to ``double`` (e.g., ``meter_t``
   is ``quantity<si::metre, double>``), consistent with ns-3 conventions.

   Assigning an integer-representation quantity to a ``double``-representation
   alias (e.g., ``MHz_t freq = 0 * MHz;``) is a widening conversion and works
   correctly.  However, in template contexts where the type is deduced from
   the initial value -- such as ``std::accumulate`` -- the accumulator will
   have ``int`` representation, and assigning a ``double``-representation
   result back to it is a narrowing conversion that the compiler will reject.
   When in doubt, use a floating-point literal (``0.0 * MHz``) to ensure
   ``double`` representation.

2. ``numerical_value_in()`` performs a unit conversion and returns the bare
   representation type.  The argument is the target unit, which need not match
   the stored unit — dimensions must be compatible, enforced at compile time::

       auto d = 8.0 * m;
       d.numerical_value_in(m);   // 8.0   -- no conversion needed
       d.numerical_value_in(km);  // 0.008 -- conversion performed
       d.numerical_value_in(Hz);  // compile error -- incompatible dimensions

   The result is a raw ``double`` — unit information is intentionally
   discarded.  This is the recommended technique for passing a raw number to
   legacy code; the required unit argument makes the intent self-documenting.

Two examples:

.. code-block:: none

    distance.numerical_value_in(m)  = 8
    distance.numerical_value_in(km) = 0.008

This flexible representation type support is one reason why ``ns3::Time``
could potentially be ported to mp-units.  ``ns3::Time`` currently stores time
as ``int64_t`` with a runtime-selectable resolution (nanoseconds by default).
If runtime resolution were abandoned in favor of a fixed compile-time unit,
mp-units could express ``Time`` as
``quantity<si::nano<si::second>, int64_t>`` — preserving the integral
representation and gaining full dimensional safety and unit-conversion
arithmetic at no runtime cost.

Compile-Time Safety
*******************

Expressions with incompatible types will not compile::

    // will fail: no match for 'operator+'
    auto sum = distance + txPower;  // meters + watts

    // will fail: no match for 'operator+'
    double rawDouble{4};
    auto bad = distance + rawDouble;    // quantity + raw double

mp-units uses C++ concepts to constrain its operator templates, so the
compiler error message names the actual quantity types rather than a wall
of template instantiation noise.  For example, the first error above
produces:

.. code-block:: none

    error: invalid operands to binary expression
           ('quantity<metre{}, double>' and
            'Watt_t' (aka 'quantity<mp_units::si::watt, double>'))
        auto sum = distance + txPower;
                   ~~~~~~~~ ^ ~~~~~~~

The types are stated in the error rather than embedded inside template
parameter packs several levels deep.

Compatible Unit Arithmetic
**************************

Different units of the same dimension work correctly::

    auto d1 = 8.0 * m;
    auto d2 = 8.0 * km;

The result is:

.. code-block:: none

    d1 + d2 = 8008 m

The result unit is the common reference unit.  For SI-prefixed length that is
metres, so 8 m + 8 km = 8008 m (not 8.008 km).  This matters when storing the
result in an explicit type alias: ``meter_t`` receives the result without
conversion because the stored unit is already metres (spelled as ``meters``
in American English, but ``metres`` is the spelling chosen by the International
Bureau of Weights and Measures and used in this type system).

Multiplication and division track dimension changes at compile time::

    auto speed = (8.0 * m) / (2.0 * s);  // quantity<m/s, double>

The result is:

.. code-block:: none

    speed = 4 m/s

Dividing two quantities of the same dimension yields a dimensionless quantity,
not a raw ``double``.  Use ``numerical_value_in(mp_units::one)`` to extract the
ratio as a raw number::

    auto ratio = d1 / d2;                          // dimensionless quantity
    ratio.numerical_value_in(mp_units::one);       // raw double

The result is:

.. code-block:: none

    d1 / d2 = 1 m/km
    d1 / d2 as double = 0.001

A quantity variable can also be scaled by a raw number::

    auto d3 = d1 * 2.0;     // 16 m -- scales an existing quantity
    auto d4 = d2 / 1000.0;  // 8 m  -- divides an existing quantity

The results are:

.. code-block:: none

    d1 * 2.0    = 16 m
    d2 / 1000.0 = 0.008 km

This is distinct from the construction pattern ``8.0 * m``, where the right
operand is a unit object.  Here the right operand is a raw ``double`` and the
left operand is an already-typed quantity.  Both use ``operator*`` but serve
different roles: construction vs. scaling.

Logarithmic Units
*****************

mp-units does not natively support logarithmic units (dB, dBm, dBW).
|ns3| instead provides wrapper classes with type-safe arithmetic.
Like mp-units linear quantities, logarithmic quantities use multiplication
literals for construction::

    auto txPwrDbm = 20.0 * dBm;       // absolute power
    auto gain     = 10.0 * dB;        // relative gain
    dBW_t txPwrDbW{txPwrDbm};         // explicit conversion from dBm

These print out as:

.. code-block:: none

    txPwr = 100 mW
    txPwrDbm = dBm_t{txPwr} = 20 dBm
    txPwrDbW = dBW_t{txPwrDbm} = -10 dBW

Direct construction from bare numeric types (e.g., ``dBm_t{20}``) is not
allowed -- the constructor is private.  This matches mp-units, where
``MHz_t{20.0}`` also does not compile; only ``20.0 * MHz`` works.  For
linear-to-logarithmic conversion, use the public constructors that accept
linear quantities: ``dBm_t{mWatt_t{...}}``, ``dBW_t{Watt_t{...}}``,
``dB_t{scalar_t{ratio}}``.

Logarithmic Arithmetic Rules
=============================

The following arithmetic rules apply to the logarithmic wrapper types.

Linear power addition::

    txPwr + txPwr  // 200 mW

Linear power scaling::

    txPwr * 2  // 200 mW

``dBm + dB = dBm`` (adding gain to absolute power)::

    txPwrDbm + gain  // 20 dBm + 10 dB = 30 dBm

``dBm - dB = dBm`` (subtracting path loss; common in link budget calculations)::

    txPwrDbm - pathLoss  // 20 dBm - 80 dB = -60 dBm

``dBm - dBm = dB`` (computing a power ratio)::

    txPwrDbm - rxPwr  // 20 dBm - (-50 dBm) = 70 dB

``dBm + dBm`` is deleted and will not compile::

    #ifdef WONT_COMPILE
    auto bad = txPwrDbm + txPwrDbm;  // compile error
    #endif

Compound assignment operators ``+=`` and ``-=`` are available for all
logarithmic types (``dB_t``, ``dBm_t``, ``dBW_t``), useful in simulation
loops::

    dBm_t power = -60.0 * dBm;
    power += 10.0 * dB;  // -50 dBm
    power -= 10.0 * dB;  // -60 dBm

Output:

.. code-block:: none

    -60.0 * dBm after += 10.0 * dB: -50 dBm
    then again after -= 10.0 * dB: -60 dBm

Linear/Logarithmic Conversion
==============================

Conversions between linear and logarithmic types are explicit::

    mWatt_t txPwr = 100.0 * mW;
    dBm_t   txPwrDbm{txPwr};           // 100 mW -> 20 dBm
    mWatt_t back = mWatt_t(txPwrDbm);  // 20 dBm -> 100 mW

Output:

.. code-block:: none

    back = mWatt_t(txPwrDbm) = 100 mW
    wattBack = Watt_t(txPwrDbm) = 0.1 W

Extracting Raw dB Values
========================

The ``numerical_value_in(unit)`` method extracts the raw ``double`` from both
linear and logarithmic types.  For logarithmic types, the unit tag argument
(``dB``, ``dBm``, ``dBW``, ``dBm_per_Hz``, ``dBm_per_MHz``) must match the
type::

    auto p = 20.0 * dBm;   p.numerical_value_in(dBm)  // 20.0
    auto w = -10.0 * dBW;  w.numerical_value_in(dBW)   // -10.0
    auto g = 3.0 * dB;     g.numerical_value_in(dB)    //  3.0

Output:

.. code-block:: none

    txPwrDbm.numerical_value_in(dBm) = 20
    txPwrDbW.numerical_value_in(dBW) = -10
    gain.numerical_value_in(dB)      = 10

This is the same API pattern as mp-units linear quantities::

    MHz_t freq = 2400.0 * MHz;
    freq.numerical_value_in(MHz)  // 2400.0
    freq.numerical_value_in(Hz)   // 2.4e9

``dBW_t`` arithmetic follows the same rules as ``dBm_t``.  For example,
given ``dBW_t loss = -20.0 * dBW`` and ``dB_t gain = 10.0 * dB``::

    loss + gain  // -20 dBW + 10 dB = -10 dBW

Mixing linear watts and logarithmic dBW will not compile.

Converting ``loss`` to a linear ``mWatt_t``::

    mWatt_t(loss)  // -20 dBW = 10 mW

Output:

.. code-block:: none

    mWatt_t(loss) = 10 mW

``std::min`` and ``std::max`` work on all logarithmic types:

.. code-block:: none

    std::min of 3 dBm and 4 dBm is: 3 dBm

Power Spectral Density
**********************

Logarithmic PSD types support multiplication by a bandwidth to yield total
power::

    auto psd1 = -50.0 * dBm_per_Hz;
    auto psd2 = -20.0 * dBm_per_MHz;
    Hz_t  bw1{1000.0 * Hz};
    MHz_t bw2{10.0 * MHz};
    auto total1 = psd1 * bw1;  // dBm_t
    auto total2 = psd2 * bw2;  // dBm_t

Output:

.. code-block:: none

    psd1 = -50 dBm/Hz
    psd2 = -20 dBm/MHz
    bw1 = 1000 Hz
    bw2 = 10 MHz
    total1 (psd1 * bw1) = -20 dBm
    total2 (psd2 * bw2) = -10 dBm
    total3 (bw1 * psd1) = -20 dBm

``mW_per_Hz_t`` and ``mW_per_MHz_t`` are the linear counterparts of the
logarithmic PSD types.  mp-units performs the dimensional reduction
automatically on multiplication::

    mW_per_Hz_t  noisePsd{1.0e-9 * mW / Hz};       // 1 nW/Hz
    Hz_t         bwHz{1.0e9 * Hz};                  // 1 GHz
    auto         noiseFloor = noisePsd * bwHz;      // mWatt_t result: 1 mW

    mW_per_MHz_t channelPsd{0.1 * mW / MHz};        // 0.1 mW/MHz
    MHz_t        bwMHz{20.0 * MHz};                 // 20 MHz
    auto         channelPower = channelPsd * bwMHz; // mWatt_t result: 2 mW

Output:

.. code-block:: none

    noiseFloor   (1 nW/Hz * 1 GHz)      = 1 mW
    channelPower (0.1 mW/MHz * 20 MHz)  = 2 mW

CommandLine Integration
***********************

``dB_t`` and ``dBm_t`` work with the |ns3| ``CommandLine`` class.  Three
input formats are accepted::

    --cmdLineDecibel=5_dB   // token format (recommended)
    --cmdLineDecibel=5      // raw number, native unit assumed
    --cmdLineDecibel="5 dB" // standard notation, must be shell-quoted

Note: ``5_dB`` is not a C++ user-defined literal.  It is a string token parsed
by ``DeserializeFromString``.  The underscore is a separator between the number
and the unit label in the serialized form.

``operator<<`` displays values in standard notation (``5 dB``), while
``SerializeToString`` (used by ``--PrintAttributes`` and config files) emits
the token form (``5_dB``).  These intentionally differ: the token form is a
single whitespace-free string that can be passed directly on the command line
or written to a config file without quoting.

Pasting a displayed value (e.g., from ``NS_LOG`` output) back to the command
line requires either quoting (``--cmdLineDecibel="5 dB"``) or converting the
space to an underscore (``--cmdLineDecibel=5_dB``).

Attribute Integration
*********************

Attribute wrappers are available for ``DbValue``, ``DbmValue``, ``MhzValue``,
and ``DbmPerMhzValue``.

``DbValue`` / ``DbmValue``
==========================

For gain and power attributes::

    .AddAttribute("TxGain",
                  "Transmission gain.",
                  DbValue{0.0 * dB},
                  MakeDbAccessor(&MyClass::SetGain, &MyClass::GetGain),
                  MakeDbChecker())

All of these forms set the same value::

    obj->SetAttribute("TxGain", DbValue{2.0 * dB});
    obj->SetAttribute("TxGain", StringValue{"2_dB"});  // token format
    obj->SetAttribute("TxGain", StringValue{"2 dB"});  // standard notation
    obj->SetAttribute("TxGain", StringValue{"2"});     // raw number
    obj->SetAttribute("TxGain", DoubleValue{2});        // numeric converter

``MhzValue``
============

For channel frequency attributes::

    .AddAttribute("Frequency",
                  "Channel center frequency.",
                  MhzValue{MHz_t{2400.0 * MHz}},
                  MakeMhzAccessor(&MyClass::m_frequency),
                  MakeMhzChecker())

All four input forms work::

    obj->SetAttribute("Frequency", MhzValue{MHz_t{5180.0 * MHz}});
    obj->SetAttribute("Frequency", DoubleValue{5180});       // raw double -> MHz
    obj->SetAttribute("Frequency", StringValue{"5180_MHz"});
    obj->SetAttribute("Frequency", StringValue{"5180"});     // raw number -> MHz

``MhzValue`` accepts ``DoubleValue`` (treating the raw ``double`` as MHz) for
backward compatibility with legacy attribute settings.

``DbmPerMhzValue``
==================

For noise or interference mask attributes::

    .AddAttribute("NoisePsd",
                  "Noise power spectral density.",
                  DbmPerMhzValue{-174.0 * dBm_per_MHz},
                  MakeDbmPerMhzAccessor(&MyClass::m_noisePsd),
                  MakeDbmPerMhzChecker())

Input forms::

    obj->SetAttribute("NoisePsd", DbmPerMhzValue{-174.0 * dBm_per_MHz});
    obj->SetAttribute("NoisePsd", DoubleValue{-174});               // raw double
    obj->SetAttribute("NoisePsd", StringValue{"-174_dBm_per_MHz"});
    obj->SetAttribute("NoisePsd", StringValue{"-174"});             // raw number

TupleValue Integration
======================

``DbValue`` and ``DbmValue`` can be combined in |ns3| ``TupleValue`` to model
composite attributes, for example a (tx_power, antenna_gain) pair::

    .AddAttribute("TxConfig",
                  "Transmit power (dBm) and antenna gain (dB).",
                  TupleValue<DbmValue, DbValue>(
                      {-20.0 * dBm, 0.0 * dB}),
                  MakeTupleAccessor<DbmValue, DbValue>(
                      &MyClass::m_txConfig),
                  MakeTupleChecker<DbmValue, DbValue>(
                      MakeDbmChecker(), MakeDbChecker()))

All input formats carry through ``TupleValue`` string deserialization::

    obj->SetAttribute("TxConfig", StringValue("{-15_dBm, 3_dB}"));
    obj->SetAttribute("TxConfig", StringValue("{-15, 3}"));

This works because ``TupleValue`` is generic over any ``AttributeValue``
subclass.  ``DbmValue`` and ``DbValue`` satisfy that interface directly —
no special ``TupleValue`` support was needed beyond the standard
``AttributeValue`` machinery.

Time: mp-units vs ns3::Time
***************************

mp-units time quantities are completely distinct from ``ns3::Time``.  There is
no implicit conversion between them; the bridge is explicit in both directions.

mp-units time quantities carry their unit at compile time and deduce their
representation type from the numeric literal::

    auto t1 = 1.5 * s;    // quantity<si::second, double>
    auto t2 = 100.0 * ms; // quantity<si::milli<si::second>, double>
    auto t3 = 10.0 * ns;  // quantity<si::nano<si::second>, double>

These print as:

.. code-block:: none

    t1 = 1.5 s
    t2 = 100 ms
    t3 = 10 ns

Converting an mp-units time to ``ns3::Time`` — extract the numerical value in a
known unit and pass to the corresponding ``ns3::Time`` factory::

    Time nsTime1 = Seconds(t1.numerical_value_in(s));
    Time nsTime2 = Seconds(t2.numerical_value_in(s));

These print as:

.. code-block:: none

    nsTime1 = +1.5s
    nsTime2 = +100ms

Converting ``ns3::Time`` to mp-units — extract via a ``Get*()`` accessor and
multiply by the matching unit symbol::

    Time simTime = MilliSeconds(250);
    auto mpTime = simTime.GetSeconds() * s;    // double representation

``GetSeconds()`` returns ``double``, so ``mpTime`` has representation type
``double``.  ``GetNanoSeconds()`` returns ``int64_t``; multiplying directly by
``ns`` gives a ``quantity<si::nanosecond, int64_t>``, which may be unexpected.
Cast to ``double`` first if a floating-point representation is needed::

    auto mpTimeNs = static_cast<double>(simTime.GetNanoSeconds()) * ns;

Output:

.. code-block:: none

    simTime  = +250ms
    mpTime   = 0.25 s
    mpTimeNs = 2.5e+08 ns

``ns3::Time`` stores time as ``int64_t`` at a runtime-selectable resolution
(nanoseconds by default).  mp-units time quantities carry their unit at compile
time and use whatever representation type was used in construction.  The two
types model time differently and coexist; ns-3 simulation infrastructure
continues to use ``ns3::Time`` throughout.

The mp-units design leaves open the possibility of integrating ``ns3::Time``
more deeply in the future.  The primary prerequisite would be replacing
``ns3::Time``'s runtime-selectable resolution with a compile-time fixed unit.
