#include "ns3/core-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/internet-module.h"
#include "ns3/applications-module.h"
#include "ns3/mobility-module.h"
#include "ns3/config-store-module.h"
#include "ns3/lte-module.h"
#include "ns3/netanim-module.h"
//#include "ns3/gtk-config-store.h"

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

uint32_t
 ThreeGppHttpVariables::GetEmbeddedObjectSize ()
 {
   // Validate parameters.
   if (m_embeddedObjectSizeMax <= m_embeddedObjectSizeMin)
     {
       NS_FATAL_ERROR ("`EmbeddedObjectSizeMax` attribute "
                       << " must be greater than"
                       << " the `EmbeddedObjectSizeMin` attribute.");
     }

   /*
    * Repeatedly draw one new random value until it falls in the interval
    * [min, max). The previous validation ensures this process does not loop
    * indefinitely.
    */
   uint32_t value;
   do
     {
       //value = m_embeddedObjectSizeRng->GetInteger ();
       value = 100;
     }
   while ((value < m_embeddedObjectSizeMin) || (value >= m_embeddedObjectSizeMax));

   return value;
 }




int
main (int argc, char *argv[])
{
  uint16_t numNodePairs = 2;
  Time simTime = MilliSeconds (1100);
  double distance = 60.0;
  Time interPacketInterval = MilliSeconds (100);
  bool useCa = false;
  bool disableDl = false;
  bool disableUl = false;
  bool disablePl = false;
  bool verbose = true;

  // Command line arguments
  CommandLine cmd;
  cmd.AddValue ("numNodePairs", "Number of eNodeBs + UE pairs", numNodePairs);
  cmd.AddValue ("simTime", "Total duration of the simulation", simTime);
  cmd.AddValue ("distance", "Distance between eNBs [m]", distance);
  cmd.AddValue ("interPacketInterval", "Inter packet interval", interPacketInterval);
  cmd.AddValue ("useCa", "Whether to use carrier aggregation.", useCa);
  cmd.AddValue ("disableDl", "Disable downlink data flows", disableDl);
  cmd.AddValue ("disableUl", "Disable uplink data flows", disableUl);
  cmd.AddValue ("disablePl", "Disable data flows between peer UEs", disablePl);
  cmd.Parse (argc, argv);


  ConfigStore inputConfig;
  inputConfig.ConfigureDefaults ();

  // parse again so you can override default values from the command line
  cmd.Parse(argc, argv);

  if (useCa)
   {
     Config::SetDefault ("ns3::LteHelper::UseCa", BooleanValue (useCa));
     Config::SetDefault ("ns3::LteHelper::NumberOfComponentCarriers", UintegerValue (2));
     Config::SetDefault ("ns3::LteHelper::EnbComponentCarrierManager", StringValue ("ns3::RrComponentCarrierManager"));
   }

  Ptr<LteHelper> lteHelper = CreateObject<LteHelper> ();
  Ptr<PointToPointEpcHelper> epcHelper = CreateObject<PointToPointEpcHelper> ();
  lteHelper->SetEpcHelper (epcHelper);

  Ptr<Node> pgw = epcHelper->GetPgwNode ();

   // Create a single RemoteHost
  NodeContainer remoteHostContainer;
  remoteHostContainer.Create (3);
  Ptr<Node> Server_1 = remoteHostContainer.Get (0);
  InternetStackHelper internet;
  internet.Install (remoteHostContainer);

  // Create the Internet
  PointToPointHelper p2ph;
  p2ph.SetDeviceAttribute ("DataRate", StringValue ("1Mbps"));
  p2ph.SetDeviceAttribute ("Mtu", UintegerValue (1500));
  p2ph.SetChannelAttribute ("Delay", StringValue ("2ms"));
  NetDeviceContainer internetDevices = p2ph.Install (pgw, Server_1);
  Ipv4AddressHelper ipv4h;
  ipv4h.SetBase ("1.0.0.0", "255.0.0.0");
  Ipv4InterfaceContainer internetIpIfaces = ipv4h.Assign (internetDevices);
  // interface 0 is localhost, 1 is the p2p device
  Ipv4Address remoteHostAddr = internetIpIfaces.GetAddress (1);
  NS_LOG_UNCOND (remoteHostAddr);
  NS_LOG_UNCOND (internetIpIfaces.GetAddress (0));

  Ipv4StaticRoutingHelper ipv4RoutingHelper;
  Ptr<Ipv4StaticRouting> remoteHostStaticRouting = ipv4RoutingHelper.GetStaticRouting (Server_1->GetObject<Ipv4> ());
  remoteHostStaticRouting->AddNetworkRouteTo (Ipv4Address ("7.0.0.0"), Ipv4Mask ("255.0.0.0"), 1);

  NodeContainer ueNodes;
  NodeContainer enbNodes;
  enbNodes.Create (numNodePairs);
  ueNodes.Create (numNodePairs);

  // Install Mobility Model
  Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator> ();
  for (uint16_t i = 0; i < numNodePairs; i++)
    {
      positionAlloc->Add (Vector (distance * i, 0, 0));
    }
  MobilityHelper mobility;
  mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
  mobility.SetPositionAllocator(positionAlloc);
  mobility.Install(enbNodes);
  mobility.Install(ueNodes);

  // Install LTE Devices to the nodes
  NetDeviceContainer enbLteDevs = lteHelper->InstallEnbDevice (enbNodes);
  NetDeviceContainer ueLteDevs = lteHelper->InstallUeDevice (ueNodes);

  // Install the IP stack on the UEs
  internet.Install (ueNodes);
  Ipv4InterfaceContainer ueIpIface;
  ueIpIface = epcHelper->AssignUeIpv4Address (NetDeviceContainer (ueLteDevs));
  // Assign IP address to UEs, and install applications
  for (uint32_t u = 0; u < ueNodes.GetN (); ++u)
    {
      Ptr<Node> ueNode = ueNodes.Get (u);
      // Set the default gateway for the UE
      Ptr<Ipv4StaticRouting> ueStaticRouting = ipv4RoutingHelper.GetStaticRouting (ueNode->GetObject<Ipv4> ());
      ueStaticRouting->SetDefaultRoute (epcHelper->GetUeDefaultGatewayAddress (), 1);
    }

  // Attach one UE per eNodeB
  for (uint16_t i = 0; i < numNodePairs; i++)
    {
      lteHelper->Attach (ueLteDevs.Get(i), enbLteDevs.Get(i));
      // side effect: the default EPS bearer will be activated
    }


  // Install and start applications on UEs and remote host
  uint16_t dlPort = 1100;
  uint16_t ulPort = 2000;
  uint16_t otherPort = 3000;
  ApplicationContainer clientApps;
  ApplicationContainer serverApps;


  
  for (uint32_t u = 0; u < ueNodes.GetN (); ++u)
    {
      if (!disableDl)
        {
          PacketSinkHelper dlPacketSinkHelper ("ns3::TcpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), dlPort));
          serverApps.Add (dlPacketSinkHelper.Install (ueNodes.Get (u)));
          serverApps.Start(Seconds(1.0));
          serverApps.Stop(Seconds(10.0));
          Ipv4Address wks1_Address(ueIpIface.GetAddress (u));
          InetSocketAddress wks1_Socket_Address(wks1_Address, dlPort);
          OnOffHelper dlClient ("ns3::TcpSocketFactory",wks1_Socket_Address);
          ApplicationContainer wks1_OnOffApp = dlClient.Install(Server_1);
          dlClient.SetConstantRate(DataRate (60000000));
          dlClient.SetAttribute ("PacketSize", UintegerValue (1024));
          wks1_OnOffApp.Start(Seconds(1.0));
          wks1_OnOffApp.Start(Seconds(10.0));

        }

      if (!disableUl)
        {
          ++ulPort;
          PacketSinkHelper ulPacketSinkHelper ("ns3::TcpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), ulPort));
          serverApps.Add (ulPacketSinkHelper.Install (Server_1));
          serverApps.Start(Seconds(1.0));
          serverApps.Stop(Seconds(180.0));
          Ipv4Address wks2_Address(internetIpIfaces.GetAddress (1));
          InetSocketAddress wks2_Socket_Address(wks2_Address, ulPort);
          OnOffHelper ulClient ("ns3::TcpSocketFactory",wks2_Socket_Address);
          ApplicationContainer wks2_OnOffApp = ulClient.Install(Server_1);
          ulClient.SetConstantRate(DataRate (60000000));
          ulClient.SetAttribute ("PacketSize", UintegerValue (1024));
          wks2_OnOffApp.Start(Seconds(1.0));
          wks2_OnOffApp.Start(Seconds(10.0));
        }

        
/*
      if (!disablePl && numNodePairs > 1)
        {
          ++otherPort;
          PacketSinkHelper packetSinkHelper ("ns3::UdpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), otherPort));
          serverApps.Add (packetSinkHelper.Install (ueNodes.Get (u)));

          UdpClientHelper client (ueIpIface.GetAddress (u), otherPort);
          client.SetAttribute ("Interval", TimeValue (interPacketInterval));
          client.SetAttribute ("MaxPackets", UintegerValue (1000000));
          clientApps.Add (client.Install (ueNodes.Get ((u + 1) % numNodePairs)));
        }
        */
    }
  
   // Create HTTP server helper
  //Ipv4Address ServerAddress(ServerInterfaces.GetAddress (Server_Nodes));
  //Ipv4Address BSAddress(BSinterface.GetAddress (0));
  //uint16_t Server_port = 9;
  ThreeGppHttpServerHelper serverHelper (remoteHostAddr);

  // Install HTTP server
  ApplicationContainer serverApps = serverHelper.Install (Server_1);
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


  ThreeGppHttpClientHelper clientHelper (remoteHostAddr);
  // Install HTTP client
  ApplicationContainer clientApps = clientHelper.Install (pgw);
  Ptr<ThreeGppHttpClient> httpClient = clientApps.Get (0)->GetObject<ThreeGppHttpClient> ();

  ApplicationContainer clientApps2 = clientHelper.Install (ssNodes.Get (1));
  Ptr<ThreeGppHttpClient> httpClient2 = clientApps2.Get (0)->GetObject<ThreeGppHttpClient> ();

  ApplicationContainer clientApps3 = clientHelper.Install (ssNodes.Get (0));
  Ptr<ThreeGppHttpClient> httpClient3 = clientApps3.Get (0)->GetObject<ThreeGppHttpClient> ();



  // Example of connecting to the trace sources
  httpClient->TraceConnectWithoutContext ("RxMainObject", MakeCallback (&ClientMainObjectReceived));
  httpClient->TraceConnectWithoutContext ("RxEmbeddedObject", MakeCallback (&ClientEmbeddedObjectReceived));
  httpClient->TraceConnectWithoutContext ("Rx", MakeCallback (&ClientRx));
  
  //serverApps.Start (MilliSeconds (500));
  //clientApps.Start (MilliSeconds (500));
  lteHelper->EnableTraces ();
  // Uncomment to enable PCAP tracing
  p2ph.EnablePcap("lena-simple-epc", internetDevices.Get(1));
  
  AnimationInterface anim ("lena_test.xml");
  anim.SetConstantPosition(ueNodes.Get(0),100.0,200.0);
  anim.SetConstantPosition(ueNodes.Get(1),150.0,300.0);
  //anim.SetConstantPosition(wifiStaNodes.Get(2),10.0,40.0);
  //anim.SetConstantPosition(wifiStaNodes.Get(3),10.0,50.0);
  anim.SetConstantPosition(enbNodes.Get(0),280.0,350.0);
  anim.SetConstantPosition(enbNodes.Get(1),400.0,380.0);
  anim.SetConstantPosition(remoteHostContainer.Get(0),450.0,500.0);
  anim.SetConstantPosition(remoteHostContainer.Get(1),420.0,430.0);
  //anim.SetConstantPosition(NAN_To_WAN.Get(1),40.0,30.0);
  //anim.SetConstantPosition(WAN_Nodes.Get(0),50.0,35.0);
  //anim.SetConstantPosition(WAN_Nodes.Get(1),60.0,35.0);
  //anim.SetConstantPosition(Workstation_Server.Get(0),70.0,40.0);
  //anim.SetConstantPosition(Workstation_Server.Get(1),70.0,60.0);
  //anim.SetConstantPosition(Workstation_Server.Get(2),70.0,80.0);

  Simulator::Stop (simTime);
  Simulator::Run ();

  /*GtkConfigStore config;
  config.ConfigureAttributes();*/

  Simulator::Destroy ();
  return 0;
}
