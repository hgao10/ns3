cd /mnt/raid10/hanjing/thesis/ns3/ns3-basic-sim/simulator
./waf --run="main_mixed_flows --run_dir='/mnt/raid10/hanjing/thesis/ns3/ns3-basic-sim/runs/flows_example_mixed_flows'"
cd ../runs/flows_example_mixed_flows/logs_ns3
python ../../../quick/flow_plot/flow_plot_v2.py ../../../quick/flow_plot/ ./ 0
python /mnt/raid10/hanjing/thesis/ns3/ns3-basic-sim/simulator/src/basic-apps/helper/horovod_worker_plot.py HorovodWorker_0_layer_50_progress.txt