cd ../simulator || exit 1
rm -rf ../runs/example_single/logs
mkdir ../runs/example_single/logs
./waf --run="main --run_dir='../runs/example_single'" 2>&1 | tee ../runs/example_single/logs/console.txt
rm -rf ../runs/example_ring/logs
mkdir ../runs/example_ring/logs
./waf --run="main --run_dir='../runs/example_ring'" 2>&1 | tee ../runs/example_ring/logs/console.txt
rm -rf ../runs/example_leaf_spine/logs
mkdir ../runs/example_leaf_spine/logs
./waf --run="main --run_dir='../runs/example_leaf_spine'" 2>&1 | tee ../runs/example_leaf_spine/logs/console.txt
rm -rf ../runs/example_leaf_spine_servers/logs
mkdir ../runs/example_leaf_spine_servers/logs
./waf --run="main --run_dir='../runs/example_leaf_spine_servers'" 2>&1 | tee ../runs/example_leaf_spine_servers/logs/console.txt
rm -rf ../runs/example_fat_tree_k4_servers/logs
mkdir ../runs/example_fat_tree_k4_servers/logs
./waf --run="main --run_dir='../runs/example_fat_tree_k4_servers'" 2>&1 | tee ../runs/example_fat_tree_k4_servers/logs/console.txt
