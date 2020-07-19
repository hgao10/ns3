import sh
import collections
import sys
import matplotlib.pyplot as plt
import argparse

# test_cases=collections.defaultdict(list)

horovod_prg = "HorovodWorker_0_layer_50_port_1024_progress.txt"

def run_program(main_program):
    sh.cp("/mnt/raid10/hanjing/thesis/ns3/ns3-basic-sim/simulator/run.sh", "/mnt/raid10/hanjing/thesis/ns3/ns3-basic-sim/simulator/run.sh.bak")
    # sh.sed( "-i", f"s/program/{main_program}/g", "/mnt/raid10/hanjing/thesis/ns3/ns3-basic-sim/simulator/run.sh.bak", _err="sed_err_log", _out="sed_out_log")
    sh.sed( "-i", f"s/program/{main_program}/g", "/mnt/raid10/hanjing/thesis/ns3/ns3-basic-sim/simulator/run.sh.bak")
    sh.bash("/mnt/raid10/hanjing/thesis/ns3/ns3-basic-sim/simulator/run.sh.bak")


def plot_horovod_progress(network_bw, log_dir, run_dir):
    sh.python("/mnt/raid10/hanjing/thesis/ns3/ns3-basic-sim/simulator/src/basic-apps/helper/horovod_worker_plot.py", f"{log_dir}/{horovod_prg}")
    new_log_dir = f"logs_ns3_{network_bw}"
    print(f"new_log_dir: {new_log_dir}")
    sh.cp("-r",f"{log_dir}", f"{run_dir}/{new_log_dir}")

def plot_flows(flow_id, log_dir):
    sh.cd(f"{log_dir}")
    sh.python("/mnt/raid10/hanjing/thesis/ns3/ns3-basic-sim/quick/flow_plot/flow_plot_v2.py","/mnt/raid10/hanjing/thesis/ns3/ns3-basic-sim/quick/flow_plot",f"/mnt/raid10/hanjing/thesis/ns3/ns3-basic-sim/runs/{main_program}/logs_ns3", f"{flow_id}")

def plot_utilization(node_cnt, log_dir,p_type=None):
    utilization_per_node = collections.defaultdict(list)
    timestamp_per_node = collections.defaultdict(list)
    for node_id in range(node_cnt):
        with open(f"{log_dir}/NetworkDevice_{node_id}_utilization.txt", "r") as file:
            line = file.readline()
            line = line.strip("\n")
            while len(line) != 0:
                u, time_stamp = line.split(" ")
                # convert time_stamp to ms
                time_stamp = float(time_stamp)/float(1000000000)
                time_stamp = f"{time_stamp:.2f}"
                utilization_per_node[node_id].append(float(u))
                timestamp_per_node[node_id].append(float(time_stamp))
                line = file.readline()
                line = line.strip("/n")
        _, ax= plt.subplots()
        if p_type == "hist":
            ax.hist(utilization_per_node[node_id], bins=40)
            plt.show(block=True)
        else:
            # normal line plot
            plt.plot(timestamp_per_node[node_id],utilization_per_node[node_id])
            plt.show(block=True)

if __name__ == "__main__":
    # python pfabric_horovod_run.py pfabric_flows_horovod --run_program --plot_flow=0,1,2 --plot_utilization=hist
    parser = argparse.ArgumentParser()
    parser.add_argument("program", help="specify the name of the main program to run")
    parser.add_argument("--run_program", action="store_true", help="execute the simulation specified by the main program")
    parser.add_argument("--plot_utilization", help="plot the utilization of the links", choices=["hist","line"])
    
    args=parser.parse_args()
    program = args.program
    run_dir = f"/mnt/raid10/hanjing/thesis/ns3/ns3-basic-sim/runs/{program}"
    log_dir = f"{run_dir}/logs_ns3"
    config_file = f"{run_dir}/config_ns3.properties"

    # TODO: extract server count from config file
    node_count = 1 
    if args.run_program:
        run_program(program)

    if args.plot_utilization == "hist":
        plot_utilization(node_count, log_dir, "hist" )
    elif args.plot_utilization =="line":
        plot_utilization(node_count, log_dir, "line")

