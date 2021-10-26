#include <fstream>
#include <unordered_map>

#include "ns3/applications-module.h"
#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("COL334A3Third");

const std::string outDir = "tracesThird/";

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
             DataRate dataRate, int connNum,
             std::unordered_map<uint64_t, int> &ipConnNumMap);

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

  int m_connNum;
  std::unordered_map<uint64_t, int> *m_ipConnNumMap;
};

MyApp::MyApp()
    : m_socket(0),
      m_peer(),
      m_packetSize(0),
      m_dataRate(0),
      m_sendEvent(),
      m_running(false),
      m_connNum(-1),
      m_ipConnNumMap(nullptr) {}

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
                  DataRate dataRate, int connNum,
                  std::unordered_map<uint64_t, int> &ipConnNumMap) {
  m_socket = socket;
  m_peer = address;
  m_packetSize = packetSize;
  m_dataRate = dataRate;
  m_ipConnNumMap = &ipConnNumMap;
}

void MyApp::StartApplication(void) {
  m_running = true;
  m_socket->Bind();
  m_socket->Connect(m_peer);

  Address addr;
  int res = m_socket->GetSockName(addr);
  auto addrInet = InetSocketAddress::ConvertFrom(addr);
  NS_ASSERT(res == 0);
  uint64_t ipAddr = addrInet.GetIpv4().Get();
  ipAddr <<= 16;
  ipAddr += addrInet.GetPort();
  (*m_ipConnNumMap)[ipAddr] = m_connNum;

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

static void RxDrop(Ptr<OutputStreamWrapper> stream,
                   std::unordered_map<uint64_t, int> &drops,
                   Ptr<Packet const> pConst) {
  Ptr<Packet> p = pConst->Copy();
  PppHeader pppHeader;
  p->RemoveHeader(pppHeader);
  Ipv4Header ipv4Header;
  p->RemoveHeader(ipv4Header);
  TcpHeader tcpHeader;
  p->RemoveHeader(tcpHeader);

  uint64_t ipAddr = ipv4Header.GetSource().Get();
  ipAddr <<= 16;
  ipAddr += tcpHeader.GetSourcePort();

  drops[ipAddr]++;
  *stream->GetStream() << Simulator::Now().GetSeconds() << std::endl;
}

std::vector<Ptr<Socket>> getSockets(NodeContainer &nodes, int caseNum) {
  if (caseNum == 1 || caseNum == 3) {
    std::string commonAlgo = caseNum == 1 ? "TcpNewReno" : "TcpNewRenoCSE";
    TypeId tid = TypeId::LookupByName("ns3::" + commonAlgo);
    Config::Set("/NodeList/*/$ns3::TcpL4Protocol/SocketType", TypeIdValue(tid));
    Ptr<Socket> sock1 =
        Socket::CreateSocket(nodes.Get(0), TcpSocketFactory::GetTypeId());
    Ptr<Socket> sock2 =
        Socket::CreateSocket(nodes.Get(0), TcpSocketFactory::GetTypeId());
    Ptr<Socket> sock3 =
        Socket::CreateSocket(nodes.Get(1), TcpSocketFactory::GetTypeId());
    return {sock1, sock2, sock3};
  }

  // Connection 3 uses TcpNewRenoCSE, others use TcpNewReno
  TypeId newRenoTid = TypeId::LookupByName("ns3::TcpNewReno");
  Config::Set("/NodeList/*/$ns3::TcpL4Protocol/SocketType",
              TypeIdValue(newRenoTid));
  Ptr<Socket> sock1 =
      Socket::CreateSocket(nodes.Get(0), TcpSocketFactory::GetTypeId());
  Ptr<Socket> sock2 =
      Socket::CreateSocket(nodes.Get(0), TcpSocketFactory::GetTypeId());

  TypeId newRenoCSETid = TypeId::LookupByName("ns3::TcpNewRenoCSE");
  std::stringstream nodeId;
  nodeId << nodes.Get(1)->GetId();
  std::string specificNode =
      "/NodeList/" + nodeId.str() + "/$ns3::TcpL4Protocol/SocketType";
  Config::Set(specificNode, TypeIdValue(newRenoCSETid));
  Ptr<Socket> sock3 =
      Socket::CreateSocket(nodes.Get(1), TcpSocketFactory::GetTypeId());

  return {sock1, sock2, sock3};
}

void run(int caseNum) {
  NodeContainer nodes;
  nodes.Create(3);

  PointToPointHelper pointToPoint;
  pointToPoint.SetDeviceAttribute("DataRate", StringValue("10Mbps"));
  pointToPoint.SetChannelAttribute("Delay", StringValue("3ms"));

  Ptr<RateErrorModel> em = CreateObject<RateErrorModel>();
  em->SetAttribute("ErrorRate", DoubleValue(0.00001));
  NetDeviceContainer devices13 =
      pointToPoint.Install(nodes.Get(0), nodes.Get(2));
  devices13.Get(1)->SetAttribute("ReceiveErrorModel", PointerValue(em));

  pointToPoint.SetDeviceAttribute("DataRate", StringValue("9Mbps"));
  pointToPoint.SetChannelAttribute("Delay", StringValue("3ms"));

  em = CreateObject<RateErrorModel>();
  em->SetAttribute("ErrorRate", DoubleValue(0.00001));
  NetDeviceContainer devices23 =
      pointToPoint.Install(nodes.Get(1), nodes.Get(2));
  devices23.Get(1)->SetAttribute("ReceiveErrorModel", PointerValue(em));

  InternetStackHelper stack;
  stack.Install(nodes);

  Ipv4AddressHelper address;
  address.SetBase("10.1.1.0", "255.255.255.252");
  Ipv4InterfaceContainer interfaces13 = address.Assign(devices13);

  address.SetBase("10.1.2.0", "255.255.255.252");
  Ipv4InterfaceContainer interfaces23 = address.Assign(devices23);

  uint16_t sinkPort = 8080;

  PacketSinkHelper packetSinkHelper(
      "ns3::TcpSocketFactory",
      InetSocketAddress(Ipv4Address::GetAny(), sinkPort));
  ApplicationContainer sinkApps = packetSinkHelper.Install(nodes.Get(2));
  sinkApps.Start(Seconds(0.));
  sinkApps.Stop(Seconds(31.));

  auto sockets = getSockets(nodes, caseNum);
  auto sock1 = sockets[0], sock2 = sockets[1], sock3 = sockets[2];

  Address sinkAddressConn12(
      InetSocketAddress(interfaces13.GetAddress(1), sinkPort));

  std::unordered_map<uint64_t, int> ipConnNumMap;

  Ptr<MyApp> app1 = CreateObject<MyApp>();
  app1->Setup(sock1, sinkAddressConn12, 3000, DataRate("1.5Mbps"), 1,
              ipConnNumMap);
  nodes.Get(0)->AddApplication(app1);
  app1->SetStartTime(Seconds(1.));
  app1->SetStopTime(Seconds(20.));

  Ptr<MyApp> app2 = CreateObject<MyApp>();
  app2->Setup(sock2, sinkAddressConn12, 3000, DataRate("1.5Mbps"), 2,
              ipConnNumMap);
  nodes.Get(0)->AddApplication(app2);
  app2->SetStartTime(Seconds(5.));
  app2->SetStopTime(Seconds(25.));

  Address sinkAddressConn3(
      InetSocketAddress(interfaces23.GetAddress(1), sinkPort));

  Ptr<MyApp> app3 = CreateObject<MyApp>();
  app3->Setup(sock3, sinkAddressConn3, 3000, DataRate("1.5Mbps"), 3,
              ipConnNumMap);
  nodes.Get(1)->AddApplication(app3);
  app3->SetStartTime(Seconds(15.));
  app3->SetStopTime(Seconds(30.));

  AsciiTraceHelper asciiTraceHelper;
  std::vector<Ptr<OutputStreamWrapper>> cwndStreams;
  for (size_t i = 0; i < sockets.size(); i++) {
    std::stringstream ss;
    ss << outDir << "conn-" << i + 1 << "-case-" << caseNum << ".cwnd";
    cwndStreams.push_back(asciiTraceHelper.CreateFileStream(ss.str()));
    sockets[i]->TraceConnectWithoutContext(
        "CongestionWindow", MakeBoundCallback(&CwndChange, cwndStreams.back()));
  }

  std::unordered_map<uint64_t, int> srcIpToDrops;
  std::stringstream ss;
  ss << outDir << "case-" << caseNum << ".drops";
  Ptr<OutputStreamWrapper> dropStream =
      asciiTraceHelper.CreateFileStream(ss.str());
  devices23.Get(1)->TraceConnectWithoutContext(
      "PhyRxDrop", MakeBoundCallback(&RxDrop, dropStream, srcIpToDrops));
  devices13.Get(1)->TraceConnectWithoutContext(
      "PhyRxDrop", MakeBoundCallback(&RxDrop, dropStream, srcIpToDrops));

  Simulator::Run();
  Simulator::Destroy();
}

int main(int argc, char *argv[]) {
  CommandLine cmd;
  cmd.Parse(argc, argv);

  for (int caseNum = 1; caseNum <= 3; caseNum++) {
    run(caseNum);
  }

  return 0;
}
