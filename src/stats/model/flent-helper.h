#ifndef FLENT_HELPER_H
#define FLENT_HELPER_H

#include "ns3/simulator.h"
#include <vector>
#include <string>

namespace ns3 {
class FlentHelper {
public:
FlentHelper();
void EnableFlent(uint32_t node, std::string fileName);

private:
  void TraceCwnd(uint32_t oldVal, uint32_t newVal);
  void WriteToJson(); // The async writer

  std::vector<std::pair<double, uint32_t>> m_cwndData;
  std::string m_outputFileName;
};
} // namespace ns3

#endif /* FLENT_HELPER_H */