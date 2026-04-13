#!/bin/bash
# E2E test: --files flag
# Usage: ./e2e/test_files_flag.sh [path-to-binary]

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "$SCRIPT_DIR/helpers.sh"

BINARY="${1:-./build/llama-cli}"
TEMP_FILE=$(mktemp)

echo "=== E2E: --files flag ==="

# Ensure cleanup even on failure
trap 'rm -f "$TEMP_FILE"' EXIT

check_binary "$BINARY"

# Create test file
echo "The answer to life, the universe, and everything is 42." > "$TEMP_FILE"

# Run with --files
OUTPUT=$("$BINARY" --files="$TEMP_FILE" "what is the answer?" 2>&1) || die "binary exited with error"

assert_nonempty "$OUTPUT" "--files response"
assert_contains "$OUTPUT" "42" "file content"

echo "PASS: got response (${#OUTPUT} chars)"
