import sys
import collections
import matplotlib.pyplot as plt
import pathlib
import numpy as np
import argparse
from datetime import datetime
from dataclasses import dataclass
# NS_LOG="*" ./waf --run="main_horovod --run_dir='/mnt/raid10/hanjing/thesis/ns3/ns3-basic-sim/runs/horovod_example'" > ../runs/horovod_example/simulation_logs

@dataclass
class Event:
    iter_idx: int
    layer: int
    event_name: str
    time: int 

def construct_event_list_from_progress_file(progress_file):
    event_dict = collections.defaultdict(list) # key: layer index, value: (event_name, time_ns)  
    max_time_ns = 0
    with open(progress_file, "r") as inputfile:
        # read in first line which is the header 
        # in the form of Iteration_idx,Layer_idx,Event,Time
        line = inputfile.readline().strip("\n")

        # print(line, end="")
        i = 0 
        while line !='':
            i += 1
            # print(line, end="")
            line=inputfile.readline().strip("\n")
            # data
            line = line.split(",")
            if(line == ['']):
                break
            # print(f"line: {line}")
            iter_idx = int(line[0])
            layer = int(line[1])
            event_name = line[2]
            time_ns = int(line[3])
            max_time_ns = max(max_time_ns, time_ns)
            # print(f"layer: {layer}, event_name: {event_name}, time_ns: {time_ns}")
            e = Event(iter_idx, layer, event_name, time_ns)
            event_dict[layer].append(e)
            
            # print(f"added event: {e}")
    
    num_layers = max(event_dict.keys())+1
    # print(f"num_layers: {num_layers}, max_time_ns: {max_time_ns}")
    return event_dict, num_layers, max_time_ns

def construct_timeline_and_priority_events_from_event_list(event_dict, log_dir):
    timeline_event = collections.defaultdict(list) #key: layer index, value: duration
    duration = 0
    bp_start_time = 0
    fp_start_time = 0
    send_start_time = 0
    num_layers = max(event_dict.keys())+1
    for priority in [0, 2, 6]:
        sample_f = pathlib.Path(f"{log_dir}/Priority_{priority}_samples")
        if sample_f.is_file():
            # remove file if exits
            sample_f.unlink()

    iteration_times_ns = []
    prev_BP_start_time = -1
    priority_duration_timestamp = collections.defaultdict(list)
    for key, event_list in event_dict.items():
        # first_start_sending_partition= True
        for e in event_list:
            # print(f"e in event_list: {e}")
            if e.event_name == "BP_Start" :
                bp_start_time = e.time
                first_start_sending_partition= True
                if key == num_layers-1:
                    if prev_BP_start_time != -1:
                        iteration_times_ns.append(e.time - prev_BP_start_time)
                    prev_BP_start_time = e.time                    
            elif e.event_name == "FP_Start":                    
                fp_start_time = e.time
            elif e.event_name[:5] == "Start" and first_start_sending_partition:
                send_start_time = e.time
                first_start_sending_partition = False
            elif e.event_name == "BP_Done":
                duration = int(e.time) - int(bp_start_time)
                timeline_event[key].append((bp_start_time, duration))
                # print(f"layer: {key}, iter: {e.iter_idx} Event name: {e.event_name}: duration: {duration} ")
            elif e.event_name == "FP_Done":
                duration = int(e.time) - int(fp_start_time)
                timeline_event[key].append((fp_start_time, duration))
                # print(f"layer: {key} Event name: {e.event_name}: duration: {duration} ")
                    
            elif e.event_name[:5] == "Recei":
                duration = int(e.time) - int(send_start_time)
                prio = e.event_name.split("_")[-1]
                priority_duration_timestamp[prio].append([int(duration), int(send_start_time)])

                timeline_event[key].append((send_start_time, duration))
                send_start_time = e.time
                # print(f"layer: {key} Event name: {e.event_name}: duration: {duration} ")
    
    return timeline_event, priority_duration_timestamp, iteration_times_ns

def write_to_priority_sample_files(priority_duration_timestamp,log_dir):
    available_prio_samples_files = []
    for prio, duration_stamp_pairs in priority_duration_timestamp.items():
        # sorted by timestamp in ns
        duration_stamp_pairs.sort(key= lambda x:x[1])
        prio_sample_file = pathlib.Path(f"{log_dir}/Priority_{prio}_samples")
        if prio_sample_file not in available_prio_samples_files:
            available_prio_samples_files.append(prio_sample_file)
        with open(f"{log_dir}/Priority_{prio}_samples", "a") as sample_file:
            for dur_ms, time_s in duration_stamp_pairs:
                if time_s >= 2000000000:
                    sample_file.write(f"{dur_ms/1000000:.2f},{time_s/1000000000:.1f}\n")  
    return available_prio_samples_files      


def plot_horovod_worker_event_progress(timeline_event, num_layers, max_time_ns, progress_file_abs_path, save_fig):
    # Plot the timeline
    fig, ax = plt.subplots()

    # colors = ('tab:orange', 'tab:green', 'tab:red', 'tab:purple', 'tab:grey', 'tab:pink')
    colors = ('tab:blue','tab:orange', 'tab:green', 'tab:purple', 'tab:cyan')

    len_c = len(colors)
    idx = 0

    num_timelines = len(timeline_event)
    ylim = [5, (num_timelines + 1) * 10]
    y_ticks = [ 15 + i*10 for i in range(num_timelines)]
    # ypos = [(10 * i, 9) for i in range(1, num_timelines+1)]
    bar_width = 8
    ypos = [(tick - bar_width/2, bar_width) for tick in y_ticks]
    yticklabels = []
    for key, value in timeline_event.items():
        print(f"barh values [{key}]: {value}")
        num_intervals = len(value)
        c = num_intervals//len_c * colors + colors[:num_intervals % len_c] 
        # ax.broken_barh(value, ypos[idx], facecolors = colors[idx])
        ax.broken_barh(value, ypos[idx], facecolors = c)
        idx += 1
        yticklabels.append(key)
    ax.set_ylim(ylim)

    timeline_start_time = timeline_event[num_layers-1][0][0] - 1000
    print(f"timeline_start_time : {timeline_start_time}")
    timeline_finish_time = max_time_ns +10 
    # timeline_finish_time = timeline_event[num_layers-1][-1][0] +  timeline_event[num_layers-1][-1][1] + 10
    xlim = (timeline_start_time, timeline_finish_time)
    print(f"xlim: {xlim}, ylim: {ylim}")
    ax.set_xlim(xlim)

    ax.set_yticks(y_ticks)
    # ax.set_yticklabels(yticklabels, fontsize='small')
    ax.set_yticklabels(yticklabels, fontsize=8)
    ax.set_ylabel('Layer Index')
    ax.set_xlabel('Time in ns')

    ax.grid(True)
    fig.set_size_inches(20.5, 12.5)


    if save_fig == False:
        plt.show(block=True)
    else:
        path_parent = pathlib.Path(progress_file_abs_path).parent
        plot_name = pathlib.Path(progress_file_abs_path).stem
        # curr_date = datetime.now().strftime('%H_%M_%Y_%m_%d')
        fig.savefig(f"{path_parent/plot_name}")

def ecdf(data):
    """ Compute ECDF """
    x = np.sort(data)
    n = x.size
    y = np.arange(1, n+1) / n
    return(x,y)

def plot_prio_samples(sample_file_list, save_fig):
    for f in sample_file_list:
        print(f"sample file: {f.name}")
        prio = str(f.name).split("_")[1]
        translated_prio = 0
        if prio == "6":
            scatter_plot_name = f"Priority 0 Samples"
            translated_prio = 0
        if prio == "2":
            scatter_plot_name = f"Priority 2 Samples"
            translated_prio = 2
        if prio == "0":
            scatter_plot_name = f"Priority 1 Samples"
            translated_prio = 1

        fig, (ax1, ax2, ax3) = plt.subplots(3, 1)
        y = []
        x = []
        point_cnt = 0
        with open(f, "r") as input_file:
            line = input_file.readline()
            line = line.strip("\n")
            while len(line)!= 0:
                line = line.split(",")
                duration = float(line[0])
                time_stamp = float(line[1])
                y.append(duration)
                x.append(time_stamp)
                point_cnt +=1
                line = input_file.readline()
                line = line.strip("\n")
        
        # scatter plot
        ax1.scatter(x, y,s=0.8, marker='o')
        ax1.set_xlabel(f"Simulation Time(s)")
        ax1.set_ylabel("Network Transfer time(ms)")
        ax1.set_title(f"{scatter_plot_name}")
        
        # histogram show network transfer time distribution
        ax2.hist(y, bins=40)
        ax2.set_title(f"Priority {translated_prio} Histogram")        
        
        # ecdf 
        e_x, e_y = ecdf(y)
        ax3.scatter(e_x, e_y, s=2**2)
        ax3.set_title(f"Priority {translated_prio} CDF")
        ax3.set_xlabel("Network Transfer time(ms)")

        fig.set_size_inches(12.5, 20.5)
        plt.subplots_adjust(hspace = 0.3)

        if save_fig == False:
            plt.show(block=True)
        else:
            path_parent = pathlib.Path(f).parent
            plot_name = pathlib.Path(f).stem
            # curr_date = datetime.now().strftime('%H_%M_%Y_%m_%d')
            fig.savefig(f"{path_parent/plot_name}")

        


def plot_progress_files(progress_file_lists):
    for p_file in progress_file_lists:
        event_dict, num_layers, max_time_ns = construct_event_list_from_progress_file(p_file)

        timeline_event, priority_duration_timestamp, iteration_times_ns = construct_timeline_and_priority_events_from_event_list(event_dict, log_dir)
        plot_horovod_worker_event_progress(timeline_event, num_layers, max_time_ns, p_file, True)



if __name__ == "__main__":
    # file_name = sys.argv[1]
    parser = argparse.ArgumentParser()
    parser.add_argument("run_dir", help="specify the absolute path of the run directory")
    parser.add_argument("--save_fig", action="store_true", help="specify whether the summary plot should be saved")

    # parser.add_argument("--run_program", action="store_true", help="execute the simulation specified by the main program")
    # parser.add_argument("--plot_utilization", help="plot the utilization of the links", choices=["hist","line"])
    args = parser.parse_args()
    run_dir_path_absolute = pathlib.Path(args.run_dir)
    if run_dir_path_absolute.exists == False:
        raise RuntimeError("Please enter an valid path for run directory")
        # print(f"Please enter an valid path for run directory")
        

    log_dir = run_dir_path_absolute/"logs_ns3"

    progress_file = log_dir/"HorovodWorker_0_layer_50_port_1024_progress.txt"
    event_dict, num_layers, max_time_ns = construct_event_list_from_progress_file(progress_file)

    timeline_event, priority_duration_timestamp, iteration_times_ns = construct_timeline_and_priority_events_from_event_list(event_dict, log_dir)
    plot_horovod_worker_event_progress(timeline_event, num_layers, max_time_ns, progress_file, args.save_fig)

    available_prio_samples_files = write_to_priority_sample_files(priority_duration_timestamp, log_dir)
    plot_prio_samples(available_prio_samples_files, args.save_fig)
