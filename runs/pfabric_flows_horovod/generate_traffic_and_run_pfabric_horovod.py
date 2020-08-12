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
from dataclasses import dataclass, field, fields
from typing import DefaultDict
from ring_topology_helper import *
from pfabric_flows import *


config_dir = pathlib.Path.cwd()
# TODO figure out why Path(__file__).parent wont work for following parent calls
# config_dir = pathlib.Path(__file__).parent
ns3_basic_sim_dir = config_dir.parent.parent
utilization_plot_dir = ns3_basic_sim_dir/"build"/"plot_helpers"/"utilization_plot"
sys.path.append(f"{utilization_plot_dir.absolute()}")
print(f"sys path: {sys.path}")
try:
    from plot_pfabric_FCT import *                     
except ImportError:
    print("Can't import plot_pfabric_FCT")

try:
    from horovod_worker_plot_class import *                   
except ImportError:
    print("Can't import horovod_worker_plot")


#extract duration_ns and num_servers from config file
config_file = f"{config_dir}/config_ns3.properties"
topology_file = f"{config_dir}/topology_single.properties"
hrvd_config_file=f"{config_dir}/horovod.properties"
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


def write_flow_schedule(schedule_file_path_abs, from_to_tuples, flow_size, start_time_lists):
    with open(schedule_file_path_abs, "w+") as f_out:
        for i in range(len(start_time_lists)):
            f_out.write(f"{i},{from_to_tuples[i][0]},{from_to_tuples[i][1]},{flow_size[i]},{start_time_lists[i]},,\n")
    # return the total number of flows
    return  


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
    with open(csv_file, "w+") as output_file:
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


    
def individual_pfabric_run(args):
    print(args)
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
                  f"num_hvd_{config.c_hrvd_specifics.num_workers}_"+\
                  f"link_bw_{config.c_link_bw_Mbits/1000}Gbit_"+\
                  f"run_idx_{config.c_run_idx}_"+\
                  f"ctn_{config.hrvd_expected_compute_to_network_ratio}_"+\
                  f"pftohrvd_{config.expected_pfabric_to_hrvd_load_ratio}_"+\
                  f"{curr_date}"

    new_run_dir_path_abs = f"{ns3_basic_sim_dir}/runs/{new_run_dir}" 
    sh.mkdir(f"{new_run_dir_path_abs}")
    # cp config run_dir
    new_config_file = f"{new_run_dir_path_abs}/config_ns3.properties"
    sh.cp(f"{config_file}", f"{new_config_file}")

    # copy hrvd config file to new directory
    new_hrvd_config_file = f"{new_run_dir_path_abs}/horovod.properties"
    sh.cp(f"{hrvd_config_file}", f"{new_hrvd_config_file}")
    
    # cp topology run_dir
    new_topology_file = f"{new_run_dir_path_abs}/topology_single.properties"
    sh.cp(topology_file, f"{new_topology_file}")
    
    # total nodes in the topolgy is num_workers + one additional tor
    generate_leaf_tor_topology(config.c_hrvd_specifics.num_workers+1, new_topology_file)

    # local_shell.sed_replace_in_file_plain("./config_ns3.properties", "[ARRIVAL_RATE]", str(expected_flows_per_s))
    # local_shell.sed_replace_in_file_plain("./config_ns3.properties", "[UTILIZATION_INTERVAL]", str(utilization_interval_ns[1]))
    sh.sed("-i", f"s/\\[ARRIVAL\\-RATE\\]/{config.c_flow_arr_rate}/g", f"{new_config_file}")
    sh.sed("-i", f"s/\\[UTILIZATION\\-INTERVAL\\]/{utilization_interval_ns[1]}/g", f"{new_config_file}")
    sh.sed("-i", f"s/\\[SIMULATION\\-TIME\\-NS\\]/{config.c_simulation_ns}/g", f"{new_config_file}")
    sh.sed("-i", f"s/\\[LINK\\-DATARATE\\-MBITS\\]/{config.c_link_bw_Mbits}/g", f"{new_config_file}")
    sh.sed("-i", f"s/\\[PRIORITY\\-HEX\\]/{config.c_horovod_prio}/g", f"{new_config_file}")
    sh.sed("-i", f"s/\\[RUN\\-HOROVOD\\]/{config.c_run_horovod}/g", f"{new_config_file}")
    
    # prepare horovod.properties
    sh.sed("-i", f"s/\\[NUM\\-WORKER\\]/{config.c_hrvd_specifics.num_workers}/g", f"{new_hrvd_config_file}")
    sh.sed("-i", f"s/\\[NUM\\-LAYERS\\]/{config.c_hrvd_specifics.num_layers}/g", f"{new_hrvd_config_file}")
    sh.sed("-i", f"s/\\[FUSION\\-SIZE\\]/{config.c_hrvd_specifics.fusion_buffer_size}/g", f"{new_hrvd_config_file}")
    sh.sed("-i", f"s/\\[MODEL\\-SIZE\\]/{config.c_hrvd_specifics.model_size_in_byte/(10**6)}/g", f"{new_hrvd_config_file}")
    sh.sed("-i", f"s/\\[ITER\\-TIME\\]/{config.c_hrvd_specifics.iteration_time_ms}/g", f"{new_hrvd_config_file}")

    layer_size_file=f"{new_run_dir_path_abs}/layer_weight_model_size_{config.c_hrvd_specifics.model_size_in_byte/(10**6)}MB.csv"
    fp_compute_time_file = f"{new_run_dir_path_abs}/fp_compute_iter_time_{config.c_hrvd_specifics.iteration_time_ms}_ms.csv"
    bp_compute_time_file = f"{new_run_dir_path_abs}/bp_compute_iter_time_{config.c_hrvd_specifics.iteration_time_ms}_ms.csv"
    # write to layer_size.csv
    write_to_csv(config.c_hrvd_specifics.layer_size, layer_size_file)
    write_to_csv(config.c_hrvd_specifics.fp_compute_time, fp_compute_time_file)
    write_to_csv(config.c_hrvd_specifics.bp_compute_time, bp_compute_time_file)
    
    
    schedule_file_name = f"schedule_{config.c_flow_arr_rate}.csv"

    # Write pfabric flow schedule
    servers = [x for x in range(config.c_hrvd_specifics.num_workers)]
    generate_pfabric_flows(f"{new_run_dir_path_abs}/{schedule_file_name}", \
                            servers, \
                            config.c_flow_arr_rate,\
                            config.c_simulation_ns, \
                            config.c_link_bw_Mbits, \
                            config.c_master_seed)


    # run the program
    run_pfabric_horovod_simulation(waf_bin_path, main_program, new_run_dir_path_abs)
    
    # plot link utilization
    sh.cd(f"{utilization_plot_dir}")
    plot_link_utilization_single_tor(f"{new_run_dir_path_abs}/logs_ns3", config.c_hrvd_specifics.num_workers + 1)

    # TODO plot horovod progress and priority samples  
    for i in range(config.c_hrvd_specifics.num_workers):
        progress_file_abs_path = f"{new_run_dir_path_abs}/logs_ns3/HorovodWorker_{i}_layer_50_port_1024_progress.txt"
        save_fig = True
        HorovodWorkerProgress(progress_file_abs_path, save_fig)    
    sh.cd(f"{config_dir}")


    

def expand_number_of_runs(num_runs, p_hrvd_configs):
    p_hrvd_configs_multi_runs = []
    for p_hrvd_config in p_hrvd_configs:
        master_seed =  123456789876
        for i in range(num_runs):
            new_seed = master_seed + i
            new_config = p_hrvd_config.copy()
            new_config.set_master_seed(new_seed)
            new_config.set_run_idx(i)
            print(f"p_hrvd_config: {new_config}")
            p_hrvd_configs_multi_runs.append(new_config)
    
    print(f"total number of tests: {len(p_hrvd_configs_multi_runs)}")

    return p_hrvd_configs_multi_runs

def launch_multiprocess_run(p_hrvd_configs):
    num_runs = 3
    p_hrvd_configs_multi_runs = expand_number_of_runs(num_runs, p_hrvd_configs) 
    with concurrent.futures.ProcessPoolExecutor() as executor:
        # executor.map(individual_pfabric_run, enumerate(p_hrvd_configs))
        executor.map(individual_pfabric_run, enumerate(p_hrvd_configs_multi_runs))


@dataclass
class HorovodConfig:
    num_workers: int
    num_layers: int 
    model_size_in_byte: int 
    fusion_buffer_size: int
    iteration_time_ms: int 
    layer_size: DefaultDict = field(default_factory=collections.defaultdict,init= False, repr= False)
    fp_compute_time: DefaultDict = field(default_factory=collections.defaultdict, init=False, repr= False)
    bp_compute_time: DefaultDict = field(default_factory=collections.defaultdict, init=False, repr= False)
    expected_load_byte_per_iteration: int = field(init=False)
    expected_load_MB_per_s: float = field(init=False)
    expected_network_transfer_time_ms: float = field(init=False)

    def __post_init__(self):
        if self.num_layers == 0: #
            return 
        self.calculate_FP_BP_compute_time()
        self.calculate_layer_size()
        self.fusion_buffer_size = max(self.layer_size.values()) + 1
        self.expected_load_byte_per_iteration = self.model_size_in_byte * 2 * (1 - 1/self.num_workers)
        self.expected_load_MB_per_s = self.expected_load_byte_per_iteration / (self.iteration_time_ms * (10 ** 3)) 
        self.expected_network_transfer_time_ms = 0

    def compute_to_network_time_ratio(self, link_bw_Mbits):
        self.expected_network_transfer_time_ms = self.expected_load_byte_per_iteration * 8 / (link_bw_Mbits * (10 ** 3)) 
        return self.iteration_time_ms/ self.expected_network_transfer_time_ms
        

    def calculate_FP_BP_compute_time(self):
        fp_total_time_ms = 1.0/3.0 * self.iteration_time_ms
        bp_total_time_ms = 2.0/3.0 * self.iteration_time_ms

        fp_diff_per_layer_ms = 2.0 * fp_total_time_ms / (self.num_layers * (self.num_layers -1 ))

        fp_first_layer_ms = 2.0 * fp_total_time_ms / self.num_layers

        bp_diff_per_layer_ms = 2.0 * bp_total_time_ms / (self.num_layers * (self.num_layers -1 ))

        for i in range(self.num_layers):
            self.fp_compute_time[i] = fp_first_layer_ms - i * fp_diff_per_layer_ms
            self.bp_compute_time[i] = i * bp_diff_per_layer_ms

            if i == self.num_layers-1:
                self.fp_compute_time[self.num_layers-1] = fp_diff_per_layer_ms
            if i == 0:
                self.bp_compute_time[0] = bp_diff_per_layer_ms

            # overwrite last layer's compute time in case its close to zero
            assert self.fp_compute_time[i] != 0.0, f"fp compute layer [{i}] is zero"
            
            assert self.bp_compute_time[i] != 0.0, f"bp compute layer [{i}] is zero"

    def calculate_layer_size(self):
        min_layer_size_bytes = int(2 * self.model_size_in_byte / (9 * self.num_layers))
        print(f"min_layer_size_bytes: {min_layer_size_bytes}")
        
        for i in range(self.num_layers):
            if i < self.num_layers/2:
                self.layer_size[i] = min_layer_size_bytes
            elif self.num_layers/2 <= i <= 0.75 * self.num_layers:
                self.layer_size[i] = 4 * min_layer_size_bytes
            else:
                self.layer_size[i] = 12 * min_layer_size_bytes
            
            assert self.layer_size[i] != 0.0, f"layer {i} has 0 weights"
        
    def copy(self):
        new_config = make_horovod_config()
        for field in fields(HorovodConfig):
        # print(f"{field.name}")
            setattr(new_config, field.name, getattr(self, field.name))
        return new_config


def make_horovod_config():
    return HorovodConfig(0, 0, 0, 0, 0)


@dataclass
class PfabricHorovodConfig:
    c_link_bw_Mbits : float
    c_simulation_ns : int
    c_flow_arr_rate: float
    c_horovod_prio: str
    c_run_horovod: str
    c_master_seed: int
    c_run_idx : int
    c_hrvd_specifics: HorovodConfig = field(default_factory=make_horovod_config)
    expected_pfabric_load_MB_per_s: float = field(init=False)
    expected_pfabric_to_hrvd_load_ratio: float = field(init=False)
    hrvd_expected_compute_to_network_ratio: float= field(init=False)
    expected_link_util_pfabric: float = field(init=False)
    expected_link_util_hrvd: float = field(init=False)
    expected_total_link_util: float = field(init=False)

    def __post_init__(self):
        # avg flow size = 1.7 MB * flows_per_s
        self.expected_pfabric_load_MB_per_s = self.c_flow_arr_rate * 1.7
        if self.c_run_horovod == "true":
            self.expected_pfabric_to_hrvd_load_ratio = self.expected_pfabric_load_MB_per_s/self.c_hrvd_specifics.expected_load_MB_per_s
            self.hrvd_expected_compute_to_network_ratio = self.c_hrvd_specifics.compute_to_network_time_ratio(self.c_link_bw_Mbits)
            self.calculate_expected_avg_link_usage_by_pfabric()
            self.calculate_expected_avg_link_usage_by_hrvd()    
            self.expected_total_link_util = self.expected_link_util_pfabric + self.expected_link_util_hrvd
        else:
            self.expected_pfabric_to_hrvd_load_ratio = 0
            self.hrvd_expected_compute_to_network_ratio = 0


    def calculate_expected_avg_link_usage_by_pfabric(self):
        self.expected_link_util_pfabric =  self.c_flow_arr_rate * 1.7 * 8 / self.c_link_bw_Mbits

    def calculate_expected_avg_link_usage_by_hrvd(self):
        self.expected_link_util_hrvd= self.c_hrvd_specifics.expected_load_MB_per_s * 8 / self.c_link_bw_Mbits
    
    def set_master_seed(self, new_seed):
        self.c_master_seed = new_seed

    def set_run_idx(self, new_idx):
        self.c_run_idx = new_idx

    def copy(self):
        new_config = PfabricHorovodConfig(0, 0,0, 0, 0, 0, 0, 0)
        for field in fields(PfabricHorovodConfig):
        # print(f"{field.name}")
            setattr(new_config, field.name, getattr(self, field.name))
        
        new_config.c_hrvd_specifics = new_config.c_hrvd_specifics.copy()
        return new_config


def calculate_flow_rate_based_on_link_utilization(link_bw_Mbits, link_utilization):
    return link_utilization * link_bw_Mbits / (1.7 * 8)


def calculate_iteration_time_for_hrvd(link_bw_Mbits, num_workers, model_size_in_byte, c_to_n_ratio):
    return c_to_n_ratio * model_size_in_byte * 2 * (1 - 1/num_workers) * 8 / (link_bw_Mbits * (10 ** 3))


def test_calculate_iteration_time_for_hrvd():
    num_workers= 8
    num_layers= 50
    model_size_in_byte = 100 * (10**6)
    link_bw_Mbits = 10* (10 ** 3)
    for r in [6.4, 6, 4, 2, 1]:
        iteration_time_ms = calculate_iteration_time_for_hrvd(link_bw_Mbits, num_workers, model_size_in_byte, r)


def generate_low_utilization_constant_hrvd_compute_to_nw_ratio():
    hrvd_compute_to_network_ratios = [16, 8, 4, 2]
    expected_hrvd_link_utilization = [1/x for x in hrvd_compute_to_network_ratios]
    expected_pfabric_link_utilization = [1/16 * x for x in [0.5, 1, 2, 4]]

    
    num_workers= 8
    num_layers= 50
    model_size_in_byte = 100 * (10**6)
    fusion_buffer_size = 2 * (10**6)
    link_bw_Mbits = 10000.0
    simulation_ns =12000000000
    horovod_prio ="0x10"
    run_horovod="true"
    master_seed = 123456789876
    run_idx = 0

    configs= []
    iteration_time_ms_list = [calculate_iteration_time_for_hrvd(link_bw_Mbits,num_workers,model_size_in_byte, r) for r in hrvd_compute_to_network_ratios]
    print(f"iteration_time_ms_list : {iteration_time_ms_list}")
    for i, iteration_time_ms in enumerate(iteration_time_ms_list):
        hrvd_config = HorovodConfig(num_workers,\
                                num_layers, \
                                model_size_in_byte,\
                                fusion_buffer_size, \
                                iteration_time_ms)


        expected_flow_rates = [ round(calculate_flow_rate_based_on_link_utilization(link_bw_Mbits, r)) for r in expected_pfabric_link_utilization]

        for j, flow_rate in enumerate(expected_flow_rates):
            pfabric_hrvd_config = PfabricHorovodConfig(link_bw_Mbits, simulation_ns, flow_rate, horovod_prio,run_horovod,master_seed, run_idx,hrvd_config)
            print(f"flow_rate: {flow_rate}")
            print(f"ratio between pfabric and hrvd: {pfabric_hrvd_config.expected_pfabric_to_hrvd_load_ratio}")
            print(f"ratio between hrvd compute and netwrok : {pfabric_hrvd_config.hrvd_expected_compute_to_network_ratio}, expecting: {hrvd_compute_to_network_ratios[i]}")
            print(f"total link usage: {pfabric_hrvd_config.expected_total_link_util}")            
            print(f"hrvd link utilization: {pfabric_hrvd_config.expected_link_util_hrvd}")
            configs.append(pfabric_hrvd_config)
    
    return configs


def generate_pfabric_horovod_configs_with_app_ratio():
    pfabric_to_hrvd_ratios = [0.5, 1, 2, 4, 8]
    hrvd_compute_to_network_ratios = [4, 2, 1]
    num_workers= 8
    num_layers= 50
    model_size_in_byte = 100 * (10**6)
    fusion_buffer_size = 2 * (10**6)
    link_bw_Mbits = 10000.0
    simulation_ns =12000000000
    # simulation_ns =2000000000
    horovod_prio ="0x10"
    run_horovod="true"
    master_seed = 123456789876
    run_idx = 0

    configs= []
    iteration_time_ms_list = [calculate_iteration_time_for_hrvd(link_bw_Mbits,num_workers,model_size_in_byte, r) for r in hrvd_compute_to_network_ratios]
    print(f"iteration_time_ms_list: {iteration_time_ms_list}")
    for i, iteration_time_ms in enumerate(iteration_time_ms_list):
        hrvd_config = HorovodConfig(num_workers,\
                                num_layers, \
                                model_size_in_byte,\
                                fusion_buffer_size, \
                                iteration_time_ms)

        expected_flows_load_MB_s = [r * hrvd_config.expected_load_MB_per_s  for r in pfabric_to_hrvd_ratios]

        expected_flow_rates = [ round(x/ 1.7) for x in expected_flows_load_MB_s]

        for j, flow_rate in enumerate(expected_flow_rates):
            pfabric_hrvd_config = PfabricHorovodConfig(link_bw_Mbits, simulation_ns, flow_rate, horovod_prio,run_horovod,master_seed, run_idx,hrvd_config)
            print(f"flow_rate: {flow_rate}")
            print(f"ratio between pfabric and hrvd: {pfabric_hrvd_config.expected_pfabric_to_hrvd_load_ratio}, expecting: {pfabric_to_hrvd_ratios[j]}")
            print(f"ratio between hrvd compute and netwrok : {pfabric_hrvd_config.hrvd_expected_compute_to_network_ratio}, expecting: {hrvd_compute_to_network_ratios[i]}")
            print(f"total link usage: {pfabric_hrvd_config.expected_total_link_util}")            

            configs.append(pfabric_hrvd_config)
    
    print(configs[0])
    return configs


def test_pfabric_horovod_config_class():
    num_workers = 8
    num_layers = 50
    model_size = 100 * (10 ** 6)
    iteration_time_ms = 900
    fusion_buffer_size = 4 * (10**6)

    # layer_size_dict = generate_horovod_layer_size(model_size, num_layers)
    # fp_compute_time, bp_compute_time = generate_horovod_FP_BP_compute_time(iteration_time_ms, num_layers)

    hrvd_config = HorovodConfig(num_workers,\
                                num_layers, \
                                model_size,\
                                fusion_buffer_size, \
                                iteration_time_ms)
                                # layer_size_dict, \
                                # fp_compute_time, \
                                # bp_compute_time)

    print(hrvd_config)
    link_bw_Mbits = 10000.0
    simulation_ns =12000000000
    flow_rate=10
    horovod_prio ="0x10"
    run_horovod="true"

    all_config = PfabricHorovodConfig(link_bw_Mbits, simulation_ns, flow_rate, horovod_prio, run_horovod, hrvd_config)
    print(all_config)
    print(hrvd_config)

    return [all_config]


def test_reading_layer_size():
    flows = 10
    configs = []
    link_bw_Mbits = 10000.0
    simulation_ns =12000000000
    horovod_prio ="0x10"
    run_horovod="true"
    num_workers = 3

    new_config = PfabricHorovodConfig(link_bw_Mbits, simulation_ns, flows, horovod_prio, run_horovod, num_workers)
    
    model_size = 100 * (10**6)
    num_layers = 50

    layer_size_dict = generate_horovod_layer_size(model_size, num_layers)

    optional_config_field = {"layer_size_dict": layer_size_dict}
    # new_config.add_hrvd_configs(**optional_config_field)
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
                    new_config = PfabricHorovodConfig(link_bw_Mbits, simulation_ns, flows, horovod_prio, run_horovod, num_workers)
                    # print(new_config.__dict__)
                    configs.append(new_config)
        else:
            for flows in flows_per_s_test_cases_low_utilization_5G:
                new_config = PfabricHorovodConfig(link_bw_Mbits, simulation_ns, flows, horovod_prio, run_horovod, num_workers)
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
        new_config = PfabricHorovodConfig(link_bw_Mbits, simulation_ns, flows, horovod_prio, run_horovod, num_workers)
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
                    new_config  = PfabricHorovodConfig(link_bw_Mbits, simulation_ns, flows, horovod_prio, run_horovod, num_workers)
                    # print(new_config.__dict__)
                    configs.append(new_config)
        else:
            for flows in flows_per_s_test_cases:
                new_config = PfabricHorovodConfig(link_bw_Mbits, simulation_ns, flows, horovod_prio, run_horovod, num_workers)
                configs.append(new_config)

    return configs

    
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
    
    # configs_to_test = test_pfabric_horovod_config_class()
    # configs_to_test = generate_pfabric_horovod_configs_with_app_ratio()
    configs_to_test = generate_low_utilization_constant_hrvd_compute_to_nw_ratio()
    # individual_pfabric_run((0, configs_to_test[0]))
    config = configs_to_test[0]
    launch_multiprocess_run(configs_to_test)

    