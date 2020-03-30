cd ../simulator || exit 1
rm -rf ../runs/example_single/logs
mkdir ../runs/example_single/logs
 ./waf --command-template="valgrind --time-unit=ms --tool=massif %s --run_dir='../runs/example_single'" --run "main" 2>&1 | tee ../runs/example_single/logs/console.txt
