cd ../simulator || exit 1
# Configure it only with basis to quickly prototype
./waf configure --out=build/debug_minimal
./waf -j4
