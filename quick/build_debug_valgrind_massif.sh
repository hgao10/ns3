cd ../simulator || exit 1
# Configure it only with basis to quickly check what is consuming memory
./waf configure --out=build/debug
./waf -j4
