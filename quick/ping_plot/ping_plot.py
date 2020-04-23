import numpy as np
import sys
from exputil import *
from read_pingmesh_csv import read_pingmesh_csv


def ping_plot(ping_plot_dir, logs_ns3_dir, from_id, to_id):

    # Read in the rtt_results
    rtt_results = read_pingmesh_csv(logs_ns3_dir)

    # Write results to temporary input file
    if (from_id, to_id) in rtt_results:
        rtts = rtt_results[(from_id, to_id)]
        with open(ping_plot_dir + "/temp_input.txt", "w+") as out_file:
            for val in rtts:
                if val[0]:
                    out_file.write(str(val[1]) + ",-100000000\n")
                else:
                    out_file.write(str(val[1]) + "," + str(val[2]) + "\n")
    else:
        raise ValueError("(%d, %d) is not in the results" % (from_id, to_id))

    # And perform the plotting
    local_shell = LocalShell()
    out_filename = logs_ns3_dir.replace("/", "\\/") + "\\/plot_ping_rtt_" + str(from_id) + "_to_" + str(to_id) + ".pdf"
    data_filename = (ping_plot_dir + "/temp_input.txt").replace("/", "\\/")
    local_shell.copy_file(ping_plot_dir + "/plot_ping_rtt.plt", ping_plot_dir + "/temp.plt")
    local_shell.perfect_exec("sed -i 's/\\[OUTPUT\\-FILE\\]/" + out_filename + "/g' " + ping_plot_dir + "/temp.plt")
    local_shell.perfect_exec("sed -i 's/\\[DATA\\-FILE\\]/" + data_filename + "/g' " + ping_plot_dir + "/temp.plt")
    local_shell.perfect_exec("gnuplot " + ping_plot_dir + "/temp.plt")
    print("Produced plot: " + out_filename.replace("\\", ""))
    local_shell.remove(ping_plot_dir + "/temp.plt")
    local_shell.remove(ping_plot_dir + "/temp_input.txt")


if __name__ == "__main__":
    args = sys.argv[1:]
    if len(args) != 4:
        print("Must supply exactly four arguments")
        print("Usage: python ping_plot.py [ping_plot directory] [logs_ns3 directory] [from_id] [to_id]")
        exit(1)
    else:
        ping_plot(
            args[0],
            args[1],
            int(args[2]),
            int(args[3])
        )
