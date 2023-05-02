#!/bin/bash

set -euo pipefail

TAG=manylinux2014_x86_64

echo "##[section]Building TL2cgen..."
ops/docker/ci_build.sh cpu ops/scripts/build_via_cmake.sh

echo "##[section]Packaging Python wheel for TL2cgen..."
ops/docker/ci_build.sh cpu bash -c "cd python/ && pip wheel --no-deps -v . --wheel-dir dist/"
ops/docker/ci_build.sh auditwheel_x86_64 auditwheel repair --only-plat --plat ${TAG} python/dist/*.whl
rm -v python/dist/*.whl
mv -v wheelhouse/*.whl python/dist/
ops/docker/ci_build.sh cpu python ops/scripts/rename_whl.py python/dist ${COMMIT_ID} ${TAG}
