import sys
import collections
import matplotlib.pyplot as plt


# NS_LOG="*" ./waf --run="main_horovod --run_dir='/mnt/raid10/hanjing/thesis/ns3/ns3-basic-sim/runs/horovod_example'" > ../runs/horovod_example/simulation_logs
print(sys.argv[0])
file_name = sys.argv[1]
print(file_name)
file_header = ""
num_layers = 0
class Event():
    def __init__(self, layer, name, timestamp_ns):
        self.event_name = name
        self.time = timestamp_ns 
        self.layer = layer
    def __str__(self):
        return f"event_name: {event_name}, time_ns: {time_ns}"

event_dict = collections.defaultdict(list) # key: layer index, value: (event_name, time_ns)  
timeline_event = collections.defaultdict(list) #key: layer index, value: duration
with open(file_name, "r") as inputfile:
    line = inputfile.readline().strip("\n")
    file_header = line
    print(f"header: {file_header}")
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
        print(f"line: {line}")
        layer = int(line[1])
        event_name = line[2]
        time_ns = int(line[3])
        # print(f"layer: {layer}, event_name: {event_name}, time_ns: {time_ns}")
        e = Event(layer, event_name, time_ns)
        event_dict[layer].append(e)
        print(f"added event: {e}")

num_layers = max(event_dict.keys())+1
print(f"num_layers: {num_layers}")
duration = 0
bp_start_time = 0
fp_start_time = 0
send_start_time = 0
s1 = "Start_Sending_Partition"
s1_len = len(s1)
s2 = "Received_Partition"
s2_len = len(s2)
for key, event_list in event_dict.items():
    for e in event_list:
        # print(f"e in event_list: {e}")
        if e.event_name == "BP_Start" :
            bp_start_time = e.time
        elif e.event_name == "FP_Start":
            fp_start_time = e.time
        elif e.event_name[:5] == "Start":
            send_start_time = e.time
        elif e.event_name == "BP_Done":
            duration = int(e.time) - int(bp_start_time)
            timeline_event[key].append((bp_start_time, duration))
            print(f"layer: {key} Event name: {e.event_name}: duration: {duration} ")
        elif e.event_name == "FP_Done":
            duration = int(e.time) - int(fp_start_time)
            timeline_event[key].append((fp_start_time, duration))
            print(f"layer: {key} Event name: {e.event_name}: duration: {duration} ")
        elif e.event_name[:5] == "Recei":
            duration = int(e.time) - int(send_start_time)
            timeline_event[key].append((send_start_time, duration))
            print(f"layer: {key} Event name: {e.event_name}: duration: {duration} ")
        else:
            print(f"ERROR: No SUCH EVENT: {e.event_name}")


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
timeline_finish_time = timeline_event[num_layers-1][-1][0] +  timeline_event[num_layers-1][-1][1] + 10
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

plt.show(block=True)

# plot_name = f"event_timeline_qdisc_{horovod_simulator.config}_{curr_time}"
# print(f"config: {horovod_simulator.config}, curr_time: {curr_time}")

# if savefig:
#     plt.savefig(f'./simulation_result/{plot_name}')    
        
