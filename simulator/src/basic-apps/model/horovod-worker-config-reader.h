#ifndef HOROVOD_WORKER_CONFIG_READER_H
#define HOROVOD_WORKER_CONFIG_READER_H

#include <string>
#include <vector>
#include <map>
#include <tuple>
#include <cstring>
#include <fstream>
#include <cinttypes>
#include <algorithm>
#include <regex>
#include "ns3/exp-util.h"
#include "ns3/topology.h"

namespace ns3 {

 std::map<int, uint64_t> read_layer_size(const std::string& filename);
 std::map<int, float> read_layer_computation_time(const std::string& filename);

}

#endif //HOROVOD_WORKER_CONFIG_READER_H
