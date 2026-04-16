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
#include <vector>

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("ContactParser");

struct ParsedContact
{
    std::string from;
    std::string to;
    double startTime;
    double endTime;
    uint32_t rate;
};

bool
ContactParser::ParseFile(const std::string& filename, Ptr<ContactGraph> contactGraph)
{
    NS_LOG_FUNCTION(filename << contactGraph);

    std::ifstream file(filename);
    if (!file.is_open())
    {
        NS_LOG_ERROR("Failed to open contact file: " << filename);
        return false;
    }

    std::string line;
    std::vector<ParsedContact> contacts;
    std::set<std::string> uniqueNodes;

    while (std::getline(file, line))
    {
        std::istringstream iss(line);
        std::string action;
        std::string type;
        iss >> action >> type;

        if (action == "a" && type == "contact")
        {
            std::string startStr;
            std::string endStr;
            std::string fromUri;
            std::string toUri;
            double rateDouble;
            iss >> startStr >> endStr >> fromUri >> toUri >> rateDouble;
            if (!startStr.empty() && startStr[0] == '+')
            {
                startStr.erase(0, 1);
            }
            if (!endStr.empty() && endStr[0] == '+')
            {
                endStr.erase(0, 1);
            }

            double startSeconds = std::stod(startStr);
            double endSeconds = std::stod(endStr);
            auto rate = static_cast<uint32_t>(rateDouble);

            if (startSeconds == endSeconds)
            {
                continue;
            }

            contacts.push_back({fromUri, toUri, startSeconds, endSeconds, rate});
            uniqueNodes.insert(fromUri);
            uniqueNodes.insert(toUri);
        }
    }

    file.close();

    std::vector<std::string> eidList(uniqueNodes.begin(), uniqueNodes.end());
    contactGraph->InitializeMap(eidList);

    for (const auto& contact : contacts)
    {
        Time tStart = Seconds(contact.startTime);
        Time tEnd = Seconds(contact.endTime);

        Simulator::Schedule(tStart,
                            &ContactGraph::AddContact,
                            contactGraph,
                            contact.from,
                            contact.to,
                            contact.rate);

        Simulator::Schedule(tEnd,
                            &ContactGraph::RemoveContact,
                            contactGraph,
                            contact.from,
                            contact.to);
    }

    NS_LOG_INFO("Successfully scheduled " << contacts.size() << " contacts across "
                                          << uniqueNodes.size() << " unique nodes.");
    return true;
}

std::vector<NetDeviceContainer>
ContactParser::CreateP2pLinks(const std::string& filename,
                              NodeContainer nodes,
                              const std::vector<std::string>& nodeUris)
{
    NS_LOG_FUNCTION(filename);
    std::vector<NetDeviceContainer> createdDevices;

    std::map<std::string, uint32_t> uriToIndex;
    for (uint32_t i = 0; i < nodeUris.size(); ++i)
    {
        uriToIndex[nodeUris[i]] = i;
    }

    std::ifstream file(filename);
    if (!file.is_open())
    {
        NS_LOG_ERROR("Failed to open file to create P2P links: " << filename);
        return createdDevices;
    }

    std::string line;

    struct LinkProps
    {
        double dataRate = 0.0;
        double delaySec = 0.0;
    };

    std::map<std::pair<uint32_t, uint32_t>, LinkProps> linkMap;

    while (std::getline(file, line))
    {
        std::istringstream iss(line);
        std::string action;
        std::string type;
        iss >> action >> type;

        if (action == "a" && (type == "contact" || type == "range"))
        {
            std::string startStr;
            std::string endStr;
            std::string fromUri;
            std::string toUri;
            double value;

            iss >> startStr >> endStr >> fromUri >> toUri >> value;

            if (uriToIndex.find(fromUri) == uriToIndex.end() ||
                uriToIndex.find(toUri) == uriToIndex.end())
            {
                continue;
            }

            uint32_t u = uriToIndex[fromUri];
            uint32_t v = uriToIndex[toUri];
            uint32_t minNode = std::min(u, v);
            uint32_t maxNode = std::max(u, v);
            auto key = std::make_pair(minNode, maxNode);

            if (type == "contact")
            {
                if (value > linkMap[key].dataRate)
                {
                    linkMap[key].dataRate = value;
                }
            }
            else if (type == "range")
            {
                if (value > linkMap[key].delaySec)
                {
                    linkMap[key].delaySec = value;
                }
            }
        }
    }
    file.close();

    for (const auto& pair : linkMap)
    {
        uint32_t u = pair.first.first;
        uint32_t v = pair.first.second;
        double rate = pair.second.dataRate;
        double delay = pair.second.delaySec;

        if (u >= nodes.GetN() || v >= nodes.GetN())
        {
            continue;
        }

        if (rate > 0)
        {
            PointToPointHelper p2p;
            p2p.SetDeviceAttribute("DataRate",
                                   DataRateValue(DataRate(static_cast<uint64_t>(rate))));
            p2p.SetChannelAttribute("Delay", TimeValue(Seconds(delay)));

            NodeContainer linkNodes;
            linkNodes.Add(nodes.Get(u));
            linkNodes.Add(nodes.Get(v));

            NetDeviceContainer devices = p2p.Install(linkNodes);
            createdDevices.push_back(devices);

            NS_LOG_INFO("Installed P2P channel between "
                        << nodeUris[u] << " and " << nodeUris[v] << " | Rate: "
                        << static_cast<uint64_t>(rate) << " bps | Delay: " << delay << " s");
        }
    }

    return createdDevices;
}

} // namespace ns3
