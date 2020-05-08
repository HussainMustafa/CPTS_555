#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/applications-module.h"
#include "ns3/mobility-module.h"
#include "ns3/config-store-module.h"
#include "ns3/wimax-module.h"
#include "ns3/internet-module.h"
#include "ns3/global-route-manager.h"
#include "ns3/ipcs-classifier-record.h"
#include "ns3/service-flow.h"
#include <iostream>
#include "ns3/csma-module.h"
#include "ns3/internet-module.h"
#include "ns3/yans-wifi-helper.h"
#include "ns3/ssid.h"
#include "ns3/point-to-point-module.h"
#include "ns3/netanim-module.h"
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/applications-module.h"
#include "ns3/mobility-module.h"
#include "ns3/config-store-module.h"
#include "ns3/wimax-module.h"
#include "ns3/csma-module.h"
#include <iostream>
#include "ns3/global-route-manager.h"
#include "ns3/internet-module.h"
#include "ns3/vector.h"


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
       value = 1000;
     }
   while ((value < m_embeddedObjectSizeMin) || (value >= m_embeddedObjectSizeMax));

   return value;
 }

#define MAXSS 1000
#define MAXDIST 10 // km

int main (int argc, char *argv[])
{
  bool verbose = false;
  uint32_t Server_Nodes = 3;
  uint32_t NAN_To_WAN_CSMA = 3;

  //Packet::EnableChecking ();
  int duration = 7, schedType = 0;
  WimaxHelper::SchedulerType scheduler = WimaxHelper::SCHED_TYPE_SIMPLE;

  CommandLine cmd;
  cmd.AddValue ("scheduler", "type of scheduler to use with the network devices", schedType);
  cmd.AddValue ("duration", "duration of the simulation in seconds", duration);
  cmd.AddValue ("verbose", "turn on all WimaxNetDevice log components", verbose);
  cmd.Parse (argc, argv);
  LogComponentEnable ("UdpClient", LOG_LEVEL_INFO);
  LogComponentEnable ("UdpServer", LOG_LEVEL_INFO);
  Time::SetResolution (Time::NS);
  LogComponentEnableAll (LOG_PREFIX_TIME);
  //LogComponentEnable ("ThreeGppHttpExample", LOG_INFO);
  switch (schedType)
    {
    case 0:
      scheduler = WimaxHelper::SCHED_TYPE_SIMPLE;
      break;
    case 1:
      scheduler = WimaxHelper::SCHED_TYPE_MBQOS;
      break;
    case 2:
      scheduler = WimaxHelper::SCHED_TYPE_RTPS;
      break;
    default:
      scheduler = WimaxHelper::SCHED_TYPE_SIMPLE;
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

  NodeContainer ssNodes;
  ssNodes.Create (4);

  NodeContainer bsNodes;
  bsNodes.Add (NAN_Nodes.Get (0));

  bsNodes.Create (1);

  WimaxHelper wimax;
  Ptr<SimpleOfdmWimaxChannel> channel;

  channel = CreateObject<SimpleOfdmWimaxChannel> ();
  channel->SetPropagationModel (SimpleOfdmWimaxChannel::COST231_PROPAGATION);

  NetDeviceContainer ssDevs, bsDevs, bsDevsOne;
  Ptr<ConstantPositionMobilityModel> BSPosition;
  Ptr<RandomWaypointMobilityModel> SSPosition[MAXSS];
  Ptr<RandomRectanglePositionAllocator> SSPosAllocator[MAXSS];

  ssDevs = wimax.Install (ssNodes,
                          WimaxHelper::DEVICE_TYPE_SUBSCRIBER_STATION,
                          WimaxHelper::SIMPLE_PHY_TYPE_OFDM,
                          channel,
                          scheduler);
  Ptr<WimaxNetDevice> dev = wimax.Install (bsNodes.Get(0), WimaxHelper::DEVICE_TYPE_BASE_STATION, WimaxHelper::SIMPLE_PHY_TYPE_OFDM, channel, scheduler);
  BSPosition = CreateObject<ConstantPositionMobilityModel> ();

  BSPosition->SetPosition (Vector (1000, 0, 0));
  bsNodes.Get (0)->AggregateObject (BSPosition);
  
  bsDevs.Add (dev);
  wimax.EnableAscii ("bs-devices", bsDevs);
  wimax.EnableAscii ("ss-devices", ssDevs);

  Ptr<SubscriberStationNetDevice> ss[4];
  //ss = ssDevs.Get (0)->GetObject<SubscriberStationNetDevice> ();
  //ss->SetModulationType (WimaxPhy::MODULATION_TYPE_QAM16_12);


  for (int i = 0; i < 4; i++)
    {
      ss[i] = ssDevs.Get (i)->GetObject<SubscriberStationNetDevice> ();
      ss[i]->SetModulationType (WimaxPhy::MODULATION_TYPE_QAM16_12);
    }

  Ptr<BaseStationNetDevice> bs;

  bs = bsDevs.Get (0)->GetObject<BaseStationNetDevice> ();
  /*
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
  mobility.Install (ssNodes);

  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobility.Install (bsNodes);
  */
  InternetStackHelper stack;
  stack.Install (bsNodes);
  stack.Install (ssNodes);
  stack.Install (NAN_To_WAN);
  stack.Install (Workstation_Server);

  Ipv4AddressHelper address;
  address.SetBase ("10.1.1.0", "255.255.255.0");
  bsDevsOne.Add (bs);
  

  Ipv4InterfaceContainer SSinterfaces = address.Assign (ssDevs);
  Ipv4InterfaceContainer BSinterface = address.Assign (bsDevsOne);

  address.SetBase ("10.1.2.0", "255.255.255.0");

  Ipv4InterfaceContainer NAN_Interfaces = address.Assign (NAN_Device);

  address.SetBase ("10.1.3.0", "255.255.255.0");

  Ipv4InterfaceContainer WAN_Interfaces = address.Assign (WAN_Device);

  address.SetBase ("10.1.4.0", "255.255.255.0");

  Ipv4InterfaceContainer NAN_To_WAN_Interfaces = address.Assign (NAN_To_WANDevices);

  address.SetBase ("10.1.5.0", "255.255.255.0");

  Ipv4InterfaceContainer ServerInterfaces = address.Assign (ServerDevices);

  if (verbose)
    {
      wimax.EnableLogComponents ();  // Turn on all wimax logging
    }
  /*------------------------------*/
   if (true)
    {
      LogComponentEnable ("UdpEchoClientApplication", LOG_LEVEL_INFO);
      LogComponentEnable ("UdpEchoServerApplication", LOG_LEVEL_INFO);
    }
  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

   // Create HTTP server helper
  Ipv4Address ServerAddress(ServerInterfaces.GetAddress (Server_Nodes));
  Ipv4Address BSAddress(BSinterface.GetAddress (0));
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
  // Install HTTP client
  
  ApplicationContainer clientApps = clientHelper.Install (ssNodes.Get (2));
  Ptr<ThreeGppHttpClient> httpClient = clientApps.Get (0)->GetObject<ThreeGppHttpClient> ();

  ApplicationContainer clientApps2 = clientHelper.Install (ssNodes.Get (1));
  Ptr<ThreeGppHttpClient> httpClient2 = clientApps2.Get (0)->GetObject<ThreeGppHttpClient> ();

  ApplicationContainer clientApps3 = clientHelper.Install (ssNodes.Get (0));
  Ptr<ThreeGppHttpClient> httpClient3 = clientApps3.Get (0)->GetObject<ThreeGppHttpClient> ();



  // Example of connecting to the trace sources
  //httpClient->TraceConnectWithoutContext ("RxMainObject", MakeCallback (&ClientMainObjectReceived));
  //httpClient->TraceConnectWithoutContext ("RxEmbeddedObject", MakeCallback (&ClientEmbeddedObjectReceived));
  //httpClient->TraceConnectWithoutContext ("Rx", MakeCallback (&ClientRx));
/*
httpClient2->TraceConnectWithoutContext ("RxMainObject", MakeCallback (&ClientMainObjectReceived));
  httpClient2->TraceConnectWithoutContext ("RxEmbeddedObject", MakeCallback (&ClientEmbeddedObjectReceived));
  httpClient2->TraceConnectWithoutContext ("Rx", MakeCallback (&ClientRx));

httpClient3->TraceConnectWithoutContext ("RxMainObject", MakeCallback (&ClientMainObjectReceived));
  httpClient3->TraceConnectWithoutContext ("RxEmbeddedObject", MakeCallback (&ClientEmbeddedObjectReceived));
  httpClient3->TraceConnectWithoutContext ("Rx", MakeCallback (&ClientRx));
*/
  //wimax.EnablePcap ("wimax-simple-ss0", ssNodes.Get (0)->GetId (), ss[0]->GetIfIndex ());
  //wimax.EnablePcap ("wimax-simple-ss1", ssNodes.Get (1)->GetId (), ss[1]->GetIfIndex ());
  //wimax.EnablePcap ("wimax-simple-ss2", ssNodes.Get (2)->GetId (), ss[2]->GetIfIndex ());
  //wimax.EnablePcap ("wimax-simple-ss0", ssNodes.Get (3)->GetId (), ss[3]->GetIfIndex ());
  //wimax.EnablePcap ("wimax-simple-ss0", ssNodes.Get (4)->GetId (), ss[4]->GetIfIndex ());
  //wimax.EnablePcap ("wimax-simple-bs0", bsNodes.Get (0)->GetId (), bs->GetIfIndex ());
  Server.EnablePcap ("wimax-simple-server", ServerDevices.Get (Server_Nodes), true);
/*
  IpcsClassifierRecord DlClassifierUgs (Ipv4Address ("0.0.0.0"),
                                        Ipv4Mask ("0.0.0.0"),
                                        SSinterfaces.GetAddress (1),
                                        Ipv4Mask ("255.255.255.255"),
                                        0,
                                        65000,
                                        100,
                                        100,
                                        17,
                                        1);
  ServiceFlow DlServiceFlowUgs = wimax.CreateServiceFlow (ServiceFlow::SF_DIRECTION_DOWN,
                                                          ServiceFlow::SF_TYPE_RTPS,
                                                          DlClassifierUgs);
 */
  IpcsClassifierRecord UlClassifierUgs (SSinterfaces.GetAddress (2),
                                        Ipv4Mask ("255.255.255.0"),
                                        ServerAddress,
                                        Ipv4Mask ("255.255.255.0"),
                                        0,
                                        65000,
                                        0,
                                        65000,
                                        6,
                                        1);
  ServiceFlow UlServiceFlowUgs = wimax.CreateServiceFlow (ServiceFlow::SF_DIRECTION_DOWN,
                                                          ServiceFlow::SF_TYPE_RTPS,
                                                          UlClassifierUgs);
  
  IpcsClassifierRecord UlClassifierUgs_2 (SSinterfaces.GetAddress (1),
                                        Ipv4Mask ("255.255.255.0"),
                                        ServerAddress,
                                        Ipv4Mask ("255.255.255.0"),
                                        0,
                                        65000,
                                        0,
                                        65000,
                                        6,
                                        1);
                                        
  ServiceFlow UlServiceFlowUgs_2 = wimax.CreateServiceFlow (ServiceFlow::SF_DIRECTION_UP,
                                                          ServiceFlow::SF_TYPE_RTPS,
                                                          UlClassifierUgs_2);

  IpcsClassifierRecord UlClassifierUgs_3 (SSinterfaces.GetAddress (0),
                                        Ipv4Mask ("255.255.255.0"),
                                        ServerAddress,
                                        Ipv4Mask ("255.255.255.0"),
                                        0,
                                        65000,
                                        0,
                                        65000,
                                        6,
                                        1);
                                        
ServiceFlow UlServiceFlowUgs_3 = wimax.CreateServiceFlow (ServiceFlow::SF_DIRECTION_UP,
                                                          ServiceFlow::SF_TYPE_RTPS,
                                                          UlClassifierUgs_3);                                                        
 

  ss[2]->AddServiceFlow (UlServiceFlowUgs);
  ss[1]->AddServiceFlow (UlServiceFlowUgs_2);
  ss[0]->AddServiceFlow (UlServiceFlowUgs_3);
  //ss[3]->AddServiceFlow (UlServiceFlowUgs);
  //ss[4]->AddServiceFlow (UlServiceFlowUgs);
  //ss[4]->AddServiceFlow (DlServiceFlowUgs);

  NS_LOG_INFO ("Starting simulation.....");

  AnimationInterface anim ("Wimax_AMI.xml");
  anim.SetConstantPosition(ssNodes.Get(0),100.0,200.0);
  //anim.SetConstantPosition(ssNodes.Get(1),10.0,30.0);
  //anim.SetConstantPosition(ssNodes.Get(2),10.0,40.0);
  anim.SetConstantPosition(bsNodes.Get(0),200.0,350.0);
  anim.SetConstantPosition(NAN_Nodes.Get(0),250.0,350.0);
  anim.SetConstantPosition(NAN_Nodes.Get(1),350.0,350.0);
  anim.SetConstantPosition(NAN_To_WAN.Get(0),400.0,400.0);
  anim.SetConstantPosition(NAN_To_WAN.Get(1),400.0,300.0);
  anim.SetConstantPosition(WAN_Nodes.Get(0),500.0,350.0);
  anim.SetConstantPosition(WAN_Nodes.Get(1),600.0,350.0);
  anim.SetConstantPosition(Workstation_Server.Get(0),700.0,400.0);
  anim.SetConstantPosition(Workstation_Server.Get(1),700.0,600.0);
  anim.SetConstantPosition(Workstation_Server.Get(2),700.0,800.0);
  anim.SetConstantPosition(Workstation_Server.Get(3),700.0,800.0);


  Simulator::Stop (Seconds (320.0));
  Simulator::Run ();

  //ss[0] = 0;
  //ss = 0;
  //bs = 0;

  Simulator::Destroy ();
  NS_LOG_INFO ("Done.");

  return 0;
}
