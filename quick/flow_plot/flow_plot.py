import sys
from exputil import *


def flow_plot(flow_plot_dir, logs_ns3_dir, flow_id):

    # Calculate rate in 10ms windows
    interval_ns = 10000000  # 10ms
    with open(logs_ns3_dir + "/flow_" + str(flow_id) + "_rate.txt", "w+") as f_rate:
        with open(logs_ns3_dir + "/flow_" + str(flow_id) + "_progress.txt", "r") as f_in:
            last_update_ns = 0
            last_progress_byte = 0
            for line in f_in:
                spl = line.split(",")
                line_flow_id = int(spl[0])
                line_time_ns = int(spl[1])
                line_progress_byte = int(spl[2])
                if line_time_ns > last_update_ns + interval_ns:
                    f_rate.write("%d,%d,%.2f\n" % (
                        line_flow_id,
                        last_update_ns + 0.00001,
                        float(line_progress_byte - last_progress_byte) / float(line_time_ns - last_update_ns) * 8000.0)
                    )
                    f_rate.write("%d,%d,%.2f\n" % (
                        line_flow_id,
                        line_time_ns,
                        float(line_progress_byte - last_progress_byte) / float(line_time_ns - last_update_ns) * 8000.0)
                                 )
                    last_update_ns = line_time_ns
                    last_progress_byte = line_progress_byte

    # Perform the plots
    local_shell = LocalShell()
    logs_ns3_dir = logs_ns3_dir.replace("/", "\\/")
    for t in ["cwnd", "progress", "rtt", "rate"]:
        out_filename = logs_ns3_dir + "\\/plot_flow_" + str(flow_id) + "_" + t + ".pdf"
        data_filename = logs_ns3_dir + "\\/flow_" + str(flow_id) + "_" + t + ".txt"
        local_shell.copy_file(flow_plot_dir + "/plot_" + t + ".plt", flow_plot_dir + "/temp.plt")
        local_shell.perfect_exec("sed -i 's/\\[OUTPUT\\-FILE\\]/" + out_filename + "/g' " + flow_plot_dir + "/temp.plt")
        local_shell.perfect_exec("sed -i 's/\\[DATA\\-FILE\\]/" + data_filename + "/g' " + flow_plot_dir + "/temp.plt")
        local_shell.perfect_exec("gnuplot " + flow_plot_dir + "/temp.plt")
        local_shell.remove(flow_plot_dir + "/temp.plt")


if __name__ == "__main__":
    args = sys.argv[1:]
    if len(args) != 3:
        print("Must supply exactly three arguments")
        print("Usage: python flow_plot.py [flow-plot directory] [logs_ns3 directory] [flow-id]")
        exit(1)
    else:
        flow_plot(
            args[0],
            args[1],
            int(args[2])
        )
