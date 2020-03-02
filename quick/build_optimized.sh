bash extract_and_copy.sh
cd ../simulator || exit 1
./waf configure --build-profile=optimized
./waf
