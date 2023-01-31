// filename -> blackhole.cc
#include "ns3/aodv-module.h"
#include "ns3/netanim-module.h"
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/applications-module.h"
#include "ns3/mobility-module.h"
#include "ns3/wifi-module.h"
#include "ns3/netanim-module.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/mobility-module.h"
#include "myapp.h"

NS_LOG_COMPONENT_DEFINE("Blackhole");
using namespace ns3;
void
ReceivePacket(Ptr <
  const Packet > p,
    const Address & addr) {
  std::cout << Simulator::Now().GetSeconds() << "\t" << p -> GetSize() << "\n";
}
int main(int argc, char * argv[]) {
  bool enableFlowMonitor = true;
  std::string phyMode("DsssRate1Mbps");
  CommandLine cmd;
  cmd.AddValue("EnableMonitor", "Enable Flow Monitor", enableFlowMonitor);
  cmd.AddValue("phyMode", "Wifi Phy mode", phyMode);
  cmd.Parse(argc, argv);
  // Explicitly create the nodes required by the topology.
  NS_LOG_INFO("Create nodes.");
  NodeContainer c; // ALL Nodes
  NodeContainer not_malicious;
  NodeContainer malicious;
  c.Create(20); //create 20 nodes
  for (int i = 0; i < 20; i++) {
    if (i != 0 && i != 5 && i != 15) {
    //if (1) {
      not_malicious.Add(c.Get(i));
    } else {
      malicious.Add(c.Get(i));
    }
  }
  // Set up WiFi
  WifiHelper wifi;
  YansWifiPhyHelper wifiPhy = YansWifiPhyHelper::Default();
  wifiPhy.SetPcapDataLinkType(YansWifiPhyHelper::DLT_IEEE802_11);
  YansWifiChannelHelper wifiChannel;
  wifiChannel.SetPropagationDelay("ns3::ConstantSpeedPropagationDelayModel");
  wifiChannel.AddPropagationLoss("ns3::TwoRayGroundPropagationLossModel",
    "SystemLoss", DoubleValue(1),
    "HeightAboveZ", DoubleValue(1.5));
  // For range near 250m
  wifiPhy.Set("TxPowerStart", DoubleValue(33));
  wifiPhy.Set("TxPowerEnd", DoubleValue(33));
  wifiPhy.Set("TxPowerLevels", UintegerValue(1));
  wifiPhy.Set("TxGain", DoubleValue(0));
  wifiPhy.Set("RxGain", DoubleValue(0));
  wifiPhy.Set("RxSensitivity", DoubleValue(-61.8));
  wifiPhy.Set("CcaEdThreshold", DoubleValue(-64.8));
  wifiPhy.SetChannel(wifiChannel.Create());
  // Add a non-QoS upper mac
  WifiMacHelper wifiMac;
  wifiMac.SetType("ns3::AdhocWifiMac");
  // Set 802.11b standard
  wifi.SetStandard(WIFI_PHY_STANDARD_80211b);
  wifi.SetRemoteStationManager("ns3::ConstantRateWifiManager",
    "DataMode", StringValue(phyMode),
    "ControlMode", StringValue(phyMode));
  NetDeviceContainer devices;
  devices = wifi.Install(wifiPhy, wifiMac, c);
  // Enable AODV
  AodvHelper aodv;
  AodvHelper malicious_aodv;
  // Set up internet stack
  InternetStackHelper internet;
  internet.SetRoutingHelper(aodv);
  internet.Install(not_malicious);
  // false gives us the normal behaviour
  malicious_aodv.Set("IsMalicious", BooleanValue(true)); 
  internet.SetRoutingHelper(malicious_aodv);
  internet.Install(malicious);
  // Set up Addresses
  Ipv4AddressHelper ipv4;
  NS_LOG_INFO("Assign IP Addresses.");
  ipv4.SetBase("10.1.1.0", "255.255.255.0");
  Ipv4InterfaceContainer ifcont = ipv4.Assign(devices);
  NS_LOG_INFO("Create Applications.");
  // UDP connection from N5 to N15
  uint16_t sinkPort = 15;
  Address sinkAddress(InetSocketAddress(ifcont.GetAddress(5), sinkPort));
  PacketSinkHelper packetSinkHelper("ns3::UdpSocketFactory",
    InetSocketAddress(Ipv4Address::GetAny(),
      sinkPort));
  ApplicationContainer sinkApps = packetSinkHelper.Install(c.Get(5));
  sinkApps.Start(Seconds(0.));
  sinkApps.Stop(Seconds(1000.));
  //source at N5
  Ptr < Socket > ns3UdpSocket = Socket::CreateSocket(c.Get(5),
    UdpSocketFactory::GetTypeId());
  // Create UDP application at N5
  Ptr < MyApp > app = CreateObject < MyApp > ();
  app -> Setup(ns3UdpSocket, sinkAddress, 1024, 48, DataRate("1000Kbps"));
  c.Get(7) -> AddApplication(app);
  app -> SetStartTime(Seconds(15.));
  app -> SetStopTime(Seconds(1000.));
  // Set Mobility for all nodes
  MobilityHelper mobility;
  Ptr < ListPositionAllocator > positionAlloc =
    CreateObject < ListPositionAllocator > ();
  int pos = 200;
  for (int i = 0; i < 20; i++) {
    positionAlloc -> Add(Vector(pos, 0, 0));
    pos = pos + 200;
  }
  mobility.SetPositionAllocator(positionAlloc);
 // mobility.SetMobilityModel("ns3::RandomWaypointMobilityModel", "PositionAllocator", PointerValue(positionAlloc) );
  mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
  mobility.Install(c);
  AnimationInterface anim("blackhole.xml");
  int pos2 = 0;
  for (int i = 0; i < 10; i++) {
    AnimationInterface::SetConstantPosition(c.Get(i), pos2, 500);
    pos2 = pos2 + 150;
  }
  pos2 = 0;
  for (int i = 10; i < 20; i++) {
    AnimationInterface::SetConstantPosition(c.Get(i), pos2, 600);
    pos2 = pos2 + 150;
  }
  anim.EnablePacketMetadata(true);
  Ptr < OutputStreamWrapper > routingStream =
    Create < OutputStreamWrapper > ("blackhole.routes", std::ios::out);
  aodv.PrintRoutingTableAllAt(Seconds(45), routingStream);
  // Trace Received Packets
  Config::ConnectWithoutContext(
    "/NodeList/*/ApplicationList/*/$ns3::PacketSink/Rx",
    MakeCallback( & ReceivePacket));
  // Calculate Throughput using Flowmonitor
  FlowMonitorHelper flowmon;
  Ptr < FlowMonitor > monitor = flowmon.InstallAll();
  // Run simulation
  NS_LOG_INFO("Run Simulation.");
  Simulator::Stop(Seconds(1000.0));
  Simulator::Run();
  monitor -> CheckForLostPackets();
  Ptr < Ipv4FlowClassifier > classifier =
    DynamicCast < Ipv4FlowClassifier > (flowmon.GetClassifier());
  std::map < FlowId, FlowMonitor::FlowStats > stats = monitor -> GetFlowStats();
  for (
    std::map < FlowId, FlowMonitor::FlowStats > ::const_iterator i = stats.begin(); i != stats.end();
    ++i) {
    Ipv4FlowClassifier::FiveTuple t = classifier -> FindFlow(i -> first);
    std::cout << "Flow " << i -> first << " (" <<
      t.sourceAddress << " -> " << t.destinationAddress << ")\n";
    std::cout << " Tx Bytes: " << i -> second.txBytes << "\n";
    std::cout << " Rx Bytes: " << i -> second.rxBytes << "\n";
    std::cout << " Throughput: " <<
      i -> second.rxBytes * 8.0 /
      (i -> second.timeLastRxPacket.GetSeconds() -
        i -> second.timeFirstTxPacket.GetSeconds()) /
      1024 / 1024 <<
      " Mbps\n";
  }
  monitor -> SerializeToXmlFile("blackhole.flowmon", true, true); // output file
}
