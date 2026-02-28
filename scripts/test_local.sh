#!/bin/bash
# Test script for running Unity tests locally

set -e

mkdir -p .pio/testing

echo "Running tests and generating summary..."
# Run tests and capture output. 
# We use --json-output and redirect to the expected summary file.
pio test -e native_test --json-output > .pio/testing/last_summary.json

echo ""
echo "Tests completed. Summary saved to .pio/testing/last_summary.json"
# Also output the JSON so it's visible in the logs
cat .pio/testing/last_summary.json
