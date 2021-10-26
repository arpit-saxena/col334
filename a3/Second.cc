#include <fstream>

#include "ns3/applications-module.h"
#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("COL334A3Second");

// Taken from examples/tutorial/sixth.cc
class MyApp : public Application {
 public:
  MyApp();
  virtual ~MyApp();

  /**
   * Register this type.
   * \return The TypeId.
   */
  static TypeId GetTypeId(void);
  void Setup(Ptr<Socket> socket, Address address, uint32_t packetSize,
             DataRate dataRate);
  uint32_t GetNumPacketsSent() { return m_numPacketsSent; };

 private:
  virtual void StartApplication(void);
  virtual void StopApplication(void);

  void ScheduleTx(void);
  void SendPacket(void);

  Ptr<Socket> m_socket;
  Address m_peer;
  uint32_t m_packetSize;
  DataRate m_dataRate;
  EventId m_sendEvent;
  bool m_running;
  uint32_t m_numPacketsSent;
};

MyApp::MyApp()
    : m_socket(0),
      m_peer(),
      m_packetSize(0),
      m_dataRate(0),
      m_sendEvent(),
      m_running(false),
      m_numPacketsSent(0) {}

MyApp::~MyApp() { m_socket = 0; }

/* static */
TypeId MyApp::GetTypeId(void) {
  static TypeId tid = TypeId("MyApp")
                          .SetParent<Application>()
                          .SetGroupName("Tutorial")
                          .AddConstructor<MyApp>();
  return tid;
}

void MyApp::Setup(Ptr<Socket> socket, Address address, uint32_t packetSize,
                  DataRate dataRate) {
  m_socket = socket;
  m_peer = address;
  m_packetSize = packetSize;
  m_dataRate = dataRate;
}

void MyApp::StartApplication(void) {
  m_running = true;
  m_socket->Bind();
  m_socket->Connect(m_peer);
  SendPacket();
}

void MyApp::StopApplication(void) {
  m_running = false;

  if (m_sendEvent.IsRunning()) {
    Simulator::Cancel(m_sendEvent);
  }

  if (m_socket) {
    m_socket->Close();
  }
}

void MyApp::SendPacket(void) {
  Ptr<Packet> packet = Create<Packet>(m_packetSize);
  m_socket->Send(packet);
  m_numPacketsSent++;

  ScheduleTx();
}

void MyApp::ScheduleTx(void) {
  if (m_running) {
    Time tNext(Seconds(m_packetSize * 8 /
                       static_cast<double>(m_dataRate.GetBitRate())));
    m_sendEvent = Simulator::Schedule(tNext, &MyApp::SendPacket, this);
  }
}

static void CwndChange(Ptr<OutputStreamWrapper> stream, uint32_t oldCwnd,
                       uint32_t newCwnd) {
  *stream->GetStream() << Simulator::Now().GetSeconds() << "\t" << newCwnd
                       << std::endl;
}

static void RxDrop(Ptr<OutputStreamWrapper> stream, Ptr<const Packet> p) {
  *stream->GetStream() << Simulator::Now().GetSeconds() << std::endl;
}

void run(std::string channelDataRate, std::string appDataRate,
         std::string outDir) {
  const std::string protocolClass = "ns3::TcpNewReno";

  Config::SetDefault("ns3::TcpL4Protocol::SocketType",
                     StringValue(protocolClass));
  TypeId tid = TypeId::LookupByName(protocolClass);
  Config::Set("/NodeList/*/$ns3::TcpL4Protocol/SocketType", TypeIdValue(tid));

  NodeContainer nodes;
  nodes.Create(2);

  PointToPointHelper pointToPoint;
  pointToPoint.SetDeviceAttribute("DataRate", StringValue(channelDataRate));
  pointToPoint.SetChannelAttribute("Delay", StringValue("3ms"));

  NetDeviceContainer devices;
  devices = pointToPoint.Install(nodes);

  Ptr<RateErrorModel> em = CreateObject<RateErrorModel>();
  em->SetAttribute("ErrorRate", DoubleValue(0.00001));
  devices.Get(1)->SetAttribute("ReceiveErrorModel", PointerValue(em));

  InternetStackHelper stack;
  stack.Install(nodes);

  Ipv4AddressHelper address;
  address.SetBase("10.1.1.0", "255.255.255.252");
  Ipv4InterfaceContainer interfaces = address.Assign(devices);

  uint16_t sinkPort = 8080;
  Address sinkAddress(InetSocketAddress(interfaces.GetAddress(1), sinkPort));
  PacketSinkHelper packetSinkHelper(
      "ns3::TcpSocketFactory",
      InetSocketAddress(Ipv4Address::GetAny(), sinkPort));
  ApplicationContainer sinkApps = packetSinkHelper.Install(nodes.Get(1));
  sinkApps.Start(Seconds(0.));
  sinkApps.Stop(Seconds(31.));

  Ptr<Socket> ns3TcpSocket =
      Socket::CreateSocket(nodes.Get(0), TcpSocketFactory::GetTypeId());

  Ptr<MyApp> app = CreateObject<MyApp>();
  app->Setup(ns3TcpSocket, sinkAddress, 3000, DataRate(appDataRate));
  nodes.Get(0)->AddApplication(app);
  app->SetStartTime(Seconds(1.));
  app->SetStopTime(Seconds(30.));

  std::string name = "ch-" + channelDataRate + "-ap-" + appDataRate;

  AsciiTraceHelper asciiTraceHelper;
  Ptr<OutputStreamWrapper> stream =
      asciiTraceHelper.CreateFileStream(outDir + name + ".cwnd");
  ns3TcpSocket->TraceConnectWithoutContext(
      "CongestionWindow", MakeBoundCallback(&CwndChange, stream));

  Ptr<OutputStreamWrapper> dropStream =
      asciiTraceHelper.CreateFileStream(outDir + name + ".drops");
  devices.Get(1)->TraceConnectWithoutContext(
      "PhyRxDrop", MakeBoundCallback(&RxDrop, dropStream));

  Simulator::Run();
  Simulator::Destroy();

  std::cout << "Total packets sent for " << name << ": "
            << app->GetNumPacketsSent() << '\n';
}

int main(int argc, char *argv[]) {
  CommandLine cmd;
  cmd.Parse(argc, argv);

  std::cout << "Part a:\n";
  auto channelRates = {"2Mbps", "4Mbps", "10Mbps", "20Mbps", "50Mbps"};
  for (auto chRate : channelRates) {
    std::cout << "\t";
    run(chRate, "2Mbps", "tracesSecond/a/");
  }
  std::cout << '\n';

  std::cout << "Part b:\n";
  auto appRates = {"0.5Mbps", "1Mbps", "2Mbps", "4Mbps", "10Mbps"};
  for (auto apRate : appRates) {
    std::cout << "\t";
    run("6Mbps", apRate, "tracesSecond/b/");
  }

  return 0;
}
