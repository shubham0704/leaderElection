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
 *
 * Author: Shubham Bhardwaj (git-id: shubham0704) (Adapted from third.cc)
 */


#include "ns3/core-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/csma-module.h"
#include "ns3/network-module.h"
#include "ns3/applications-module.h"
#include "ns3/wifi-module.h"
#include "ns3/mobility-module.h"
#include "ns3/internet-module.h"
#include "ns3/netanim-module.h"
#include "ns3/basic-energy-source.h"
#include "ns3/simple-device-energy-model.h"
#include "ns3/position-allocator.h"

#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/filereadstream.h"
#include <cstdio>
#include <iostream>
#include <vector>
using namespace ns3;
using namespace rapidjson;

NS_LOG_COMPONENT_DEFINE ("WirelessAnimationExample");

FILE* fp = fopen("/home/master/Desktop/main_json.txt","r");

int 
main (int argc, char *argv[])
{
  
  LogComponentEnable ("UdpEchoClientApplication", LOG_LEVEL_INFO);
  LogComponentEnable ("UdpEchoServerApplication", LOG_LEVEL_INFO);
  char readBuffer[65536];
  FileReadStream is(fp, readBuffer, sizeof(readBuffer));
  Document d;
  d.ParseStream(is);
  fclose(fp);
  StringBuffer buffer;
  Writer<StringBuffer> writer(buffer);
  d.Accept(writer);
  std::cout << buffer.GetString() << std::endl;
  NS_LOG_INFO (buffer.GetString());
  // took json data into document d

  //assign centers in json to a 2-d vector
  
  std::vector<std::vector<float>> centers;
 //iterator over a centers json 
  const Value& centers_json = d["centers"];
  int j = 0; 
  
   for(SizeType i=0;i<centers_json.Size();i++){
	    std::vector<float> temp;
        temp.push_back(centers_json[i][0].GetDouble());
        temp.push_back(centers_json[i][1].GetDouble());
        centers.push_back(temp);
        j++;
  }
  //iterate over dataMat
  std::vector<std::vector<float> > dataMat;
  const Value& dataMat_json = d["dataMat"];
  j=0;
  for(SizeType i=0;i<dataMat_json.Size();i++){
	    std::vector<float> temp;
        temp.push_back(dataMat_json[i][0].GetDouble());
        temp.push_back(dataMat_json[i][1].GetDouble());
        dataMat.push_back(temp);
        j++;
  }
  //iterate over cluster assigned
  std::vector<int> cluster_assigned;
  const Value& cluster_assigned_json = d["cluster_assigned"];
  j=0;
  for(SizeType i=0;i<cluster_assigned_json.Size();i++){
        cluster_assigned.push_back(cluster_assigned_json[i].GetInt());
  }
  //add point from dataMat into ListPositionAllocator
  Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator> ();
  
  for(size_t i=0;i<dataMat.size();i++){
        positionAlloc->Add (Vector (dataMat[i][0]+10, dataMat[i][1]+10,0.0));       
  }
  
  uint32_t nWifi = (int)(dataMat.size());
  uint32_t nAp = (int)(centers.size());
  CommandLine cmd;
  cmd.AddValue ("nWifi", "Number of wifi STA devices", nWifi);
  
//now make the centers as the access points
  //Ptr<ListPositionAllocator> positionAlloc_new = CreateObject<ListPositionAllocator> ();
   for(size_t i=0;i<centers.size();i++){
        positionAlloc->Add (Vector (centers[i][0]+10, centers[i][1]+10,0.0));       
  }
 
  
  cmd.Parse (argc,argv);
  NodeContainer allNodes;
  NodeContainer wifiStaNodes;
  wifiStaNodes.Create (nWifi);
  allNodes.Add (wifiStaNodes);
  NodeContainer wifiApNodes ;
  wifiApNodes.Create (nAp);
  allNodes.Add (wifiApNodes);

  YansWifiChannelHelper channel = YansWifiChannelHelper::Default ();
  YansWifiPhyHelper phy = YansWifiPhyHelper::Default ();
  phy.SetChannel (channel.Create ());

  WifiHelper wifi;
  wifi.SetRemoteStationManager ("ns3::AarfWifiManager");

  WifiMacHelper mac;
  Ssid ssid = Ssid ("ns-3-ssid");
  mac.SetType ("ns3::StaWifiMac",
               "Ssid", SsidValue (ssid),
               "ActiveProbing", BooleanValue (false));

  NetDeviceContainer staDevices;
  staDevices = wifi.Install (phy, mac, wifiStaNodes);
  mac.SetType ("ns3::ApWifiMac",
               "Ssid", SsidValue (ssid));

  NetDeviceContainer apDevices;
  apDevices = wifi.Install (phy, mac, wifiApNodes);

//error starts from here
/*
  NodeContainer p2pNodes;
  p2pNodes.Add (wifiApNodes.Get(0));
  //this is the first csma node
  p2pNodes.Create (1);
  allNodes.Add (p2pNodes.Get (1));

  PointToPointHelper pointToPoint;
  pointToPoint.SetDeviceAttribute ("DataRate", StringValue ("5Mbps"));
  pointToPoint.SetChannelAttribute ("Delay", StringValue ("2ms"));

  NetDeviceContainer p2pDevices;
  p2pDevices = pointToPoint.Install (p2pNodes);
*/
  NodeContainer csmaNodes;  

  csmaNodes.Add (wifiApNodes);     
  csmaNodes.Create (2);
  allNodes.Add (csmaNodes.Get (nAp));
  allNodes.Add (csmaNodes.Get (nAp+1));

  CsmaHelper csma;
  csma.SetChannelAttribute ("DataRate", StringValue ("100Mbps"));
  csma.SetChannelAttribute ("Delay", TimeValue (NanoSeconds (6560)));

  NetDeviceContainer csmaDevices;
  csmaDevices = csma.Install (csmaNodes);

  // Mobility

  MobilityHelper mobility;


  mobility.SetMobilityModel ("ns3::RandomWalk2dMobilityModel",
                             "Bounds", RectangleValue (Rectangle (-50, 50, -50, 50))); 
  mobility.SetPositionAllocator (positionAlloc);
  mobility.Install (wifiStaNodes);
  //mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobility.Install (wifiApNodes);
  //this is for the two blue csma nodes on the corner
  AnimationInterface::SetConstantPosition (csmaNodes.Get (nAp), 10, 30); 
  AnimationInterface::SetConstantPosition (csmaNodes.Get (nAp+1), 10, 33); 

  //Ptr<BasicEnergySource> energySource = CreateObject<BasicEnergySource>();
  //Ptr<SimpleDeviceEnergyModel> energyModel = CreateObject<SimpleDeviceEnergyModel>();   

  // Install internet stack

  InternetStackHelper stack;
  stack.Install (allNodes);

  // Install Ipv4 addresses

  Ipv4AddressHelper address;
  address.SetBase ("10.1.1.0", "255.255.255.0");
  //Ipv4InterfaceContainer p2pInterfaces;
  //p2pInterfaces = address.Assign (p2pDevices);
  //address.SetBase ("10.1.2.0", "255.255.255.0");

  Ipv4InterfaceContainer csmaInterfaces;
  csmaInterfaces = address.Assign (csmaDevices);
  address.SetBase ("10.1.2.0", "255.255.255.0");
  Ipv4InterfaceContainer staInterfaces;
  staInterfaces = address.Assign (staDevices);
  Ipv4InterfaceContainer apInterface;
  apInterface = address.Assign (apDevices);

  // Install applications

  UdpEchoServerHelper echoServer (9);
  ApplicationContainer serverApps = echoServer.Install (csmaNodes.Get (nAp));
  serverApps.Start (Seconds (1.0));
  serverApps.Stop (Seconds (15.0));

  UdpEchoClientHelper echoClient (csmaInterfaces.GetAddress (nAp+1), 9);
  echoClient.SetAttribute ("MaxPackets", UintegerValue (10));
  echoClient.SetAttribute ("Interval", TimeValue (Seconds (1.)));
  echoClient.SetAttribute ("PacketSize", UintegerValue (1024));
  ApplicationContainer clientApps = echoClient.Install (wifiStaNodes);
  clientApps.Start (Seconds (2.0));
  clientApps.Stop (Seconds (15.0));
/*
  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();
  Simulator::Stop (Seconds (15.0));
*/
  AnimationInterface anim ("wireless-animation-new.xml"); // Mandatory
  for (uint32_t i = 0; i < wifiStaNodes.GetN (); ++i)
    {
      anim.UpdateNodeDescription (wifiStaNodes.Get (i), "STA"); // Optional
      if (cluster_assigned[i]==0)  
      anim.UpdateNodeColor (wifiStaNodes.Get (i), 244, 67, 54);
      else if (cluster_assigned[i]==1)
      anim.UpdateNodeColor (wifiStaNodes.Get (i), 33, 150, 243);
      else
      anim.UpdateNodeColor (wifiStaNodes.Get (i), 118, 255, 3);
        
    }
  for (uint32_t i = 0; i < wifiApNodes.GetN (); ++i)
    {
      anim.UpdateNodeDescription (wifiApNodes.Get (i), "AP"); // Optional
      anim.UpdateNodeColor (wifiApNodes.Get (i), 0, 0, 0); // Optional
    }
  for (uint32_t i = 0; i < csmaNodes.GetN (); ++i)
    {
      anim.UpdateNodeDescription (csmaNodes.Get (i), "CSMA"); // Optional
      anim.UpdateNodeColor (csmaNodes.Get (i), 0, 0, 255); // Optional 
    }

  anim.EnablePacketMetadata (); // Optional
  anim.EnableIpv4RouteTracking ("routingtable-wireless-new.xml", Seconds (0), Seconds (5), Seconds (0.25)); //Optional
  anim.EnableWifiMacCounters (Seconds (0), Seconds (10)); //Optional
  anim.EnableWifiPhyCounters (Seconds (0), Seconds (10)); //Optional
  Simulator::Run ();
  Simulator::Destroy ();

  return 0;
}
