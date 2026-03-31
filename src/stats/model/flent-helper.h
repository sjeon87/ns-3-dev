#ifndef FLENT_HELPER_H
#define FLENT_HELPER_H

#include "ns3/object.h"
#include "ns3/ptr.h"
#include "ns3/type-id.h"

#include <string>
#include <vector>

namespace ns3
{

/**
 * @ingroup stats
 * @brief Helper class to trace data and export it in a Flent-compatible JSON format.
 */
class FlentHelper : public Object
{
  public:
    /**
     * @brief Register this type.
     * @return The TypeId.
     */
    static TypeId GetTypeId(void);

    FlentHelper();
    virtual ~FlentHelper();

    /**
     * @brief Enables tracing for a specific node and sets the output filename.
     * @param node The node ID to trace.
     * @param fileName The name of the JSON file to be generated.
     */
    void EnableFlent(uint32_t node, std::string fileName);

    /**
     * @brief Trace sink for Congestion Window changes.
     * @param oldVal The previous CWND value.
     * @param newVal The new CWND value.
     */
    void TraceCwnd(uint32_t oldVal, uint32_t newVal);

    /**
     * @brief Writes the collected trace data to the specified JSON file.
     */
    void WriteToJson();

  private:
    /** Collected CWND data points (timestamp, value). */
    std::vector<std::pair<double, uint32_t>> m_cwndData;
    /** Output filename for the JSON data. */
    std::string m_outputFileName;
};

} // namespace ns3

#endif // FLENT_HELPER_H
