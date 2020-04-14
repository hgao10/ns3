cd ../simulator || exit 1

for experiment in "flows_example_single" "flows_example_single_many_small_flows" "flows_example_single_one_large_flow" \
                  "flows_example_ring" "flows_example_leaf_spine" "flows_example_leaf_spine_servers" "flows_example_fat_tree_k4_servers" \
                  "flows_example_big_grid" "flows_example_big_grid_one_flow"  #  Too long in debug: "flows_example_single_one_10g"
do
  rm -rf ../runs/${experiment}/logs_ns3
  mkdir ../runs/${experiment}/logs_ns3
  ./waf --run="main_flows --run_dir='../runs/${experiment}'" 2>&1 | tee ../runs/${experiment}/logs_ns3/console.txt
done

for experiment in "pingmesh_example_grid" "pingmesh_example_single"
do
  rm -rf ../runs/${experiment}/logs_ns3
  mkdir ../runs/${experiment}/logs_ns3
  ./waf --run="main_pingmesh --run_dir='../runs/${experiment}'" 2>&1 | tee ../runs/${experiment}/logs_ns3/console.txt
done
