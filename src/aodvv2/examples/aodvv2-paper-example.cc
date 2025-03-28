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

 #include "ns3/aodv-module.h"
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
  * @brief Test script.
  *
  * This script creates a random network topology and then send some pings from random nodes:
  *
  *  Examples:
  *
  *  Node1 ------ Node2      Node1 ----- Node2
  *    |        /    |         |         |   \
  *    |       /     |         |         |    Node3
  *  Node3    /    Node4     Node4 ------+-----^
  *    |     /                 |          \
  *    |    /                  |          Node5
  *  Node5                   Node6
  *
  */
 class Aodvv2PaperExample
 {
   public:
     Aodvv2PaperExample();
     /**
      * @brief Configure script parameters
      * @param argc is the command line argument count
      * @param argv is the command line arguments
      * @return true on successful configuration
      */
     bool Configure(int argc, char** argv);
     /// Run simulation
     void Run();
     /**
      * Report results
      * @param os the output stream
      */
     void Report(std::ostream& os);
 
   private:
     // parameters
     /// Number of nodes
     uint32_t size;
     /// Distance between nodes, meters
     double step;
     /// Simulation time, seconds
     double totalTime;
     /// Write per-device PCAP traces if true
     bool pcap;
     /// Print routes if true
     bool printRoutes;
     /// Type of routing protocol
     std::string routingProtocol;
     /// Ping pairs
     std::string pingPairs;
     /// Type of network topology
     std::string topologyType;
     /// File containing the network topology
     std::string topologyFile;
     /// Print node positions to file if true
     bool printPositions = false;
     /// Save results to file path
     std::string resultFile;
 
     // network
     /// nodes used in the example
     NodeContainer nodes;
     /// devices used in the example
     NetDeviceContainer devices;
     /// interfaces used in the example
     Ipv4InterfaceContainer interfaces;
     /// Energy models
     DeviceEnergyModelContainer energyModels;
     /// Energy sources
     EnergySourceContainer energySources;
     /// Adjacency matrix
     std::vector<std::vector<int>> adjacencyMatrix;
     /// Map of nodes and their corresponding MetricNode
     std::map<Ptr<Node>, aodvv2::MetricNode> m_metricNodes;
 
   private:
     /// Create the nodes
     void CreateNodes();
     /// Create the devices
     void CreateDevices();
     /// Create the network
     void InstallInternetStack();
     /// Create the simulation applications
     void InstallApplications();
     /// Add a ping application
     void AddPingApplication(uint32_t srcNodeIndex, uint32_t dstNodeIndex, Time offset);
     /// Create the adjacency matrix
     void CreateAdjacencyMatrix();
     /// Print nodes positions
     void PrintNodes();
     /// Add a metric node to the map
     bool AddMetricNode(Ptr<Node> node, const aodvv2::MetricNode& metricNode);
     /// Get if there is a route between two nodes
     bool HasRoute(uint32_t src, uint32_t dst);
     /// Get if there is a route between two nodes using DFS
     bool HasRouteDFS(uint32_t src, uint32_t dst, std::vector<bool>& visited);
     /// Save node positions to CSV file
     void SaveNodePositionsToCsv(const std::string& filename);
     /// Read topology from file
     void ReadTopologyFromFile(const std::string& filename);
     /// Save simulation results to file
     void SaveSimulationResults(const std::string& filename);
 };
 
 int
 main(int argc, char** argv)
 {
     Aodvv2PaperExample test;
     if (!test.Configure(argc, argv))
     {
         NS_FATAL_ERROR("Configuration failed. Aborted.");
     }
 
     test.Run();
     test.Report(std::cout);
     return 0;
 }
 
 //-----------------------------------------------------------------------------
 Aodvv2PaperExample::Aodvv2PaperExample()
     : size(10),
       step(50),
       totalTime(10),
       pcap(true),
       printRoutes(true),
       routingProtocol("aodvv2"),
       topologyType("random")
 {
 }
 
 bool
 Aodvv2PaperExample::Configure(int argc, char** argv)
 {
     // Enable AODVv2 logs by default. Comment this if too noisy
     // LogComponentEnable("Aodvv2RoutingProtocol", LOG_LEVEL_ALL);
 
     RngSeedManager::SetSeed(12345);
     CommandLine cmd(__FILE__);
 
     cmd.AddValue("pcap", "Write PCAP traces.", pcap);
     cmd.AddValue("printRoutes", "Print routing table dumps.", printRoutes);
     cmd.AddValue("size", "Number of nodes.", size);
     cmd.AddValue("time", "Simulation time, s.", totalTime);
     cmd.AddValue("step", "Grid step, m", step);
     cmd.AddValue("routingProtocol", "Type of routing protocol", routingProtocol);
     cmd.AddValue("pingPairs", "Ping pairs.", pingPairs);
     cmd.AddValue("topologyType", "Type of network topology (random, custom, circle)", topologyType);
     cmd.AddValue("topologyFile", "File containing the network topology.", topologyFile);
     cmd.AddValue("printPositions", "Print node positions to file.", printPositions);
     cmd.AddValue("resultFile", "Save results to file path.", resultFile);
     cmd.Parse(argc, argv);
     return true;
 }
 
 void
 Aodvv2PaperExample::Run()
 {
     if (topologyType == "custom")
     {
         size = 5;
         step = 5;
     }
 
     //  Config::SetDefault ("ns3::WifiRemoteStationManager::RtsCtsThreshold", UintegerValue (1)); //
     //  enable rts cts all the time.
     if (topologyFile.empty())
     {
         CreateNodes();
     }
     else
     {
         ReadTopologyFromFile(topologyFile);
     }
 
     CreateDevices();
     CreateAdjacencyMatrix();
     InstallInternetStack();
     InstallApplications();
 
     if (printPositions)
     {
         SaveNodePositionsToCsv("node_positions.csv");
     }
 
     // PrintNodes();
 
     std::cout << "Starting simulation for " << totalTime << " s ...\n";
 
     Simulator::Stop(Seconds(totalTime));
     Simulator::Run();
 
     if (!resultFile.empty())
     {
         SaveSimulationResults(resultFile);
     }
 
     Simulator::Destroy();
 }
 
 void
 Aodvv2PaperExample::Report(std::ostream&)
 {
 }
 
 void
 Aodvv2PaperExample::CreateNodes()
 {
     Ptr<UniformRandomVariable> rand = CreateObject<UniformRandomVariable>();
     std::cout << "Creating " << (unsigned)size << " nodes with topology type '" << topologyType
               << "' and step " << step << " m apart.\n";
     nodes.Create(size);
     // Name nodes
     for (uint32_t i = 0; i < size; ++i)
     { 
         std::ostringstream os;
         os << "node-" << i;
         Names::Add(os.str(), nodes.Get(i));
 
         AddMetricNode(nodes.Get(i),
                       aodvv2::MetricNode(nodes.Get(i)));
     }
 
     // Create positions based on topology
     MobilityHelper mobility;
     double dimension = step * 3;
 
     if (topologyType == "custom")
     {
         Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator>();
 
         positionAlloc->Add(Vector(0.0, 0.0, 0.0));
         positionAlloc->Add(Vector(step * 8, 0.0, 0.0));
         positionAlloc->Add(Vector(0.0, step * 8, 0.0));
         positionAlloc->Add(Vector(step * 8, step * 8, 0.0));
         positionAlloc->Add(Vector(step * 15, step * 15, 0.0));
 
         mobility.SetPositionAllocator(positionAlloc);
     }
     else if (topologyType == "random")
     {
         mobility.SetPositionAllocator(
             "ns3::RandomRectanglePositionAllocator",
             "X",
             StringValue("ns3::UniformRandomVariable[Min=0.0|Max=" + std::to_string(dimension) +
                         "]"),
             "Y",
             StringValue("ns3::UniformRandomVariable[Min=0.0|Max=" + std::to_string(dimension) +
                         "]"));
     }
     else if (topologyType == "circle")
     {
         Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator>();
         double radius = (step * size) / (2 * M_PI);
         double angleStep = 2 * M_PI / size;
 
         for (uint32_t i = 0; i < size; ++i)
         {
             double x = radius * std::cos(i * angleStep);
             double y = radius * std::sin(i * angleStep);
             positionAlloc->Add(Vector(x, y, 0.0));
         }
 
         mobility.SetPositionAllocator(positionAlloc);
     }
     else
     {
         NS_FATAL_ERROR("Unknown topology type: " << topologyType);
     }
 
     mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
     mobility.Install(nodes);
 }
 
 void
 Aodvv2PaperExample::CreateDevices()
 {
     WifiMacHelper wifiMac;
     wifiMac.SetType("ns3::AdhocWifiMac");
     YansWifiPhyHelper wifiPhy;
     YansWifiChannelHelper wifiChannel = YansWifiChannelHelper::Default();
     wifiPhy.SetChannel(wifiChannel.Create());
     WifiHelper wifi;
     wifi.SetRemoteStationManager("ns3::ConstantRateWifiManager",
                                  "DataMode",
                                  StringValue("OfdmRate6Mbps"),
                                  "RtsCtsThreshold",
                                  UintegerValue(0));
     devices = wifi.Install(wifiPhy, wifiMac, nodes);
 
     WifiRadioEnergyModelHelper radioEnergyHelper;
     radioEnergyHelper.Set("TxCurrentA", DoubleValue(0.0174));
     energyModels = radioEnergyHelper.Install(devices, energySources);
 
     if (pcap)
     {
         wifiPhy.EnablePcapAll(std::string(routingProtocol + "-e2"));
     }
 }
 
 void
 Aodvv2PaperExample::InstallInternetStack()
 {
     InternetStackHelper stack;
 
     if (routingProtocol == "aodv")
     {
         AodvHelper aodv;
         stack.SetRoutingHelper(aodv);
     }
     else if (routingProtocol == "aodvv2")
     {
         Aodvv2Helper<Ipv4RoutingHelper> aodvv2;
         stack.SetRoutingHelper(aodvv2);
     }
     else
     {
         NS_FATAL_ERROR("Unknown routing protocol: " << routingProtocol);
     }
 
     stack.Install(nodes);
     Ipv4AddressHelper address;
     address.SetBase("10.0.0.0", "255.0.0.0");
     interfaces = address.Assign(devices);
 
     if (printRoutes)
     {
         std::ofstream file;
         file.open("output-route.csv");
         file << "source,destination,hops\n";
         file.close();
 
         Ptr<OutputStreamWrapper> routingStream =
             Create<OutputStreamWrapper>(routingProtocol + ".routes", std::ios::out);
         Ipv4RoutingHelper::PrintRoutingTableAllAt(Seconds(8), routingStream);
     }
 }
 
 void
 Aodvv2PaperExample::InstallApplications()
 {
     std::cout << "\n";
     if (pingPairs != "")
     {
         std::vector<std::string> pairs;
         std::stringstream ss(pingPairs);
         std::string pair;
 
         while (std::getline(ss, pair, ','))
         {
             pairs.push_back(pair);
         }
 
         for (const auto& p : pairs)
         {
             uint32_t srcNodeIndex = std::stoul(p.substr(0, p.find(':')));
             uint32_t dstNodeIndex = std::stoul(p.substr(p.find(':') + 1));
             AddPingApplication(srcNodeIndex, dstNodeIndex, Seconds(0));
         }
     }
     else
     {
         if (topologyType == "custom")
         {
             uint32_t srcNodeIndex = 0;
             uint32_t dstNodeIndex = size - 1;
 
             std::cout << "Node " << srcNodeIndex << " pinging " << dstNodeIndex << std::endl;
 
             AddPingApplication(srcNodeIndex, dstNodeIndex, Seconds(0));
         }
         else
         {
             uint32_t maxPings = size / 2;
             Ptr<UniformRandomVariable> rand = CreateObject<UniformRandomVariable>();
             std::set<std::pair<uint32_t, uint32_t>> pairs;
 
             for (uint32_t i = 0; i < maxPings; ++i)
             {
                 std::vector<uint32_t> nodesList(size);
                 std::iota(nodesList.begin(), nodesList.end(), 0);
 
                 uint32_t srcNodeIndex = rand->GetInteger(0, size - 1);
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
 
                 std::cout << "Node " << srcNodeIndex << " pinging " << dstNodeIndex << std::endl;
 
                 AddPingApplication(srcNodeIndex, dstNodeIndex, MilliSeconds(100 * i));
 
                 pairs.insert(std::make_pair(srcNodeIndex, dstNodeIndex));
             }
         }
     }
 }
 
 void
 Aodvv2PaperExample::AddPingApplication(uint32_t srcNodeIndex, uint32_t dstNodeIndex, Time offset)
 {
     PingHelper ping(interfaces.GetAddress(dstNodeIndex));
     ping.SetAttribute("VerboseMode", EnumValue(Ping::VerboseMode::VERBOSE));
 
     ApplicationContainer p = ping.Install(nodes.Get(srcNodeIndex));
     p.Start(offset);
     p.Stop(Seconds(totalTime) - Seconds(0.001));
 }
 
 void
 Aodvv2PaperExample::CreateAdjacencyMatrix()
 {
     adjacencyMatrix.assign(size, std::vector<int>(size, 0));
 
     for (uint32_t i = 0; i < size; ++i)
     {
         Ptr<MobilityModel> nodeMobility = nodes.Get(i)->GetObject<MobilityModel>();
 
         for (uint32_t j = 0; j < size; ++j)
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
 Aodvv2PaperExample::PrintNodes()
 {
     std::cout << "\n\n";
 
     for (uint32_t i = 0; i < size; ++i)
     {
         Ptr<Node> node = nodes.Get(i);
         Ptr<Ipv4> ipv4 = node->GetObject<Ipv4>();
         Ipv4InterfaceAddress addr = ipv4->GetAddress(1, 0);
         Ipv4Address ip = addr.GetLocal();
         Ptr<MobilityModel> nodeMobility = nodes.Get(i)->GetObject<MobilityModel>();
 
         std::cout << "Node " << i << " IP " << ip << " at " << nodeMobility->GetPosition()
                   << " neighbors: ";
 
         for (uint32_t j = 0; j < size; ++j)
         {
             if (adjacencyMatrix[i][j] == 1)
             {
                 std::cout << j << " ";
             }
         }
         std::cout << std::endl;
     }
 
     std::cout << "\nAdjacency Matrix:\n";
     for (uint32_t i = 0; i < size; ++i)
     {
         for (uint32_t j = 0; j < size; ++j)
         {
             std::cout << adjacencyMatrix[i][j] << " ";
         }
         std::cout << std::endl;
     }
     std::cout << "\n\n";
 }
 
 bool
 Aodvv2PaperExample::AddMetricNode(Ptr<Node> node, const aodvv2::MetricNode& metricNode)
 {
     auto result = m_metricNodes.insert(std::make_pair(node, metricNode));
     return result.second;
 }
 
 bool
 Aodvv2PaperExample::HasRoute(uint32_t src, uint32_t dst)
 {
     std::vector<bool> visited(size, false);
     return HasRouteDFS(src, dst, visited);
 }
 
 bool
 Aodvv2PaperExample::HasRouteDFS(uint32_t src, uint32_t dst, std::vector<bool>& visited)
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
 
 void
 Aodvv2PaperExample::SaveNodePositionsToCsv(const std::string& filename)
 {
     Ptr<UniformRandomVariable> rand = CreateObject<UniformRandomVariable>();
     std::ofstream file(filename);
     file << "x,y,z\n";
 
     for (uint32_t i = 0; i < nodes.GetN(); ++i)
     {
         Ptr<MobilityModel> mobility = nodes.Get(i)->GetObject<MobilityModel>();
         Vector pos = mobility->GetPosition();
         file << pos.x << "," << pos.y << "," << pos.z << "\n";
     }
 
     file.close();
 }
 
 void
 Aodvv2PaperExample::ReadTopologyFromFile(const std::string& filename)
 {
     std::ifstream file(filename);
     if (!file.is_open())
     {
         NS_FATAL_ERROR("Unable to open topology file: " << filename);
     }
 
     std::string line;
     // Skip the first line (header)
     std::getline(file, line);
     size = 0;
 
     while (std::getline(file, line))
     {
         size++;
         std::istringstream iss(line);
         std::vector<std::string> tokens;
         std::string token;
 
         while (std::getline(iss, token, ','))
         {
             tokens.push_back(token);
         }
 
         if (tokens.size() < 5)
         {
             NS_FATAL_ERROR("Invalid line in topology file: " << line);
         }
 
         double x = std::stod(tokens[0]);
         double y = std::stod(tokens[1]);
         double z = std::stod(tokens[2]);
 
         Ptr<Node> node = CreateObject<Node>();
         nodes.Add(node);
 
         std::ostringstream os;
         os << "node-" << nodes.GetN() - 1;
         Names::Add(os.str(), node);
 
         Ptr<ConstantPositionMobilityModel> mobility = CreateObject<ConstantPositionMobilityModel>();
         mobility->SetPosition(Vector(x, y, z));
         node->AggregateObject(mobility);
 
         AddMetricNode(node, aodvv2::MetricNode(node));
     }
 
     file.close();
 }
 
 void
 Aodvv2PaperExample::SaveSimulationResults(const std::string& filename)
 {
     std::ofstream file(filename, std::ios_base::app);
     if (!file.is_open())
     {
         std::cerr << "Unable to open file: " << filename << std::endl;
         return;
     }
 
     file.close();
 }
 