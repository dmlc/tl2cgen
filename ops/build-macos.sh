#!/bin/bash

set -euo pipefail

echo "##[section]Building MacOS Python wheels..."
ops/scripts/build_macos_python_wheels.sh ${CIBW_PLATFORM_ID} ${COMMIT_ID}

echo "##[section]Uploading MacOS Python wheels to S3..."
python -m awscli s3 cp wheelhouse/tl2cgen-*.whl s3://tl2cgen-wheels/ --acl public-read || true
