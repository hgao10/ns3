cd ../simulator || exit 1
# Configure it with all the code coverage, all the examples, all the tests
./waf configure --enable-gcov --enable-examples --enable-tests --out=build/debug_all
./waf -j4
