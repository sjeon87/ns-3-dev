#include "flent-helper.h"

#include "ns3/log.h"
#include "ns3/simulator.h"

#include <fstream>

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("FlentHelper");

NS_OBJECT_ENSURE_REGISTERED(FlentHelper);

TypeId
FlentHelper::GetTypeId(void)
{
  static TypeId tid = TypeId("ns3::FlentHelper")
                        .SetParent<Object>()
                        .SetGroupName("Stats")
                        .AddConstructor<FlentHelper>();
  return tid;
}

FlentHelper::FlentHelper()
{
}

FlentHelper::~FlentHelper()
{
}

void
FlentHelper::EnableFlent(uint32_t node, std::string fileName)
{
  m_outputFileName = fileName;
  Simulator::ScheduleDestroy(&FlentHelper::WriteToJson, this);
}

void
FlentHelper::TraceCwnd(uint32_t oldVal, uint32_t newVal)
{
  m_cwndData.emplace_back(Simulator::Now().GetSeconds(), newVal);
}

void
FlentHelper::WriteToJson()
{
  std::ofstream file;
  file.open(m_outputFileName);
  file << "{\n  \"cwnd\": [\n";
  for (size_t i = 0; i < m_cwndData.size(); ++i)
    {
      file << "    [" << m_cwndData[i].first << ", " << m_cwndData[i].second << "]";
      if (i < m_cwndData.size() - 1)
        {
          file << ",";
        }
      file << "\n";
    }
  file << "  ]\n}\n";
  file.close();
}

} // namespace ns3