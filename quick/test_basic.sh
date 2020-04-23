mkdir -p test_results
rm -f test_results/test_results_basic_sim.txt
rm -f test_results/test_results_basic_apps.txt
cd ../simulator || exit 1
lcov --directory build/debug_all --zerocounters
python test.py -v -s "basic-sim" -t ../quick/test_results/test_results_basic_sim
python test.py -v -s "basic-apps" -t ../quick/test_results/test_results_basic_apps
cat ../quick/test_results/test_results_basic_sim.txt
cat ../quick/test_results/test_results_basic_apps.txt
