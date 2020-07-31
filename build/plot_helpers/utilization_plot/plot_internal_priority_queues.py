import argparse
import pathlib
import collections
import matplotlib.pyplot as plt

def plot_priority_queues(priority_log_dir, node_id):
    n_prio = 3
    band_timestamp_bytes = [] # key: band , value: (timestamp, bytes)
    # band_bytes = collections.defaultdict() # key: band , value: bytes corresponding to the last instance of the timestamp

    for i in range(n_prio):
        prio_file_name = priority_log_dir/"logs_ns3"/f"Node_{node_id}_queue_band_{i}.txt"
        # key: time stamp, value, bytes corresponding to the last instance of the timestamp
        timestamp_bytes_dict = collections.defaultdict() 
        with open(prio_file_name, "r") as data_file:
            lines = data_file.readlines()
            for line in lines:
                line = line.strip("\n")
                if len(line) != 0:
                    _, before_bytes, _, after_bytes, time_ns = line.split()
                    timestamp_bytes_dict[int(time_ns)] = int(after_bytes)
        band_timestamp_bytes.append(timestamp_bytes_dict)

    fig, ax = plt.subplots()
    for i in range(n_prio):
        ax.plot([time for time in band_timestamp_bytes[i].keys()], [byte for byte in band_timestamp_bytes[i].values()], label=f"band_{i}")
    
    ax.legend()
    fig.set_size_inches(18.5, 10.5)
    plt.show(block=True)


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("priority_log_dir", help="specify the absolute path of the directory of the priority log files")
    parser.add_argument("node_id", type=int, help="specify the network device id")
    
    args = parser.parse_args()
    priority_log_dir = pathlib.Path(args.priority_log_dir)
    if priority_log_dir.exists == False:
        raise RuntimeError("Please enter an valid path for priority log directory")
    plot_priority_queues(priority_log_dir, args.node_id)

