#ifndef READING_HELPER_H
#define READING_HELPER_H

#include <string>
#include <vector>
#include <map>
#include <tuple>
#include <cstring>
#include <fstream>
#include <cinttypes>
#include <algorithm>
#include <regex>
#include "simon-util.h"

namespace ns3 {

struct schedule_entry {
    int64_t flow_id;
    int64_t from_node_id;
    int64_t to_node_id;
    int64_t size_byte;
    int64_t start_time_ns;
    std::string additional_parameters;
    std::string metadata;
};

void read_schedule(const std::string& filename, int64_t num_nodes, std::vector<schedule_entry>& schedule);

}

#endif //READING_HELPER_H
