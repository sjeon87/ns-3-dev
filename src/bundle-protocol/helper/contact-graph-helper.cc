#include "ns3/contact-graph-helper.h"
#include "ns3/contact-graph-routing.h"
#include "ns3/contact-parser.h"
#include "ns3/log.h"
#include "ns3/fatal-error.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE("ContactGraphHelper");

ContactGraphHelper::ContactGraphHelper() 
    : m_filename("") 
{
}

void 
ContactGraphHelper::SetContactPlan(const std::string& filename) 
{
    m_filename = filename;
}

Ptr<ContactGraph> 
ContactGraphHelper::Install() 
{
    if (m_filename.empty()) {
        NS_FATAL_ERROR("Contact plan filename not set. Call SetContactPlan() before Install().");
    }

    Ptr<ContactGraph> graph = CreateObject<ContactGraph>();
    
    bool success = ContactParser::ParseFile(m_filename, graph);
    
    if (!success) {
        NS_FATAL_ERROR("Failed to parse the contact plan: " << m_filename);
    }

    NS_LOG_INFO("ContactGraphHelper successfully installed the graph.");

    return graph;
}

} // namespace ns3