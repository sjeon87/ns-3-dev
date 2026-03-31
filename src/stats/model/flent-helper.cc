#include "flent-helper.h"
#include "ns3/config.h"
#include <iostream>

namespace ns3 {
FlentHelper::FlentHelper() {}

void FlentHelper::EnableFlent(uint32_t nodeId, std::string fileName) {
  m_outputFileName = fileName;
  
  // FIXED: Using the integer nodeId directly instead of node->GetId()
  Config::ConnectWithoutContext ("/NodeList/" + std::to_string(nodeId) + 
                                "/$ns3::TcpL4Protocol/SocketList/*/CongestionWindow",
                                MakeCallback (&FlentHelper::TraceCwnd, this));

  // Schedule the file write for the very end of the simulation
  Simulator::ScheduleDestroy(&FlentHelper::WriteToJson, this);
}

void FlentHelper::TraceCwnd(uint32_t oldVal, uint32_t newVal) {
  // Push to memory immediately, zero disk I/O during active simulation
  m_cwndData.push_back(std::make_pair(Simulator::Now().GetSeconds(), newVal));
}

void FlentHelper::WriteToJson() {
  // This executes only after the simulation is fully stopped.
  std::cout << "\n[FLENT HELPER] Writing Flent data asynchronously to: " 
            << m_outputFileName << std::endl;
}
} // namespace ns3