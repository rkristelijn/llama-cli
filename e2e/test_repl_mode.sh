#!/bin/bash
# E2E test: REPL mode (interactive)
# Usage: ./e2e/test_repl_mode.sh [path-to-binary]

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "$SCRIPT_DIR/helpers.sh"

BINARY="${1:-./build/llama-cli}"

echo "=== E2E: REPL mode ==="

check_binary "$BINARY"

# Test REPL starts, responds, and exits cleanly
# Using mock provider for deterministic E2E test
OUTPUT=$(echo -e "hello\nexit" | LLAMA_PROVIDER=mock "$BINARY" 2>&1) || die "binary exited with error"
assert_nonempty "$OUTPUT" "REPL response"
assert_contains "$OUTPUT" "hello" "greeting"
assert_contains "$OUTPUT" "[MOCK MODE]" "mock indicator"

echo "PASS: REPL started and exited (${#OUTPUT} chars output)"
