# for num_active_bursts in 50 #, 100, 200, 400 
# do 
#     sed -i '/num_of_active_bursts/d' /mnt/raid10/hanjing/thesis/ns3/ns3-basic-sim/runs/ppbp_horovod/config_ns3.properties
#     echo "num_of_active_bursts=$num_active_bursts" >> /mnt/raid10/hanjing/thesis/ns3/ns3-basic-sim/runs/ppbp_horovod/config_ns3.properties
#     bash /mnt/raid10/hanjing/thesis/ns3/ns3-basic-sim/simulator/run_program.sh ppbp_horovod
# done 

import sh
import collections


main_program = "ppbp_horovod"
run_dir = f"/mnt/raid10/hanjing/thesis/ns3/ns3-basic-sim/runs/{main_program}"
log_dir = f"{run_dir}/logs_ns3"
config_file = f"{run_dir}/config_ns3.properties"
bash_script_path = "/mnt/raid10/hanjing/thesis/ns3/ns3-basic-sim/simulator/run_program.sh"
network_bw = ""
test_cases=collections.defaultdict(list)
test_cases["num_of_active_bursts"] = [200]
horovod_prg = "HorovodWorker_0_layer_50_port_1024_progress.txt"
for config_name, tests in test_cases.items():
    print(f"{config_name} Test:")
    # for new_config in test_cases["num_of_active_bursts"]:
    for new_config in tests:
        with open(config_file, "r+") as f:
            d = f.readlines()
            f.seek(0)
            for i in d:
                print(f"{i}")
                if i.find("link_data_rate_megabit_per_s") != -1:
                    network_bw = i.split("=")[1].strip("\n")+"Mbits"
                if i.find("num_of_active_bursts") == -1:
                    f.write(i)
                else:
                    f.write(f"num_of_active_bursts={new_config}")
            f.truncate()
        sh.cp("/mnt/raid10/hanjing/thesis/ns3/ns3-basic-sim/simulator/run.sh", "/mnt/raid10/hanjing/thesis/ns3/ns3-basic-sim/simulator/run.sh.bak")
        # sh.sed( "-i", f"s/program/{main_program}/g", "/mnt/raid10/hanjing/thesis/ns3/ns3-basic-sim/simulator/run.sh.bak", _err="sed_err_log", _out="sed_out_log")
        sh.sed( "-i", f"s/program/{main_program}/g", "/mnt/raid10/hanjing/thesis/ns3/ns3-basic-sim/simulator/run.sh.bak")
        new_log_dir = f"logs_ns3_{network_bw}_ActiveBurstsCt_{new_config}"
        print(f"new_log_dir: {new_log_dir}")
        sh.bash("/mnt/raid10/hanjing/thesis/ns3/ns3-basic-sim/simulator/run.sh.bak")
        sh.python("/mnt/raid10/hanjing/thesis/ns3/ns3-basic-sim/simulator/src/basic-apps/helper/horovod_worker_plot.py", f"{log_dir}/{horovod_prg}")
        # sh.cp("-r",f"{log_dir}", f"{run_dir}/{new_log_dir}")