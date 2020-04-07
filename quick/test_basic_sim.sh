mkdir -p test-results
cd ../simulator || exit 1
lcov --directory build/debug --zerocounters
python test.py -v -s "basic-sim" -t ../quick/test-results/test-results-basic-sim
