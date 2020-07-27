from networkload import *
import collections
import pathlib
import exputil
import pfabric_horovod_run
import sh
from datetime import datetime
import importlib.util
import sys

config_dir = pathlib.Path.cwd()
ns3_basic_sim_dir = config_dir.parent.parent

utilization_plot_dir = ns3_basic_sim_dir/"build"/"plot_helpers"/"utilization_plot"
sys.path.append(f"{utilization_plot_dir}")
print(f"sys path: {sys.path}")
import plot_pfabric

# plot_pfabric.plot_pfabric_utilization("ns3")
# spec = importlib.util.spec_from_file_location("plot_pfabric", f"{plot_pfabric}")
# generate_plot = importlib.util.module_from_spec(spec)
# spec.loader.exec_module(generate_plot)
# generate_plot.plot_pfabric_utilization("false")


#extract duration_ns and num_servers from config file
config_file = f"{config_dir}/config_ns3.properties"
print(f"config file: {config_file}")
with open(config_file, "r") as f:
    d = f.readlines()
    f.seek(0)
    for i in d:
        if i.find("simulation_end_time_ns") != -1:
            duration_ns = int(i.split("=")[1].strip("\n"))

# duration_ns = 10 * 1000 * 1000 * 1000  # 10 seconds
print(f"duration_ns: {duration_ns}")
num_servers = 3
local_shell = exputil.LocalShell()
main_program = "pfabric_flows_horovod"

curr_date = datetime.now().strftime('%H_%M_%Y_%m_%d')
interval_ns_test_cases = [("1s", 1000000000), ("100ms", 100000000), ("10ms", 10000000)]
flows_per_s_test_cases = [10, 50, 100, 200, 300, 400, 500, 600, 700]


for utilization_interval_ns in  [("100ms", 100000000)]: #interval_ns_test_cases:
    for expected_flows_per_s in [400, 500, 600, 700]: #, 400, 500, 600, 700]:# flows_per_s_test_cases:
        new_run_dir = f"{main_program}_{utilization_interval_ns[0]}_arrival_{expected_flows_per_s}_{curr_date}"
        new_run_dir_path_abs = f"/mnt/raid10/hanjing/thesis/ns3/ns3-basic-sim/runs/{new_run_dir}" 
        sh.mkdir(f"{new_run_dir_path_abs}")
        # cp config run_dir
        new_config_file = f"{new_run_dir_path_abs}/config_ns3.properties"
        sh.cp(f"{config_file}", f"{new_config_file}")

        # cp topology run_dir
        new_topology_file = f"{new_run_dir_path_abs}/topology_single.properties"
        sh.cp("topology_single.properties", f"{new_topology_file}")

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
        print("Expected utilization: " + str(expected_flows_per_s * cdf_mean_byte / 1.25e+9))

        # key: server_source_id, value: flow_ids
        flows_ids_dict = collections.defaultdict(list) 
        # Write schedule
        def write_schedule_ring_topology(from_to_tuple_dict, flow_size, start_time_lists, start_time_counts):
            j = 0
            with open(f"{new_run_dir_path_abs}/{schedule_file_name}", "w+") as f_out:
                for i in range(start_time_counts):
                    for list_from_to in from_to_tuple_dict.values():
                        print(f"from_to_tuples: {from_to_tuples}")
                        f_out.write(f"{j},{list_from_to[i][0]},{list_from_to[i][1]},{flow_size[i]},{start_time_lists[i]},,\n")
                        j+=1
            # return the total number of flows
            return j 

        write_schedule_ring_topology(expand_from_to_tuples, list_flow_size_byte, list_start_time_ns,num_starts)

        # run the program
        pfabric_horovod_run.run_program(main_program, new_run_dir)

        # plot utilization
        sh.cd(f"{utilization_plot_dir}")
        plot_pfabric.plot_pfabric_utilization(f"{new_run_dir_path_abs}/logs_ns3")

        sh.cd(f"{config_dir}")


        

