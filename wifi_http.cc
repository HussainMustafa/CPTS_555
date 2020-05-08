#include "ns3/core-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/network-module.h"
#include "ns3/applications-module.h"
#include "ns3/mobility-module.h"
#include "ns3/csma-module.h"
#include "ns3/internet-module.h"
#include "ns3/yans-wifi-helper.h"
#include "ns3/ssid.h"
#include "ns3/netanim-module.h"


using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("ThreeGppHttpExample");

void
CourseChange (std::string context, Ptr<const MobilityModel> model)
{
Vector position = model->GetPosition ();
NS_LOG_UNCOND (context <<
" x = " << position.x << ", y = " << position.y);
}
void
ServerConnectionEstablished (Ptr<const ThreeGppHttpServer>, Ptr<Socket>)
{
  NS_LOG_INFO ("Client has established a connection to the server.");
}

void
MainObjectGenerated (uint32_t size)
{
  NS_LOG_INFO ("Server generated a main object of " << size << " bytes.");
}

void
EmbeddedObjectGenerated (uint32_t size)
{
  NS_LOG_INFO ("Server generated an embedded object of " << size << " bytes.");
}

void
ServerTx (Ptr<const Packet> packet)
{
  NS_LOG_INFO ("Server sent a packet of " << packet->GetSize () << " bytes.");
}

void
ClientRx (Ptr<const Packet> packet, const Address &address)
{
  NS_LOG_INFO ("Client received a packet of " << packet->GetSize () << " bytes from " << address);
}

void
ClientMainObjectReceived (Ptr<const ThreeGppHttpClient>, Ptr<const Packet> packet)
{
  Ptr<Packet> p = packet->Copy ();
  ThreeGppHttpHeader header;
  p->RemoveHeader (header);
  if (header.GetContentLength () == p->GetSize ()
      && header.GetContentType () == ThreeGppHttpHeader::MAIN_OBJECT)
    {
      NS_LOG_INFO ("Client has successfully received a main object of "
                   << p->GetSize () << " bytes.");
    }
  else
    {
      NS_LOG_INFO ("Client failed to parse a main object. ");
    }
}

void
ClientEmbeddedObjectReceived (Ptr<const ThreeGppHttpClient>, Ptr<const Packet> packet)
{
  Ptr<Packet> p = packet->Copy ();
  ThreeGppHttpHeader header;
  p->RemoveHeader (header);
  if (header.GetContentLength () == p->GetSize ()
      && header.GetContentType () == ThreeGppHttpHeader::EMBEDDED_OBJECT)
    {
      NS_LOG_INFO ("Client has successfully received an embedded object of "
                   << p->GetSize () << " bytes.");
    }
  else
    {
      NS_LOG_INFO ("Client failed to parse an embedded object. ");
    }
}


int 
main (int argc, char *argv[])
{
  bool verbose = true;
  uint32_t Server_Nodes = 3;
  uint32_t NAN_To_WAN_CSMA = 3;
  uint32_t nWifi = 18;
  bool tracing = true;

  CommandLine cmd;
  cmd.AddValue ("Server_Nodes", "Number of \"extra\" CSMA nodes/devices", Server_Nodes);
  cmd.AddValue ("nWifi", "Number of wifi STA devices", nWifi);
  cmd.AddValue ("verbose", "Tell echo applications to log if true", verbose);
  cmd.AddValue ("tracing", "Enable pcap tracing", tracing);

  cmd.Parse (argc,argv);

  // The underlying restriction of 18 is due to the grid position
  // allocator's configuration; the grid layout will exceed the
  // bounding box if more than 18 nodes are provided.
  if (nWifi > 18)
    {
      std::cout << "nWifi should be 18 or less; otherwise grid layout exceeds the bounding box" << std::endl;
      return 1;
    }

  if (verbose)
    {
      Time::SetResolution (Time::NS);
      LogComponentEnableAll (LOG_PREFIX_TIME);
      LogComponentEnable ("UdpEchoClientApplication", LOG_LEVEL_INFO);
      LogComponentEnable ("UdpEchoServerApplication", LOG_LEVEL_INFO);
      LogComponentEnable ("ThreeGppHttpExample", LOG_INFO);
    }
  NodeContainer NAN_Nodes;
  NAN_Nodes.Create (2);

  PointToPointHelper NAN_Connectivity;
  NAN_Connectivity.SetDeviceAttribute ("DataRate", StringValue ("1Mbps"));
  NAN_Connectivity.SetChannelAttribute ("Delay", StringValue ("100ms"));

  NetDeviceContainer NAN_Device;
  NAN_Device = NAN_Connectivity.Install (NAN_Nodes);

  NodeContainer WAN_Nodes;
  WAN_Nodes.Create (2);
  
  PointToPointHelper WAN_Connectivity;
  WAN_Connectivity.SetDeviceAttribute ("DataRate", StringValue ("1Mbps"));
  WAN_Connectivity.SetChannelAttribute ("Delay", StringValue ("100ms"));

  NetDeviceContainer WAN_Device;
  WAN_Device = WAN_Connectivity.Install (WAN_Nodes);

  NodeContainer NAN_To_WAN;
  NAN_To_WAN.Add (WAN_Nodes.Get (0));
  NAN_To_WAN.Add (NAN_Nodes.Get (1));
  NAN_To_WAN.Create (NAN_To_WAN_CSMA);

  CsmaHelper NAN_To_WANcsma;
  NAN_To_WANcsma.SetChannelAttribute ("DataRate", StringValue ("1Mbps"));
  NAN_To_WANcsma.SetChannelAttribute ("Delay", StringValue ("100ms"));

  NetDeviceContainer NAN_To_WANDevices;
  NAN_To_WANDevices = NAN_To_WANcsma.Install (NAN_To_WAN);





  NodeContainer Workstation_Server;
  Workstation_Server.Add (WAN_Nodes.Get (1));
  Workstation_Server.Create (Server_Nodes);

  CsmaHelper Server;
  Server.SetChannelAttribute ("DataRate", StringValue ("1Mbps"));
  Server.SetChannelAttribute ("Delay", StringValue ("100ms"));

  NetDeviceContainer ServerDevices;
  ServerDevices = Server.Install (Workstation_Server);

  

  //NodeContainer wifiStaNodes;
  NodeContainer wifiStaNodes;
  wifiStaNodes.Create (nWifi);
  /*
  Ptr<Node> PV1 = CreateObject<Node> ();
  Ptr<Node> PV2 = CreateObject<Node> ();
  Ptr<Node> AMI = CreateObject<Node> ();
  Ptr<Node> DG = CreateObject<Node> ();

  wifiStaNodes.Add(PV1);
  wifiStaNodes.Add(PV2);
  wifiStaNodes.Add(AMI);
  wifiStaNodes.Add(DG);;
  */
  NodeContainer wifiApNode = NAN_Nodes.Get (0);

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
  apDevices = wifi.Install (phy, mac, wifiApNode);

  MobilityHelper mobility;

  mobility.SetPositionAllocator ("ns3::GridPositionAllocator",
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
  //p2pNodes_2.Add (p2pNodes_1.Get (0));
  //p2pNodes_1.Add (p2pNodes_2.Get (1));

  InternetStackHelper stack;
  stack.Install (Workstation_Server);
  stack.Install (NAN_To_WAN);
  stack.Install (wifiApNode);
  stack.Install (wifiStaNodes);
  //stack.Install (p2pNodes_2.Get(1));
  //stack.Install (p2pNodes_1.Get(0));
  

  Ipv4AddressHelper address;

  address.SetBase ("10.1.1.0", "255.255.255.0");
  address.Assign (staDevices);
  address.Assign (apDevices);

  address.SetBase ("10.1.2.0", "255.255.255.0");
  Ipv4InterfaceContainer NAN_Interfaces;
  NAN_Interfaces = address.Assign (NAN_Device);

  address.SetBase ("10.1.3.0", "255.255.255.0");
  Ipv4InterfaceContainer WAN_Interfaces;
  WAN_Interfaces = address.Assign (WAN_Device);

  address.SetBase ("10.1.4.0", "255.255.255.0");
  Ipv4InterfaceContainer NAN_To_WAN_Interfaces;
  NAN_To_WAN_Interfaces = address.Assign (NAN_To_WANDevices); 
 
  address.SetBase ("10.1.5.0", "255.255.255.0");
  Ipv4InterfaceContainer ServerInterfaces;
  ServerInterfaces = address.Assign (ServerDevices);


/*
  UdpEchoServerHelper echoServer (9);
  ApplicationContainer serverApps = echoServer.Install (Workstation_Server.Get (Server_Nodes));
  serverApps.Start (Seconds (1.0));
  serverApps.Stop (Seconds (10.0));

  Ipv4Address ServerAddress(ServerInterfaces.GetAddress (Server_Nodes));
  uint16_t Server_port = 9;
  
  UdpEchoClientHelper echoClient (ServerAddress, Server_port);
  echoClient.SetAttribute ("MaxPackets", UintegerValue (1));
  echoClient.SetAttribute ("Interval", TimeValue (Seconds (1.5)));
  echoClient.SetAttribute ("PacketSize", UintegerValue (1024));

  ApplicationContainer clientApps_1 = 
  echoClient.Install (wifiStaNodes.Get (nWifi - 1));
  clientApps_1.Start (Seconds (2.0));
  clientApps_1.Stop (Seconds (10.0));

  ApplicationContainer clientApps_2 = 
  echoClient.Install (wifiStaNodes.Get (nWifi - 2));
  clientApps_2.Start (Seconds (2.0));
  clientApps_2.Stop (Seconds (10.0));

  ApplicationContainer clientApps_3 = 
  echoClient.Install (wifiStaNodes.Get (nWifi - 3));
  clientApps_3.Start (Seconds (2.0));
  clientApps_3.Stop (Seconds (10.0));
*/
   // Create HTTP server helper
  Ipv4Address ServerAddress(ServerInterfaces.GetAddress (Server_Nodes));
  //uint16_t Server_port = 9;
  ThreeGppHttpServerHelper serverHelper (ServerAddress);

  // Install HTTP server
  ApplicationContainer serverApps = serverHelper.Install (Workstation_Server.Get (Server_Nodes));
  Ptr<ThreeGppHttpServer> httpServer = serverApps.Get (0)->GetObject<ThreeGppHttpServer> ();

  // Example of connecting to the trace sources
  httpServer->TraceConnectWithoutContext ("ConnectionEstablished",
                                          MakeCallback (&ServerConnectionEstablished));
  httpServer->TraceConnectWithoutContext ("MainObject", MakeCallback (&MainObjectGenerated));
  httpServer->TraceConnectWithoutContext ("EmbeddedObject", MakeCallback (&EmbeddedObjectGenerated));
  httpServer->TraceConnectWithoutContext ("Tx", MakeCallback (&ServerTx));

  // Setup HTTP variables for the server
  PointerValue varPtr;
  httpServer->GetAttribute ("Variables", varPtr);
  Ptr<ThreeGppHttpVariables> httpVariables = varPtr.Get<ThreeGppHttpVariables> ();
  httpVariables->SetMainObjectSizeMean (1024); // 100kB
  httpVariables->SetMainObjectSizeStdDev (0); // 40kB

  
 
  ThreeGppHttpClientHelper clientHelper (ServerAddress);
 // clientHelper.SetAttribute ("MaxPackets", UintegerValue (1));
  // Install HTTP client
  ApplicationContainer clientApps = clientHelper.Install (wifiStaNodes.Get (nWifi - 1));
  Ptr<ThreeGppHttpClient> httpClient = clientApps.Get (0)->GetObject<ThreeGppHttpClient> ();

   // Setup HTTP variables for the client
  PointerValue hC;
  httpClient->GetAttribute ("Variables", hC);
  Ptr<ThreeGppHttpVariables> hV = hC.Get<ThreeGppHttpVariables> ();
  hV->SetMainObjectSizeMean (1024); // 100kB
  hV->SetMainObjectSizeStdDev (0); // 40kB


  ApplicationContainer clientApps_2 = clientHelper.Install (wifiStaNodes.Get (nWifi - 2));
  Ptr<ThreeGppHttpClient> httpClient_2 = clientApps_2.Get (0)->GetObject<ThreeGppHttpClient> ();
 
  PointerValue hC_2;
  httpClient->GetAttribute ("Variables", hC_2);
  Ptr<ThreeGppHttpVariables> hV_2 = hC_2.Get<ThreeGppHttpVariables> ();
  hV_2->SetMainObjectSizeMean (65535); // 100kB
  hV_2->SetMainObjectSizeStdDev (0); // 40kB


   ApplicationContainer clientApps_3 = clientHelper.Install (wifiStaNodes.Get (nWifi - 3));
  Ptr<ThreeGppHttpClient> httpClient_3 = clientApps_3.Get (0)->GetObject<ThreeGppHttpClient> ();

   ApplicationContainer clientApps_4 = clientHelper.Install (wifiStaNodes.Get (nWifi - 4));
  Ptr<ThreeGppHttpClient> httpClient_4 = clientApps_4.Get (0)->GetObject<ThreeGppHttpClient> ();

   ApplicationContainer clientApps_5 = clientHelper.Install (wifiStaNodes.Get (nWifi - 5));
  Ptr<ThreeGppHttpClient> httpClient_5 = clientApps_5.Get (0)->GetObject<ThreeGppHttpClient> ();

   ApplicationContainer clientApps_6 = clientHelper.Install (wifiStaNodes.Get (nWifi - 6));
  Ptr<ThreeGppHttpClient> httpClient_6 = clientApps_6.Get (0)->GetObject<ThreeGppHttpClient> ();

   ApplicationContainer clientApps_7 = clientHelper.Install (wifiStaNodes.Get (nWifi - 7));
  Ptr<ThreeGppHttpClient> httpClient_7 = clientApps_7.Get (0)->GetObject<ThreeGppHttpClient> ();

  ApplicationContainer clientApps_8 = clientHelper.Install (wifiStaNodes.Get (nWifi - 8));
  Ptr<ThreeGppHttpClient> httpClient_8 = clientApps_8.Get (0)->GetObject<ThreeGppHttpClient> ();

   ApplicationContainer clientApps_9 = clientHelper.Install (wifiStaNodes.Get (nWifi - 9));
  Ptr<ThreeGppHttpClient> httpClient_9 = clientApps_9.Get (0)->GetObject<ThreeGppHttpClient> ();

     ApplicationContainer clientApps_10 = clientHelper.Install (wifiStaNodes.Get (nWifi - 10));
  Ptr<ThreeGppHttpClient> httpClient_10 = clientApps_10.Get (0)->GetObject<ThreeGppHttpClient> ();
 
     ApplicationContainer clientApps_11 = clientHelper.Install (wifiStaNodes.Get (nWifi - 11));
  Ptr<ThreeGppHttpClient> httpClient_11 = clientApps_11.Get (0)->GetObject<ThreeGppHttpClient> ();

     ApplicationContainer clientApps_12 = clientHelper.Install (wifiStaNodes.Get (nWifi - 12));
  Ptr<ThreeGppHttpClient> httpClient_12 = clientApps_12.Get (0)->GetObject<ThreeGppHttpClient> ();

      ApplicationContainer clientApps_13 = clientHelper.Install (wifiStaNodes.Get (nWifi - 13));
  Ptr<ThreeGppHttpClient> httpClient_13 = clientApps_13.Get (0)->GetObject<ThreeGppHttpClient> ();


    ApplicationContainer clientApps_14 = clientHelper.Install (wifiStaNodes.Get (nWifi - 14));
  Ptr<ThreeGppHttpClient> httpClient_14 = clientApps_14.Get (0)->GetObject<ThreeGppHttpClient> ();



    ApplicationContainer clientApps_15 = clientHelper.Install (wifiStaNodes.Get (nWifi - 15));
  Ptr<ThreeGppHttpClient> httpClient_15 = clientApps_15.Get (0)->GetObject<ThreeGppHttpClient> ();


    ApplicationContainer clientApps_16 = clientHelper.Install (wifiStaNodes.Get (nWifi - 16));
  Ptr<ThreeGppHttpClient> httpClient_16 = clientApps_16.Get (0)->GetObject<ThreeGppHttpClient> ();

    ApplicationContainer clientApps_17 = clientHelper.Install (wifiStaNodes.Get (nWifi - 17));
  Ptr<ThreeGppHttpClient> httpClient_17 = clientApps_17.Get (0)->GetObject<ThreeGppHttpClient> ();


  // Example of connecting to the trace sources
  httpClient->TraceConnectWithoutContext ("RxMainObject", MakeCallback (&ClientMainObjectReceived));
  httpClient->TraceConnectWithoutContext ("RxEmbeddedObject", MakeCallback (&ClientEmbeddedObjectReceived));
  httpClient->TraceConnectWithoutContext ("Rx", MakeCallback (&ClientRx));


  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();
  // To stop the simulation
  Simulator::Stop (Seconds (180.0));

  if (tracing == true)
    {
      //pointToPoint_1.EnablePcapAll ("third_1");
      //phy.EnablePcap ("WiFi_phy_AP", apDevices.Get (0));
      Server.EnablePcap("server",ServerDevices.Get (Server_Nodes));
      //phy.EnablePcap("WiFi_phy_STA", staDevices);
      //csma.EnablePcap ("third_1", csmaDevices.Get (0), true);
    }
  /*
  std::ostringstream oss;
  oss <<
    "/NodeList/" << wifiStaNodes.Get (nWifi - 1)->GetId () <<
    "/$ns3::MobilityModel/CourseChange";
  Config::Connect (oss.str (), MakeCallback (&CourseChange));
  */
  
  // To visualize the code in NetAnim
  
  AnimationInterface anim ("WiFi_HTTP.xml");
  anim.SetConstantPosition(wifiStaNodes.Get(0),10.0,20.0);
  anim.SetConstantPosition(wifiStaNodes.Get(1),10.0,30.0);
  anim.SetConstantPosition(wifiStaNodes.Get(2),10.0,40.0);
  //anim.SetConstantPosition(wifiStaNodes.Get(3),10.0,50.0);
  anim.SetConstantPosition(wifiApNode.Get(0),20.0,35.0);
  anim.SetConstantPosition(NAN_Nodes.Get(0),25.0,35.0);
  anim.SetConstantPosition(NAN_Nodes.Get(1),35.0,35.0);
  anim.SetConstantPosition(NAN_To_WAN.Get(0),40.0,40.0);
  anim.SetConstantPosition(NAN_To_WAN.Get(1),40.0,30.0);
  anim.SetConstantPosition(WAN_Nodes.Get(0),50.0,35.0);
  anim.SetConstantPosition(WAN_Nodes.Get(1),60.0,35.0);
  anim.SetConstantPosition(Workstation_Server.Get(0),70.0,40.0);
  anim.SetConstantPosition(Workstation_Server.Get(1),70.0,60.0);
  anim.SetConstantPosition(Workstation_Server.Get(2),70.0,80.0);

  Simulator::Run ();
  Simulator::Destroy ();
  return 0;
}
