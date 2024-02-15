#!/bin/bash

set -euo pipefail

echo "##[section]Installing lcov and Ninja..."
sudo apt-get install lcov ninja-build

echo "##[section]Building TL2cgen..."
mkdir build/
cd build/
cmake .. -DTEST_COVERAGE=ON -DCMAKE_BUILD_TYPE=Debug -DBUILD_CPP_TEST=ON -GNinja
ninja
cd ..

echo "##[section]Running Google C++ tests..."
./build/tl2cgen_cpp_test

echo "##[section]Running Python integration tests..."
export PYTHONPATH='./python'
python -m pytest --cov=tl2cgen -v -rxXs --fulltrace --durations=0 tests/python

echo "##[section]Collecting coverage data..."
lcov --directory . --capture --output-file coverage.info
lcov --remove coverage.info '*/usr/*' --output-file coverage.info
lcov --remove coverage.info '*/build/_deps/*' --output-file coverage.info
