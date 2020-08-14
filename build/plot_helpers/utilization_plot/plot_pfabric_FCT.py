import sh
import utilization_plot
from datetime import datetime
from exputil import *
import collections
import matplotlib.pyplot as plt
from datetime import datetime
import pathlib
import numpy as np
from networkload import *
import argparse
from dataclasses import dataclass
from typing import Tuple
import statistics
from horovod_worker_plot_class import *
from ecdf import ecdf
def delta_percent(a, divider):    
    return (a-divider)/divider * 100


# all dataclasses require python 3.7+
@dataclass 
class FlowStats:
    min_FCT_ms: float 
    max_FCT_ms: float
    median_FCT_ms: float
    avg_FCT_ms: float
    percentile_90th_ms: float
    percentile_99th_ms: float
    ecdf_x_y: Tuple
    flows_cnt: int

    def show_ecdf(self, block):
        (x, y) = self.ecdf_x_y
        plt.scatter(x=x, y=y)
        plt.show(block=block)
    
    def compare_avg_FCT(self, other):
        return delta_percent(self.avg_FCT_ms, other.avg_FCT_ms)
    
    def compare_99th_pct(self, other):
        return delta_percent(self.percentile_99th_ms, other.percentile_99th_ms)

    def compare_90th_pct(self, other):
        return delta_percent(self.percentile_90th_ms, other.percentile_90th_ms)




@dataclass
class AllFlowStats:
    small_flows: FlowStats
    large_flows: FlowStats
    mid_flows: FlowStats
    all_flows: FlowStats

    def compare_avg_FCT(self, other):
        delta = collections.defaultdict()
        delta["small"] = self.small_flows.compare_avg_FCT(other.small_flows)
        delta["large"] = self.large_flows.compare_avg_FCT(other.large_flows)
        delta["mid"] = self.mid_flows.compare_avg_FCT(other.mid_flows)
        delta["all"] = self.all_flows.compare_avg_FCT(other.all_flows)
        
        return delta

    def compare_99th_pct(self, other):
        delta = collections.defaultdict()
        delta["small"] = self.small_flows.compare_99th_pct(other.small_flows)
        delta["large"] = self.large_flows.compare_99th_pct(other.large_flows)
        delta["mid"] = self.mid_flows.compare_99th_pct(other.mid_flows)
        delta["all"] = self.all_flows.compare_99th_pct(other.all_flows)

        return delta

    def compare_90th_pct(self, other):
        delta = collections.defaultdict()
        delta["small"] = self.small_flows.compare_90th_pct(other.small_flows)
        delta["large"] = self.large_flows.compare_90th_pct(other.large_flows)
        delta["mid"] = self.mid_flows.compare_90th_pct(other.mid_flows)
        delta["all"] = self.all_flows.compare_90th_pct(other.all_flows)

        return delta
        

def ns_to_ms(ns_val):
    return float(ns_val)/float(10**6)


def generate_flow_stats(durations_ns):
    min_FCT_ms = ns_to_ms(min(durations_ns))
    max_FCT_ms = ns_to_ms(max(durations_ns))
    median_FCT_ms = ns_to_ms(statistics.median(durations_ns))
    avg_FCT_ms = ns_to_ms(sum(durations_ns)/len(durations_ns))
    percentile_99th_ms = ns_to_ms(np.percentile(durations_ns,99))
    percentile_90th_ms = ns_to_ms(np.percentile(durations_ns,90))
    ecdf_x_y = ecdf(durations_ns)
    flows_cnt = len(durations_ns)

    return FlowStats(min_FCT_ms, max_FCT_ms, median_FCT_ms, avg_FCT_ms, percentile_90th_ms, percentile_99th_ms, ecdf_x_y, flows_cnt)


def plot_link_utilization_direct_connect_btw_servers(log_dir, num_nodes):
    # log_dir = "/mnt/raid10/hanjing/thesis/ns3/ns3-basic-sim/runs/pfabric_flows_horovod/logs_ns3"
    data_out_dir = log_dir
    pdf_out_dir = log_dir
    from_to_node_id = []
    for i in range(num_nodes):
        for j in range(num_nodes):
            if i != j:
                from_to_node_id.append((i,j))

    for n1, n2 in from_to_node_id:
        utilization_plot.utilization_plot(log_dir, data_out_dir, pdf_out_dir, n1, n2)


def plot_link_utilization_single_tor(log_dir, num_nodes):
    data_out_dir = log_dir
    pdf_out_dir = log_dir
    from_to_node_id = []
    for i in range(num_nodes-1):
        # link between the server and tor in each direction
        from_to_node_id.append((i,num_nodes-1))
        from_to_node_id.append((num_nodes-1, i))

    for n1, n2 in from_to_node_id:
        utilization_plot.utilization_plot(log_dir, data_out_dir, pdf_out_dir, n1, n2)


def plot_utilization_and_hrvd_progress(new_run_dir_path_abs, number_workers, save_fig):
    plot_link_utilization_single_tor(f"{new_run_dir_path_abs}/logs_ns3", number_workers + 1)

    for i in range(number_workers):
        progress_file_abs_path = f"{new_run_dir_path_abs}/logs_ns3/HorovodWorker_{i}_layer_50_port_1024_progress.txt"
        save_fig = True
        HorovodWorkerProgress(progress_file_abs_path, save_fig)    

@dataclass
class Flow:
    f_id:  int
    source: int
    target: int
    size_KB: int
    start_time: int
    duration: int
    end_time: int
    sent_bytes: int
    finished_status: str

    def __post_init__(self):
        # initialized with unit Byte in __init__
        self.size_KB = float(self.size_KB/1000)
        self.throughput_Gbits = self.size_KB * 8 * (10 ** 3) / self.duration
        

def plot_multi_FCT_runs(run_dirs):
    small_finished_flows_duration = []
    large_finished_flows_duration = []

    midsize_finished_flows_duration = []
    all_finished_flows_duration = []
    flows_count = collections.defaultdict(int)
    total_entries = 0
    for run_dir in run_dirs:
        run_dir_root = "/mnt/raid10/hanjing/thesis/ns3/ns3-basic-sim/runs"
        data_file = f"{run_dir_root}/{run_dir}/logs_ns3/flows.csv"

        data_file_path = pathlib.Path(data_file)
        if data_file_path.exists() == False:
            print(f"data_file: {data_file_path} doesn't exist")
            return None, None, None
        # Read in csv data
        flows_csv_columns = read_csv_direct_in_columns(
            data_file,
            # flows_id, source, target, size,    start_time (ns), end_time(ns), duration(ns), sent(bytes),finished_state, meta_data
            "pos_int,pos_int,pos_int,pos_int,pos_int,pos_int,pos_int,pos_int,string,string"
        )

        num_entries = len(flows_csv_columns[0])
        total_entries += num_entries
        flows_id = flows_csv_columns[0]
        source = flows_csv_columns[1]
        target = flows_csv_columns[2]
        size = flows_csv_columns[3]
        start_time = flows_csv_columns[4]
        end_time = flows_csv_columns[5]
        duration = flows_csv_columns[6]
        sent_bytes = flows_csv_columns[7]
        finished_state = flows_csv_columns[8]

        small_flows_threshold_KB = 100      #100 KB 
        large_flows_threshold_KB = 10000    #10MB 

        # Only consider flows that were completed between warmup and cooldown period
        warmup_period_ns = 2 * (10**9) 
        cooldown_period_ns = 2 * (10**9)
        last_flow_start_timestamp_ns = start_time[-1]
        valid_period_upperbound = last_flow_start_timestamp_ns - cooldown_period_ns


        for i in range(num_entries):
            curr_flow = Flow(flows_id[i], source[i], target[i], size[i], start_time[i], duration[i], end_time[i],sent_bytes[i], finished_state[i]  )

            # print(curr_flow)

            if warmup_period_ns < curr_flow.start_time < valid_period_upperbound:  
                flows_count["all"] +=1
                if curr_flow.size_KB <= small_flows_threshold_KB:
                    if curr_flow.finished_status == "YES":
                        small_finished_flows_duration.append(curr_flow.duration)
                        all_finished_flows_duration.append(curr_flow.duration)
                    flows_count["small"] += 1
                elif curr_flow.size_KB >= large_flows_threshold_KB:
                    if curr_flow.finished_status == "YES":
                        large_finished_flows_duration.append(curr_flow.duration)
                        all_finished_flows_duration.append(curr_flow.duration)
                    flows_count["large"] += 1
                else: # flows between [100KB, 10MB]
                    if curr_flow.finished_status == "YES":
                        midsize_finished_flows_duration.append(curr_flow.duration)
                        all_finished_flows_duration.append(curr_flow.duration)
                    flows_count["mid"] += 1

    print("Collected flows:")
    print(f"  > {flows_count['all']} out of {total_entries} flows were in the measurement interval")
    print("  > Within the interval:")
    print(f"    >> All flows...... {len(all_finished_flows_duration)}/{flows_count['all']} completed")
    print(f"    >> Small flows.... {len(small_finished_flows_duration)}/{flows_count['small']} completed")
    print(f"    >> Medium flows.... {len(midsize_finished_flows_duration)}/{flows_count['mid']} completed")
    print(f"    >> Large flows.... {len(large_finished_flows_duration)}/{flows_count['large']} completed")

    small_flows_stats = generate_flow_stats(small_finished_flows_duration)
    large_flows_stats = generate_flow_stats(large_finished_flows_duration)
    mid_flows_stats = generate_flow_stats(midsize_finished_flows_duration)
    all_flows_stats = generate_flow_stats(all_finished_flows_duration)
    
    run_flow_summary = AllFlowStats(small_flows_stats, large_flows_stats, mid_flows_stats, all_flows_stats)

    print(run_flow_summary)

    return run_flow_summary

def plot_flows_FCT(run_dir):
    run_dir_root = "/mnt/raid10/hanjing/thesis/ns3/ns3-basic-sim/runs"
    data_file = f"{run_dir_root}/{run_dir}/logs_ns3/flows.csv"

    data_file_path = pathlib.Path(data_file)
    if data_file_path.exists() == False:
        print(f"data_file: {data_file_path} doesn't exist")
        return None, None, None
    # Read in csv data
    flows_csv_columns = read_csv_direct_in_columns(
        data_file,
        # flows_id, source, target, size,    start_time (ns), end_time(ns), duration(ns), sent(bytes),finished_state, meta_data
        "pos_int,pos_int,pos_int,pos_int,pos_int,pos_int,pos_int,pos_int,string,string"
    )

    num_entries = len(flows_csv_columns[0])
    flows_id = flows_csv_columns[0]
    source = flows_csv_columns[1]
    target = flows_csv_columns[2]
    size = flows_csv_columns[3]
    start_time = flows_csv_columns[4]
    end_time = flows_csv_columns[5]
    duration = flows_csv_columns[6]
    sent_bytes = flows_csv_columns[7]
    finished_state = flows_csv_columns[8]

    small_flows_threshold_KB = 100      #100 KB 
    large_flows_threshold_KB = 10000    #10MB 

    small_finished_flows_duration = []
    large_finished_flows_duration = []

    midsize_finished_flows_duration = []
    all_finished_flows_duration = []

    # Only consider flows that were completed between warmup and cooldown period
    warmup_period_ns = 2 * (10**9) 
    cooldown_period_ns = 2 * (10**9)
    last_flow_start_timestamp_ns = start_time[-1]
    valid_period_upperbound = last_flow_start_timestamp_ns - cooldown_period_ns

    flows_count = collections.defaultdict(int)

    for i in range(num_entries):
        curr_flow = Flow(flows_id[i], source[i], target[i], size[i], start_time[i], duration[i], end_time[i],sent_bytes[i], finished_state[i]  )

        print(curr_flow)

        if warmup_period_ns < curr_flow.start_time < valid_period_upperbound:  
            flows_count["all"] +=1
            if curr_flow.size_KB <= small_flows_threshold_KB:
                if curr_flow.finished_status == "YES":
                    small_finished_flows_duration.append(curr_flow.duration)
                    all_finished_flows_duration.append(curr_flow.duration)
                flows_count["small"] += 1
            elif curr_flow.size_KB >= large_flows_threshold_KB:
                if curr_flow.finished_status == "YES":
                    large_finished_flows_duration.append(curr_flow.duration)
                    all_finished_flows_duration.append(curr_flow.duration)
                flows_count["large"] += 1
            else: # flows between [100KB, 10MB]
                if curr_flow.finished_status == "YES":
                    midsize_finished_flows_duration.append(curr_flow.duration)
                    all_finished_flows_duration.append(curr_flow.duration)
                flows_count["mid"] += 1

    # assert len(small_finished_flows_duration) == flows_count["small"], "Some small flows are not finished during considered interval"
    # assert len(large_finished_flows_duration) == flows_count["large"], "Some large flows are not finished during considered interval"

    print("Collected flows:")
    print(f"  > {flows_count['all']} out of {num_entries} flows were in the measurement interval")
    print("  > Within the interval:")
    print(f"    >> All flows...... {len(all_finished_flows_duration)}/{flows_count['all']} completed")
    print(f"    >> Small flows.... {len(small_finished_flows_duration)}/{flows_count['small']} completed")
    print(f"    >> Medium flows.... {len(midsize_finished_flows_duration)}/{flows_count['mid']} completed")
    print(f"    >> Medium flows.... {len(large_finished_flows_duration)}/{flows_count['large']} completed")

    small_flows_stats = generate_flow_stats(small_finished_flows_duration)
    large_flows_stats = generate_flow_stats(large_finished_flows_duration)
    mid_flows_stats = generate_flow_stats(midsize_finished_flows_duration)
    all_flows_stats = generate_flow_stats(all_finished_flows_duration)
    
    run_flow_summary = AllFlowStats(small_flows_stats, large_flows_stats, mid_flows_stats, all_flows_stats)

    print(run_flow_summary)

    return run_flow_summary


def extract_average_utilization(run_dir):
    run_dir_root = "/mnt/raid10/hanjing/thesis/ns3/ns3-basic-sim/runs"

    data_file = f"{run_dir_root}/{run_dir}/logs_ns3/utilization_summary.txt"
    data_file_path = pathlib.Path(data_file)
    if not data_file_path.exists:
        return None

    with open(data_file, "r") as file:
        lines = file.readlines()
        link_utilization = 0
        for i in range(1, len(lines)):
            line = lines[i]
            line = line.strip("\n").split()
            link_utilization += float(line[-1].strip("%"))
        # from_0_to_1_link = line[1]
        # from_0_to_1_link = from_0_to_1_link.strip("\n")
        # from_0_to_1_link = from_0_to_1_link.split()
        # link_utilization = float(from_0_to_1_link[-1].strip("%"))
        # # print(f"from_0_to_1_link: {from_0_to_1_link}, utilization: {link_utilization}")
        link_utilization = link_utilization/(len(lines)-1)
        print(f"link_utilization: {link_utilization}")
    return link_utilization


def plot_FCT_summary(input_run_dirs_timestamp, link_cap_Gbits, save_fig):


    # key: flows_per_s, value: utilization in string
    utilization_dict = collections.defaultdict() 
    expected_flows_bw = collections.defaultdict()
    horovod_p0_bw = collections.defaultdict()
    leftover_p0_bw = collections.defaultdict()

    bg_flows_summary = collections.defaultdict()
    both_p0_flows_summary = collections.defaultdict()
    hrvd_p2_flows_summary = collections.defaultdict()
    
    small_flows_delta_percent = []
    large_flows_delta_percent = []
    small_flows_99th_delta_percent = []

    small_flows_bg_delta_percent = []
    large_flows_bg_delta_percent = []
    small_flows_99th_bg_delta_percent = []
    
    horovod_iteration_times_s = []
    baseline_horovod_iteration_times_s = []

    for flows_per_s, bg_alone_time_stamp, hrvd_p0_time_stamp, hrvd_p2_time_stamp in input_run_dirs_timestamp:
        print(f"Flows per second: {flows_per_s}")
        run_dir = f"pfabric_flows_horovod_100ms_arrival_{flows_per_s}_{hrvd_p0_time_stamp}"
        hrvd_p2_run_dir = f"pfabric_flows_horovod_100ms_arrival_{flows_per_s}_{hrvd_p2_time_stamp}"
        bg_run_dir = f"pfabric_flows_horovod_100ms_arrival_{flows_per_s}_{bg_alone_time_stamp}"
        
        utilization_dict[flows_per_s] = extract_average_utilization(run_dir)
        expected_flows_bw[flows_per_s] = get_flows_expected_utilization(link_cap_Gbits, flows_per_s)
        horovod_p0_bw[flows_per_s] = utilization_dict[flows_per_s] - expected_flows_bw[flows_per_s]
        leftover_p0_bw[flows_per_s] = 100.0 - utilization_dict[flows_per_s] 
        
        bg_flows_summary[flows_per_s] = plot_flows_FCT(bg_run_dir)
        both_p0_flows_summary[flows_per_s] = plot_flows_FCT(run_dir)
        hrvd_p2_flows_summary[flows_per_s] = plot_flows_FCT(hrvd_p2_run_dir)

        if hrvd_p2_flows_summary[flows_per_s].small_flows.avg_FCT_ms != None and hrvd_p2_flows_summary[flows_per_s].large_flows.avg_FCT_ms != None:
        
            avg_FCT_delta = hrvd_p2_flows_summary[flows_per_s].compare_avg_FCT(both_p0_flows_summary[flows_per_s])
            pct_99th_deleta = hrvd_p2_flows_summary[flows_per_s].compare_99th_pct(both_p0_flows_summary[flows_per_s])

            small_flows_delta_percent.append(avg_FCT_delta["small"])
            large_flows_delta_percent.append(avg_FCT_delta["large"])
            small_flows_99th_delta_percent.append(pct_99th_deleta["small"])

            avg_FCT_delta = hrvd_p2_flows_summary[flows_per_s].compare_avg_FCT(bg_flows_summary[flows_per_s])
            pct_99th_deleta = hrvd_p2_flows_summary[flows_per_s].compare_99th_pct(bg_flows_summary[flows_per_s])

            small_flows_bg_delta_percent.append(avg_FCT_delta["small"])
            large_flows_bg_delta_percent.append(avg_FCT_delta["large"])
            small_flows_99th_bg_delta_percent.append(pct_99th_deleta["small"])

        else:
            small_flows_delta_percent.append(None)
            large_flows_delta_percent.append(None)
            small_flows_99th_delta_percent.append(None)

            small_flows_bg_delta_percent.append(None)
            large_flows_bg_delta_percent.append(None)
            small_flows_99th_bg_delta_percent.append(None)
        
        # plot horovod iteration times
        run_dir_root = "/mnt/raid10/hanjing/thesis/ns3/ns3-basic-sim/runs"
        comparison_log_dir = f"{run_dir_root}/{hrvd_p2_run_dir}/logs_ns3"
        horovod_progress_txt = f"{run_dir_root}/{hrvd_p2_run_dir}/logs_ns3/HorovodWorker_0_layer_50_port_1024_progress.txt"

        # directory - Horovod P0 and bg P0
        baseline_log_dir = f"{run_dir_root}/{run_dir}/logs_ns3"
        baseline_horovod_progress_txt = f"{baseline_log_dir}/HorovodWorker_0_layer_50_port_1024_progress.txt"

        if pathlib.Path(comparison_log_dir).exists() == False or pathlib.Path(horovod_progress_txt).exists() == False:
            horovod_iteration_times_s.append(None)
        else:
            horovodprogress = HorovodWorkerProgress(horovod_progress_txt, True)
            horovod_iteration_times_s.append(horovodprogress.avg_iter_time_s)

        if pathlib.Path(baseline_log_dir).exists() == False or pathlib.Path(baseline_horovod_progress_txt).exists() == False:
            baseline_horovod_iteration_times_s.append(None)
        else:
            horovodprogress = HorovodWorkerProgress(baseline_horovod_progress_txt, True)
            horovod_iteration_times_s.append(horovodprogress.avg_iter_time_s)

        horovod_iter_delta = []
        for i in range(len(horovod_iteration_times_s)):
            if baseline_horovod_iteration_times_s[i] != None and horovod_iteration_times_s[i] != None:
                horovod_iter_delta.append(f"{(horovod_iteration_times_s[i] - baseline_horovod_iteration_times_s[i])/baseline_horovod_iteration_times_s[i] * 100: .1f}%")
            else:
                horovod_iter_delta.append(None)

    fig, (ax1, ax2, ax3, ax4, ax5, ax6, ax7) = plt.subplots(1,7)

    plot_stacked_bw_distribution(ax7, \
                                [x for x in utilization_dict.values()], \
                                [x for x in expected_flows_bw.values()], \
                                [x for x in horovod_p0_bw.values()], 
                                [x for x in leftover_p0_bw.values()])
    # fig.suptitle("Average FCT vs Link Utilization")
    ax1.plot([x for x in utilization_dict.values()],[x.small_flows.avg_FCT_ms for x in both_p0_flows_summary.values()], label='horovod P0+bg P0', marker='x')
    ax1.plot([x for x in utilization_dict.values()],[x.small_flows.avg_FCT_ms for x in hrvd_p2_flows_summary.values()], label = 'horovod P2+bg P0', marker='o')
    ax1.plot([x for x in utilization_dict.values()],[x.small_flows.avg_FCT_ms for x in bg_flows_summary.values()], label = 'bg only', marker='v')
    
    ax2.plot([x for x in utilization_dict.values()],[x.large_flows.avg_FCT_ms for x in both_p0_flows_summary.values()], label='horovod P0+bg P0', marker='x' )
    ax2.plot([x for x in utilization_dict.values()],[x.large_flows.avg_FCT_ms for x in hrvd_p2_flows_summary.values()], label = 'horovod P2+bg P0', marker='o')
    ax2.plot([x for x in utilization_dict.values()],[x.large_flows.avg_FCT_ms for x in bg_flows_summary.values()], label = 'bg only', marker='v')
    
    ax3.plot([x for x in utilization_dict.values()],[x.small_flows.percentile_99th_ms for x in both_p0_flows_summary.values()], label = 'horovod P0+bg P0', marker='x' )
    ax3.plot([x for x in utilization_dict.values()],[x.small_flows.percentile_99th_ms for x in hrvd_p2_flows_summary.values()], label = 'horovod P2+bg P0', marker='o' )
    ax3.plot([x for x in utilization_dict.values()],[x.small_flows.percentile_99th_ms for x in bg_flows_summary.values()], label = 'bg only', marker='v' )
    
    ax4.plot([x for x in utilization_dict.values()],small_flows_delta_percent, label = 'small flows', marker='x' )
    ax4.plot([x for x in utilization_dict.values()],large_flows_delta_percent, label = 'large flows', marker='o' )
    ax4.plot([x for x in utilization_dict.values()],small_flows_99th_delta_percent, label = '99th small flows', marker='v' )

    ax5.plot([x for x in utilization_dict.values()],small_flows_bg_delta_percent, label = 'small flows', marker='x' )
    ax5.plot([x for x in utilization_dict.values()],large_flows_bg_delta_percent, label = 'large flows', marker='o' )
    ax5.plot([x for x in utilization_dict.values()],small_flows_99th_bg_delta_percent, label = '99th small flows', marker='v' )
    
    ax6.plot([x for x in utilization_dict.values()],horovod_iteration_times_s, label = 'horovod_P2_bg_P0', marker='x')
    ax6.plot([x for x in utilization_dict.values()],baseline_horovod_iteration_times_s, label = 'horovod_P0_bg_P0', marker='v')

    for i, (x, y) in enumerate(zip([x for x in utilization_dict.values()], horovod_iteration_times_s)):
        ax6.annotate(horovod_iter_delta[i], # this is the text
                        (x,y), # this is the point to label
                        textcoords="offset points", # how to position the text
                        xytext=(0,10), # distance from text to points (x,y)
                        ha='center') # horizontal alignment can be left, right or center


    ax1.set_title("Avg FCT for Small flows\n (<100KB)")
    ax2.set_title("Avg FCT for Large flows\n (>10MB)")
    ax3.set_title("99th Percentile FCT for Small flows\n (<100KB)")
    ax4.set_title("Avg FCT Change % \n P2 vs P0")
    # ax4.set_ylabel("Increase in Percentage")
    ax5.set_title("Avg FCT Change % \n P2 vs Bg Alone")
    # ax5.set_ylabel("Increase in Percentage")
    ax6.set_title("Horovod Iteration Time in s")
    
    curr_date = datetime.now().strftime('%H_%M_%Y_%m_%d')
    fig.text(0.5, 0.04, "utilization %", ha='center', va='center')
    fig.text(0.06, 0.5, "average FCT in ms", ha='center', va='center', rotation='vertical')

    ax1.legend()
    ax2.legend()
    ax3.legend()
    ax4.legend()
    ax5.legend()
    ax6.legend()
    ax7.legend()
    
    # fig.set_size_inches(18.5, 10.5)
    fig.set_size_inches(26.5, 10.5)
    if save_fig == False:
        plt.show(block=True)
    else:
        fig.savefig(f"small_large_small_99th_flows_FCT_{curr_date}_single_pkt_dev_queue_w_new_tcpto_10G_low_util", bbox_inches='tight')



def construct_run_dir(flow_rate, hrvprio, run_idx, ctn_ratio, pftohvd_ratio, timestamp):
    #  "pfabric_flows_horovod_100ms_arrival_23_runhvd_true_hrvprio_0x10_num_hvd_8_link_bw_10.0Gbit_run_idx_0_ctn_16.0_pftohrvd_0.50048_02_30_2020_08_12"
    # pfabric_flows_horovod_100ms_arrival_92_runhvd_true_hrvprio_0x08_num_hvd_8_link_bw_10.0Gbit_run_idx_0_ctn_8.0_pftohrvd_1.00096_02_04_2020_08_13
    dir_name =f"pfabric_flows_horovod_100ms_arrival_{flow_rate}_runhvd_true_hrvprio_{hrvprio}_num_hvd_8_link_bw_10.0Gbit_run_idx_{run_idx}_ctn_{ctn_ratio}_pftohrvd_{pftohvd_ratio}_{timestamp}" 
    return dir_name


def construct_list_of_dirs():
    flow_rates = [23, 46, 92, 184]
    hrvprio_list = ["0x10", "0x08"]
    run_idx_list = [0, 1, 2]
    ctn_ratio_list = [16.0, 8.0, 4.0] # TODO add the test results of 2
    ptohvd_ratio_dic = collections.defaultdict() # key: ctn_ratio
    ptohvd_ratio_dic[16.0] = [0.50048, 1.00096, 2.00192, 4.00384] 
    ptohvd_ratio_dic[8.0] = [0.25024, 0.50048, 1.00096, 2.00192] 
    ptohvd_ratio_dic[4.0] = [0.12512,  0.25024, 0.50048, 1.00096]
    # ptohvd_ratio_dic[4.0] = [0.12512,  0.25024, 0.50048, 1.00096]

    
    timestamp = ["02_30_2020_08_12", "02_04_2020_08_13"]

    list_run_dirs = []
    root_run_dir = pathlib.Path("/mnt/raid10/hanjing/thesis/ns3/ns3-basic-sim/runs")
    if root_run_dir.exists() == False:
        print(f"runs directory doesn't exist: {root_run_dir}")
    for i, hrvprio in enumerate(hrvprio_list):
        for ctn_r in ctn_ratio_list:
            for j, f_rate in enumerate(flow_rates):
                for r_idx in run_idx_list:
                    dir_name = construct_run_dir(f_rate, hrvprio, r_idx, ctn_r, ptohvd_ratio_dic[ctn_r][j], timestamp[i])
                    print(f"dir_name: {dir_name}")
                    dir_path = root_run_dir/dir_name
                    if dir_path.exists() == False:
                        print(f"Can't locate run dir: {dir_name}")
                        return []
                    else:
                        list_run_dirs.append(dir_name)
    
    return list_run_dirs


def extract_hrvd_avg_time(progress_file_abs_path):
    with open(progress_file_abs_path, "r") as inputfile:
        lines = inputfile.readlines()        
        avg_time_s =float(lines[-1].strip("\n").split(",")[1])
        print(f"avg_time_s: {avg_time_s}")
    
    return avg_time_s

def plot_FCT_per_compute_to_network_ratio(run_dir_lists, ctn_ratios, flow_rates, num_runs, link_cap_Gbits):
    dir_idx = 0
    flow_summary = collections.defaultdict()
    hrvd_summary = collections.defaultdict()
    link_util = collections.defaultdict()
    flows_util = collections.defaultdict()
    hrvd_util = collections.defaultdict()
    leftover_util = collections.defaultdict()

    run_root_dir = "/mnt/raid10/hanjing/thesis/ns3/ns3-basic-sim/runs"
    num_workers = 8
    # for run_dir in run_dir_lists:
    for prio in ["0x10", "0x08"]:
        for ctn_r in ctn_ratios:
            for flow_rate in flow_rates:
                num_run_dirs = []
                total_link_utilization = []
                for num_run in range(num_runs):
                    num_run_dirs.append(run_dir_lists[dir_idx])
                    hrvd_avg_iter_time_s = 0
                    total_link_utilization.append(extract_average_utilization(run_dir_lists[dir_idx]))
                    for n_worker in range(num_workers):
                        print(f"run_dir: {run_dir_lists[dir_idx]}")
                        hrvd_progress_file = f"{run_root_dir}/{run_dir_lists[dir_idx]}/logs_ns3/HorovodWorker_{n_worker}_iteration_summary.txt"
                        # hrvd_progress_avg_iter_time_s = extract_hrvd_avg_time(hrvd_progress_file)
                        hrvd_avg_iter_time_s += extract_hrvd_avg_time(hrvd_progress_file)
                    hrvd_avg_iter_time_s = hrvd_avg_iter_time_s/num_workers
                    dir_idx +=1
                # flow_summary = plot_multi_FCT_runs(num_run_dirs)
                flow_summary[(prio, ctn_r, flow_rate)] = plot_multi_FCT_runs(num_run_dirs)
                hrvd_summary[(prio, ctn_r, flow_rate)] = hrvd_avg_iter_time_s
                link_util[(prio, ctn_r, flow_rate)] = sum(total_link_utilization)/len(total_link_utilization)
                flows_util[(prio, ctn_r, flow_rate)] = get_flows_expected_utilization(link_cap_Gbits, flow_rate)
                hrvd_util[(prio, ctn_r, flow_rate)] = link_util[(prio, ctn_r, flow_rate)] - flows_util[(prio, ctn_r, flow_rate)]
                leftover_util[(prio, ctn_r, flow_rate)]= 100.0 - link_util[(prio, ctn_r, flow_rate)] 

                print(f"flow_rate: {flow_rate}, small_flows: {flow_summary[(prio, ctn_r, flow_rate)].small_flows.avg_FCT_ms}")
                print(f"horovod time: {hrvd_summary[(prio, ctn_r, flow_rate)]}")


    for ctn_r in ctn_ratios:
        num_plots = 7
        fig, (ax1, ax2, ax3, ax4, ax5, ax6, ax7) = plt.subplots(1,num_plots)

        ax1.plot(flow_rates,[flow_summary[("0x10", ctn_r, f_rate)].small_flows.avg_FCT_ms for f_rate in flow_rates], label='Same Prio', marker='x')
        ax1.plot(flow_rates,[flow_summary[("0x08", ctn_r, f_rate)].small_flows.avg_FCT_ms for f_rate in flow_rates], label='HRVD LoPrio', marker='x')

        ax5.plot(flow_rates, [flow_summary[("0x10", ctn_r, f_rate)].compare_avg_FCT(flow_summary[("0x08", ctn_r, f_rate)])["small"] for f_rate in flow_rates], label='Small Flows', marker='x')
        ax5.plot(flow_rates, [flow_summary[("0x10", ctn_r, f_rate)].compare_avg_FCT(flow_summary[("0x08", ctn_r, f_rate)])["large"] for f_rate in flow_rates],label='Large Flows', marker='x')
        # ax5.plot(flow_rates, [flow_summary[("0x10", ctn_r, f_rate)].compare_99th_pct(flow_summary[("0x08", ctn_r, f_rate)])["small"] for f_rate in flow_rates],label='99th Small Flows', marker='x')
        # ax5.plot(flow_rates, [flow_summary[("0x10", ctn_r, f_rate)].compare_90th_pct(flow_summary[("0x08", ctn_r, f_rate)])["small"] for f_rate in flow_rates],label='90th Small Flows', marker='x')

        ax2.plot(flow_rates,[flow_summary[("0x10", ctn_r, f_rate)].large_flows.avg_FCT_ms for f_rate in flow_rates], label='Same Prio', marker='x')
        ax2.plot(flow_rates,[flow_summary[("0x08", ctn_r, f_rate)].large_flows.avg_FCT_ms for f_rate in flow_rates], label='HRVD LoPrio', marker='x')

        ax3.plot(flow_rates,[flow_summary[("0x10", ctn_r, f_rate)].small_flows.percentile_99th_ms for f_rate in flow_rates], label='Same Prio', marker='x')
        ax3.plot(flow_rates,[flow_summary[("0x08", ctn_r, f_rate)].small_flows.percentile_99th_ms for f_rate in flow_rates], label='HRVD LoPrio', marker='x')

        ax4.plot(flow_rates,[hrvd_summary[("0x10", ctn_r, f_rate)] for f_rate in flow_rates], label='Same Prio', marker='x')
        ax4.plot(flow_rates,[hrvd_summary[("0x08", ctn_r, f_rate)] for f_rate in flow_rates], label='HRVD LoPrio', marker='x')

        hrvd_iter_delta = [(hrvd_summary[("0x08", ctn_r, f_rate)] - hrvd_summary[("0x10", ctn_r, f_rate)])/ hrvd_summary[("0x10", ctn_r, f_rate)] for f_rate in flow_rates]

        for i, (x, y) in enumerate(zip(flow_rates, [hrvd_summary[("0x08", ctn_r, f_rate)] for f_rate in flow_rates])):
            ax4.annotate(f"{hrvd_iter_delta[i] * 100: .1f}%", # this is the text
                            (x,y), # this is the point to label
                            textcoords="offset points", # how to position the text
                            xytext=(0,10), # distance from text to points (x,y)
                            ha='center') # horizontal alignment can be left, right or center

        ax6.plot(flow_rates,[link_util[("0x10", ctn_r, f_rate)] for f_rate in flow_rates], label='Same Prio', marker='x')
        ax6.plot(flow_rates,[link_util[("0x08", ctn_r, f_rate)] for f_rate in flow_rates], label='HRVD LoPrio', marker='x')


        # TODO create a plot for lower priority and compare
        plot_stacked_bw_distribution(ax7, \
                                    flow_rates, \
                                    [ flows_util[("0x10", ctn_r, f_rate)] for f_rate in flow_rates], \
                                    [ hrvd_util[("0x10", ctn_r, f_rate)] for f_rate in flow_rates], \
                                    [ leftover_util[("0x10", ctn_r, f_rate)] for f_rate in flow_rates])

        ax1.set_title("Avg FCT for Small flows\n (<100KB)")
        ax2.set_title("Avg FCT for Large flows\n (>10MB)")
        ax3.set_title("99th Percentile FCT for Small flows\n (<100KB)")
        ax4.set_title("Horovod Iteration Time in s")
        ax5.set_title("FCT change % \n")
        ax6.set_title("Link Utilization % \n")
        ax7.set_title("Application Link Utilization% \n")

        # x label
        fig.text(0.5, 0.04, f"Flow Rates/s with Horovod Compute to Network Ratio {ctn_r} ", ha='center', va='center')
        # y label
        fig.text(0.06, 0.5, "average FCT in ms", ha='center', va='center', rotation='vertical')


        ax1.legend()
        ax2.legend()
        ax3.legend()
        ax4.legend()
        ax5.legend()
        ax6.legend()
        fig.set_size_inches(26.5, 10.5)
        # fig.set_title(f"Horovod Compute TO Network Ratio {ctn_r}")


        plt.show(block=True)


def test_plot_FCT_per_compute_to_network_ratio():
    run_dir_lists = construct_list_of_dirs()
    print(f"run_dir_lists: {run_dir_lists}")
    ctn_ratios = [16.0, 8.0, 4.0] 
    flow_rates = [23, 46, 92, 184]
    num_runs = 3

    # TODO, extract link cap Gbit from run dir
    link_cap_Gbits = 10
    plot_FCT_per_compute_to_network_ratio(run_dir_lists,ctn_ratios, flow_rates, num_runs,link_cap_Gbits)


# def plot_application_bw_distribution(link_cap, actual_utiliazation):
def get_flows_expected_utilization(link_cap_Gbits, expected_flows_per_s):
    # Start times (ns)
    cdf_mean_byte = get_cdf_mean(CDF_PFABRIC_WEB_SEARCH_BYTE, linear_interpolate=True)
    # in percent 
    expected_utilization = (expected_flows_per_s * cdf_mean_byte / (( float(link_cap_Gbits)/8.0)*(10**9))) * 100.0
    print(f"expected_utilization: {expected_utilization} at {expected_flows_per_s}")
    return expected_utilization


def plot_stacked_bw_distribution(ax, link_utilization, bg_bw, horovod_bw, free_bw):
    barwidth = 3
    ax.bar(link_utilization, bg_bw, color='#b5ffb9', edgecolor='white', label = "pfabric", width =barwidth)
    ax.bar(link_utilization, horovod_bw, bottom=bg_bw, color='#f9bc86', edgecolor='white', label = "horovod", width =barwidth)
    ax.bar(link_utilization, free_bw, bottom=[i+j for i,j in zip(bg_bw, horovod_bw)], color='#a3acff', edgecolor='white', label="Leftover", width =barwidth)

    
if __name__  == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--save_fig", action="store_true", help="specify whether the summary plot should be saved")
    args = parser.parse_args()

    link_cap_Gbits = 10
    # input_run_dirs_timestamp = [(100, "17_03_2020_07_22","21_13_2020_07_22"),(200, "17_03_2020_07_22","21_13_2020_07_22"), (300, "17_03_2020_07_22","21_13_2020_07_22"),\
    #             (400, "17_03_2020_07_22","01_04_2020_07_23"), (500, "17_03_2020_07_22","01_04_2020_07_23"), (600, "17_03_2020_07_22","01_04_2020_07_23"),\
    #             (700, "17_03_2020_07_22","01_04_2020_07_23")]
    
    # input_run_dirs_timestamp = [(100, "17_03_2020_07_22","16_53_2020_07_25"),(200, "17_03_2020_07_22","16_53_2020_07_25"), (300, "17_03_2020_07_22","16_53_2020_07_25"),\
    #             (400, "17_03_2020_07_22","18_16_2020_07_25"), (500, "17_03_2020_07_22","00_02_2020_07_26"), (600, "17_03_2020_07_22","00_47_2020_07_26"),\
    #             (700, "17_03_2020_07_22","00_47_2020_07_26")]
    
    # new_timestamp = "01_48_2020_07_26"

    # new_timestamp = "15_18_2020_07_26"
    # flows_per_s_500_1pkt_wo_tcp_optimizer = "00_16_2020_07_28_1pkt_wo_tcp_optimizer"
    # input_run_dirs_timestamp = [(100, "17_03_2020_07_22",new_timestamp),(200, "17_03_2020_07_22",new_timestamp), (300, "17_03_2020_07_22",new_timestamp),\
    #             (400, "17_03_2020_07_22",""), (500, "17_03_2020_07_22",""), (600, "17_03_2020_07_22",new_timestamp),\
    #             (700, "17_03_2020_07_22",new_timestamp)]

    single_pkt_dev_queue_w_tcpopt = "01_53_2020_07_28"
    input_run_dirs_timestamp = [(100, "17_03_2020_07_22",single_pkt_dev_queue_w_tcpopt),(200, "17_03_2020_07_22",single_pkt_dev_queue_w_tcpopt), (300, "17_03_2020_07_22",single_pkt_dev_queue_w_tcpopt),\
            (400, "17_03_2020_07_22",single_pkt_dev_queue_w_tcpopt), (500, "17_03_2020_07_22",single_pkt_dev_queue_w_tcpopt), (600, "17_03_2020_07_22",single_pkt_dev_queue_w_tcpopt),\
            (700, "17_03_2020_07_22",single_pkt_dev_queue_w_tcpopt)]

    bg_alone = "10_18_2020_07_30"
    baseline_both_p0 = "02_03_2020_07_30"
    h_p2_single_pkt_new_TCP_to = "01_54_2020_07_30"
    input_run_dirs_timestamp = [(100, bg_alone, baseline_both_p0,h_p2_single_pkt_new_TCP_to),\
            (200, bg_alone, baseline_both_p0,h_p2_single_pkt_new_TCP_to), \
            (300, bg_alone, baseline_both_p0,h_p2_single_pkt_new_TCP_to),\
            (400, bg_alone, baseline_both_p0,h_p2_single_pkt_new_TCP_to), \
            (500, bg_alone, baseline_both_p0,h_p2_single_pkt_new_TCP_to), \
            (600, bg_alone, baseline_both_p0,h_p2_single_pkt_new_TCP_to)] #,\
            # (700, bg_alone, baseline_both_p0,h_p2_single_pkt_new_TCP_to)]

    link_cap_Gbits = 5
    # baseline_both_P0_5G = "20_57_2020_07_30"
    baseline_both_P0_5G = "23_59_2020_07_30"
    # h_p2_single_pkt_new_TCP_to_5G = "21_35_2020_07_30"
    h_p2_single_pkt_new_TCP_to_5G = "23_11_2020_07_30"
    bg_alone = "22_34_2020_07_30"
    input_run_dirs_timestamp = [(50.0, bg_alone, baseline_both_P0_5G,h_p2_single_pkt_new_TCP_to_5G),\
            (100.0, bg_alone, baseline_both_P0_5G,h_p2_single_pkt_new_TCP_to_5G), \
            (150.0, bg_alone, baseline_both_P0_5G,h_p2_single_pkt_new_TCP_to_5G),\
            (200.0, bg_alone, baseline_both_P0_5G,h_p2_single_pkt_new_TCP_to_5G), \
            (250.0, bg_alone, baseline_both_P0_5G,h_p2_single_pkt_new_TCP_to_5G), \
            (300.0, bg_alone, baseline_both_P0_5G,h_p2_single_pkt_new_TCP_to_5G)]


    # Running with 8 and failed!
    link_cap_Gbits = 5
    baseline_both_P0_5G = "02_31_2020_07_31"
    h_p2_single_pkt_new_TCP_to_5G = "02_35_2020_07_31"
    bg_alone = "02_43_2020_07_31"
    input_run_dirs_timestamp = [(50.0, bg_alone, baseline_both_P0_5G,h_p2_single_pkt_new_TCP_to_5G),\
            (100.0, bg_alone, baseline_both_P0_5G,h_p2_single_pkt_new_TCP_to_5G), \
            (150.0, bg_alone, baseline_both_P0_5G,h_p2_single_pkt_new_TCP_to_5G),\
            (200.0, bg_alone, baseline_both_P0_5G,h_p2_single_pkt_new_TCP_to_5G), \
            (250.0, bg_alone, baseline_both_P0_5G,h_p2_single_pkt_new_TCP_to_5G), \
            (300.0, bg_alone, baseline_both_P0_5G,h_p2_single_pkt_new_TCP_to_5G)]


    # running with 3 workers and 10G at low utilization
    link_cap_Gbits = 10
    baseline_both_P0 = "09_27_2020_07_31"
    h_p2_single_pkt_new_TCP_to = "09_37_2020_07_31"
    bg_alone = "10_23_2020_07_31"
    input_run_dirs_timestamp = [(10, bg_alone, baseline_both_P0,h_p2_single_pkt_new_TCP_to),\
            (25, bg_alone, baseline_both_P0,h_p2_single_pkt_new_TCP_to), \
            (50, bg_alone, baseline_both_P0,h_p2_single_pkt_new_TCP_to),\
            (100, bg_alone, baseline_both_P0,h_p2_single_pkt_new_TCP_to), \
            (150, bg_alone, baseline_both_P0,h_p2_single_pkt_new_TCP_to)]#, \
            # (200, bg_alone, baseline_both_P0,h_p2_single_pkt_new_TCP_to), \
            # (250, bg_alone, baseline_both_P0,h_p2_single_pkt_new_TCP_to),\
            # (300, bg_alone, baseline_both_P0,h_p2_single_pkt_new_TCP_to), \
            # (350, bg_alone, baseline_both_P0,h_p2_single_pkt_new_TCP_to)]


    # running with 3 workers and 10G at low utilization
    link_cap_Gbits = 5
    baseline_both_P0 = "11_15_2020_07_31"
    h_p2_single_pkt_new_TCP_to = "11_32_2020_07_31"
    bg_alone = "10_57_2020_07_31"
    input_run_dirs_timestamp = [(10, bg_alone, baseline_both_P0,h_p2_single_pkt_new_TCP_to),\
            (25, bg_alone, baseline_both_P0,h_p2_single_pkt_new_TCP_to), \
            (50, bg_alone, baseline_both_P0,h_p2_single_pkt_new_TCP_to),\
            (100, bg_alone, baseline_both_P0,h_p2_single_pkt_new_TCP_to), \
            (150, bg_alone, baseline_both_P0,h_p2_single_pkt_new_TCP_to)]#, \
            # (200, bg_alone, baseline_both_P0,h_p2_single_pkt_new_TCP_to), \
            # (250, bg_alone, baseline_both_P0,h_p2_single_pkt_new_TCP_to),\
            # (300, bg_alone, baseline_both_P0,h_p2_single_pkt_new_TCP_to), \
            # (350, bg_alone, baseline_both_P0,h_p2_single_pkt_new_TCP_to)]

    
    link_cap_Gbits = 5
    baseline_both_P0 = "runhvd_true_hrvprio_0x10_14_30_2020_08_01"
    h_p2_single_pkt_new_TCP_to = "runhvd_true_hrvprio_0x08_14_30_2020_08_01"
    bg_alone = "runhvd_false_hrvprio_0x08_16_46_2020_08_01"
    input_run_dirs_timestamp = [(1.0, bg_alone, baseline_both_P0,h_p2_single_pkt_new_TCP_to),\
            (3.0, bg_alone, baseline_both_P0,h_p2_single_pkt_new_TCP_to), \
            (5.0, bg_alone, baseline_both_P0,h_p2_single_pkt_new_TCP_to),\
            (12.5, bg_alone, baseline_both_P0,h_p2_single_pkt_new_TCP_to), \
            (25.0, bg_alone, baseline_both_P0,h_p2_single_pkt_new_TCP_to), \
            (50.0, bg_alone, baseline_both_P0,h_p2_single_pkt_new_TCP_to)]#, \

    
    link_cap_Gbits=10
    baseline_both_P0 = "runhvd_true_hrvprio_0x10_num_hvd_8_17_58_2020_08_04"
    h_p2_single_pkt_new_TCP_to="runhvd_true_hrvprio_0x08_num_hvd_8_17_58_2020_08_04"
    bg_alone="runhvd_false_hrvprio_0x08_num_hvd_8_17_58_2020_08_04"
    input_run_dirs_timestamp = [(10, bg_alone, baseline_both_P0,h_p2_single_pkt_new_TCP_to),\
            (50, bg_alone, baseline_both_P0,h_p2_single_pkt_new_TCP_to), \
            (100, bg_alone, baseline_both_P0,h_p2_single_pkt_new_TCP_to),\
            (200, bg_alone, baseline_both_P0,h_p2_single_pkt_new_TCP_to), \
            (300, bg_alone, baseline_both_P0,h_p2_single_pkt_new_TCP_to), \
            (400, bg_alone, baseline_both_P0,h_p2_single_pkt_new_TCP_to)]#, \


    print(f"save_fig: {args.save_fig}")
    # plot_FCT_summary(input_run_dirs_timestamp, link_cap_Gbits, args.save_fig)
    t_dir = "/mnt/raid10/hanjing/thesis/ns3/ns3-basic-sim/runs/pfabric_flows_horovod_100ms_arrival_10_runhvd_true_hrvprio_0x10_num_hvd_8_link_bw_10.0Gbit_11_17_2020_08_08/logs_ns3"
    # plot_link_utilization_single_tor(t_dir, 9)
    t_dir = "/mnt/raid10/hanjing/thesis/ns3/ns3-basic-sim/runs/pfabric_flows_horovod_100ms_arrival_46_runhvd_true_hrvprio_0x10_num_hvd_8_link_bw_10.0Gbit_run_idx_2_ctn_16.0_pftohrvd_1.00096_02_30_2020_08_12"
    
    # plot_utilization_and_hrvd_progress(t_dir, 8, args.save_fig)

    # construct_list_of_dirs()
    # test_plot_FCT_per_compute_to_network_ratio()

    extract_hrvd_avg_time(f"{t_dir}/logs_ns3/HorovodWorker_0_iteration_summary.txt")

    test_plot_FCT_per_compute_to_network_ratio()