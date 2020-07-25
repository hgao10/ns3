import sh
import utilization_plot
from datetime import datetime
from exputil import *
import collections
import matplotlib.pyplot as plt
from datetime import datetime
import pathlib
import numpy as np

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
        return None, None
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
        return 

    with open(data_file, "r") as file:
        line = file.readlines()
        from_0_to_1_link = line[1]
        from_0_to_1_link = from_0_to_1_link.strip("\n")
        from_0_to_1_link = from_0_to_1_link.split()
        link_utilization = float(from_0_to_1_link[-1].strip("%"))
        # print(f"from_0_to_1_link: {from_0_to_1_link}, utilization: {link_utilization}")

    return link_utilization


def plot_FCT_summary(input_run_dirs):
    # key: flows_per_s, value: utilization in string
    utilization_dict = collections.defaultdict() 
    small_flows_FCT_dict = collections.defaultdict() 
    large_flows_FCT_dict = collections.defaultdict() 
    small_flows_99th_FCT_dict = collections.defaultdict() 
    c_small_flows_FCT_dict = collections.defaultdict() 
    c_small_flows_99th_FCT_dict = collections.defaultdict() 
    c_large_flows_FCT_dict = collections.defaultdict() 
    small_flows_delta_percent = []
    large_flows_delta_percent = []
    small_flows_99th_delta_percent = []

    for flows_per_s, baseline_time_stamp, comparison_time_stamp in input_run_dirs:
        # baseline_time_stamp = "17_03_2020_07_22"
        # comparison_time_stamp = "21_13_2020_07_22"
        print(f"Flows per second: {flows_per_s}")
        run_dir = f"pfabric_flows_horovod_100ms_arrival_{flows_per_s}_{baseline_time_stamp}"
        comparison_run_dir = f"pfabric_flows_horovod_100ms_arrival_{flows_per_s}_{comparison_time_stamp}"
        
        utilization_dict[flows_per_s] = extract_average_utilization(run_dir)
        small_flows_FCT_dict[flows_per_s], large_flows_FCT_dict[flows_per_s], small_flows_99th_FCT_dict[flows_per_s] = plot_flows_FCT(run_dir)
        c_small_flows_FCT_dict[flows_per_s], c_large_flows_FCT_dict[flows_per_s], c_small_flows_99th_FCT_dict[flows_per_s] = plot_flows_FCT(comparison_run_dir)
        
        if c_small_flows_FCT_dict[flows_per_s] != None and c_large_flows_FCT_dict[flows_per_s] != None:
            small_flows_delta_percent.append( (c_small_flows_FCT_dict[flows_per_s] - small_flows_FCT_dict[flows_per_s])/small_flows_FCT_dict[flows_per_s] * 100)
            large_flows_delta_percent.append((c_large_flows_FCT_dict[flows_per_s] - large_flows_FCT_dict[flows_per_s])/large_flows_FCT_dict[flows_per_s] * 100)
            small_flows_99th_delta_percent.append((c_small_flows_99th_FCT_dict[flows_per_s]-small_flows_99th_FCT_dict[flows_per_s])/small_flows_99th_FCT_dict[flows_per_s] * 100)
        else:
            small_flows_delta_percent.append(None)
            large_flows_delta_percent.append(None)
            small_flows_99th_delta_percent.append(None)
        
    fig, (ax1, ax2, ax3, ax4) = plt.subplots(1,4)
    # fig.suptitle("Average FCT vs Link Utilization")
    ax1.plot([x for x in utilization_dict.values()],[x for x in small_flows_FCT_dict.values()], label='background')
    ax1.plot([x for x in utilization_dict.values()],[x for x in c_small_flows_FCT_dict.values()], label = 'horovod+background')
    
    ax2.plot([x for x in utilization_dict.values()],[x for x in large_flows_FCT_dict.values()], label='background' )
    ax2.plot([x for x in utilization_dict.values()],[x for x in c_large_flows_FCT_dict.values()], label = 'horovod+background' )
    
    ax3.plot([x for x in utilization_dict.values()],[x for x in small_flows_99th_FCT_dict.values()], label = 'background' )
    ax3.plot([x for x in utilization_dict.values()],[x for x in c_small_flows_99th_FCT_dict.values()], label = 'horovod+background' )
    
    ax4.plot([x for x in utilization_dict.values()],small_flows_delta_percent, label = 'small flows' )
    ax4.plot([x for x in utilization_dict.values()],large_flows_delta_percent, label = 'large flows' )
    ax4.plot([x for x in utilization_dict.values()],small_flows_99th_delta_percent, label = '99th small flows' )


    ax1.set_title("Avg FCT for Small flows\n (<100KB)")
    ax2.set_title("Avg FCT for Large flows\n (>10MB)")
    ax3.set_title("99th Percentile FCT for Small flows\n (<100KB)")
    ax4.set_title("Avg FCT Gain Percentage \n Running with Horovod")
    ax4.set_ylabel("Increase in Percentage")
    curr_date = datetime.now().strftime('%H_%M_%Y_%m_%d')
    fig.text(0.5, 0.04, "utilization %", ha='center', va='center')
    fig.text(0.06, 0.5, "average FCT in ms", ha='center', va='center', rotation='vertical')

    ax1.legend()
    ax2.legend()
    ax3.legend()
    ax4.legend()
    fig.set_size_inches(18.5, 10.5)
    # fig.tight_layout()
    plt.show(block=True)

    # fig.savefig(f"small_large_small_99th_flows_FCT_dict_{curr_date}", bbox_inches='tight')

if __name__  == "__main__":
    input_run_dirs = [(100, "17_03_2020_07_22","21_13_2020_07_22"),(200, "17_03_2020_07_22","21_13_2020_07_22"), (300, "17_03_2020_07_22","21_13_2020_07_22"),\
                (400, "17_03_2020_07_22","01_04_2020_07_23"), (500, "17_03_2020_07_22","01_04_2020_07_23"), (600, "17_03_2020_07_22","01_04_2020_07_23"),\
                (700, "17_03_2020_07_22","01_04_2020_07_23")]
    
    plot_FCT_summary(input_run_dirs)