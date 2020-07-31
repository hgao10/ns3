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
with open(config_file, "r") as f:
    d = f.readlines()
    f.seek(0)
    for i in d:
        if i.find("simulation_end_time_ns") != -1:
            duration_ns = int(i.split("=")[1].strip("\n"))

print(f"duration_ns: {duration_ns}")
num_servers = 3
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

def run_pfabric_horovod_simulation(waf_bin_path,main_program, run_dir_abs):
    waf = sh.Command(f"{waf_bin_path}/waf")
    sh.cd(f"{waf_bin_path}")
    waf(run=f"{main_program} --run_dir='{run_dir_abs}'", _out=f"{run_dir_abs}/simulation.log")


def individual_pfabric_run(args):
    utilization_interval_ns = ("100ms", 100000000)
    index, expected_flows_per_s = args
    time.sleep(index * 30)
    waf_bin_path = "/mnt/raid10/hanjing/thesis/ns3/ns3-basic-sim/simulator"
    main_program = "main_pfabric_flows_horovod"
    program_name = "pfabric_flows_horovod"
    new_run_dir = f"{program_name}_{utilization_interval_ns[0]}_arrival_{expected_flows_per_s}_{curr_date}"
    new_run_dir_path_abs = f"/mnt/raid10/hanjing/thesis/ns3/ns3-basic-sim/runs/{new_run_dir}" 
    sh.mkdir(f"{new_run_dir_path_abs}")
    # cp config run_dir
    new_config_file = f"{new_run_dir_path_abs}/config_ns3.properties"
    sh.cp(f"{config_file}", f"{new_config_file}")

    # cp topology run_dir
    new_topology_file = f"{new_run_dir_path_abs}/topology_single.properties"
    sh.cp(topology_file, f"{new_topology_file}")

    # local_shell.sed_replace_in_file_plain("./config_ns3.properties", "[ARRIVAL_RATE]", str(expected_flows_per_s))
    # local_shell.sed_replace_in_file_plain("./config_ns3.properties", "[UTILIZATION_INTERVAL]", str(utilization_interval_ns[1]))
    sh.sed("-i", f"s/ARRIVALRATE/{expected_flows_per_s}/g", f"{new_config_file}")
    sh.sed("-i", f"s/UTILIZATIONINTERVAL/{utilization_interval_ns[1]}/g", f"{new_config_file}")
    schedule_file_name = f"schedule_{expected_flows_per_s}.csv"
    random.seed(123456789)
    seed_start_times = random.randint(0, 100000000)
    seed_flow_size = random.randint(0, 100000000)

    # Start times (ns)
    list_start_time_ns = draw_poisson_inter_arrival_gap_start_times_ns(duration_ns, expected_flows_per_s, seed_start_times)
    num_starts = len(list_start_time_ns)

    # (From, to) tuples
    # list_from_to = draw_n_times_from_to_all_to_all(num_starts, topology.servers, seed_from_to)

    from_to_tuples = []
    for i in range(num_servers):
        if i != num_servers-1:
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
    print("Arrival rate: " + str(expected_flows_per_s) + " flows/s")
    # link_cap_Gbits = 10.0
    link_cap_Gbits = 5.0
    # print("Expected utilization: " + str(expected_flows_per_s * cdf_mean_byte / 1.25e+9))
    print("Expected utilization: " + str(expected_flows_per_s * cdf_mean_byte / ((link_cap_Gbits/8.0)*(10**9))))

    # Write schedule
    write_schedule_ring_topology(f"{new_run_dir_path_abs}/{schedule_file_name}", expand_from_to_tuples, list_flow_size_byte, list_start_time_ns,num_starts)

    # run the program
    # pfabric_horovod_run.run_program(main_program, new_run_dir)
    run_pfabric_horovod_simulation(waf_bin_path, main_program, new_run_dir_path_abs)
    # plot utilization
    sh.cd(f"{utilization_plot_dir}")
    plot_pfabric_FCT.plot_pfabric_utilization(f"{new_run_dir_path_abs}/logs_ns3")

    sh.cd(f"{config_dir}")

# expected_flows_per_s = [10, 50, 100, 200, 300, 400, 500, 600, 700]
expected_flows_per_s = [500]
def launch_multiprocess_run(flows_per_s_test_cases):
    # for utilization_interval_ns in  interval_ns_test_cases: #interval_ns_test_cases:
    #     # individual_pfabric_run(500)

    with concurrent.futures.ProcessPoolExecutor() as executor:
        executor.map(individual_pfabric_run, enumerate(flows_per_s_test_cases))


if __name__ == "__main__":
    # interval_ns_test_cases = [("1s", 1000000000), ("100ms", 100000000), ("10ms", 10000000)]
    
    # flows_per_s_test_cases = [10, 50, 100, 200, 300, 400, 500, 600, 700]
    # flows_per_s_test_cases = [300]
    flows_per_s_test_cases = [100, 200, 300, 400, 500, 600, 700]
    
    flows_per_s_test_cases_5G = [int(x)/int(2) for x in flows_per_s_test_cases]

    # launch_multiprocess_run(flows_per_s_test_cases)
    launch_multiprocess_run(flows_per_s_test_cases_5G)