#!/bin/bash
# Test script for running Unity tests locally

set -e

echo "Running Display HAL contract tests..."
pio test -e native_test -v

echo ""
echo "All tests passed!"
