from networkload import *
import random

def generate_pfabric_flows(schedule_file, servers, flow_rate_per_link, simulation_ns, link_bw_Mbits, seed):    
    total_flow_rates = len(servers) * flow_rate_per_link
    # random.seed(123456789)
    random.seed(seed)
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

    cdf_mean_byte = get_cdf_mean(CDF_PFABRIC_WEB_SEARCH_BYTE, linear_interpolate=True)
    print("Mean: " + str(cdf_mean_byte) + " bytes")
    print("Arrival rate: " + str(total_flow_rates) + " flows/s")

    # print("Expected utilization: " + str(expected_flows_per_s * cdf_mean_byte / 1.25e+9))
    print("Expected utilization: " + str(flow_rate_per_link * cdf_mean_byte / ((total_flow_rates/8.0)*(10**6))))

    write_schedule(schedule_file, num_starts, list_from_to, list_flow_size_byte, list_start_time_ns)    