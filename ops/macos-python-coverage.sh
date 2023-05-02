#!/bin/bash

set -euo pipefail

echo "##[section]Building TL2cgen..."
conda --version
python --version

# Run coverage test
set -x
rm -rf build/
mkdir build
cd build
cmake .. -DTEST_COVERAGE=ON -DBUILD_CPP_TESTS=ON -GNinja
ninja
cd ..

./build/tl2cgen_cpp_test
PYTHONPATH=./python python -m pytest --cov=tl2cgen -v -rxXs \
  --fulltrace --durations=0 tests/python
lcov --directory . --capture --output-file coverage.info
lcov --remove coverage.info '*dmlccore*' --output-file coverage.info
lcov --remove coverage.info '*fmtlib*' --output-file coverage.info
lcov --remove coverage.info '*/usr/*' --output-file coverage.info
lcov --remove coverage.info '*googletest*' --output-file coverage.info
