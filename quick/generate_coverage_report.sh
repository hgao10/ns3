rmdir -r coverage-report
mkdir -p coverage-report
cd ../simulator/build/debug_all/ || exit 1
lcov --capture --directory src/basic-sim --output-file ../../../quick/coverage-report/coverage.info
cd ../../../quick || exit 1
genhtml --output-directory coverage-report coverage-report/coverage.info
echo "Coverage report is located at: coverage-report/index.html"
if [ "$1" == "--open" ]; then
  echo "Opening it for convenience"
  xdg-open coverage-report/index.html
fi
