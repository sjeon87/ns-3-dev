#include "contact-parser.h"
#include "ns3/log.h"
#include "ns3/simulator.h"
#include <fstream>
#include <sstream>
#include <set>
#include <vector>

namespace ns3 {

NS_LOG_COMPONENT_DEFINE("ContactParser");

struct ParsedContact {
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
    if (!file.is_open()) {
        NS_LOG_ERROR("Failed to open contact file: " << filename);
        return false;
    }

    std::string line;
    std::vector<ParsedContact> contacts;
    std::set<std::string> uniqueNodes;

    while (std::getline(file, line)) {
        if (line.empty() || line.find("a contact") == std::string::npos) {
            continue;
        }

        std::istringstream iss(line);
        std::string action, type, startStr, endStr, fromNode, toNode;
        double rateDouble;

        iss >> action >> type >> startStr >> endStr >> fromNode >> toNode >> rateDouble;

        if (action == "a" && type == "contact") {
            uint32_t rate = static_cast<uint32_t>(rateDouble);
            
            double startSeconds = std::stod(startStr);
            double endSeconds = std::stod(endStr);

            contacts.push_back({fromNode, toNode, startSeconds, endSeconds, rate});
            
            uniqueNodes.insert(fromNode);
            uniqueNodes.insert(toNode);
        }
    }

    file.close();

    std::vector<std::string> eidList(uniqueNodes.begin(), uniqueNodes.end());
    contactGraph->InitializeMap(eidList);

    for (const auto& contact : contacts) {
        
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

    NS_LOG_INFO("Successfully scheduled " << contacts.size() << " contacts across " << uniqueNodes.size() << " unique nodes.");
    return true;
}

} // namespace ns3