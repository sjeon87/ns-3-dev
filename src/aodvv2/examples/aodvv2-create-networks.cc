/*
 * Copyright (c) 2024 University of Florence
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * This is an example script for AODVv2 manet routing protocol.
 *
 * Authors: Francesco Todino <todinofrancesco97@gmail.com>
 *          Tommaso Pecorella <tommaso.pecorella@unifi.it>
 */

#include "ns3/aodvv2-metric.h"
#include "ns3/aodvv2-module.h"
#include "ns3/core-module.h"
#include "ns3/energy-module.h"
#include "ns3/internet-module.h"
#include "ns3/mobility-module.h"
#include "ns3/network-module.h"
#include "ns3/ping-helper.h"
#include "ns3/point-to-point-module.h"
#include "ns3/wifi-radio-energy-model-helper.h"
#include "ns3/yans-wifi-helper.h"

#include <cmath>
#include <ctime>
#include <iostream>

using namespace ns3;
using namespace ns3::energy;

/**
 * @defgroup aodvv2-examples AODVv2 Examples
 * @ingroup aodvv2
 * @ingroup examples
 */

/**
 * @ingroup aodvv2-examples
 * @ingroup examples
 * @brief Create network script.
 *
 */
class Aodvv2CreateNetwork
{
  public:
    Aodvv2CreateNetwork();
    /**
     * @brief Configure script parameters
     * @param argc is the command line argument count
     * @param argv is the command line arguments
     * @return true on successful configuration
     */
    bool Configure(int argc, char** argv);
    /// Run simulation
    void Run();

  private:
    // parameters
    /// Number of nodes
    uint32_t numNodes;
    /// Distance between nodes, meters
    double step;
    /// Number of networks to create
    uint32_t numNetworks;
    /// Use input setup
    bool useInputSetup;
    /// Folder's output name
    std::string folderName;

    // network
    /// nodes used in the example
    NodeContainer nodes;
    /// Map of nodes and their corresponding MetricNode
    std::map<Ptr<Node>, aodvv2::MetricNode> m_metricNodes;
    /// Adjacency matrix
    std::vector<std::vector<int>> adjacencyMatrix;

  private:
    /// Create the nodes
    void CreateNodes();
    /// Create the adjacency matrix
    void CreateAdjacencyMatrix();
    /// Create the pairs
    void CreatePairs();
    /// Print node positions to CSV file
    void PrintNodePositionsToCsv(const std::string& filename);
    /// Get if there is a route between two nodes
    bool HasRoute(uint32_t src, uint32_t dst);
    /// Get if there is a route between two nodes using DFS
    bool HasRouteDFS(uint32_t src, uint32_t dst, std::vector<bool>& visited);
};

int
main(int argc, char** argv)
{
    Aodvv2CreateNetwork aodvv2CreateNetwork;
    if (!aodvv2CreateNetwork.Configure(argc, argv))
    {
        NS_FATAL_ERROR("Configuration failed. Aborted.");
    }

    aodvv2CreateNetwork.Run();
    return 0;
}

//-----------------------------------------------------------------------------
Aodvv2CreateNetwork::Aodvv2CreateNetwork()
    : numNodes(10),
      step(50),
      numNetworks(5),
      useInputSetup(false),
      folderName("_networks/")
{
}

bool
Aodvv2CreateNetwork::Configure(int argc, char** argv)
{
    // Enable AODVv2 logs by default. Comment this if too noisy
    // LogComponentEnable("Aodvv2RoutingProtocol", LOG_LEVEL_ALL);

    RngSeedManager::SetSeed(12345);
    CommandLine cmd(__FILE__);

    cmd.AddValue("folderName", "Folder's output name.", folderName);
    cmd.AddValue("useInputSetup", "Use input setup.", useInputSetup);
    cmd.AddValue("numNodes", "Number of nodes.", numNodes);
    cmd.AddValue("numNetworks", "Number of networks to create.", numNetworks);
    cmd.Parse(argc, argv);
    return true;
}

void
Aodvv2CreateNetwork::Run()
{
    if (!useInputSetup)
    {
        std::cout << "Enter the number of nodes: ";
        std::cin >> numNodes;

        std::cout << "Enter the number of networks to create: ";
        std::cin >> numNetworks;
    }

    std::ofstream file(folderName + "pairs.txt");
    file.close();

    for (uint32_t i = 0; i < numNetworks; ++i)
    {
        CreateNodes();
        CreateAdjacencyMatrix();
        CreatePairs();
        PrintNodePositionsToCsv(folderName + "node_positions_" + std::to_string(i) + ".csv");
    }
}

void
Aodvv2CreateNetwork::CreateNodes()
{
    nodes = NodeContainer();

    Ptr<UniformRandomVariable> rand = CreateObject<UniformRandomVariable>();
    std::cout << "Creating " << (unsigned)numNodes << " random nodes and step " << step
              << " m apart.\n";
    nodes.Create(numNodes);

    // Create positions based on topology
    MobilityHelper mobility;
    double dimension = step * 3;

    mobility.SetPositionAllocator(
        "ns3::RandomRectanglePositionAllocator",
        "X",
        StringValue("ns3::UniformRandomVariable[Min=0.0|Max=" + std::to_string(dimension) + "]"),
        "Y",
        StringValue("ns3::UniformRandomVariable[Min=0.0|Max=" + std::to_string(dimension) + "]"));

    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.Install(nodes);
}

void
Aodvv2CreateNetwork::CreateAdjacencyMatrix()
{
    adjacencyMatrix.assign(numNodes, std::vector<int>(numNodes, 0));

    for (uint32_t i = 0; i < numNodes; ++i)
    {
        Ptr<MobilityModel> nodeMobility = nodes.Get(i)->GetObject<MobilityModel>();

        for (uint32_t j = 0; j < numNodes; ++j)
        {
            if (i == j)
            {
                adjacencyMatrix[i][j] = 1;
                continue;
            }
            Ptr<MobilityModel> neighborMobility = nodes.Get(j)->GetObject<MobilityModel>();

            double distance = std::sqrt(
                std::pow(nodeMobility->GetPosition().x - neighborMobility->GetPosition().x, 2) +
                std::pow(nodeMobility->GetPosition().y - neighborMobility->GetPosition().y, 2));

            if (distance <= 50)
            {
                adjacencyMatrix[i][j] = 1;
            }
        }
    }
}

void
Aodvv2CreateNetwork::CreatePairs()
{
    uint32_t maxPings = numNodes / 2;
    Ptr<UniformRandomVariable> rand = CreateObject<UniformRandomVariable>();
    std::set<std::pair<uint32_t, uint32_t>> pairs;

    for (uint32_t i = 0; i < maxPings; ++i)
    {
        std::vector<uint32_t> nodesList(numNodes);
        std::iota(nodesList.begin(), nodesList.end(), 0);

        uint32_t srcNodeIndex = rand->GetInteger(0, numNodes - 1);
        uint32_t dstNodeIndex = srcNodeIndex;

        nodesList.erase(std::remove(nodesList.begin(), nodesList.end(), srcNodeIndex),
                        nodesList.end());

        while (nodesList.size() > 0)
        {
            dstNodeIndex = nodesList[rand->GetInteger(0, nodesList.size() - 1)];
            if (HasRoute(srcNodeIndex, dstNodeIndex) &&
                pairs.find(std::make_pair(srcNodeIndex, dstNodeIndex)) == pairs.end())
            {
                break;
            }
            nodesList.erase(std::remove(nodesList.begin(), nodesList.end(), dstNodeIndex),
                            nodesList.end());
        }

        if (nodesList.size() == 0 || srcNodeIndex == dstNodeIndex)
        {
            continue;
        }

        pairs.insert(std::make_pair(srcNodeIndex, dstNodeIndex));
    }

    std::ofstream file(folderName + "pairs.txt", std::ios_base::app);
    for (auto it = pairs.begin(); it != pairs.end(); ++it)
    {
        file << it->first << ":" << it->second;
        if (std::next(it) != pairs.end())
        {
            file << ",";
        }
    }
    file << "\n";
    file.close();
}

void
Aodvv2CreateNetwork::PrintNodePositionsToCsv(const std::string& filename)
{
    Ptr<UniformRandomVariable> rand = CreateObject<UniformRandomVariable>();
    std::ofstream file(filename);
    file << "x,y,z,battery,trust\n";

    for (uint32_t i = 0; i < nodes.GetN(); ++i)
    {
        Ptr<MobilityModel> mobility = nodes.Get(i)->GetObject<MobilityModel>();
        Vector pos = mobility->GetPosition();
        file << pos.x << "," << pos.y << "," << pos.z << "," << rand->GetInteger(5, 100) << ","
             << rand->GetInteger(0, 100) << "\n";
    }

    file.close();
}

bool
Aodvv2CreateNetwork::HasRoute(uint32_t src, uint32_t dst)
{
    std::vector<bool> visited(numNodes, false);
    return HasRouteDFS(src, dst, visited);
}

bool
Aodvv2CreateNetwork::HasRouteDFS(uint32_t src, uint32_t dst, std::vector<bool>& visited)
{
    if (src == dst)
    {
        return true;
    }

    visited[src] = true;

    for (size_t i = 0; i < adjacencyMatrix.size(); ++i)
    {
        if (adjacencyMatrix[src][i] == 1 && !visited[i])
        {
            if (HasRouteDFS(i, dst, visited))
            {
                return true;
            }
        }
    }

    return false;
}
