#!/bin/bash
# E2E test: sync mode (one-shot prompt)
# Usage: ./e2e/test_sync_mode.sh [path-to-binary]

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "$SCRIPT_DIR/helpers.sh"

BINARY="${1:-./build/llama-cli}"

echo "=== E2E: sync mode ==="

check_binary "$BINARY"

# Array of prompts and expected responses (Bash 3 compatible)
PROMPTS=("what is 2 plus 2?" "hello")
EXPECTED=("2" "hello")

for i in "${!PROMPTS[@]}"; do
    prompt="${PROMPTS[$i]}"
    expected="${EXPECTED[$i]}"
    
    echo "Testing: '$prompt'..."
    OUTPUT=$("$BINARY" "$prompt" 2>&1)
    
    assert_nonempty "$OUTPUT" "$prompt"
    assert_contains "$OUTPUT" "$expected" "$prompt"
    echo "PASS: '$prompt' -> contains '$expected'"
done
