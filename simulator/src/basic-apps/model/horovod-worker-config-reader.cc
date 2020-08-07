#include "horovod-worker-config-reader.h"

namespace ns3{
/**
 * Read the layer_size.csv into layer_size.
 *
 * @param filename                  File name of the schedule.csv
*/

//  template<typename T>
 std::map<int, uint64_t> read_layer_size(const std::string& filename){
    // construct to store the data
    std::map<int, uint64_t> m_layer_size;

    if (!file_exists(filename)) {
        throw std::runtime_error(format_string("File %s does not exist.", filename.c_str()));
    }
    // Open file
    std::string line;
    std::ifstream layer_file(filename);

    if(layer_file){
        while(getline(layer_file, line)) {

            //split on ,
            std::vector<std::string> comma_split = split_string(line, ",", 2);
            int layer_idx = parse_positive_int64(comma_split[0]);
            uint64_t layer_data = parse_positive_int64(comma_split[1]);
            m_layer_size[layer_idx] = layer_data;            
        }
    }
    return m_layer_size;
 }

//  template<typename T>
 std::map<int, float> read_layer_computation_time(const std::string& filename){
    // construct to store the data
    std::map<int, float> m_layer_compute_time;

    if (!file_exists(filename)) {
        throw std::runtime_error(format_string("File %s does not exist.", filename.c_str()));
    }

    // Open file
    std::string line;
    std::ifstream layer_file(filename);

    if(layer_file){
        while(getline(layer_file, line)) {
            //split on ,
            std::vector<std::string> comma_split = split_string(line, ",", 2);
            int layer_idx = parse_positive_int64(comma_split[0]);
            float layer_data = parse_positive_double(comma_split[1]);
            m_layer_compute_time[layer_idx] = layer_data;            
        }
    }
    return m_layer_compute_time;
 }

}