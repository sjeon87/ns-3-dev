#ifndef CONTACT_PARSER_H
#define CONTACT_PARSER_H

#include "contact-graph-routing.h"

#include "ns3/net-device-container.h"
#include "ns3/node-container.h"
#include "ns3/ptr.h"

#include <string>

namespace ns3
{

class ContactParser
{
  public:
    /**
     * @brief Parses a contact plan file and populates the given ContactGraph.
     * @param filename The path to the text file containing the contact data.
     * @param contactGraph A pointer to the ContactGraph instance to populate.
     * @return true if successful, false if the file could not be read.
     */
    static bool ParseFile(const std::string& filename, Ptr<ContactGraph> contactGraph);

    static std::vector<NetDeviceContainer> CreateP2pLinks(const std::string& filename,
                                                          NodeContainer nodes,
                                                          const std::vector<std::string>& nodeUris);
};

} // namespace ns3

#endif /* CONTACT_PARSER_H */
