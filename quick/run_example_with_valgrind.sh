cd ../simulator || exit 1

# Heap usage analysis
rm -rf ../runs/example_big_grid_one_flow/logs
mkdir ../runs/example_big_grid_one_flow/logs
./waf --command-template="valgrind --time-unit=ms --max-snapshots=500 --tool=massif %s --run_dir='../runs/example_big_grid_one_flow'" --run "main" 2>&1 | tee ../runs/example_big_grid_one_flow/logs/console.txt

# Memory leak check
rm -rf ../runs/example_single/logs
mkdir ../runs/example_single/logs
./waf --command-template="valgrind %s --run_dir='../runs/example_single'" --run "main" 2>&1 | tee ../runs/example_single/logs/console.txt
