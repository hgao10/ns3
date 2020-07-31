import sh
import utilization_plot
from datetime import datetime
from exputil import *
import collections
import matplotlib.pyplot as plt
from datetime import datetime
import pathlib
import numpy as np
from horovod_worker_plot import * 
from networkload import *

def plot_pfabric_utilization(log_dir):
    # log_dir = "/mnt/raid10/hanjing/thesis/ns3/ns3-basic-sim/runs/pfabric_flows_horovod/logs_ns3"
    data_out_dir = log_dir
    pdf_out_dir = log_dir

    from_to_node_id = [(0,1), (1,2),(2,0)]

    for n1, n2 in from_to_node_id:
        utilization_plot.utilization_plot(log_dir, data_out_dir, pdf_out_dir, n1, n2)


class flow:
    def init(self, id, size, source, target,start_time, duration, sent_bytes, status):
        self.id = id
        self.size_KB = size
        self.source = source
        self.target = target
        self.start_time = start_time
        self.duration = duration
        self.end_time = self.start_time + self.duration
        self.sent_bytes = sent_bytes
        self.status = status
        self.attr = collections.defaultdict()

def plot_flows_FCT(run_dir):
    run_dir_root = "/mnt/raid10/hanjing/thesis/ns3/ns3-basic-sim/runs"
    data_file = f"{run_dir_root}/{run_dir}/logs_ns3/flows.csv"

    data_file_path = pathlib.Path(data_file)
    if data_file_path.exists() == False:
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
    for i in range(num_entries):
        f_id = flows_id[i]
        from_node = source[i]
        to_node = target[i]
        f_size = size[i]
        f_size_KB = float(f_size/1000)
        f_duration = duration[i]
        f_finished_state = finished_state[i]
        # print(f"f id: {f_id}, from: {from_node}, to: {to_node}, f_size_KB:{f_size_KB:.2f},f_duration:{f_duration}, state:{f_finished_state} ")
        if f_finished_state == "YES":
            if f_size_KB <= small_flows_threshold_KB:
                small_finished_flows_duration.append(f_duration)
            elif f_size_KB >= large_flows_threshold_KB:
                large_finished_flows_duration.append(f_duration)

    average_small_flows_FCT_ms = sum(small_finished_flows_duration)/len(small_finished_flows_duration)        
    average_small_flows_FCT_ms = float(average_small_flows_FCT_ms)/float(1000000)

    small_flows_99th_FCT_ms = np.percentile(small_finished_flows_duration,99)
    small_flows_99th_FCT_ms = float(small_flows_99th_FCT_ms)/float(1000000)


    average_large_flows_FCT_ms = sum(large_finished_flows_duration)/len(large_finished_flows_duration)        
    average_large_flows_FCT_ms = float(average_large_flows_FCT_ms)/float(1000000)
    # print(f"small flow fct: {average_small_flows_FCT_ms}, cnt: {len(small_finished_flows_duration)}")
    # print(f"large flow fct: {average_large_flows_FCT_ms}, cnt: {len(large_finished_flows_duration)}")
    print(f"99th percentile: {small_flows_99th_FCT_ms}")
    print(f"average small percentile: {average_small_flows_FCT_ms}")
    print(f"average large percentile: {average_large_flows_FCT_ms}")

    return average_small_flows_FCT_ms, average_large_flows_FCT_ms, small_flows_99th_FCT_ms

def extract_average_utilization(run_dir):
    run_dir_root = "/mnt/raid10/hanjing/thesis/ns3/ns3-basic-sim/runs"

    data_file = f"{run_dir_root}/{run_dir}/logs_ns3/utilization_summary.txt"
    data_file_path = pathlib.Path(data_file)
    if not data_file_path.exists:
        return None

    with open(data_file, "r") as file:
        line = file.readlines()
        from_0_to_1_link = line[1]
        from_0_to_1_link = from_0_to_1_link.strip("\n")
        from_0_to_1_link = from_0_to_1_link.split()
        link_utilization = float(from_0_to_1_link[-1].strip("%"))
        # print(f"from_0_to_1_link: {from_0_to_1_link}, utilization: {link_utilization}")

    return link_utilization


def plot_FCT_summary(input_run_dirs_timestamp, link_cap_Gbits):
    # key: flows_per_s, value: utilization in string
    utilization_dict = collections.defaultdict() 
    expected_flows_bw = collections.defaultdict()
    horovod_p0_bw = collections.defaultdict()
    leftover_p0_bw = collections.defaultdict()

    bg_small_flows_FCT_dict = collections.defaultdict()
    bg_large_flows_FCT_dict = collections.defaultdict()
    bg_small_flows_99th_FCT_dict = collections.defaultdict()

    small_flows_FCT_dict = collections.defaultdict() 
    large_flows_FCT_dict = collections.defaultdict() 
    small_flows_99th_FCT_dict = collections.defaultdict() 
    
    c_small_flows_FCT_dict = collections.defaultdict() 
    c_small_flows_99th_FCT_dict = collections.defaultdict() 
    c_large_flows_FCT_dict = collections.defaultdict() 

    small_flows_delta_percent = []
    large_flows_delta_percent = []
    small_flows_99th_delta_percent = []

    small_flows_bg_delta_percent = []
    large_flows_bg_delta_percent = []
    small_flows_99th_bg_delta_percent = []
    
    horovod_iteration_times_s = []
    baseline_horovod_iteration_times_s = []

    for flows_per_s, bg_alone_time_stamp, hrvd_p0_time_stamp, hrvd_p2_time_stamp in input_run_dirs_timestamp:
        # baseline_time_stamp = "17_03_2020_07_22"
        # comparison_time_stamp = "21_13_2020_07_22"
        print(f"Flows per second: {flows_per_s}")
        run_dir = f"pfabric_flows_horovod_100ms_arrival_{flows_per_s}_{hrvd_p0_time_stamp}"
        hrvd_p2_run_dir = f"pfabric_flows_horovod_100ms_arrival_{flows_per_s}_{hrvd_p2_time_stamp}"
        bg_run_dir = f"pfabric_flows_horovod_100ms_arrival_{flows_per_s}_{bg_alone_time_stamp}"
        
        utilization_dict[flows_per_s] = extract_average_utilization(run_dir)
        expected_flows_bw[flows_per_s] = get_flows_expected_utilization(link_cap_Gbits, flows_per_s)
        horovod_p0_bw[flows_per_s] = utilization_dict[flows_per_s] - expected_flows_bw[flows_per_s]
        leftover_p0_bw[flows_per_s] = 100.0 - utilization_dict[flows_per_s] 

        bg_small_flows_FCT_dict[flows_per_s], bg_large_flows_FCT_dict[flows_per_s], bg_small_flows_99th_FCT_dict[flows_per_s] = plot_flows_FCT(bg_run_dir)
        small_flows_FCT_dict[flows_per_s], large_flows_FCT_dict[flows_per_s], small_flows_99th_FCT_dict[flows_per_s] = plot_flows_FCT(run_dir)
        c_small_flows_FCT_dict[flows_per_s], c_large_flows_FCT_dict[flows_per_s], c_small_flows_99th_FCT_dict[flows_per_s] = plot_flows_FCT(hrvd_p2_run_dir)
        
        if c_small_flows_FCT_dict[flows_per_s] != None and c_large_flows_FCT_dict[flows_per_s] != None:
            small_flows_delta_percent.append( (c_small_flows_FCT_dict[flows_per_s] - small_flows_FCT_dict[flows_per_s])/small_flows_FCT_dict[flows_per_s] * 100)
            large_flows_delta_percent.append((c_large_flows_FCT_dict[flows_per_s] - large_flows_FCT_dict[flows_per_s])/large_flows_FCT_dict[flows_per_s] * 100)
            small_flows_99th_delta_percent.append((c_small_flows_99th_FCT_dict[flows_per_s]-small_flows_99th_FCT_dict[flows_per_s])/small_flows_99th_FCT_dict[flows_per_s] * 100)

            small_flows_bg_delta_percent.append( (c_small_flows_FCT_dict[flows_per_s] - bg_small_flows_FCT_dict[flows_per_s])/bg_small_flows_FCT_dict[flows_per_s] * 100)
            large_flows_bg_delta_percent.append((c_large_flows_FCT_dict[flows_per_s] - bg_large_flows_FCT_dict[flows_per_s])/bg_large_flows_FCT_dict[flows_per_s] * 100)
            small_flows_99th_bg_delta_percent.append((c_small_flows_99th_FCT_dict[flows_per_s]-bg_small_flows_99th_FCT_dict[flows_per_s])/bg_small_flows_99th_FCT_dict[flows_per_s] * 100)
            
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
        horovod_iteration_time_summary = f"{run_dir_root}/{hrvd_p2_run_dir}/logs_ns3/HorovodWorker_0_iteration_summary.txt"

        # directory - Horovod P0 and bg P0
        baseline_log_dir = f"{run_dir_root}/{run_dir}/logs_ns3"
        baseline_horovod_progress_txt = f"{baseline_log_dir}/HorovodWorker_0_layer_50_port_1024_progress.txt"
        baseline_horovod_iteration_time_summary = f"{baseline_log_dir}/HorovodWorker_0_iteration_summary.txt"


        if pathlib.Path(comparison_log_dir).exists() == False or pathlib.Path(horovod_progress_txt).exists() == False:
            horovod_iteration_times_s.append(None)
        else:
            calculate_avg_horovod_iter_and_log_summary(horovod_progress_txt, horovod_iteration_time_summary, comparison_log_dir, horovod_iteration_times_s)

        if pathlib.Path(baseline_log_dir).exists() == False or pathlib.Path(baseline_horovod_progress_txt).exists() == False:
            baseline_horovod_iteration_times_s.append(None)
        else:
            calculate_avg_horovod_iter_and_log_summary(baseline_horovod_progress_txt, baseline_horovod_iteration_time_summary, baseline_log_dir, baseline_horovod_iteration_times_s)

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
    ax1.plot([x for x in utilization_dict.values()],[x for x in small_flows_FCT_dict.values()], label='horovod P0+bg P0', marker='x')
    ax1.plot([x for x in utilization_dict.values()],[x for x in c_small_flows_FCT_dict.values()], label = 'horovod P2+bg P0', marker='o')
    ax1.plot([x for x in utilization_dict.values()],[x for x in bg_small_flows_FCT_dict.values()], label = 'bg only', marker='v')
    
    ax2.plot([x for x in utilization_dict.values()],[x for x in large_flows_FCT_dict.values()], label='horovod P0+bg P0', marker='x' )
    ax2.plot([x for x in utilization_dict.values()],[x for x in c_large_flows_FCT_dict.values()], label = 'horovod P2+bg P0', marker='o')
    ax2.plot([x for x in utilization_dict.values()],[x for x in bg_large_flows_FCT_dict.values()], label = 'bg only', marker='v')
    
    ax3.plot([x for x in utilization_dict.values()],[x for x in small_flows_99th_FCT_dict.values()], label = 'horovod P0+bg P0', marker='x' )
    ax3.plot([x for x in utilization_dict.values()],[x for x in c_small_flows_99th_FCT_dict.values()], label = 'horovod P2+bg P0', marker='o' )
    ax3.plot([x for x in utilization_dict.values()],[x for x in bg_small_flows_99th_FCT_dict.values()], label = 'bg only', marker='v' )
    
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
    plt.show(block=True)

    # fig.savefig(f"small_large_small_99th_flows_FCT_{curr_date}_single_pkt_dev_queue_w_new_tcpto_10G", bbox_inches='tight')


def calculate_avg_horovod_iter_and_log_summary(horovod_progress_txt, horovod_iteration_time_summary, log_dir, horovod_iteration_times_s):
    with open(horovod_iteration_time_summary, "w") as s_file:
        s_file.write("Iter Idx, Iter Duration in S\n")
        if pathlib.Path(horovod_progress_txt).exists() == False:
            horovod_iteration_times_s.append(None)
            s_file.write("0, 0\n")
        else:
            event_dict, _, _ = construct_event_list_from_progress_file(horovod_progress_txt)
            _, _, iter_time_ns = construct_timeline_and_priority_events_from_event_list(event_dict, log_dir)
            print(f"log_dir: {log_dir}")
            avg_iter_time_ns = None
            if len(iter_time_ns) != 0:
                avg_iter_time_ns = sum(iter_time_ns)/len(iter_time_ns)
                avg_iter_time_s = float(avg_iter_time_ns)/float(1000000000)
                horovod_iteration_times_s.append(avg_iter_time_s)
                for i in range(len(iter_time_ns)):
                    s_file.write(f"{i}, {iter_time_ns[i]/10**9}\n")
                s_file.write("Iter Cnt, Avg Iter Duration\n")
                s_file.write(f"{len(iter_time_ns)}, {avg_iter_time_s}\n")
            else:
                horovod_iteration_times_s.append(None)
                s_file.write("0, 0\n")

    return horovod_iteration_times_s


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

    plot_FCT_summary(input_run_dirs_timestamp, link_cap_Gbits)