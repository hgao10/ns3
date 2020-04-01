cd ../simulator || exit 1
./waf configure --build-profile=optimized --out=build/optimized
./waf -j4
