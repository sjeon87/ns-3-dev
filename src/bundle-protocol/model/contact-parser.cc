#include "contact-parser.h"

#include "ns3/data-rate.h"
#include "ns3/log.h"
#include "ns3/point-to-point-helper.h"
#include "ns3/simulator.h"
#include "ns3/string.h"

#include <algorithm>
#include <fstream>
#include <map>
#include <set>
#include <sstream>
#include <tuple>
#include <vector>

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("ContactParser");

/**
 * @brief Type definition for a unique contact identifier.
 */
using ContactKey = std::tuple<std::string, std::string, double, double>;

/**
 * @brief Used for parsing contact attributes.
 */
struct ContactData
{
    uint32_t rate = 0;         ///< Data rate of the contact in bps
    double delaySeconds = 0.0; ///< Propagation delay across the link in seconds
};

bool
ContactParser::ParseFile(const std::string& filename, Ptr<BaseRoutingEngine> contactGraph)
{
    NS_LOG_FUNCTION(filename << contactGraph);

    std::ifstream file(filename);
    if (!file.is_open())
    {
        NS_LOG_ERROR("Failed to open contact file: " << filename);
        return false;
    }

    std::string line;
    std::map<ContactKey, ContactData> contactMap;
    std::set<std::string> uniqueNodes;

    while (std::getline(file, line))
    {
        std::istringstream iss(line);
        std::string action;
        std::string type;

        if (iss >> action >> type && action == "a" && (type == "contact" || type == "range"))
        {
            std::string startStr;
            std::string endStr;
            std::string fromUri;
            std::string toUri;
            double valueDouble = 0.0;

            if (!(iss >> startStr >> endStr >> fromUri >> toUri >> valueDouble))
            {
                NS_LOG_WARN("Skipping malformed contact-plan line: " << line);
                continue;
            }

            if (!startStr.empty() && startStr[0] == '+')
            {
                startStr.erase(0, 1);
            }
            if (!endStr.empty() && endStr[0] == '+')
            {
                endStr.erase(0, 1);
            }

            double startSeconds;
            double endSeconds;
            try
            {
                startSeconds = std::stod(startStr);
                endSeconds = std::stod(endStr);
            }
            catch (const std::exception&)
            {
                NS_LOG_WARN("Skipping contact-plan line with unparsable start/end time: " << line);
                continue;
            }

            if (startSeconds >= endSeconds)
            {
                NS_LOG_WARN("Skipping contact-plan line with non-positive-duration window "
                            << "(start=" << startSeconds << " end=" << endSeconds << "): " << line);
                continue;
            }

            ContactKey key = std::make_tuple(fromUri, toUri, startSeconds, endSeconds);

            if (type == "contact")
            {
                contactMap[key].rate = static_cast<uint32_t>(valueDouble);
            }
            else if (type == "range")
            {
                contactMap[key].delaySeconds = valueDouble;
            }

            uniqueNodes.insert(fromUri);
            uniqueNodes.insert(toUri);
        }
    }

    file.close();

    std::vector<std::string> eidList(uniqueNodes.begin(), uniqueNodes.end());
    contactGraph->InitializeMap(eidList);

    for (const auto& kv : contactMap)
    {
        std::string from = std::get<0>(kv.first);
        std::string to = std::get<1>(kv.first);
        Time tStart = Seconds(std::get<2>(kv.first));
        Time tEnd = Seconds(std::get<3>(kv.first));
        uint32_t rate = kv.second.rate;
        Time delay = Seconds(kv.second.delaySeconds);

        double durationSeconds = (tEnd - tStart).GetSeconds();
        auto totalVolume = static_cast<uint32_t>(static_cast<double>(rate) * durationSeconds / 8.0);

        NS_LOG_DEBUG("Contact " << from << " -> " << to << " | rate=" << rate << " bps"
                                << " | duration=" << durationSeconds << " s"
                                << " | totalVolume=" << totalVolume << " bytes");

        contactGraph->AddTimedContact(from, to, tStart, tEnd, rate, delay, totalVolume);

        Simulator::Schedule(tEnd, &BaseRoutingEngine::RemoveContact, contactGraph, from, to);
    }

    NS_LOG_INFO("Successfully scheduled "
                << contactMap.size() << " combined contacts/ranges across " << uniqueNodes.size()
                << " unique nodes.");
    return true;
}

} // namespace ns3
