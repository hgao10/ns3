rmdir -r coverage_report
mkdir -p coverage_report
cd ../simulator/build/debug_all/ || exit 1
lcov --capture --directory src/basic-sim --directory src/basic-apps --output-file ../../../quick/coverage_report/coverage.info
cd ../../../quick || exit 1
genhtml --output-directory coverage_report coverage_report/coverage.info
echo "Coverage report is located at: coverage_report/index.html"
if [ "$1" == "--open" ]; then
  echo "Opening it for convenience"
  xdg-open coverage_report/index.html
fi
