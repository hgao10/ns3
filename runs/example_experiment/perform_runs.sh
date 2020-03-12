cd ../../simulator || exit 1

for arrival_rate in 10 50 100 150 200 250 300 350 400
do
  folder_name="../runs/example_experiment/runs/run_perfect_schedule_${arrival_rate}"
  rm -rf ${folder_name}/logs
  mkdir ${folder_name}/logs
  ./waf --run="main --run_dir='${folder_name}'" 2>&1 | tee ${folder_name}/logs/console.txt
  for seed in 0 1 2 3 4
  do
    folder_name="../runs/example_experiment/runs/run_random_schedule_s${seed}_${arrival_rate}"
    rm -rf ${folder_name}/logs
    mkdir ${folder_name}/logs
    ./waf --run="main --run_dir='${folder_name}'" 2>&1 | tee ${folder_name}/logs/console.txt
	done
done
