#ifndef TCPNEWRENOCSE_H
#define TCPNEWRENOCSE_H

#include "tcp-congestion-ops.h"

namespace ns3 {
class TcpNewRenoCSE : public TcpNewReno {
 public:
  static TypeId GetTypeId(void);

 protected:
  virtual uint32_t SlowStart(Ptr<TcpSocketState> tcb,
                             uint32_t segmentsAcked) override;
  virtual void CongestionAvoidance(Ptr<TcpSocketState> tcb,
                                   uint32_t segmentsAcked) override;
};
};  // namespace ns3

#endif /* TCPNEWRENOCSE_H */