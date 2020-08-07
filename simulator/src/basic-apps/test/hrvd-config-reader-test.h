#include "ns3/basic-simulation.h"
#include "ns3/test.h"
#include "ns3/exp-util.h"
#include "ns3/horovod-worker-config-reader.h"
#include "ns3/topology-ptop.h"
#include "ns3/ipv4-arbiter-routing-helper.h"
#include "test-helpers.h"

using namespace ns3;

const std::string hrvd_worker_config_reader_test_dir = ".tmp-hrvd_worker_config_reader_test";

void prepare_hrvd_layer_reader_test_config(){

    mkdir_if_not_exists(hrvd_worker_config_reader_test_dir);
}

void cleanup_schedule_reader_test(){
    remove_dir_if_exists(hrvd_worker_config_reader_test_dir);
}


class HorovodWorkerConfigReaderTestCase : public TestCase
{

public:
    HorovodWorkerConfigReaderTestCase (): TestCase("horovod-worker-config-reader normal test") {}
    
    void DoRun(){

        prepare_hrvd_layer_reader_test_config();

        std::ofstream hrvd_config_file(hrvd_worker_config_reader_test_dir + "hrvd_config.csv");
        hrvd_config_file << "0, 5" <<std::endl;
        hrvd_config_file << "2, 10" <<std::endl;

        hrvd_config_file.close();

        std::map<int, uint64_t> layer_size_bytes = read_layer_size<uint64_t>(hrvd_config_file);

        ASSERT_EQUAL(layer_size_bytes.size(), 2);
        ASSERT_EQUAL(layer_size_bytes[0], 5);
        ASSERT_EQUAL(layer_size_bytes[2], 10);

        std::ofstream hrvd_config_file_float(hrvd_worker_config_reader_test_dir + "hrvd_config_float.csv");
        hrvd_config_file_float << "0, 5.0" <<std::endl;
        hrvd_config_file_float << "2, 10.2" <<std::endl;

        hrvd_config_file_float.close();

        std::map<int, float> layer_compute_time = read_layer_computation_time(hrvd_config_file_float);

        ASSERT_EQUAL(layer_size_bytes.size(), 2);
        ASSERT_EQUAL(layer_size_bytes[0], 5.0);
        ASSERT_EQUAL(layer_size_bytes[2], 10.2);

    }
}

