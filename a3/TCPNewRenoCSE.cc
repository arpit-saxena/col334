#include "TcpNewRenoCSE.h"

#include "ns3/log.h"

namespace ns3 {
NS_LOG_COMPONENT_DEFINE("TcpNewRenoCSE");

NS_OBJECT_ENSURE_REGISTERED(TcpNewRenoCSE);

TypeId TcpNewRenoCSE::GetTypeId(void) {
  static TypeId tid = TypeId("ns3::TcpNewRenoCSE")
                          .SetParent<TcpNewReno>()
                          .SetGroupName("Internet")
                          .AddConstructor<TcpNewRenoCSE>();
  return tid;
}

uint32_t TcpNewRenoCSE::SlowStart(Ptr<TcpSocketState> tcb,
                                  uint32_t segmentsAcked) {
  NS_LOG_FUNCTION(this << tcb << segmentsAcked);

  if (segmentsAcked >= 1) {
    double adder = std::pow(tcb->m_segmentSize, 1.9) / tcb->m_cWnd.Get();
    adder = std::max(1.0, adder);
    tcb->m_cWnd += static_cast<uint32_t>(adder);
    NS_LOG_INFO("In SlowStart, updated to cwnd " << tcb->m_cWnd << " ssthresh "
                                                 << tcb->m_ssThresh);
    return segmentsAcked - 1;
  }

  return 0;
}

void TcpNewRenoCSE::CongestionAvoidance(Ptr<TcpSocketState> tcb,
                                        uint32_t segmentsAcked) {
  NS_LOG_FUNCTION(this << tcb << segmentsAcked);

  if (segmentsAcked > 0) {
    tcb->m_cWnd += static_cast<uint32_t>(0.5 * tcb->m_segmentSize);
    NS_LOG_INFO("In CongAvoid, updated to cwnd " << tcb->m_cWnd << " ssthresh "
                                                 << tcb->m_ssThresh);
  }
}
}  // namespace ns3