from networkload import *
import collections
import pathlib
import exputil
import pfabric_horovod_run
import sh
from datetime import datetime
import importlib.util
import sys
import concurrent.futures
import time
from dataclasses import dataclass, field
from typing import DefaultDict


config_dir = pathlib.Path.cwd()
# TODO figure out why Path(__file__).parent wont work for following parent calls
# config_dir = pathlib.Path(__file__).parent
ns3_basic_sim_dir = config_dir.parent.parent
utilization_plot_dir = ns3_basic_sim_dir/"build"/"plot_helpers"/"utilization_plot"
sys.path.append(f"{utilization_plot_dir.absolute()}")
print(f"sys path: {sys.path}")
try:
    import plot_pfabric_FCT                     
except ImportError:
    print("Can't import plot_pfabric_FCT")


#extract duration_ns and num_servers from config file
config_file = f"{config_dir}/config_ns3.properties"
topology_file = f"{config_dir}/topology_single.properties"
print(f"config file: {config_file}")

local_shell = exputil.LocalShell()
main_program = "pfabric_flows_horovod"

curr_date = datetime.now().strftime('%H_%M_%Y_%m_%d')

def write_schedule_ring_topology(schedule_file_path_abs, from_to_tuple_dict, flow_size, start_time_lists, start_time_counts):
    j = 0
    with open(schedule_file_path_abs, "w+") as f_out:
        for i in range(start_time_counts):
            for list_from_to in from_to_tuple_dict.values():
                # print(f"from_to_tuples: {from_to_tuples}")
                f_out.write(f"{j},{list_from_to[i][0]},{list_from_to[i][1]},{flow_size[i]},{start_time_lists[i]},,\n")
                j+=1
    # return the total number of flows
    return j 

def generate_ring_topology_file(num_nodes, topology_file):
    if num_nodes <3:
        raise RuntimeError("Need at least 3 nodes for a ring topology")
    with open(topology_file, "w") as output_file:
        output_file.write(f"num_nodes={num_nodes}\n")
        output_file.write(f"num_undirected_edges={num_nodes}\n")
        set_switches = [str(x) for x in range(num_nodes)]
        set_switches = ",".join(set_switches)
        output_file.write(f"switches=set({set_switches})\n")
        output_file.write(f"switches_which_are_tors=set({set_switches})\n")
        output_file.write(f"servers=set()\n")
        set_undirected_edges=[]
        for i in range(num_nodes):
            if i != num_nodes-1:
                set_undirected_edges.append(f"{i}-{i+1}")
            else:
                set_undirected_edges.append(f"{i}-{0}")
        set_undirected_edges = ",".join(set_undirected_edges)
        output_file.write(f"undirected_edges=set({set_undirected_edges})\n")


def run_pfabric_horovod_simulation(waf_bin_path,main_program, run_dir_abs):
    waf = sh.Command(f"{waf_bin_path}/waf")
    sh.cd(f"{waf_bin_path}")
    waf(run=f"{main_program} --run_dir='{run_dir_abs}'", _out=f"{run_dir_abs}/simulation.log", _err_to_out=True)


def generate_horovod_layer_size(model_size_in_byte, num_layers):
    layer_size_dict = collections.defaultdict()
    # layer_size_file = output_dir/"layer_size.csv"
    min_layer_size_bytes = int(2 * model_size_in_byte / (9 * num_layers))
    print(f"min_layer_size_bytes: {min_layer_size_bytes}")
    
    total_layer_size = 0
    for i in range(num_layers):
        if i < num_layers/2:
            layer_size_dict[i] = min_layer_size_bytes
        elif num_layers/2 <= i <= 0.75 * num_layers:
            layer_size_dict[i] = 4 * min_layer_size_bytes
        else:
            layer_size_dict[i] = 12 * min_layer_size_bytes
        
        assert layer_size_dict[i] != 0.0, f"layer {i} has 0 weights"
        total_layer_size += layer_size_dict[i]
        # output_file.write(f"{i}, {layer_size_dict[i]}\n")
    
    print(f"model_size: {model_size_in_byte}, num_layers: {num_layers}, total_bytes: {total_layer_size}")
    return layer_size_dict


def write_to_csv(data, csv_file):
    with open(csv_file, "w") as output_file:
        for key, value in data.items():
            output_file.write(f"{key},{value}\n")


def map_layer_to_ringallreduce(layer_size_dict, fusion_size, output_dir):
    outputfile = output_dir/"ringallreduce_map"
    fusion_count = 0
    # TODO finish the implementation



def test_generate_horovod_layer_size(output_dir):
    model_size = [100 * (10**6), 200 * (10**6)]
    num_layers = [50, 100, 150]

    for m_size in model_size:
        for n_layer in num_layers:
            output_file = output_dir/f"layer_size_config_model_{m_size}_num_layers_{n_layer}.csv"
            generate_horovod_layer_size(m_size, n_layer)


def generate_horovod_compute_time(iteration_time, num_layers, batchsize_multiplier, output_dir):
    fp_compute_time_file = output_dir/"fp_compute_time.csv"
    bp_compute_time_file = output_dir/"bp_compute_time.csv"



def estimate_network_comm_time(link_bw_Mbits, num_workers, model_size_in_byte):
    total_bytes_transferred = model_size_in_byte * 2 * (num_workers - 1)
    time_ms = total_bytes_transferred * 8 / (link_bw_Mbits * (10**3))
    return time_ms


def cal_compute_vs_network_ratio(iteration_time, network_time):
    return float(iteration_time) / float(network_time)

    
def individual_pfabric_run(args):
    utilization_interval_ns = ("100ms", 100000000)
    index, config = args
    time.sleep(index * 30)
    waf_bin_path = f"{ns3_basic_sim_dir}/simulator"
    main_program = "main_pfabric_flows_horovod"
    program_name = "pfabric_flows_horovod"
    new_run_dir = f"{program_name}_"+\
                  f"{utilization_interval_ns[0]}_"+\
                  f"arrival_{config.c_flow_arr_rate}_"+\
                  f"runhvd_{config.c_run_horovod}_"+\
                  f"hrvprio_{config.c_horovod_prio}_"+\
                  f"num_hvd_{config.c_num_workers}_"+\
                  f"link_bw_{config.c_link_bw_Mbits/1000}Gbit_"+\
                  f"{curr_date}"

    new_run_dir_path_abs = f"{ns3_basic_sim_dir}/runs/{new_run_dir}" 
    sh.mkdir(f"{new_run_dir_path_abs}")
    # cp config run_dir
    new_config_file = f"{new_run_dir_path_abs}/config_ns3.properties"
    sh.cp(f"{config_file}", f"{new_config_file}")

    # cp topology run_dir
    new_topology_file = f"{new_run_dir_path_abs}/topology_single.properties"
    sh.cp(topology_file, f"{new_topology_file}")
    generate_ring_topology_file(config.c_num_workers, new_topology_file)

    # local_shell.sed_replace_in_file_plain("./config_ns3.properties", "[ARRIVAL_RATE]", str(expected_flows_per_s))
    # local_shell.sed_replace_in_file_plain("./config_ns3.properties", "[UTILIZATION_INTERVAL]", str(utilization_interval_ns[1]))
    sh.sed("-i", f"s/\\[ARRIVAL\\-RATE\\]/{config.c_flow_arr_rate}/g", f"{new_config_file}")
    sh.sed("-i", f"s/\\[UTILIZATION\\-INTERVAL\\]/{utilization_interval_ns[1]}/g", f"{new_config_file}")
    sh.sed("-i", f"s/\\[SIMULATION\\-TIME\\-NS\\]/{config.c_simulation_ns}/g", f"{new_config_file}")
    sh.sed("-i", f"s/\\[LINK\\-DATARATE\\-MBITS\\]/{config.c_link_bw_Mbits}/g", f"{new_config_file}")
    sh.sed("-i", f"s/\\[PRIORITY\\-HEX\\]/{config.c_horovod_prio}/g", f"{new_config_file}")
    sh.sed("-i", f"s/\\[RUN\\-HOROVOD\\]/{config.c_run_horovod}/g", f"{new_config_file}")
    sh.sed("-i", f"s/\\[NUM\\-HVD\\]/{config.c_num_workers}/g", f"{new_config_file}")

    print("Going to configure hrvd layer size")
    configure_hrvd_layer_size(config.c_hrvd_specifics, new_run_dir_path_abs)
    print("finished configuring hrvd layer size")
    

    schedule_file_name = f"schedule_{config.c_flow_arr_rate}.csv"
    random.seed(123456789)
    seed_start_times = random.randint(0, 100000000)
    seed_flow_size = random.randint(0, 100000000)

    # Start times (ns)
    list_start_time_ns = draw_poisson_inter_arrival_gap_start_times_ns(config.c_simulation_ns, config.c_flow_arr_rate, seed_start_times)
    num_starts = len(list_start_time_ns)

    # (From, to) tuples
    from_to_tuples = []
    for i in range(config.c_num_workers):
        if i != config.c_num_workers-1:
            from_to_tuples.append((i, i+1))
        else:
            from_to_tuples.append((i, 0))

    expand_from_to_tuples = collections.defaultdict(list)
    for i in from_to_tuples:
        expand_from_to_tuples[i] = [i for j in range(num_starts)]    

    # print(f"expand_from_to_tuples: {expand_from_to_tuples}")
    # Flow sizes in byte
    list_flow_size_byte = list(
        round(x) for x in draw_n_times_from_cdf(num_starts, CDF_PFABRIC_WEB_SEARCH_BYTE, True, seed_flow_size)
    )

    cdf_mean_byte = get_cdf_mean(CDF_PFABRIC_WEB_SEARCH_BYTE, linear_interpolate=True)
    print("Mean: " + str(cdf_mean_byte) + " bytes")
    print("Arrival rate: " + str(config.c_flow_arr_rate) + " flows/s")

    # print("Expected utilization: " + str(expected_flows_per_s * cdf_mean_byte / 1.25e+9))
    print("Expected utilization: " + str(config.c_flow_arr_rate * cdf_mean_byte / ((config.c_link_bw_Mbits/8.0)*(10**6))))

    # Write schedule
    write_schedule_ring_topology(f"{new_run_dir_path_abs}/{schedule_file_name}", expand_from_to_tuples, list_flow_size_byte, list_start_time_ns,num_starts)

    # run the program
    run_pfabric_horovod_simulation(waf_bin_path, main_program, new_run_dir_path_abs)
    
    # plot utilization
    sh.cd(f"{utilization_plot_dir}")
    plot_pfabric_FCT.plot_pfabric_utilization(f"{new_run_dir_path_abs}/logs_ns3")

    sh.cd(f"{config_dir}")

# expected_flows_per_s = [10, 50, 100, 200, 300, 400, 500, 600, 700]

def launch_multiprocess_run(p_hrvd_configs):
    with concurrent.futures.ProcessPoolExecutor() as executor:
        executor.map(individual_pfabric_run, enumerate(p_hrvd_configs))


@dataclass
class pfabric_horovod_config:
    c_link_bw_Mbits : float
    c_simulation_ns : int
    c_flow_arr_rate: float
    c_horovod_prio: str
    c_run_horovod: str
    c_num_workers: int
    c_hrvd_specifics: DefaultDict = field(default_factory=collections.defaultdict)
    
    def add_hrvd_configs(self, **kwargs):
        for key, value in kwargs.items():
            self.c_hrvd_specifics[key] = value



def test_pfabric_horovod_config_class():
    config = pfabric_horovod_config(1, 1, 1, 1, 1, 1)
    print(config)
    more_config = {"size":0, "time": 2}
    config.add_hrvd_configs(**more_config)
    print(config)


def configure_hrvd_layer_size(hrvd_configs_dict, new_run_dir_path_abs):
    print(f"configure_hrvd_layer_size: hrvd_dict: {hrvd_configs_dict}")
    if "layer_size_dict" in hrvd_configs_dict.keys():
        # write to a file
        layer_size_filename = "layer_size.csv"
        full_path = pathlib.Path(new_run_dir_path_abs)/layer_size_filename
        print(f"layer_size file path: {full_path}")
        write_to_csv(hrvd_configs_dict["layer_size_dict"], full_path)

        # TODO, check if "hrvd_layer_size" exist if not, write new line instead of replacing

    # return 

def test_reading_layer_size():
    flows = 10
    configs = []
    link_bw_Mbits = 10000.0
    simulation_ns =12000000000
    horovod_prio ="0x10"
    run_horovod="true"
    num_workers = 3

    new_config = pfabric_horovod_config(link_bw_Mbits, simulation_ns, flows, horovod_prio, run_horovod, num_workers)
    
    model_size = 100 * (10**6)
    num_layers = 50

    layer_size_dict = generate_horovod_layer_size(model_size, num_layers)

    optional_config_field = {"layer_size_dict": layer_size_dict}
    new_config.add_hrvd_configs(**optional_config_field)
    # print(f"new_config: {new_config}")
    # print(f"hrvd_specific config: {new_config.c_hrvd_specifics['layer_size_dict']}")
    configs.append(new_config)

    return configs


def Test_5G_low_utilization():
    # Test 5G low utilization
    flows_per_s_test_cases_low_utilization = [2, 6, 10, 25, 50, 100]    
    flows_per_s_test_cases_low_utilization_5G =[int(x)/int(2) for x in flows_per_s_test_cases_low_utilization] 
    configs = []
    link_bw_Mbits = 5000.0
    simulation_ns =12000000000
    horovod_prio ="0x10"
    run_horovod="true"
    num_workers = 3
    # test_flows = [2]
    for run_horovod in ["true", "false"]:            
        if run_horovod == "true":
            for horovod_prio in ["0x10", "0x08"]:
                for flows in flows_per_s_test_cases_low_utilization_5G:
                # for flows in test_flows:
                    new_config = pfabric_horovod_config(link_bw_Mbits, simulation_ns, flows, horovod_prio, run_horovod, num_workers)
                    # print(new_config.__dict__)
                    configs.append(new_config)
        else:
            for flows in flows_per_s_test_cases_low_utilization_5G:
                new_config = pfabric_horovod_config(link_bw_Mbits, simulation_ns, flows, horovod_prio, run_horovod, num_workers)
                configs.append(new_config)

    return configs

def Test_5G_Bg_only():
    flows_per_s_test_cases_low_utilization = [2, 6, 10, 25, 50, 100]    
    flows_per_s_test_cases_low_utilization_5G =[int(x)/int(2) for x in flows_per_s_test_cases_low_utilization] 
    configs = []
    link_bw_Mbits = 5000.0
    simulation_ns =12000000000
    horovod_prio ="0x08"
    run_horovod="false"
    num_workers = 3

    for flows in flows_per_s_test_cases_low_utilization_5G:
        new_config = pfabric_horovod_config(link_bw_Mbits, simulation_ns, flows, horovod_prio, run_horovod, num_workers)
        configs.append(new_config)

    return configs

def Test_10G_8_workers():
    # Test 5G low utilization
    flows_per_s_test_cases = [10, 50, 100, 200, 300, 400] 
    # flows_per_s_test_cases = [10]   
    configs = []
    link_bw_Mbits = 10000.0
    simulation_ns =12000000000
    horovod_prio ="0x10"
    run_horovod="true"
    num_workers = 8

    run_horovod_test_cases=["true", "false"]
    horovod_prio_cases = ["0x10", "0x08"]
    # test_flows = [2]
    # for run_horovod in ["true"]:            
    for run_horovod in run_horovod_test_cases:            
        if run_horovod == "true":
            for horovod_prio in horovod_prio_cases:
                for flows in flows_per_s_test_cases:
                # for flows in test_flows:
                    new_config  = pfabric_horovod_config(link_bw_Mbits, simulation_ns, flows, horovod_prio, run_horovod, num_workers)
                    # print(new_config.__dict__)
                    configs.append(new_config)
        else:
            for flows in flows_per_s_test_cases:
                new_config = pfabric_horovod_config(link_bw_Mbits, simulation_ns, flows, horovod_prio, run_horovod, num_workers)
                configs.append(new_config)

    return configs

def test_topology_gen_func():
    num_nodes = [5,8]
    for n in num_nodes:
        topology_file = config_dir/f"test_topology_file_{n}"
        generate_ring_topology_file(n, topology_file)

    
if __name__ == "__main__":
    # interval_ns_test_cases = [("1s", 1000000000), ("100ms", 100000000), ("10ms", 10000000)]
    
    # flows_per_s_test_cases = [10, 50, 100, 200, 300, 400, 500, 600, 700]
    # flows_per_s_test_cases = [300]
    flows_per_s_test_cases = [100, 200, 300, 400, 500, 600, 700]
    
    flows_per_s_test_cases_5G = [int(x)/int(2) for x in flows_per_s_test_cases]
    # flows_per_s_test_cases_low_utilization = [10, 25, 50, 100, 150, 200, 250, 300, 350]
    flows_per_s_test_cases_low_utilization = [2, 6, 10, 25, 50, 100]    
    flows_per_s_test_cases_low_utilization_5G =[int(x)/int(2) for x in flows_per_s_test_cases_low_utilization] 
    # launch_multiprocess_run(flows_per_s_test_cases)
    
    configs_to_test = Test_5G_Bg_only()

    configs_to_test = Test_10G_8_workers()

    # test_pfabric_horovod_config_class()
    configs_to_test = test_reading_layer_size()

    # launch_multiprocess_run(configs_to_test)
    individual_pfabric_run((0, configs_to_test[0]))