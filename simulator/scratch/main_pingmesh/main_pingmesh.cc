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
#include "ns3/pingmesh-scheduler.h"

using namespace ns3;

int main(int argc, char *argv[]) {

    // No buffering of printf
    setbuf(stdout, nullptr);
    
    // Retrieve run directory
    CommandLine cmd;
    std::string run_dir = "";
    cmd.Usage("Usage: ./waf --run=\"main_pingmesh --run_dir='<path/to/run/directory>'\"");
    cmd.AddValue("run_dir",  "Run directory", run_dir);
    cmd.Parse(argc, argv);
    if (run_dir.compare("") == 0) {
        printf("Usage: ./waf --run=\"main_pingmesh --run_dir='<path/to/run/directory>'\"");
        return 0;
    }

    // Start the simulation using this run directory
    BasicSimulation simulation(run_dir);
    PingmeshScheduler pingmeshScheduler(&simulation); // Requires filename_schedule to be present in the configuration
    pingmeshScheduler.Schedule();
    simulation.Run();
    pingmeshScheduler.WriteResults();
    simulation.Finalize();

    return 0;

}
