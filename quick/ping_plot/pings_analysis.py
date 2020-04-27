import numpy as np
import sys
from read_pingmesh_csv import read_pingmesh_csv


def ping_plot(logs_ns3_dir):

    # Read in the rtt_results
    rtt_results = read_pingmesh_csv(logs_ns3_dir)

    # Print the top 10 with biggest delta between minimum and maximum RTT
    deltas = []
    for from_to in rtt_results:
        rtts_ns = []
        for val in rtt_results[from_to]:
            if not val[0]:
                rtts_ns.append(val[2])
        if len(rtts_ns) > 0:
            deltas.append((np.max(rtts_ns) - np.min(rtts_ns), from_to, np.min(rtts_ns), np.max(rtts_ns)))
        else:
            print("No RTT results for: " + str(from_to))
    deltas = sorted(deltas, reverse=True)
    print("TOP 25: WORST GAP MINIMUM AND MAXIMUM RTT")
    print("---------")
    print("From    To      Largest delta (ms)  Min. RTT (ms)       Max. RTT (ms)")
    for i in range(min(len(deltas), 25)):
        print("%-8d%-8d%-20.2f%-20.2f%-20.2f"
              % (
                  deltas[i][1][0],  # From
                  deltas[i][1][1],  # To
                  deltas[i][0] / 1000000.0,  # Largest delta (ms)
                  deltas[i][2] / 1000000.0,  # Min. RTT (ms)
                  deltas[i][3] / 1000000.0   # Max. RTT (ms)
              ))


if __name__ == "__main__":
    args = sys.argv[1:]
    if len(args) != 1:
        print("Must supply exactly one argument")
        print("Usage: python ping_plot.py [logs_ns3 directory]")
        exit(1)
    else:
        ping_plot(
            args[0]
        )
