#!/bin/bash

set -euo pipefail

echo "##[section]Running integration tests for Java runtime (tl2cgen4j)..."
cd java_runtime/tl2cgen4j
mvn --batch-mode test
