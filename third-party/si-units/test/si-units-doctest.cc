#define DOCTEST_CONFIG_IMPLEMENT

// clang-format off
#include "doctest.h"
// clang-format on

#include "../model/units-aliases.h"
#include "../model/units-angle.h"
#include "../model/units-frequency.h"
#include "../model/units-power.h"
#include "../model/units-ratio.h"
#include "../model/units-time.h"
#include "../model/si-units-vector.h"

using namespace si_units;

#define CHECK_TOL(lhs, rhs, tolerance)                                                             \
    CHECK(lhs >= (rhs - tolerance));                                                               \
    CHECK(lhs <= (rhs + tolerance))

/// A stream buffer for testing.
/// @details See TestInputOperatorPositives() function for usage
struct StreamBuffer : public std::streambuf
{
    /// Constructor.
    /// @param str The string to use as the stream buffer.
    StreamBuffer(const std::string& str)
    {
        auto cstr = str.c_str();
        auto beg = const_cast<char*>(cstr);
        auto len = str.size();
        setg(beg, beg, beg + len);
    }
};

/// Test input operators - positive cases
/// @tparam T type
/// @param tvs test vectors
template <typename T>
void
TestInputOperatorPositives(const std::vector<std::string>& tvs)
{
    T got;
    for (auto& tv : tvs)
    {
        T want{tv};
        StreamBuffer buf(tv);
        std::istream is(&buf);
        is >> got;
        CHECK(got == want);
    }
}

/// Test input operators - negative cases
/// @tparam T type
/// @param tvs test vectors
template <typename T>
void
TestInputOperatorNegatives(const std::vector<std::string>& tvs)
{
    T got;
    for (auto& tv : tvs)
    {
        StreamBuffer buf(tv);
        std::istream is(&buf);
        is >> got;
        CHECK(is.fail());
    }
}

TEST_CASE("angle-degree")
{
    CHECK(degree_t{1} == degree_t{1.0});
    CHECK(degree_t{-1} == degree_t{-1.0});
    CHECK(0_degree == -(0_degree));
    CHECK(0_degree == 0.0_degree);
    CHECK(degree_t{"20.1 degree"} == 20.1_degree);

    CHECK(30_degree == 30.0_degree);
    CHECK(30_degree != 40.0_degree);
    CHECK(30_degree < 40_degree);
    CHECK(30_degree <= 40_degree);
    CHECK(30_degree <= 30_degree);
    CHECK(40_degree > 30_degree);
    CHECK(40_degree >= 30_degree);
    CHECK(30_degree >= 30_degree);

    CHECK((30_degree + 40_degree) == 70_degree);
    CHECK((100_degree + 150_degree) == 250_degree);
    CHECK((100_degree - 150_degree) == -50_degree);
    CHECK((100_degree - 350_degree) == -250_degree);

    CHECK((300_degree * 2.5) == 750_degree);
    CHECK((2.5 * 300_degree) == 750_degree);
    CHECK((300_degree / 4.0) == 75_degree);

    CHECK(degree_t{100}.normalize() == 100_degree);
    CHECK(degree_t{170}.normalize() == 170_degree);
    CHECK(degree_t{190}.normalize() == -170_degree);
    CHECK(degree_t{370}.normalize() == 10_degree);
    CHECK(degree_t{-100}.normalize() == -100_degree);
    CHECK(degree_t{-170}.normalize() == -170_degree);
    CHECK(degree_t{-190}.normalize() == 170_degree);
    CHECK(degree_t{-370}.normalize() == -10_degree);

    CHECK((123.4_degree).str() == "123.4 degree");
    CHECK((123.4_degree).str(false) == "123.4degree");
    CHECK(degree_t::from_radian(radian_t{M_PI}) == 180_degree);
    CHECK(degree_t{180}.to_radian() == radian_t{M_PI});
    CHECK(degree_t{180}.in_radian() == M_PI);
    CHECK(degree_t{123.4}.in_degree() == 123.4);

    // Conversion from string
    CHECK(degree_t::from_str("90degree").value() == 90_degree);
    CHECK(degree_t::from_str("90.0degree").value() == 90_degree);
    CHECK(degree_t::from_str("90.00degree").value() == 90_degree);
    CHECK(degree_t::from_str("3.14degree").value() == 3.14_degree);
    CHECK(degree_t::from_str("3.14 degree").value() == 3.14_degree);
    CHECK(degree_t::from_str("  3.14  degree  ").value() == 3.14_degree);
    CHECK(degree_t::from_str("-3.14degree").value() == -3.14_degree);
    CHECK(degree_t::from_str("3.14 Degree").has_value() == false);
    CHECK(degree_t::from_str("3.14_degree").has_value() == false);

    TestInputOperatorPositives<degree_t>({"12.3degree", "12.3 degree", " 12.3  degree "});
    TestInputOperatorNegatives<degree_t>(
        {"12.3Degree", "12.3'", "12.3_degree", "12.3degree_t", "12.3", "12.3dBm"});

    CHECK(degree_t{"90degree"} == 90_degree);
    CHECK(degree_t{"90.0degree"} == 90_degree);
    CHECK(degree_t{"90.00degree"} == 90_degree);
    CHECK(degree_t{"3.14degree"} == 3.14_degree);
    CHECK(degree_t{"3.14 degree"} == 3.14_degree);
    CHECK(degree_t{"  3.14  degree  "} == 3.14_degree);
    CHECK(degree_t{"-3.14degree"} == -3.14_degree);
}

/// Test radian_t
TEST_CASE("angle-radian")
{
    CHECK(radian_t{1} == radian_t{1.0});
    CHECK(radian_t{-1} == radian_t{-1.0});
    CHECK(0_radian == -(0_radian));
    CHECK(0_radian == 0.0_radian);
    CHECK(radian_t{"1.5 radian"} == 1.5_radian);

    CHECK((30_radian == 30.0_radian));
    CHECK((30_radian != 40.0_radian));
    CHECK((30_radian < 40_radian));
    CHECK((30_radian <= 40_radian));
    CHECK((30_radian <= 30_radian));
    CHECK((40_radian > 30_radian));
    CHECK((40_radian >= 30_radian));
    CHECK((30_radian >= 30_radian));

    CHECK((30_radian + 40_radian) == 70_radian);
    CHECK((100_radian + 150_radian) == 250_radian);
    CHECK((100_radian - 150_radian) == -50_radian);
    CHECK((100_radian - 350_radian) == -250_radian);

    CHECK((300_radian * 2.5) == 750_radian);
    CHECK((2.5 * 300_radian) == 750_radian);
    CHECK((300_radian / 4.0) == 75_radian);

    // Normalization is subject to the floating-point precision error. Adopt the rough
    // comparison at will.
    CHECK(radian_t{0.75 * M_PI}.normalize() == radian_t{0.75 * M_PI});
    CHECK(radian_t{1.25 * M_PI}.normalize() == radian_t{-0.75 * M_PI});
    CHECK(radian_t{2.00 * M_PI}.normalize() == radian_t{0.00 * M_PI});
    CHECK_TOL(radian_t{2.25 * M_PI}.normalize().in_radian(),
              radian_t{0.25 * M_PI}.in_radian(),
              1e-10 /* sufficient resolution */);
    CHECK(radian_t{-0.75 * M_PI}.normalize() == radian_t{-0.75 * M_PI});
    CHECK(radian_t{-1.25 * M_PI}.normalize() == radian_t{0.75 * M_PI});
    CHECK(radian_t{-2.00 * M_PI}.normalize() == radian_t{0.00 * M_PI});
    CHECK(radian_t{-2.25 * M_PI}.normalize() == radian_t{-0.25 * M_PI});

    CHECK((123.4_radian).str() == "123.4 radian");
    CHECK((123.4_radian).str(false) == "123.4radian");
    CHECK(radian_t::from_degree(180_degree) == radian_t{M_PI});
    CHECK(radian_t{M_PI}.to_degree() == 180_degree);
    CHECK(radian_t{M_PI}.in_degree() == 180);
    CHECK(radian_t{123.4}.in_radian() == 123.4);

    // Conversion from string
    CHECK(radian_t::from_str("3.14radian").value() == 3.14_radian);
    CHECK(radian_t::from_str("3.14 radian").value() == 3.14_radian);
    CHECK(radian_t::from_str("  3.14  radian  ").value() == 3.14_radian);
    CHECK(radian_t::from_str("-3.14radian").value() == -3.14_radian);
    CHECK(radian_t::from_str("3.14 Radian").has_value() == false);
    CHECK(radian_t::from_str("3.14_radian").has_value() == false);

    TestInputOperatorPositives<radian_t>({"12.3radian", "12.3 radian", " 12.3  radian "});
    TestInputOperatorNegatives<radian_t>(
        {"12.3Radian", "12.3_radian", "12.3radian_t", "12.3", "12.3degree"});
}

TEST_CASE("dB")
{
    // Notations
    CHECK(dB_t{0} == dB_t{0.});
    CHECK(dB_t{0} == dB_t{0.0});
    CHECK(dB_t{0} == dB_t{-0});
    CHECK(dB_t{0} == 0_dB);
    CHECK(dB_t{0} == 0._dB);
    CHECK(dB_t{0} == 0.0_dB);
    CHECK(dB_t{0} == -0_dB);
    CHECK(dB_t{0} == -0._dB);
    CHECK(dB_t{0} == -0.0_dB);
    CHECK(dB_t{0} == dB_t(0.0));
    CHECK(dB_t{0} == dB_t{0_dB});
    CHECK(dB_t{0} == dB_t(0_dB));
    CHECK(dB_t{"1.5 dB"} == 1.5_dB);

    // Equality, inequality
    CHECK(dB_t{10} == 10_dB);
    CHECK(dB_t{-10} == -10_dB);
    CHECK((dB_t{10} != 10_dB) == false);
    CHECK((dB_t{10} == 20.0_dB) == false); // NOLINT
    CHECK((dB_t{10} != 20_dB));            // NOLINT

    // Comparison
    CHECK(1_dB < 2_dB);
    CHECK(2_dB > 1_dB);
    CHECK(1_dB <= 1_dB);
    CHECK(1_dB <= 2_dB);
    CHECK(2_dB >= 1_dB);
    CHECK(2_dB >= 2_dB);
    CHECK(-1_dB < 2_dB);
    CHECK(2_dB > -1_dB);
    CHECK((10_dB < 20_dB));
    CHECK((10_dB <= 20_dB));
    CHECK((10_dB > 20_dB) == false);
    CHECK((10_dB >= 20_dB) == false);

    // Arithmetic
    CHECK((1_dB + 2_dB) == 3_dB);
    CHECK((3_dB - 1_dB) == 2_dB);
    CHECK((3_dB - 9_dB) == -6_dB);
    CHECK((5_dB += 10_dB) == 15_dB);
    CHECK((5_dB -= 10_dB) == -5_dB);
    CHECK(-8_dB == (0_dB - 8_dB));

    // Utilities
    CHECK(dB_t{123}.str() == "123.0 dB");     // NOLINT
    CHECK(dB_t{123}.str(false) == "123.0dB"); // NOLINT
    CHECK(dB_t{123.45}.val == 123.45);        // NOLINT
    CHECK(dB_t{123.45}.str() == "123.5 dB");  // NOLINT
    CHECK(dB_t{20}.to_linear() == 100.0);

    // Conversion from string
    CHECK(dB_t::from_str("3.14dB").value() == 3.14_dB);
    CHECK(dB_t::from_str("3.14 dB").value() == 3.14_dB);
    CHECK(dB_t::from_str("  3.14  dB  ").value() == 3.14_dB);
    CHECK(dB_t::from_str("-3.14dB").value() == -3.14_dB);
    CHECK(dB_t::from_str("3.14 Db").has_value() == false);
    CHECK(dB_t::from_str("3.14_dB").has_value() == false);

    TestInputOperatorPositives<dB_t>({"12.3dB", "12.3 dB", " 12.3  dB "});
    TestInputOperatorNegatives<dB_t>(
        {"12.3db", "12.3DB", "12.3_dB", "12.3dB_t", "12.3", "12.3dBm"});
}

TEST_CASE("dBr")
{
    // Notations
    CHECK(dBr_t{0} == dBr_t{0.});
    CHECK(dBr_t{0} == dBr_t{0.0});
    CHECK(dBr_t{0} == dBr_t{-0});
    CHECK(dBr_t{0} == 0_dBr);
    CHECK(dBr_t{0} == 0._dBr);
    CHECK(dBr_t{0} == 0.0_dBr);
    CHECK(dBr_t{0} == -0_dBr);
    CHECK(dBr_t{0} == -0._dBr);
    CHECK(dBr_t{0} == -0.0_dBr);
    CHECK(dBr_t{0} == dBr_t(0.0));
    CHECK(dBr_t{0} == dBr_t{0_dBr});
    CHECK(dBr_t{0} == dBr_t(0_dBr));

    // Equality, inequality
    CHECK(dBr_t{10} == 10_dBr);
    CHECK(dBr_t{-10} == -10_dBr);
    CHECK((dBr_t{10} != 10_dBr) == false);
    CHECK((dBr_t{10} == 20.0_dBr) == false); // NOLINT
    CHECK((dBr_t{10} != 20_dBr));            // NOLINT

    // Comparison
    CHECK(1_dBr < 2_dBr);
    CHECK(2_dBr > 1_dBr);
    CHECK(1_dBr <= 1_dBr);
    CHECK(1_dBr <= 2_dBr);
    CHECK(2_dBr >= 1_dBr);
    CHECK(2_dBr >= 2_dBr);
    CHECK(-1_dBr < 2_dBr);
    CHECK(2_dBr > -1_dBr);
    CHECK((10_dBr < 20_dBr));
    CHECK((10_dBr <= 20_dBr));
    CHECK((10_dBr > 20_dBr) == false);
    CHECK((10_dBr >= 20_dBr) == false);

    // Arithmetic
    CHECK((1_dBr + 2_dBr) == 3_dBr);
    CHECK((3_dBr - 1_dBr) == 2_dBr);
    CHECK((3_dBr - 9_dBr) == -6_dBr);
    CHECK((5_dBr += 10_dBr) == 15_dBr);
    CHECK((5_dBr -= 10_dBr) == -5_dBr);
    CHECK(-8_dBr == (0_dBr - 8_dBr));

    // Utilities
    CHECK(dBr_t{123}.str() == "123.0 dB");     // NOLINT
    CHECK(dBr_t{123}.str(false) == "123.0dB"); // NOLINT
    CHECK(dBr_t{123.45}.val == 123.45);        // NOLINT
    CHECK(dBr_t{123.45}.str() == "123.5 dB");  // NOLINT
    CHECK(dBr_t{20}.to_linear() == 100.0);

    // Conversion from string
    CHECK(dBr_t::from_str("3.14dB").value() == 3.14_dBr);
    CHECK(dBr_t::from_str("3.14 dB").value() == 3.14_dBr);
    CHECK(dBr_t::from_str("  3.14  dB  ").value() == 3.14_dBr);
    CHECK(dBr_t::from_str("-3.14dB").value() == -3.14_dBr);
    CHECK(dBr_t::from_str("3.14 Db").has_value() == false);
    CHECK(dBr_t::from_str("3.14_dB").has_value() == false);

    TestInputOperatorPositives<dBr_t>({"12.3dB", "12.3 dB", " 12.3  dB "});
    // TestInputOperatorNegatives<dBr_t>(
    //     {"12.3DBR", "12.3dbr", "12.3_dBr", "12.3dBr_t", "12.3", "12.3dB"});
}

TEST_CASE("mWatt")
{
    // Notations
    CHECK(mWatt_t{0} == 0_mWatt);
    CHECK(mWatt_t{"1.5 mWatt"} == 1.5_mWatt);

    // Equality, inequality
    CHECK_TOL(1_mWatt, 1e9_pWatt, 1_pWatt); // NOLINT

    // Comparison
    CHECK(1_mWatt < 2_mWatt);
    CHECK(2_mWatt > 1_mWatt);
    CHECK(1_mWatt <= 1_mWatt);
    CHECK(1_mWatt <= 2_mWatt);
    CHECK(2_mWatt >= 1_mWatt);
    CHECK(2_mWatt >= 2_mWatt);

    // Arithmetic
    CHECK((1_mWatt + 2_mWatt) == 3_mWatt);
    CHECK((3_mWatt - 1_mWatt) == 2_mWatt);
    CHECK((3_mWatt - 9_mWatt) == -6_mWatt);
    CHECK((5_mWatt += 10_mWatt) == 15_mWatt);
    CHECK((5_mWatt -= 10_mWatt) == -5_mWatt);
    CHECK(-8_mWatt == (0_mWatt - 8_mWatt));

    // Utilities
    CHECK(mWatt_t{123}.str() == "123.0 mWatt");     // NOLINT
    CHECK(mWatt_t{123}.str(false) == "123.0mWatt"); // NOLINT
    CHECK(mWatt_t{123.45}.str() == "123.5 mWatt");  // NOLINT
    CHECK(mWatt_t{100}.in_dBm() == 20.0);
    CHECK(mWatt_t{123.45}.in_Watt() == 0.12345); // NOLINT
    CHECK(mWatt_t{123.45}.in_mWatt() == 123.45); // NOLINT

    TestInputOperatorPositives<mWatt_t>({"12.3mWatt", "12.3 mWatt", " 12.3  mWatt "});
    TestInputOperatorNegatives<mWatt_t>(
        {"12.3MWATT", "12.3mW", "12.3_mWatt", "12.3mWatt_t", "12.3", "12.3dBm"});
}

TEST_CASE("Watt")
{
    // Notations
    CHECK(Watt_t{0} == 0_Watt);
    CHECK(Watt_t{0} == Watt_t{0.});
    CHECK(Watt_t{0} == Watt_t{0.0});
    CHECK(Watt_t{0} == Watt_t{-0});
    CHECK(Watt_t{0} == 0_Watt);
    CHECK(Watt_t{0} == 0._Watt);
    CHECK(Watt_t{0} == 0.0_Watt);
    CHECK(Watt_t{0} == -0_Watt);
    CHECK(Watt_t{0} == -0._Watt);
    CHECK(Watt_t{0} == -0.0_Watt);
    CHECK(Watt_t{"1.5 Watt"} == 1.5_Watt);

    // Equality, inequality
    CHECK(Watt_t{10} == 10_Watt);
    CHECK(Watt_t{-10} == -10_Watt);
    CHECK((Watt_t{10} != 10_Watt) == false);
    CHECK((Watt_t{10} == 20.0_Watt) == false); // NOLINT
    CHECK((Watt_t{10} == 10.0_Watt));          // NOLINT
    CHECK((Watt_t{10} != 20_Watt));

    // Comparison
    CHECK(1_Watt < 2_Watt);
    CHECK(2_Watt > 1_Watt);
    CHECK(1_Watt <= 1_Watt);
    CHECK(1_Watt <= 2_Watt);
    CHECK(2_Watt >= 1_Watt);
    CHECK(2_Watt >= 2_Watt);
    CHECK(-2_Watt < 1_Watt);
    CHECK(-2_Watt < -1_Watt);
    CHECK(-1_Watt > -2_Watt);
    CHECK(1_Watt > -2_Watt);
    CHECK((10_Watt < 20_Watt));
    CHECK((10_Watt <= 20_Watt));
    CHECK((10_Watt > 20_Watt) == false);
    CHECK((10_Watt >= 20_Watt) == false);

    // Arithmetic
    CHECK((1_Watt + 2_Watt) == 3_Watt);
    CHECK((3_Watt - 1_Watt) == 2_Watt);
    CHECK((3_Watt - 9_Watt) == -6_Watt);
    CHECK((5_Watt += 10_Watt) == 15_Watt);
    CHECK((5_Watt -= 10_Watt) == -5_Watt);
    CHECK(-8_Watt == (0_Watt - 8_Watt));

    // Utilities
    CHECK(Watt_t{123}.str() == "123.0 Watt");     // NOLINT
    CHECK(Watt_t{123}.str(false) == "123.0Watt"); // NOLINT
    CHECK(Watt_t{123.45}.str() == "123.5 Watt");  // NOLINT
    CHECK(Watt_t{100}.in_dBm() == 50.0);
    CHECK(Watt_t{1.2345}.in_mWatt() == 1234.5); // NOLINT
    CHECK(Watt_t{123.45}.in_Watt() == 123.45);  // NOLINT

    TestInputOperatorPositives<Watt_t>({"12.3Watt", "12.3 Watt", " 12.3  Watt "});
    TestInputOperatorNegatives<Watt_t>(
        {"12.3watt", "12.3W", "12.3_Watt", "12.3Watt_t", "12.3", "12.3mWatt"});
}

TEST_CASE("dBm")
{
    // Notations
    CHECK(dBm_t{0} == dBm_t{0.});
    CHECK(dBm_t{0} == dBm_t{0.0});
    CHECK(dBm_t{0} == dBm_t{-0});
    CHECK(dBm_t{0} == 0_dBm);
    CHECK(dBm_t{0} == 0._dBm);
    CHECK(dBm_t{0} == 0.0_dBm);
    CHECK(dBm_t{0} == -0_dBm);
    CHECK(dBm_t{0} == -0._dBm);
    CHECK(dBm_t{0} == -0.0_dBm);
    CHECK(dBm_t{"1.5 dBm"} == 1.5_dBm);

    // Equality, inequality

    CHECK(dBm_t{10} == 10_dBm);
    CHECK(dBm_t{-10} == -10_dBm);
    CHECK((dBm_t{10} != 10_dBm) == false);
    CHECK((dBm_t{10} == 20.0_dBm) == false); // NOLINT
    CHECK((dBm_t{10} != 20_dBm));

    // Comparison
    CHECK(1_dBm < 2_dBm);
    CHECK(2_dBm > 1_dBm);
    CHECK(1_dBm <= 1_dBm);
    CHECK(1_dBm <= 2_dBm);
    CHECK(2_dBm >= 1_dBm);
    CHECK(2_dBm >= 2_dBm);
    CHECK(-1_dBm < 2_dBm);
    CHECK(2_dBm > -1_dBm);
    CHECK((10_dBm < 20_dBm));
    CHECK((10_dBm <= 20_dBm));
    CHECK((10_dBm > 20_dBm) == false);
    CHECK((10_dBm >= 20_dBm) == false);

    // Arithmetic
#if false // IMPLEMENTED BUT NOT ALLOWED TO BE USED
        CHECK_TOL((20_dBm + 20_dBm).val == (23.0102999566_dBm).val == 1e-4);
        CHECK_TOL((20_dBm += 20_dBm).val == (23.0102999566_dBm).val == 1e-4);
        CHECK_TOL((20_dBm - 10_dBm).val == (19.5424250944_dBm).val == 1e-4);
        CHECK_TOL((20_dBm -= 10_dBm).val == (19.5424250944_dBm).val == 1e-4);
        CHECK_TOL((20_dBm - 19_dBm).val == (13.1317467562_dBm).val == 1e-4);
#endif    // IMPLEMENTED BUT NOT ALLOWED TO BE USED

    // Utilities
    CHECK(dBm_t{123}.str() == "123.0 dBm");     // NOLINT
    CHECK(dBm_t{123}.str(false) == "123.0dBm"); // NOLINT
    CHECK(dBm_t{123.45}.str() == "123.5 dBm");  // NOLINT
    CHECK(dBm_t{20}.in_mWatt() == 100.0);
    // Need tolerance due to math precision error on M1 Ultra with --ffast-math
    CHECK_TOL(dBm_t{20}.in_Watt(), 0.1, 1e-10);
    CHECK(dBm_t{123.45}.in_dBm() == 123.45); // NOLINT

    TestInputOperatorPositives<dBm_t>({"12.3dBm", "12.3 dBm", " 12.3  dBm "});
    TestInputOperatorNegatives<dBm_t>(
        {"12.3DBM", "12.3dbm", "12.3_dBm", "12.3dBm_t", "12.3", "12.3Watt"});
}

TEST_CASE("dBm-and-dB")
{
    CHECK((10_dBm + 20_dB) == 30_dBm);
    CHECK((10_dBm - 20_dB) == -10_dBm);
    CHECK((10_dB + 20_dBm) == 30_dBm);  // Commutativity
    CHECK((10_dB - 20_dBm) == -10_dBm); // Commutativity
}

TEST_CASE("mWatt-and-Watt")
{
    // Equality, inequality
    CHECK(mWatt_t{0} == 0_Watt);
    CHECK(Watt_t{10} == 10000_mWatt);
    CHECK((Watt_t{1} != 1000_mWatt) == false);
    CHECK((Watt_t{2} == 1000_mWatt) == false); // NOLINT
    CHECK((mWatt_t{1} == 0.001_Watt));         // NOLINT
    CHECK((mWatt_t{10} != 20_Watt));

    // Comparison
    CHECK(1_mWatt < 2_Watt);
    CHECK(2_mWatt > 0.001_Watt);
    CHECK(1000_mWatt <= 1_Watt);
    CHECK(2_mWatt >= 0.001_Watt);
    CHECK((10_mWatt < 20_Watt));
    CHECK((2000_mWatt <= 2_Watt));
    CHECK((10_mWatt > 10_Watt) == false);
    CHECK((10_mWatt >= 20_Watt) == false);
    CHECK(1_Watt < 2000_mWatt);
    CHECK(2_Watt > 0.001_mWatt);
    CHECK(0.001_Watt <= 1_mWatt);
    CHECK(2_Watt >= 2_mWatt);
    CHECK((0.1_Watt < 200_mWatt));
    CHECK((2_Watt <= 2000_mWatt));
    CHECK((0.1_Watt > 100_mWatt) == false);
    CHECK((1_Watt >= 2000_mWatt) == false);

    // Arithmetic
    CHECK((1_mWatt + 2_Watt) == 2001_mWatt);
    CHECK((3_mWatt - 0.001_Watt) == 2_mWatt);
    CHECK((5_mWatt += 0.01_Watt) == 15_mWatt);
    CHECK((5_mWatt -= 0.002_Watt) == 3_mWatt);
    CHECK(8_mWatt == (8_mWatt - 0_Watt));
    CHECK((1_Watt + 2_mWatt) == 1002_mWatt);
    CHECK((0.03_Watt - 1_mWatt) == 29_mWatt);
    CHECK((1_Watt += 0.01_Watt) == 1.01_Watt);
    CHECK((4_Watt -= 0.2_Watt) == 3.8_Watt);
    CHECK(8_Watt == (8_Watt - 0_mWatt));
}

TEST_CASE("dBm-and-mWatt")
{
    // dBm_t-mWatt_t
    CHECK(20_dBm == (100_mWatt).to_dBm());
    CHECK(20_dBm == dBm_t::from_mWatt(100_mWatt));
    CHECK((20_dBm).to_mWatt() == 100_mWatt);
    CHECK(mWatt_t::from_dBm(20_dBm) == 100_mWatt);

    // dBm_t-Watt_t
    CHECK(10_dBm == (0.01_Watt).to_dBm());
    CHECK(10_dBm == dBm_t::from_Watt(0.01_Watt));
    CHECK((10_dBm).to_Watt() == 0.01_Watt);
    CHECK(Watt_t::from_dBm(10_dBm) == 0.01_Watt);

    // Watt_t-mWatt_t
    CHECK(0.1_Watt == (100_mWatt).to_Watt());
    CHECK(0.1_Watt == Watt_t::from_mWatt(100_mWatt));
    CHECK((0.1_Watt).to_mWatt() == 100_mWatt);
    CHECK(mWatt_t::from_Watt(0.1_Watt) == 100_mWatt);
}

TEST_CASE("Hz")
{
    CHECK(Hz_t{123.} == 123_Hz);
    CHECK(Hz_t{123.45} == 123.45_Hz);
    CHECK((-123_Hz) == Hz_t{-123});
    CHECK(Hz_t{123000.} == 123_kHz);
    CHECK(Hz_t{123000000.} == 123_MHz);
    CHECK(Hz_t{123000000000.} == 123_GHz);
    CHECK(Hz_t{123000000000000.} == 123_THz);
    CHECK(Hz_t{123000000000000.} == 123000000000000_Hz);
    CHECK(Hz_t{"123 Hz"} == 123_Hz);
    CHECK(Hz_t{"123.45Hz"} == 123.45_Hz);

    CHECK(10_Hz + 20_Hz == 30_Hz);
    CHECK(10_MHz - 20_MHz == -10_MHz);
    CHECK((10_MHz - 20_MHz != 40_MHz));
    CHECK((10_MHz - 20_MHz == 40_MHz) == false);
    CHECK((10_kHz < 20_kHz));
    CHECK((10_kHz <= 20_kHz));
    CHECK((10_kHz <= 10_kHz));

    CHECK((10_kHz > 20_kHz) == false);
    CHECK((10_kHz >= 20_kHz) == false);
    CHECK((10_kHz >= 10_kHz));

    CHECK((10_Hz += 100_Hz) == 110_Hz);
    CHECK((10_Hz -= 100_Hz) == -90_Hz);
    CHECK((1_kHz / 4) == 250_Hz);
    CHECK((1_kHz / 4_Hz) == 250.0);
    CHECK((1_kHz * 4) == 4_kHz);
    CHECK((4 * 1_kHz) == 4_kHz);
    // CHECK((1_Hz * MilliSeconds(1)) == 0.001);
    // CHECK((1_kHz * MilliSeconds(1)) == 1.0);
    // CHECK((1_MHz * MilliSeconds(1)) == 1000.0);
    // CHECK((MilliSeconds(1) * 1_MHz) == 1000.0);
    // CHECK((MilliSeconds(1) * 1_kHz) == 1.0);
    // CHECK((MilliSeconds(1) * 1_Hz) == 0.001);

    CHECK((123_Hz).str() == "123 Hz");
    CHECK((123_Hz).str(false) == "123Hz");
    CHECK((123_kHz).str() == "123 kHz");
    CHECK((123_MHz).str() == "123 MHz");
    CHECK((123_GHz).str() == "123 GHz");
    CHECK((123_THz).str() == "123 THz");
    CHECK((123000_THz).str() == "123000 THz");

    CHECK((123_GHz).in_Hz() == 123000000000);
    CHECK((123_GHz).in_kHz() == 123000000.0);
    CHECK((123_GHz).in_MHz() == 123000.0);
    CHECK((123.45e6_kHz).in_Hz() == 123450000000);
    CHECK((123.45e6_kHz).in_kHz() == 123450000);
    CHECK((123.45e6_kHz).in_MHz() == 123450);
    CHECK((123.456789e6_kHz).in_MHz() == 123456.789);

    CHECK(kHz_t{123.4} == 123.4_kHz);
    CHECK(MHz_t{123.4} == 123.4_MHz);
    CHECK(GHz_t{123.4} == 123.4_GHz);
    CHECK(THz_t{123.4} == 123.4_THz);
    CHECK(kHz_t{123} == 123_kHz);
    CHECK(MHz_t{123} == 123_MHz);
    CHECK(GHz_t{123} == 123_GHz);
    CHECK(THz_t{123} == 123_THz);

    CHECK(kHz_t{123.4} == 123400_Hz);
    CHECK(MHz_t{123.4} == 123400000_Hz);
    CHECK(GHz_t{123.4} == 123400000000_Hz);
    CHECK(THz_t{123.4} == 123400000000000_Hz);

    CHECK(kHz_t(1200_Hz) == 1.2_kHz);
    CHECK(MHz_t(1200_kHz) == 1.2_MHz);
    CHECK(GHz_t(1200_MHz) == 1.2_GHz);
    CHECK(THz_t(1200_GHz) == 1.2_THz);

    CHECK(kHz_t(1.2_kHz) == 1.2_kHz);
    CHECK(Hz_t{1.2_kHz} == 1200_Hz);
    CHECK(Hz_t{1.2_MHz} == 1200_kHz);
    CHECK(MHz_t(1.2_THz) == 1200_GHz);

    // Conversion from string
    CHECK(Hz_t::from_str("3.14Hz").value() == 3.14_Hz);
    CHECK(Hz_t::from_str("3.14 Hz").value() == 3.14_Hz);
    CHECK(Hz_t::from_str("  3.14  Hz  ").value() == 3.14_Hz);
    CHECK(Hz_t::from_str("-3.14Hz").value() == -3.14_Hz);
    CHECK(Hz_t::from_str("3.14 hz").has_value() == false);
    CHECK(Hz_t::from_str("3.14_Hz").has_value() == false);

    // Consistent parsing regardless of the struct type
    CHECK(Hz_t::from_str("3.14kHz").value() == 3.14_kHz);
    CHECK(Hz_t::from_str("3.14MHz").value() == 3.14_MHz);
    CHECK(Hz_t::from_str("3.14GHz").value() == 3.14_GHz);
    CHECK(Hz_t::from_str("3.14THz").value() == 3.14_THz);
    CHECK(kHz_t::from_str("3.14kHz").value() == 3.14_kHz);
    CHECK(MHz_t::from_str("3.14MHz").value() == 3.14_MHz);
    CHECK(GHz_t::from_str("3.14GHz").value() == 3.14_GHz);
    CHECK(THz_t::from_str("3.14THz").value() == 3.14_THz);
    CHECK(Hz_t::from_str("3.14GHz").value() == 3.14_GHz);
    CHECK(kHz_t::from_str("3.14GHz").value() == 3.14_GHz);
    CHECK(MHz_t::from_str("3.14THz").value() == 3.14_THz);
    CHECK(GHz_t::from_str("3.14kHz").value() == 3.14_kHz);
    CHECK(THz_t::from_str("3.14MHz").value() == 3.14_MHz);

    TestInputOperatorPositives<Hz_t>({"12.3Hz", "12.3 Hz", " 12.3  Hz "});
    TestInputOperatorNegatives<Hz_t>(
        {"12.3hz", "12.3HZ", "12.3hZ", "12.3_MHz", "12.3MHz_t", "12.3", "12.3dBm"});
    TestInputOperatorPositives<kHz_t>({"12.3kHz", "12.3 kHz"});
    TestInputOperatorNegatives<kHz_t>({"12.3khz", "12.3KHZ"});
    TestInputOperatorPositives<MHz_t>({"12.3MHz", "12.3 MHz"});
    TestInputOperatorNegatives<MHz_t>({"12.3Mhz", "12.3MHZ"});
    TestInputOperatorPositives<GHz_t>({"12.3GHz", "12.3 GHz"});
    TestInputOperatorNegatives<GHz_t>({"12.3Ghz", "12.3GHZ"});
    TestInputOperatorPositives<THz_t>({"12.3THz", "12.3 THz"});
    TestInputOperatorNegatives<THz_t>({"12.3Thz", "12.3THZ"});

    CHECK(MHz_t::from_str("3.14 MHz").value() == 3.14_MHz);
    CHECK(MHz_t{"123MHz"} == 123_MHz);
    CHECK(MHz_t{"123.45 MHz"} == 123.45_MHz);
    CHECK(MHz_t{0.5}.IsMultipleOf(0.1_MHz));
    CHECK(MHz_t{0.5}.IsMultipleOf(0.15_MHz) == false);
    CHECK(MHz_t{20}.IsMultipleOf(5_MHz));
    CHECK(MHz_t{20}.IsMultipleOf(20_MHz));
    CHECK(MHz_t{80}.IsMultipleOf(20_MHz));
    CHECK(MHz_t{80}.IsMultipleOf(40_MHz));
    CHECK(MHz_t{80}.IsMultipleOf(21_MHz) == false);

    TestInputOperatorPositives<MHz_t>({"12.3MHz", "12.3 MHz", " 12.3  MHz "});
    TestInputOperatorNegatives<MHz_t>(
        {"12.3mhz", "12.3Mhz", "12.3mHz", "12.3MHZ", "12.3_MHz", "12.3MHz_t", "12.3", "12.3dBm"});
}

TEST_CASE("mWatt-and-double")
{
    // Arithmetic
    CHECK((1_mWatt * 2.0) == 2_mWatt);
    CHECK((1_mWatt / 2.0) == 0.5_mWatt);
    CHECK((2.0 * 1_mWatt) == 2_mWatt);
}

TEST_CASE("dBm_per_Hz")
{
    CHECK(dBm_per_Hz_t{-43.21} == -43.21_dBm_per_Hz); // NOLINT
    CHECK(dBm_per_Hz_t{"1.5 dBm/Hz"} == 1.5_dBm_per_Hz);

    // Utilities
    CHECK(dBm_per_Hz_t{123}.val == 123.0);                // NOLINT
    CHECK(dBm_per_Hz_t{123}.str() == "123.0 dBm/Hz");     // NOLINT
    CHECK(dBm_per_Hz_t{123}.str(false) == "123.0dBm/Hz"); // NOLINT
    CHECK(dBm_per_Hz_t{123.45}.val == 123.45);            // NOLINT
    CHECK(dBm_per_Hz_t{123.45}.str() == "123.5 dBm/Hz");  // NOLINT
    CHECK(dBm_per_Hz_t{123.45}.in_dBm() == 123.45);       // NOLINT
    CHECK(dBm_per_Hz_t{123} == 123_dBm_per_Hz);           // NOLINT

    CHECK_TOL(dBm_per_Hz_t{-80.0}.OverBandwidth(2_Hz), -77_dBm, 0.1_dB);   // NOLINT
    CHECK_TOL(dBm_per_Hz_t{-80.0}.OverBandwidth(0.5_Hz), -83_dBm, 0.1_dB); // NOLINT
    CHECK(dBm_per_Hz_t{-80.0}.OverBandwidth(1_MHz) == -20_dBm);            // NOLINT
    CHECK(dBm_per_Hz_t{-80.0}.OverBandwidth(100_kHz) == -30_dBm);          // NOLINT

    CHECK(dBm_per_Hz_t::AveragePsd(-20_dBm, 1_MHz) == dBm_per_Hz_t{-80}); // NOLINT

    TestInputOperatorPositives<dBm_per_Hz_t>({"12.3dBm/Hz", "12.3 dBm/Hz", " 12.3  dBm/Hz "});
    TestInputOperatorNegatives<dBm_per_Hz_t>(
        {"12.3dbm/Hz", "12.3dBm/hz", "12.3dBm_per_Hz", "12.3", "12.3dBm"});
}

TEST_CASE("dBm_per_MHz")
{
    CHECK(dBm_per_MHz_t{-43.21} == -43.21_dBm_per_MHz); // NOLINT
    CHECK(dBm_per_MHz_t{"1.5 dBm/MHz"} == 1.5_dBm_per_MHz);

    // Utilities
    CHECK(dBm_per_MHz_t{123}.val == 123.0);                 // NOLINT
    CHECK(dBm_per_MHz_t{123}.str() == "123.0 dBm/MHz");     // NOLINT
    CHECK(dBm_per_MHz_t{123}.str(false) == "123.0dBm/MHz"); // NOLINT
    CHECK(dBm_per_MHz_t{123.45}.val == 123.45);             // NOLINT
    CHECK(dBm_per_MHz_t{123.45}.str() == "123.5 dBm/MHz");  // NOLINT
    CHECK(dBm_per_MHz_t{123.45}.in_dBm() == 123.45);        // NOLINT
    CHECK(dBm_per_MHz_t{123} == 123_dBm_per_MHz);           // NOLINT

    CHECK(dBm_per_MHz_t::AveragePsd(-20_dBm, 1_MHz) == dBm_per_MHz_t{-20}); // NOLINT

    CHECK_TOL(dBm_per_MHz_t{-80.0}.OverBandwidth(2_MHz), -77_dBm, 0.1_dB);   // NOLINT
    CHECK_TOL(dBm_per_MHz_t{-80.0}.OverBandwidth(500_kHz), -83_dBm, 0.1_dB); // NOLINT
    CHECK(dBm_per_MHz_t{-80.0}.OverBandwidth(1_MHz) == -80_dBm);             // NOLINT
    CHECK(dBm_per_MHz_t{-80.0}.OverBandwidth(100_kHz) == -90_dBm);           // NOLINT

    CHECK(dBm_per_MHz_t::AveragePsd(-20_dBm, 1_MHz) == dBm_per_MHz_t{-20});

    TestInputOperatorPositives<dBm_per_MHz_t>({"12.3dBm/MHz", "12.3 dBm/MHz", " 12.3  dBm/MHz "});
    TestInputOperatorNegatives<dBm_per_MHz_t>(
        {"12.3dbm/MHz", "12.3dBm/mhz", "12.3dBm_per_MHz", "12.3", "12.3dBm/Hz"});
}

TEST_CASE("Vectors")
{
    { // dB_t Empty vector
        std::vector<double> tvs;
        auto got1 = dB_t::from_doubles(tvs);
        auto got2 = dB_t::to_doubles(got1);
        auto got3 = dB_t::from_doubles(got2);
        CHECK((tvs == got2));
        CHECK((got1 == got3));
        for (size_t idx = 0; idx < tvs.size(); ++idx)
        {
            CHECK(got1[idx].val == tvs[idx]);
        }
    }

    std::vector<double> tvs = {0.1, -0.2, 1.3, -4.5, 5.6e7, -8e-9};
    { // dB_t
        auto got1 = dB_t::from_doubles(tvs);
        auto got2 = dB_t::to_doubles(got1);
        auto got3 = dB_t::from_doubles(got2);
        CHECK((tvs == got2));
        CHECK((got1 == got3));
        for (size_t idx = 0; idx < tvs.size(); ++idx)
        {
            CHECK(got1[idx].val == tvs[idx]);
        }
    }

    { // dBm_t
        auto got1 = dBm_t::from_doubles(tvs);
        auto got2 = dBm_t::to_doubles(got1);
        auto got3 = dBm_t::from_doubles(got2);
        CHECK((tvs == got2));
        CHECK((got1 == got3));
        for (size_t idx = 0; idx < tvs.size(); ++idx)
        {
            CHECK(got1[idx].val == tvs[idx]);
        }
    }

    { // mWatt_t
        auto got1 = mWatt_t::from_doubles(tvs);
        auto got2 = mWatt_t::to_doubles(got1);
        auto got3 = mWatt_t::from_doubles(got2);
        CHECK((tvs == got2));
        CHECK((got1 == got3));
        for (size_t idx = 0; idx < tvs.size(); ++idx)
        {
            CHECK(got1[idx].val == tvs[idx]);
        }
    }

    { // Watt_t
        auto got1 = Watt_t::from_doubles(tvs);
        auto got2 = Watt_t::to_doubles(got1);
        auto got3 = Watt_t::from_doubles(got2);
        CHECK((tvs == got2));
        CHECK((got1 == got3));
        for (size_t idx = 0; idx < tvs.size(); ++idx)
        {
            CHECK(got1[idx].val == tvs[idx]);
        }
    }

    { // dBm_per_Hz_t
        auto got1 = dBm_per_Hz_t::from_doubles(tvs);
        auto got2 = dBm_per_Hz_t::to_doubles(got1);
        auto got3 = dBm_per_Hz_t::from_doubles(got2);
        CHECK((tvs == got2));
        CHECK((got1 == got3) == true);
        for (size_t idx = 0; idx < tvs.size(); ++idx)
        {
            CHECK(got1[idx].val == tvs[idx]);
        }
    }

    { // dBm_per_MHz_t
        auto got1 = dBm_per_MHz_t::from_doubles(tvs);
        auto got2 = dBm_per_MHz_t::to_doubles(got1);
        auto got3 = dBm_per_MHz_t::from_doubles(got2);
        CHECK((tvs == got2));
        CHECK((got1 == got3) == true);
        for (size_t idx = 0; idx < tvs.size(); ++idx)
        {
            CHECK(got1[idx].val == tvs[idx]);
        }
    }

    { // Hz_t
        std::vector<double> tvs = {1, -2, 3000, -4000000};
        auto got1 = Hz_t::from_doubles(tvs);
        auto got2 = Hz_t::to_doubles(got1);
        auto got3 = Hz_t::from_doubles(got2);
        CHECK((tvs == got2));
        CHECK((got1 == got3) == true);
        for (size_t idx = 0; idx < tvs.size(); ++idx)
        {
            CHECK(got1[idx].val == tvs[idx]);
        }
    }
}

TEST_CASE("nSEC")
{
    CHECK(nSEC_t{123} == 123_nSEC);
    CHECK(nSEC_t{123000} == 123_uSEC);
    CHECK(nSEC_t{123000000} == 123_mSEC);
    CHECK(nSEC_t{123000000000} == 123_SEC);
    CHECK(nSEC_t{123000000000000} == 123000_SEC);

    CHECK(10_nSEC + 20_nSEC == 30_nSEC);
    CHECK(10_mSEC - 20_mSEC == -10_mSEC);
    CHECK((10_mSEC - 20_mSEC != 40_mSEC));
    CHECK((10_mSEC - 20_mSEC == 40_mSEC) == false);
    CHECK((10_uSEC < 20_uSEC));
    CHECK((10_uSEC <= 20_uSEC));
    CHECK((10_uSEC <= 10_uSEC));
    CHECK((20_nSEC + 10_SEC) == 10000000020_nSEC);
    CHECK((20_nSEC - 10_SEC) == -9999999980_nSEC);

    CHECK((10_uSEC > 20_uSEC) == false);
    CHECK((10_uSEC >= 20_uSEC) == false);
    CHECK((10_uSEC >= 10_uSEC));

    CHECK((10_nSEC += 100_nSEC) == 110_nSEC);
    CHECK((10_nSEC -= 100_nSEC) == -90_nSEC);

    CHECK((123_nSEC).str() == "123 nSEC");
    CHECK((123_nSEC).str(false) == "123nSEC");
    CHECK((123_uSEC).str() == "123 uSEC");
    CHECK((123_mSEC).str() == "123 mSEC");
    CHECK((123_SEC).str() == "123 SEC");
    CHECK(nSEC_t{123456789}.in_usec() == 123456);
    CHECK(nSEC_t{123456789}.in_msec() == 123);
    CHECK(nSEC_t{123456789}.in_sec() == 0);
}

TEST_CASE("percent")
{
    // Equality
    CHECK(percent_t{}.val == 0.);
    CHECK(percent_t{0.}.val == 0.);
    CHECK(percent_t{31}.val == 31);
    CHECK(percent_t{31.4}.val == 31.4);
    CHECK(percent_t{31.41592}.val == 31.41592);
    CHECK(percent_t{314.1}.val == 314.1);
    CHECK(percent_t{-31.4}.val == -31.4);
    CHECK(percent_t{3.14} == percent_t{3.14});
    CHECK(percent_t{3.14} == 3.14_percent);
    CHECK(percent_t::from_ratio(0.125) == percent_t{12.5});
    CHECK(percent_t{12.5}.to_ratio() == 0.125);
    CHECK(percent_t{50} == 50_percent);
    CHECK(percent_t{"50.1 percent"} == 50.1_percent);

    // Arithmetic
    CHECK(percent_t{10} + percent_t{20} == percent_t{30});
    CHECK(percent_t{20} + percent_t{10} == percent_t{30});
    CHECK(percent_t{30} - percent_t{10} == percent_t{20});
    CHECK(percent_t{10} - percent_t{30} == percent_t{-20});
    CHECK(percent_t{10} * 10.0 == percent_t{100});
    CHECK(percent_t{10} * -10.0 == percent_t{-100});
    CHECK(percent_t{10} / 10.0 == percent_t{1});
    CHECK(10.0 * percent_t{10} == percent_t{100});

    // Comparison
    CHECK(percent_t{10} < percent_t{20});
    CHECK(percent_t{-100} < percent_t{0});
    CHECK(percent_t{10} <= percent_t{20});
    CHECK(percent_t{20} <= percent_t{20});
    CHECK(percent_t{20} > percent_t{10});
    CHECK(percent_t{0} > percent_t{-100});
    CHECK(percent_t{20} >= percent_t{10});
    CHECK(percent_t{20} <= percent_t{20});

    // Inequality
    CHECK(percent_t{31.41592}.val != 31.41593);
    CHECK(percent_t{31.4159265358979} != percent_t{31.4159265358978});

    // Assignment
    percent_t got{20};
    got += percent_t{0.1};
    auto want = percent_t{20.1};
    CHECK(got == want);

    // String
    CHECK(got.str() == "20.1 %");
    CHECK(got.str(false) == "20.1%");
    CHECK(percent_t::from_str("20.1 %").value() == got);
    CHECK(percent_t::from_str("20.1%").value() == got);
    CHECK(percent_t::from_str("20.1 percent").value() == got);
    CHECK(percent_t::from_str("20.1percent").value() == got);

    // Banned operations
    // CHECK(percent_t{30} * percent_t{10} == percent_t{3});
    // CHECK(percent_t{30} / percent_t{10} == percent_t{300});
}

TEST_CASE("metric-prefixes")
{
    CHECK(MetricStrToNum("123k").value() == 123000);
    CHECK(MetricStrToNum("123 k").value() == 123000);
    CHECK(MetricStrToNum("123 k ").value() == 123000);
    CHECK(MetricStrToNum("123  k ").value() == 123000);
    CHECK(MetricStrToNum(" 123  k ").value() == 123000);
    CHECK(MetricStrToNum("123.0k").value() == 123000);
    CHECK(MetricStrToNum("123.456k").value() == 123456);
    CHECK(MetricStrToNum("123M").value() == 123000000);
    CHECK(MetricStrToNum("123G").value() == 123000000000);
    CHECK(MetricStrToNum("123T").value() == 123000000000000);
    CHECK(MetricStrToNum("123A").has_value() == false);

    CHECK(NumToMetricStr(123) == "123");
    CHECK(NumToMetricStr(1230) == "1230");
    CHECK(NumToMetricStr(12300) == "12300");
    CHECK(NumToMetricStr(123000) == "123k");
    CHECK(NumToMetricStr(1230000) == "1230k");
    CHECK(NumToMetricStr(12300000) == "12300k");
    CHECK(NumToMetricStr(123000000) == "123M");
    CHECK(NumToMetricStr(123000000000) == "123G");
    CHECK(NumToMetricStr(123000000000000) == "123T");
    CHECK(NumToMetricStr(123400) == "123400");
    CHECK(NumToMetricStr(-123000) == "-123k");
    CHECK(NumToMetricStr(123, true) == "123 ");
}

TEST_CASE("si-units-vector")
{
    // Test default constructor
    {
        SiUnitsVec<kHz_t> vec;
        CHECK(vec.size() == 0);
    }

    // Test size constructor
    {
        SiUnitsVec<kHz_t> vec(3);
        CHECK(vec.size() == 3);
        CHECK(vec.vals[0] == kHz_t{0});
        CHECK(vec.vals[1] == kHz_t{0});
        CHECK(vec.vals[2] == kHz_t{0});
    }

    // Test vector constructor (move semantics)
    {
        std::vector<kHz_t> input = {100_kHz, 200_kHz, 300_kHz};
        SiUnitsVec<kHz_t> vec(input);
        CHECK(vec.size() == 3);
        CHECK(vec.vals[0] == 100_kHz);
        CHECK(vec.vals[1] == 200_kHz);
        CHECK(vec.vals[2] == 300_kHz);
    }

    // Test string representation
    {
        std::vector<kHz_t> input = {10_kHz, 20_kHz};
        SiUnitsVec<kHz_t> vec(input);
        auto result = vec.str();
        CHECK(result == "10 kHz 20 kHz ");
    }

    // Test equality operator
    {
        std::vector<kHz_t> input1 = {100_kHz, 200_kHz};
        std::vector<kHz_t> input2 = {100_kHz, 200_kHz};
        std::vector<kHz_t> input3 = {100_kHz, 300_kHz};

        SiUnitsVec<kHz_t> vec1(input1);
        kHzVec_t vec2(input2);
        SiUnitsVec<kHz_t> vec3(input3);

        CHECK(vec1 == vec2);
        CHECK(!(vec1 == vec3));
    }

    // Test inequality operator
    {
        std::vector<kHz_t> input1 = {100_kHz, 200_kHz};
        std::vector<kHz_t> input2 = {100_kHz, 300_kHz};

        SiUnitsVec<kHz_t> vec1(input1);
        SiUnitsVec<kHz_t> vec2(input2);

        CHECK(vec1 != vec2);
        CHECK(!(vec1 != vec1));
    }

    // Test output stream operator
    {
        std::vector<kHz_t> input = {5_kHz, 15_kHz};
        SiUnitsVec<kHz_t> vec(input);

        std::ostringstream oss;
        oss << vec;
        CHECK(oss.str() == "5 kHz 15 kHz ");
    }

    // Test input stream operator
    {
        SiUnitsVec<kHz_t> vec(2);
        std::string input = "123.4 567.8";
        StreamBuffer buf(input);
        std::istream is(&buf);

        is >> vec;

        CHECK(vec.vals[0].val == 123.4);
        CHECK(vec.vals[1].val == 567.8);
    }

    // Test input stream operator with EOF
    {
        SiUnitsVec<kHz_t> vec(3);
        std::string input = "123.4"; // Only one value for 3-element vector
        StreamBuffer buf(input);
        std::istream is(&buf);

        is >> vec;

        CHECK(vec.vals[0].val == 123.4);
        // Remaining elements should be unchanged (still 0)
        CHECK(vec.vals[1].val == 0);
        CHECK(vec.vals[2].val == 0);
    }

    // Test with dBm_t
    {
        std::vector<dBm_t> input = {10_dBm, 20_dBm, 30_dBm};
        dBmVec_t vec(input);
        CHECK(vec.size() == 3);
        CHECK(vec.vals[0] == 10_dBm);
        CHECK(vec.vals[1] == 20_dBm);
        CHECK(vec.vals[2] == 30_dBm);
    }

    // Test with degree_t
    {
        std::vector<degree_t> input = {45_degree, 90_degree};
        SiUnitsVec<degree_t> vec(input);
        CHECK(vec.size() == 2);
        CHECK(vec.vals[0] == 45_degree);
        CHECK(vec.vals[1] == 90_degree);
    }

    // Test empty vector constructor
    {
        std::vector<kHz_t> input;
        SiUnitsVec<kHz_t> vec(input);
        CHECK(vec.size() == 0);
    }

    // Test single element vector
    {
        std::vector<kHz_t> input = {42_kHz};
        SiUnitsVec<kHz_t> vec(input);
        CHECK(vec.size() == 1);
        CHECK(vec.vals[0] == 42_kHz);
        CHECK(vec.str() == "42 kHz ");
    }

    // Test large vector
    {
        std::vector<kHz_t> input;
        for (size_t i = 0; i < 100; ++i) {
            input.push_back(kHz_t{static_cast<double>(i)});
        }

        SiUnitsVec<kHz_t> vec(input);
        CHECK(vec.size() == 100);
        CHECK(vec.vals[0] == 0_kHz);
        CHECK(vec.vals[50] == 50_kHz);
        CHECK(vec.vals[99] == 99_kHz);
    }
}
