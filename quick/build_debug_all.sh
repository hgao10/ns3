cd ../simulator || exit 1
# Configure it with all the code coverage, all the examples, all the tests
./waf configure --enable-gcov --enable-examples --enable-tests --out=build/debug
./waf -j4
# To show all NS-3 logs:  NS_LOG="*=*|all" ./waf --run="main --run_dir='../runs/example_single'" &> a.txt
