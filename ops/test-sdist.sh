#!/bin/bash

set -euo pipefail

echo "##[section]Building a source distribution..."
make pippack
python -m build --sdist .

echo "##[section]Testing the source distribution..."
python -m pip install -v tl2cgen-*.tar.gz
python -m pytest -v -rxXs --fulltrace --durations=0 tests/python/test_basic.py

# Deploy source distribution to S3
for file in ./tl2cgen-*.tar.gz
do
  mv "${file}" "${file%.tar.gz}+${COMMIT_ID}.tar.gz"
done
python -m awscli s3 cp tl2cgen-*.tar.gz s3://tl2cgen-wheels/ --acl public-read || true
