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
#include "ns3/rng-seed-manager.h"
#include "myapp.h"

#include <iostream>
#include <iomanip>
#include <ctime>
#include <sstream>
#include <time.h>

NS_LOG_COMPONENT_DEFINE("Blackhole");
using namespace ns3;

double GetDistance_From (Ptr<Node> node1, Ptr<Node> node2);

void ReceivePacket(Ptr <
              const Packet > p,
              const Address & addr) {
	// std::cout << Simulator::Now().GetSeconds() << "\t" << p -> GetSize() << "\n"; 
}
int main(int argc, char * argv[]) {
	int timen = time(NULL);
	RngSeedManager::SetSeed( timen );
	LogComponentEnable ("AodvRoutingProtocol", LOG_LEVEL_WARN); // LogComponentEnable ("Blackhole", LOG_LEVEL_WARN);
	NS_LOG_WARN ("Main started ");
	bool enableFlowMonitor = true;
	std::string phyMode("DsssRate1Mbps");
	CommandLine cmd;
	cmd.AddValue("EnableMonitor", "Enable Flow Monitor", enableFlowMonitor);
	cmd.AddValue("phyMode", "Wifi Phy mode", phyMode);
	cmd.Parse(argc, argv);
	NS_LOG_INFO("Create nodes.");
	NodeContainer c; // ALL Nodes
	NodeContainer not_malicious;
	NodeContainer malicious;
	NodeContainer constant_nodes;
	NodeContainer random_nodes;

	c.Create(20); //create 20 nodes
	
	for (int i = 0; i < 20; i++) {
		if (i == 6 ||  i == 7 || i == 12 || i == 14 || i == 18 || i == 19){
			constant_nodes.Add(c.Get(i));
			not_malicious.Add(c.Get(i));
		}
		else if (i == 0 || i == 5 || i == 15) {
			malicious.Add(c.Get(i));
			random_nodes.Add(c.Get(i));
		} else {
			not_malicious.Add(c.Get(i));
			random_nodes.Add(c.Get(i));
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
	AodvHelper aodv;
	AodvHelper malicious_aodv;
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


	// UDP connection from N7 to N19 - 10.1.1.8 -> 10.1.1.20 
	uint16_t sinkPort = 15;
	Address sinkAddress(InetSocketAddress(ifcont.GetAddress(19), sinkPort));
	PacketSinkHelper packetSinkHelper("ns3::UdpSocketFactory",
	                                  InetSocketAddress(Ipv4Address::GetAny(),
	                                          sinkPort));
	ApplicationContainer sinkApps = packetSinkHelper.Install(c.Get(19));
	sinkApps.Start(Seconds(0.));
	sinkApps.Stop(Seconds(100.));

	Ptr < Socket > ns3UdpSocket = Socket::CreateSocket(c.Get(7),
	                              UdpSocketFactory::GetTypeId());

	Ptr < MyApp > app = CreateObject < MyApp > ();
	app -> Setup(ns3UdpSocket, sinkAddress, 1024, 48, DataRate("1000Kbps"));
	c.Get(7) -> AddApplication(app);
	app -> SetStartTime(Seconds(15.));
	app -> SetStopTime(Seconds(100.));



	// Set Mobility for all nodes
	// radom mobility for nodes other than static 
	
	MobilityHelper mobility;
	ObjectFactory ofact;
	ofact.SetTypeId("ns3::RandomRectanglePositionAllocator");
	ofact.Set ("X", StringValue ("ns3::UniformRandomVariable[Min=0.0|Max=700.0]"));
	ofact.Set ("Y", StringValue ("ns3::UniformRandomVariable[Min=0.0|Max=700.0]"));
	Ptr<PositionAllocator> positionAlloc = ofact.Create()->GetObject<PositionAllocator>();
	mobility.SetMobilityModel("ns3::RandomWaypointMobilityModel", 
	                          "Speed", StringValue ("ns3::UniformRandomVariable[Min=0|Max=60]"),
	                          "Pause", StringValue ("ns3::ConstantRandomVariable[Constant=0.0]"),
	                          "PositionAllocator", PointerValue(positionAlloc));
	mobility.SetPositionAllocator (positionAlloc);
	mobility.Install(random_nodes);
	

	//static mobility for 6 7 12 14 18 20 
	MobilityHelper mobility_cst;
	Ptr < ListPositionAllocator > positionAlloc_cst =
	  CreateObject < ListPositionAllocator > ();
	positionAlloc_cst -> Add(Vector(125, 125, 0)); 
	positionAlloc_cst -> Add(Vector(50, 50, 0)); 
	positionAlloc_cst -> Add(Vector(375, 125, 0));
	positionAlloc_cst -> Add(Vector(125, 375, 0));
	positionAlloc_cst -> Add(Vector(375, 375, 0));
	positionAlloc_cst -> Add(Vector(400, 400, 0));
	mobility_cst.SetPositionAllocator(positionAlloc_cst);
	mobility_cst.SetMobilityModel("ns3::ConstantPositionMobilityModel");
	mobility_cst.Install(constant_nodes);



  // Animation sauvegard for Netanim
	AnimationInterface anim("blackhole.xml");
	anim.EnablePacketMetadata(true);
	Ptr < OutputStreamWrapper > routingStream =
	    Create < OutputStreamWrapper > ("blackhole.routes", std::ios::out);
	aodv.PrintRoutingTableAllAt(Seconds(45), routingStream);
	Config::ConnectWithoutContext(
	    "/NodeList/*/ApplicationList/*/$ns3::PacketSink/Rx",
	    MakeCallback( & ReceivePacket));
		
  // Flow Monitor  ---------------------------------- 

	FlowMonitorHelper flowmon;
	Ptr < FlowMonitor > monitor = flowmon.InstallAll();
	
	// Run simulation  ---------------------------------- 
	NS_LOG_INFO("Run Simulation.");
	Simulator::Stop(Seconds(100.0));
	Simulator::Run();
	monitor -> CheckForLostPackets();
	Ptr < Ipv4FlowClassifier > classifier =
	    DynamicCast < Ipv4FlowClassifier > (flowmon.GetClassifier());
	std::map < FlowId, FlowMonitor::FlowStats > stats = monitor -> GetFlowStats();
	std::cout << "[+]  " << timen << "\n";
	for (
	    std::map < FlowId, FlowMonitor::FlowStats > ::const_iterator i = stats.begin(); i != stats.end();
	    ++i) {
	
		Ipv4FlowClassifier::FiveTuple t = classifier -> FindFlow(i -> first);
		std::cout << ">-------------------------------------------<\n";
		std::cout << " Flow " << i -> first << " (" <<
		          t.sourceAddress << " -> " << t.destinationAddress << ")\n";
		std::cout << ">-------------------------------------------<\n";
		std::cout << " Tx Bytes: \t\t" << i -> second.txBytes << "\n";
		std::cout << " Rx Bytes: \t\t" << i -> second.rxBytes << "\n";
		std::cout << " Throughput: \t\t" <<
		          i -> second.rxBytes * 8.0 /
		          (i -> second.timeLastRxPacket.GetSeconds() -
		           i -> second.timeFirstTxPacket.GetSeconds()) /
		          1024 <<
		          " Kbps\n";
	}

	// File name : blackhole-%d-%H-%M-%S.flowmon

	auto t = std::time(nullptr);
    auto tm = *std::localtime(&t);

    std::ostringstream oss;
    oss << std::put_time(&tm, "%d-%H-%M-%S");
    auto s_datetime = oss.str();

    bool enableFlowmonExport = true;

    if (enableFlowmonExport)
		monitor -> SerializeToXmlFile("/home/ubuntu/results/blackhole-" + s_datetime + ".flowmon", true, true); // output file
}
