/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "ns3/core-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/network-module.h"
#include "ns3/applications-module.h"
#include "ns3/mobility-module.h"
#include "ns3/csma-module.h"
#include "ns3/internet-module.h"
#include "ns3/yans-wifi-helper.h"
#include "ns3/ssid.h"
#include "ns3/wifi-module.h"
#include "ns3/flow-monitor-helper.h"
#include "ns3/flow-monitor-module.h"

// Default Network Topology
//
//   Wifi 10.1.3.0
//                 AP
//  *    *    *    *
//  |    |    |    |    10.1.1.0
// n5   n6   n7   n0 -------------- n1   n2   n3   n4
//                   point-to-point  |    |    |    |
//                                   ================
//                                     LAN 10.1.2.0

// Davi -> 
using namespace ns3;

void CourseChange(std::string context, Ptr<const MobilityModel> model){
  Vector position = model->GetPosition ();
  NS_LOG_UNCOND (context << " x = " << position.x << ", y = " << position.y);
}

NS_LOG_COMPONENT_DEFINE ("networkOnetExample");

int main (int argc, char *argv[]) {
  bool verbose = true;
  uint32_t nCsma = 4;
  uint32_t nWifi = 4;
  bool tracing = false;
  //uint32_t payloadSize = 1472;                       /* Transport layer payload size in bytes. */
  uint32_t packetSize = 1024;
  uint32_t startTime = 1;
  uint32_t simulationTime = 20;

  CommandLine cmd;
  cmd.AddValue("nCsma", "Number of \"extra\" CSMA nodes/devices", nCsma);
  cmd.AddValue("nWifi", "Number of WIFI STA devices", nWifi);
  cmd.AddValue("verbose", "Tell echo applications to log if true", verbose);
  cmd.AddValue("tracing", "Enable pcap tracing", tracing);
  cmd.AddValue("packetSize", "Packet Size of the UdpEchoClientHelper ", packetSize);
  cmd.AddValue("simulationTime", "Simulation time", simulationTime);
  cmd.Parse (argc, argv);

  uint32_t endTime = simulationTime;

  // The underlying restriction of 18 is due to the grid position
  // allocator1's configuration; the grid layout will exceed the
  // bounding box if more than 10 nodes are provided
  if(nWifi > 18){
    std::cout << "nWifi should be 18 or less: otherwise grid layout exceeds the bounding box" << std::endl;
    return 1;
  }
  
  //Time::SetResolution (Time::NS);

  if(verbose){
     LogComponentEnable ("networkOnetExample", LOG_LEVEL_ALL);
     LogComponentEnable ("UdpEchoClientApplication", LOG_LEVEL_INFO);
     LogComponentEnable ("UdpEchoServerApplication", LOG_LEVEL_INFO);
  }

  // Exemplo erro model no canal do node
  //p2pDevices.Get (1)->SetAttribute ("ReceiveErrorModel", PointerValue (em));
  //p2pDevices.Get (0)->SetAttribute ("ReceiveErrorModel", PointerValue (em));

  NodeContainer wifiStaNodes;
  wifiStaNodes.Create (nWifi);

  NodeContainer wifiApNode = wifiStaNodes.Get (0);

  YansWifiChannelHelper channel = YansWifiChannelHelper::Default ();

  YansWifiPhyHelper phy = YansWifiPhyHelper::Default ();
  phy.SetChannel (channel.Create ());
  phy.SetErrorRateModel("ns3::YansErrorRateModel");
  

  WifiHelper wifi;
  wifi.SetRemoteStationManager ("ns3::AarfWifiManager");

  WifiMacHelper mac;

  Ssid ssid = Ssid ("network-1-ssid");

  mac.SetType ("ns3::StaWifiMac",
               "Ssid", SsidValue (ssid),
              "ActiveProbing", BooleanValue (false));


  NetDeviceContainer  staDevices;
  staDevices = wifi.Install (phy, mac, wifiStaNodes);

  mac.SetType ("ns3::ApWifiMac",
            "Ssid", SsidValue (ssid));

  NetDeviceContainer apDevices;
  apDevices = wifi.Install (phy, mac, wifiApNode);

  MobilityHelper mobility;

  mobility.SetPositionAllocator("ns3::GridPositionAllocator",
                                "MinX", DoubleValue (0.0),
                                "MinY", DoubleValue (0.0),
                                "DeltaX", DoubleValue (5.0),
                                "DeltaY", DoubleValue (10.0),
                                "GridWidth", UintegerValue (3),
                                "LayoutType", StringValue ("RowFirst"));

  mobility.SetMobilityModel ("ns3::RandomWalk2dMobilityModel",
                             "Bounds", RectangleValue (Rectangle (-50, 50, -50, 50)));
  mobility.Install (wifiStaNodes);

  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobility.Install (wifiApNode);

 // ----------------------------------------------------------------------- //

  NodeContainer csmaNodes;
  csmaNodes.Add (wifiStaNodes.Get (nWifi - 1));
  csmaNodes.Create (nCsma);

  CsmaHelper csma;
  csma.SetChannelAttribute ("DataRate", StringValue ("100Mbps"));
  csma.SetChannelAttribute ("Delay", TimeValue (NanoSeconds (6560)));

  NetDeviceContainer csmaDevices;
  csmaDevices = csma.Install (csmaNodes);

  Ptr<RateErrorModel> em = CreateObject<RateErrorModel> ();
  em->SetAttribute ("ErrorRate", DoubleValue (0.00002));

  // ------------------------------------------------------------------------- //

  NodeContainer wifiStaNodes2;
  wifiStaNodes2.Add (csmaNodes.Get (nCsma));
  wifiStaNodes2.Create (nWifi-1);

  NodeContainer wifiApNode2 = wifiStaNodes2.Get (0);

  YansWifiChannelHelper channel2 = YansWifiChannelHelper::Default ();
  //channel.SetPropagationDelay ("ns3::ConstantSpeedPropagationDelayModel");
  //channel.AddPropagationLoss ("ns3::FriisPropagationLossModel", "Frequency", DoubleValue (5e9));

  YansWifiPhyHelper phy2 = YansWifiPhyHelper::Default ();
  phy2.SetChannel (channel2.Create ());
  //phy2.SetErrorRateModel("ns3::YansErrorRateModel");
  

  WifiHelper wifi2;
  wifi2.SetRemoteStationManager ("ns3::AarfWifiManager");

  WifiMacHelper mac2;

  Ssid ssid2 = Ssid ("network-2-ssid");

  mac2.SetType ("ns3::StaWifiMac",
                 "Ssid", SsidValue (ssid2),
                 "ActiveProbing", BooleanValue (false));


  NetDeviceContainer  staDevices2;
  staDevices2 = wifi2.Install (phy2, mac2, wifiStaNodes2);

  mac2.SetType ("ns3::ApWifiMac",
           "Ssid", SsidValue (ssid2));


  NetDeviceContainer apDevices2;
  apDevices2 = wifi2.Install (phy2, mac2, wifiApNode2);

  mobility.SetPositionAllocator("ns3::GridPositionAllocator",
                                "MinX", DoubleValue (0.0),
                                "MinY", DoubleValue (0.0),
                                "DeltaX", DoubleValue (5.0),
                                "DeltaY", DoubleValue (10.0),
                                "GridWidth", UintegerValue (3),
                                "LayoutType", StringValue ("RowFirst"));

  mobility.SetMobilityModel ("ns3::RandomWalk2dMobilityModel",
                             "Bounds", RectangleValue (Rectangle (-50, 50, -50, 50)));
  mobility.Install (wifiStaNodes2);

  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobility.Install (wifiApNode2);

  //-------------------------------------------------------------------------------------- //

  NodeContainer csmaNodes2;
  csmaNodes2.Add (wifiStaNodes2.Get (nWifi - 1));
  csmaNodes2.Create (nCsma);

  CsmaHelper csma2;
  csma2.SetChannelAttribute ("DataRate", StringValue ("100Mbps"));
  csma2.SetChannelAttribute ("Delay", TimeValue (NanoSeconds (6560)));

  NetDeviceContainer csmaDevices2;
  csmaDevices2 = csma2.Install (csmaNodes2);

  //Ptr<RateErrorModel> em = CreateObject<RateErrorModel> ();
  //em->SetAttribute ("ErrorRate", DoubleValue (0.1));



  // *********** Addeding WIFI end *************************//

  InternetStackHelper stack;
  stack.InstallAll ();

   //stack.InstallAll (wifiApNode);
  //stack.Install (wifiStaNodes);

  //stack.Install (csmaNodes);
  //stack.Install (csmaNodes2);

  //stack.Install (wifiApNode2);
  //stack.Install (wifiStaNodes2);

  Ipv4AddressHelper address;

  Ipv4InterfaceContainer apInterface;
  Ipv4InterfaceContainer staInterface;

  Ipv4InterfaceContainer apInterface2;
  Ipv4InterfaceContainer staInterface2;

  address.SetBase ("10.1.1.0", "255.255.255.0");
  staInterface = address.Assign (staDevices);
  apInterface = address.Assign (apDevices);

  address.SetBase ("10.1.2.0", "255.255.255.0");
  Ipv4InterfaceContainer csmaInterfaces;
  csmaInterfaces = address.Assign (csmaDevices);

  address.SetBase ("10.1.3.0", "255.255.255.0");
  staInterface2 = address.Assign (staDevices2);
  apInterface2 = address.Assign (apDevices2);

  address.SetBase ("10.1.4.0", "255.255.255.0");
  Ipv4InterfaceContainer csma2Interfaces;
  csma2Interfaces = address.Assign (csmaDevices2);

  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

  UdpEchoServerHelper echoServer (9);
  UdpEchoServerHelper echoServer2 (10);

  ApplicationContainer serverApps = echoServer.Install (wifiStaNodes.Get (nWifi - 1));
  serverApps.Start (Seconds (startTime));
  serverApps.Stop (Seconds (endTime));

  ApplicationContainer serverApps2 = echoServer2.Install (csmaNodes2.Get (nCsma - 1));
  serverApps2.Start (Seconds (startTime));
  serverApps2.Stop (Seconds (endTime));
 
  UdpEchoClientHelper echoClient (staInterface.GetAddress (nWifi - 1), 9);
  echoClient.SetAttribute ("MaxPackets", UintegerValue (10000));
  echoClient.SetAttribute ("Interval", TimeValue (Seconds (0.1)));
  echoClient.SetAttribute ("PacketSize", UintegerValue (packetSize));

  UdpEchoClientHelper echoClient2 (csma2Interfaces.GetAddress (nCsma - 1), 10);
  echoClient2.SetAttribute ("MaxPackets", UintegerValue (10000));
  echoClient2.SetAttribute ("Interval", TimeValue (Seconds (0.1)));
  echoClient2.SetAttribute ("PacketSize", UintegerValue (packetSize));

  ApplicationContainer clientApps;
  ApplicationContainer clientApps2;

  // Instalando o cliente de echo nos STAs
  for ( uint32_t i = 0; i < nCsma; i++ ){
    if(i != 0){     
      clientApps.Add(echoClient.Install (csmaNodes2.Get (i)));       
    }
  }   

  for ( uint32_t i = 0; i < nWifi; i++ ){
    if(i != 0){     
      clientApps2.Add(echoClient2.Install (wifiStaNodes.Get (i)));       
    }
  }   

  clientApps.Start (Seconds (startTime + 1));
  clientApps.Stop (Seconds (endTime));

  clientApps2.Start (Seconds (startTime + 1));
  clientApps2.Stop (Seconds (endTime));

                        
  Simulator::Stop (Seconds (simulationTime));

  if (tracing == true){
      //pointToPoint.EnablePcapAll ("networkOnetExampleP2P");
      //phy.EnablePcap ("networkOnetExamplePhy", apDevices.Get (0));
      //phy.EnablePcapAll ("Wifi");
      //csma2.EnablePcap ("networkOnetExampleCsma", csmaDevices.Get (0), true);       
  }

  //std::ostringstream oss;
  //oss << "/NodeList/" << wifiStaNodes.Get (nWifi - 1) ->GetId () << "/$ns3::MobilityModel/CourseChange";
  //Config::Connect (oss.str (), MakeCallback (&CourseChange ));

  Ptr<FlowMonitor> flowMonitor;
  FlowMonitorHelper flowHelper;
  flowMonitor = flowHelper.InstallAll();

  Simulator::Run (); 
  Simulator::Destroy ();

  flowMonitor -> CheckForLostPackets ();
  
  uint64_t txPacketsum = 0, rxPacketsum = 0, DropPacketsum = 0, LostPacketsum = 0;
  double Delaysum = 0, avgDelay = 0, throughput = 0, probSucc = 0, probLoss = 0;

  Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier> (flowHelper.GetClassifier ());
  std::map<FlowId, FlowMonitor::FlowStats> stats = flowMonitor->GetFlowStats ();
  
  for (std::map <FlowId, FlowMonitor::FlowStats>::const_iterator i = stats.begin (); i != stats.end (); ++i) {
    txPacketsum += i->second.txPackets;
    rxPacketsum += i->second.rxPackets;
    LostPacketsum += i->second.lostPackets;
    DropPacketsum += i->second.packetsDropped .size ();
    Delaysum += i->second.delaySum. GetSeconds ();
  }

  throughput = ( rxPacketsum * 1024 * 8) / ( simulationTime * 1000);
  probSucc = ( rxPacketsum * 100) / txPacketsum ;
  probLoss = ( (txPacketsum - rxPacketsum) * 100) / txPacketsum ;
  avgDelay = Delaysum / txPacketsum ;

  //std::cout << std::endl << "*** Flow monitor statistics ***" << std::endl;
  //std::cout << "  Tx Packets:   " << stats[1].txPackets << std::endl;
  //std::cout << "  Tx Bytes:   " << stats[1].txBytes << std::endl;
  //std::cout << "  Offered Load: " << stats[1].txBytes * 8.0 / (stats[1].timeLastTxPacket.GetSeconds () - stats[1].timeFirstTxPacket.GetSeconds ()) / 1000000 << " Mbps" << std::endl;
  //std::cout << "  Rx Packets:   " << stats[1].rxPackets << std::endl;
  //std::cout << "  Rx Bytes:   " << stats[1].rxBytes<< std::endl;
  //std::cout << "  Throughput: " << stats[1].rxBytes * 8.0 / (stats[1].timeLastRxPacket.GetSeconds () - stats[1].timeFirstRxPacket.GetSeconds ()) / 1000000 << " Mbps" << std::endl;
  //std::cout << "  Mean delay:   " << stats[1].delaySum.GetSeconds () / stats[1].rxPackets << std::endl;
  //std::cout << "  Mean jitter:   " << stats[1].jitterSum.GetSeconds () / (stats[1].rxPackets - 1) << std::endl;    
  // não esquecer de enviar os outros dados para fazer os calculos no R
  //flowMonitor->SerializeToXmlFile("data02.flowmon", true, true); 

  NS_LOG_INFO (std::endl << " SIMULATION STATISTICS");
  NS_LOG_INFO (" number of Devices: " << nCsma);
  NS_LOG_INFO (" All Tx Packets: " << txPacketsum );
  NS_LOG_INFO (" All Rx Packets: " << rxPacketsum );
  NS_LOG_INFO (" All Delay: " << avgDelay);
  NS_LOG_INFO (" All Lost Packets: " << LostPacketsum );
  NS_LOG_INFO (" All Drop Packets: " << DropPacketsum );
  NS_LOG_INFO (" Packets Success Ratio: " << probSucc << "%");
  NS_LOG_INFO (" Packets Lost Ratio: " << probLoss << "%");
  NS_LOG_INFO (" Throughput (Kbps): " << throughput );

  std::string fileMetric ="./scratch/networkDavi.dat";
  std::ofstream fileData;

  //NS_LOG_INFO (" Packets Lost Ratio: " << probLoss << "%");
  //NS_LOG_INFO (" Throughput (Kbps): " << throughput );

  fileData.open (fileMetric , std::ios::out | std::ios::app);

  fileData << nCsma << ", " << txPacketsum << ", " << rxPacketsum << ", " <<
  throughput << ", " << probSucc << ", " << probLoss << ", " << avgDelay << "\n";

  fileData.close ();

 
  return 0;
}
