from networkload import *
import random

def generate_pfabric_flows(schedule_file, servers, flow_rate_per_link, simulation_ns):    
    total_flow_rates = servers * flow_rate_per_link
    random.seed(123456789)
    seed_start_times = random.randint(0, 100000000)
    seed_flow_size = random.randint(0, 100000000)
    seed_from_to = random.randint(0, 100000000)

    # Start times (ns)
    list_start_time_ns = draw_poisson_inter_arrival_gap_start_times_ns(simulation_ns, total_flow_rates, seed_start_times)
    num_starts = len(list_start_time_ns)

    # Flow sizes in byte
    list_flow_size_byte = list(
        round(x) for x in draw_n_times_from_cdf(num_starts, CDF_PFABRIC_WEB_SEARCH_BYTE, True, seed_flow_size)
    )

    list_from_to = draw_n_times_from_to_all_to_all(num_starts, servers, seed_from_to)

    write_schedule(schedule_file, num_starts, list_from_to, list_flow_size_byte, list_start_time_ns)    


# currently unused 
def generate_flow_to_tuples_ring_topology(num_nodes):

    from_to_tuples = []

    for i in range(num_nodes):
        if i != num_nodes-1:
            from_to_tuples.append((i, i+1))
        else:
            from_to_tuples.append((i, 0))

    return from_to_tuples


def generate_leaf_tor_topology(num_nodes, topology_file):
    if num_nodes < 4:
        raise RuntimeError("Need at least 4 nodes for 3 horovod workers to participate in a logical ring topology")
    with open(topology_file, "w") as output_file:
        output_file.write(f"num_nodes={num_nodes}\n")
        output_file.write(f"num_undirected_edges={num_nodes-1}\n")
        set_switches = num_nodes-1
        output_file.write(f"switches=set({set_switches})\n")
        output_file.write(f"switches_which_are_tors=set({set_switches})\n")
        servers =[str(x) for x in range(num_nodes-1)]
        set_servers=",".join(servers)
        output_file.write(f"servers=set({set_servers})\n")
        set_undirected_edges=[]
        for i in range(num_nodes-1):
            set_undirected_edges.append(f"{i}-{num_nodes-1}")
        set_undirected_edges = ",".join(set_undirected_edges)
        output_file.write(f"undirected_edges=set({set_undirected_edges})\n")


# currently unused, no need for physical ring topology 
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
