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
#include "ns3/flow-scheduler.h"
#include "ns3/pingmesh-scheduler.h"
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
    cmd.Usage("Usage: ./waf --run=\"main_flows_and_pingmesh --run_dir='<path/to/run/directory>'\"");
    cmd.AddValue("run_dir",  "Run directory", run_dir);
    cmd.Parse(argc, argv);
    if (run_dir.compare("") == 0) {
        printf("Usage: ./waf --run=\"main_flows_and_pingmesh --run_dir='<path/to/run/directory>'\"");
        return 0;
    }

    // Load basic simulation config
    BasicSimulation simulation(run_dir);

    // Read point-to-point topology, and install routing arbiters
    Ipv4ArbiterRoutingHelper routingHelper;
    Ptr<TopologyPtop> topology = CreateObject<TopologyPtop>(&simulation, &routingHelper);
    ArbiterEcmpHelper::InstallArbiters(simulation, topology);

    // Optimize TCP
    TcpOptimizer::OptimizeUsingWorstCaseRtt(simulation, topology->GetWorstCaseRttEstimateNs());

    // Schedule flows
    FlowScheduler flowScheduler(&simulation, topology); // Requires filename_schedule to be present in the configuration
    flowScheduler.Schedule();

    // Schedule pings
    PingmeshScheduler pingmeshScheduler(&simulation, topology); // Requires pingmesh_interval_ns to be present in the configuration
    pingmeshScheduler.Schedule();

    // Run simulation
    simulation.Run();

    // Write results
    flowScheduler.WriteResults();
    pingmeshScheduler.WriteResults();

    // Finalize the simulation
    simulation.Finalize();


    return 0;

}
