#include <si-units>
#include <iostream>

int main()
{
    std::cout << "=== SI Units Library Example ===" << std::endl;
    std::cout << std::endl;

    // Frequency examples
    auto freq1 = 2.4_GHz;
    auto freq2 = MHz_t{5800};
    std::cout << "Frequency 1: " << freq1.str() << std::endl;
    std::cout << "Frequency 2: " << freq2.str() << " = " << freq2.in_GHz() << " GHz" << std::endl;
    std::cout << std::endl;

    // Power examples
    auto power1 = 20_dBm;
    auto power2 = mWatt_t{100};
    std::cout << "Power 1: " << power1.str() << std::endl;
    std::cout << "Power 2: " << power2.str() << " = " << power2.to_dBm().str() << std::endl;

    // Power arithmetic (logarithmic addition)
    auto power3 = 20_dBm + 3_dB;  // Valid: adding dB to dBm
    std::cout << "Power 1 + 3dB: " << power3.str() << std::endl;
    std::cout << std::endl;

    // Angle examples
    auto angle1 = 90_degree;
    auto angle2 = degree_t{45};
    std::cout << "Angle 1: " << angle1.str() << " = " << angle1.to_radian().str() << std::endl;
    std::cout << "Angle 2: " << angle2.str() << std::endl;
    std::cout << std::endl;

    // Time examples
    auto time1 = 1000_nSEC;
    std::cout << "Time 1: " << time1.str() << " = " << time1.in_usec() << " μs" << std::endl;
    std::cout << std::endl;

    // Ratio examples
    auto ratio1 = 50_percent;
    std::cout << "Ratio 1: " << ratio1.str() << std::endl;
    std::cout << std::endl;

    // Demonstrate type safety
    std::cout << "=== Type Safety Demonstration ===" << std::endl;
    std::cout << "The following operations are VALID:" << std::endl;
    std::cout << "  - Adding dB to dBm: " << (10_dBm + 3_dB).str() << std::endl;
    std::cout << "  - Adding linear powers: " << (10_mWatt + 5_mWatt).str() << std::endl;
    std::cout << "  - Converting units: dBm -> mWatt" << std::endl;
    std::cout << std::endl;
    std::cout << "The following operations would FAIL at compile time:" << std::endl;
    std::cout << "  - auto invalid = 10_dBm + 20_mWatt;  // Error: incompatible types" << std::endl;
    std::cout << "  - auto invalid = 10_MHz + 20_dBm;    // Error: incompatible dimensions" << std::endl;
    std::cout << std::endl;

    std::cout << "=== Example completed successfully ===" << std::endl;
    return 0;
}
