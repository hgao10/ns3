cd ../simulator || exit 1

for experiment in "example_single" "example_single_many_small_flows" "example_single_one_large_flow" \
                  "example_ring" "example_leaf_spine" "example_leaf_spine_servers" "example_fat_tree_k4_servers" \
                  "example_big_grid" "example_big_grid_one_flow"  #  Too long in debug: "example_single_one_10g"
do
  rm -rf ../runs/${experiment}/logs_ns3
  mkdir ../runs/${experiment}/logs_ns3
  ./waf --run="main --run_dir='../runs/${experiment}'" 2>&1 | tee ../runs/${experiment}/logs_ns3/console.txt
done
