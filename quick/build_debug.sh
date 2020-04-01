cd ../simulator || exit 1
./waf configure --enable-gcov --enable-examples --enable-tests --out=build/debug
./waf -j4
