import sys
import collections
import matplotlib.pyplot as plt
import pathlib
import numpy as np
import argparse
from datetime import datetime
from dataclasses import dataclass
from ecdf import ecdf
# NS_LOG="*" ./waf --run="main_horovod --run_dir='/mnt/raid10/hanjing/thesis/ns3/ns3-basic-sim/runs/horovod_example'" > ../runs/horovod_example/simulation_logs

@dataclass
class Event:
    iter_idx: int
    layer: int
    event_name: str
    time: int 


class HorovodWorkerProgress:
    def __init__(self, progress_filename, save_fig):
        self.progress_file_abs_path = pathlib.Path(progress_filename)
        if self.progress_file_abs_path.exists() == False:
            raise ValueError(f"{progress_filename} doesn't exist")
        self.log_dir = self.progress_file_abs_path.parent
        # filename is in the form of HorovodWorker_0_layer_50_port_1024_progress.txt
        self.worker_id = int(self.progress_file_abs_path.stem.split("_")[1])

        # key: layer index, value: (event_name, time_ns)
        self.event_dict= collections.defaultdict(list)
        self.max_time_ns = 0
        self.num_layers = 0
        self.iteration_time_list_ns = []
        self.timeline_event = collections.defaultdict(list) #key: layer index, value: duration
        self.priority_duration_timestamp = collections.defaultdict(list)
        self.avg_iter_time_ns = 0
        self.avg_iter_time_s = 0
        self.available_prio_samples_files = []

        self.construct_event_list()
        self.construct_timeline_and_priority_events_from_event_list()
        self.write_iteration_time_summary()
        self.write_to_priority_sample_files()

        
        self.plot_event_progress(save_fig)
        self.plot_prio_samples(save_fig)


    def construct_event_list(self):
        with open(self.progress_file_abs_path, "r") as inputfile:
            # read in first line which is the header in the form of Iteration_idx,Layer_idx,Event,Time
            line = inputfile.readline().strip("\n")
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
                self.max_time_ns = max(self.max_time_ns, time_ns)
                # print(f"layer: {layer}, event_name: {event_name}, time_ns: {time_ns}")
                e = Event(iter_idx, layer, event_name, time_ns)
                self.event_dict[layer].append(e)
                
        self.num_layers = max(self.event_dict.keys())+1


    def construct_timeline_and_priority_events_from_event_list(self):
        duration = 0
        bp_start_time = 0
        fp_start_time = 0
        send_start_time = 0
        for priority in [0, 2, 6]:
            sample_f =self.log_dir/f"Priority_{priority}_samples"
            if sample_f.is_file():
                # remove file if exits
                sample_f.unlink()

        prev_BP_start_time = -1
        for key, event_list in self.event_dict.items():
            # first_start_sending_partition= True
            for e in event_list:
                # print(f"e in event_list: {e}")
                if e.event_name == "BP_Start" :
                    bp_start_time = e.time
                    first_start_sending_partition= True
                    if key == self.num_layers-1:
                        if prev_BP_start_time != -1:
                            self.iteration_time_list_ns.append(e.time - prev_BP_start_time)
                        prev_BP_start_time = e.time                    
                elif e.event_name == "FP_Start":                    
                    fp_start_time = e.time
                elif e.event_name[:5] == "Start" and first_start_sending_partition:
                    send_start_time = e.time
                    first_start_sending_partition = False
                elif e.event_name == "BP_Done":
                    duration = int(e.time) - int(bp_start_time)
                    self.timeline_event[key].append((bp_start_time, duration))
                    # print(f"layer: {key}, iter: {e.iter_idx} Event name: {e.event_name}: duration: {duration} ")
                elif e.event_name == "FP_Done":
                    duration = int(e.time) - int(fp_start_time)
                    self.timeline_event[key].append((fp_start_time, duration))
                    # print(f"layer: {key} Event name: {e.event_name}: duration: {duration} ")
                        
                elif e.event_name[:5] == "Recei":
                    duration = int(e.time) - int(send_start_time)
                    prio = e.event_name.split("_")[-1]
                    self.priority_duration_timestamp[prio].append([int(duration), int(send_start_time)])

                    self.timeline_event[key].append((send_start_time, duration))
                    send_start_time = e.time
                    # print(f"layer: {key} Event name: {e.event_name}: duration: {duration} ")
        
        self.avg_iter_time_ns = sum(self.iteration_time_list_ns)/len(self.iteration_time_list_ns)
        self.avg_iter_time_s = float(self.avg_iter_time_ns)/float(1000000000)


    def write_iteration_time_summary(self):
        self.iteration_summary_file_name = self.log_dir/f"HorovodWorker_{self.worker_id}_iteration_summary.txt"
        with open(self.iteration_summary_file_name, "w") as s_file:
            s_file.write("Iter Idx, Iter Duration in S\n")
            if len(self.iteration_time_list_ns) != 0:
                for i in range(len(self.iteration_time_list_ns)):
                    s_file.write(f"{i}, {self.iteration_time_list_ns[i]/10**9}\n")
                s_file.write("Iter Cnt, Avg Iter Duration\n")
                s_file.write(f"{len(self.iteration_time_list_ns)}, {self.avg_iter_time_s}\n")
            else:
                s_file.write("0, 0\n")


    def write_to_priority_sample_files(self):
        for prio, duration_stamp_pairs in self.priority_duration_timestamp.items():
            # sorted by timestamp in ns
            duration_stamp_pairs.sort(key= lambda x:x[1])
            prio_sample_file = self.log_dir/f"Priority_{prio}_samples.txt"
            if prio_sample_file not in self.available_prio_samples_files:
                self.available_prio_samples_files.append(prio_sample_file)
            with open(prio_sample_file, "a") as sample_file:
                for dur_ms, time_s in duration_stamp_pairs:
                    if time_s >= 2000000000:
                        sample_file.write(f"{dur_ms/1000000:.2f},{time_s/1000000000:.1f}\n")  
        

    def plot_event_progress(self, save_fig):
        # Plot the timeline
        fig, ax = plt.subplots()

        # colors = ('tab:orange', 'tab:green', 'tab:red', 'tab:purple', 'tab:grey', 'tab:pink')
        colors = ('tab:blue','tab:orange', 'tab:green', 'tab:purple', 'tab:cyan')

        len_c = len(colors)
        idx = 0

        num_timelines = len(self.timeline_event)
        ylim = [5, (num_timelines + 1) * 10]
        y_ticks = [ 15 + i*10 for i in range(num_timelines)]
        # ypos = [(10 * i, 9) for i in range(1, num_timelines+1)]
        bar_width = 8
        ypos = [(tick - bar_width/2, bar_width) for tick in y_ticks]
        yticklabels = []
        for key, value in self.timeline_event.items():
            print(f"barh values [{key}]: {value}")
            num_intervals = len(value)
            c = num_intervals//len_c * colors + colors[:num_intervals % len_c] 
            # ax.broken_barh(value, ypos[idx], facecolors = colors[idx])
            ax.broken_barh(value, ypos[idx], facecolors = c)
            idx += 1
            yticklabels.append(key)
        ax.set_ylim(ylim)

        timeline_start_time = self.timeline_event[self.num_layers-1][0][0] - 1000
        print(f"timeline_start_time : {timeline_start_time}")
        timeline_finish_time = self.max_time_ns +10 
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
            # path_parent = pathlib.Path(progress_file_abs_path).parent
            plot_name = self.progress_file_abs_path.stem
            # curr_date = datetime.now().strftime('%H_%M_%Y_%m_%d')
            fig.savefig(self.log_dir/f"{plot_name}")


    def plot_prio_samples(self, save_fig):
        for f in self.available_prio_samples_files:
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

                plot_name = pathlib.Path(f).stem
                # curr_date = datetime.now().strftime('%H_%M_%Y_%m_%d')
                fig.savefig(self.log_dir/f"{plot_name}")


def plot_progress_files(progress_file_lists,log_dir, save_fig):
    for p_file in progress_file_lists:
        horovod_progress = HorovodWorkerProgress(p_file, save_fig)


if __name__ == "__main__":
    run_dir_parent_dir = "/mnt/raid10/hanjing/thesis/ns3/ns3-basic-sim/runs"
    run_dir = "pfabric_flows_horovod_100ms_arrival_10_runhvd_true_hrvprio_0x10_num_hvd_8_link_bw_10.0Gbit_11_17_2020_08_08_test"
    progress_file_abs_path = f"{run_dir_parent_dir}/{run_dir}/logs_ns3/HorovodWorker_0_layer_50_port_1024_progress.txt"

    save_fig = True
    test_p = HorovodWorkerProgress(progress_file_abs_path, save_fig)