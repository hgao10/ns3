#include <map>
#include <iostream>
#include <fstream>
#include <string>
#include <ctime>
#include <iostream>
#include <fstream>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
#include <chrono>
#include <stdexcept>
#include "ns3/basic-simulation.h"
#include "ns3/horovod-scheduler.h"
#include "ns3/ppbp-scheduler.h"
#include "ns3/topology-ptop.h"
#include "ns3/tcp-optimizer.h"
#include "ns3/arbiter-ecmp-helper.h"
#include "ns3/ipv4-arbiter-routing-helper.h"

using namespace ns3;

int main(int argc, char *argv[]) {

    // No buffering of printf
    setbuf(stdout, nullptr);
    
    // Retrieve run directory
    CommandLine cmd;
    std::string run_dir = "";
    cmd.Usage("Usage: ./waf --run=\"main_horovod --run_dir='<path/to/run/directory>'\"");
    cmd.AddValue("run_dir",  "Run directory", run_dir);
    cmd.Parse(argc, argv);
    if (run_dir.compare("") == 0) {
        printf("Usage: ./waf --run=\"main_horovod --run_dir='<path/to/run/directory>'\"");
        return 0;
    }

    // Load basic simulation environment
    Ptr<BasicSimulation> basicSimulation = CreateObject<BasicSimulation>(run_dir);
    std::cout<<"sim log dir: "<<basicSimulation->GetLogsDir()<<std::endl;
    // Read point-to-point topology, and install routing arbiters
    Ptr<TopologyPtop> topology = CreateObject<TopologyPtop>(basicSimulation, Ipv4ArbiterRoutingHelper());
    ArbiterEcmpHelper::InstallArbiters(basicSimulation, topology);

    // Optimize TCP
    TcpOptimizer::OptimizeUsingWorstCaseRtt(basicSimulation, topology->GetWorstCaseRttEstimateNs());

    // Schedule PPBP background traffic
    PPBPScheduler ppbpscheduler(basicSimulation, topology); // Requires filename_schedule to be present in the configuration
    // ppbpscheduler.Schedule(1025, 0x10, 50, 5);
    ppbpscheduler.Schedule(1025, 0x10, topology->GetNumberOfActiveBursts(), 0.2);
    HorovodScheduler horovodscheduler(basicSimulation, topology); // Requires filename_schedule to be present in the configuration
    /*
        ToS to Priority band mapping for pfifo-fast-queue-disc
            0x10 -> 0
            0x06 -> 1
            0x08 -> 2
    */
    // horovodscheduler.Schedule(1024, 0x08);
    horovodscheduler.Schedule(1024);

    // Run simulation
    basicSimulation->Run();

    // Finalize the simulation
    basicSimulation->Finalize();
            
    return 0;

}
