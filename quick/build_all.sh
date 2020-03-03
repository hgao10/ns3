bash extract_and_copy.sh
cd ../simulator || exit 1
./waf configure --enable-gcov --enable-examples --enable-tests
./waf -j4
