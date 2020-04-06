flow_size_byte=100000
runtime_s=10
num_nodes=4
seeds=(123456789 987654321 543212345 111111111 88888888)

mkdir -p runs
for arrival_rate in 10 50 100 150 200 250 300 350 400 450 500
do
  folder_name="runs/run_perfect_schedule_${arrival_rate}"
  mkdir -p ${folder_name}
  python schedule_generator_perfect.py ${folder_name}/schedule.csv ${arrival_rate} ${flow_size_byte} ${runtime_s} ${num_nodes}
  cp config_ns3.properties ${folder_name}
  cp topology_tor_with_4_servers.properties ${folder_name}
  for seed in 0 1 2 3 4
  do
    folder_name="runs/run_random_schedule_s${seed}_${arrival_rate}"
    mkdir -p ${folder_name}
    python schedule_generator_random.py ${folder_name}/schedule.csv ${arrival_rate} ${flow_size_byte} ${runtime_s} ${num_nodes} "${seeds[seed]}"
    cp config_ns3.properties ${folder_name}
    cp topology_tor_with_4_servers.properties ${folder_name}
	done
done
