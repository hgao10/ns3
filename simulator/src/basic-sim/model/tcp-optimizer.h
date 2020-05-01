#ifndef TCP_OPTIMIZER_H
#define TCP_OPTIMIZER_H

#include "ns3/basic-simulation.h"
#include "ns3/ipv4-routing-helper.h"

namespace ns3 {

    class TcpOptimizer
    {
    public:
        static void OptimizeBasic(BasicSimulation& simulation);
        static void OptimizeUsingWorstCaseRtt(BasicSimulation& basicSimulation, int64_t worst_case_rtt_ns);
    private:
        static void CommonSense();
    };

} // namespace ns3

#endif /* TCP_OPTIMIZER_H */
