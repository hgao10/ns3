from networkload import *
import collections
import pathlib

#extract duration_ns and num_servers from config file
config_dir = pathlib.Path.cwd()
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

expected_flows_per_s = 100
random.seed(123456789)
seed_start_times = random.randint(0, 100000000)
seed_from_to = random.randint(0, 100000000)
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

# key: server_source_id, value: flow_ids
flows_ids_dict = collections.defaultdict(list) 
# Write schedule
def write_schedule_ring_topology(from_to_tuple_dict, flow_size, start_time_lists, start_time_counts):
    j = 0
    with open("schedule.csv", "w+") as f_out:
        for i in range(start_time_counts):
            for list_from_to in from_to_tuple_dict.values():
                print(f"from_to_tuples: {from_to_tuples}")
                f_out.write(f"{j},{list_from_to[i][0]},{list_from_to[i][1]},{flow_size[i]},{start_time_lists[i]},,\n")
                j+=1
    # return the total number of flows
    return j 

write_schedule_ring_topology(expand_from_to_tuples, list_flow_size_byte, list_start_time_ns,num_starts)