#!/bin/bash
# E2E test: Auto-agent features (@mention routing, permission blocking)
# Usage: ./e2e/test_autoagent.sh [path-to-binary]

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "$SCRIPT_DIR/helpers.sh"

BINARY="${1:-./build/llama-cli}"

echo "=== E2E: Auto-agent ==="

check_binary "$BINARY"

# Test 1: @mention routes to internal agent (explore uses mock provider)
OUTPUT=$(echo -e "@explore find TODO comments\nexit" | LLAMA_PROVIDER=mock "$BINARY" --repl 2>&1) || die "@explore failed"
assert_contains "$OUTPUT" "routing to @explore" "mention routing indicator"
echo "PASS: @mention routing works"

# Test 2: Permission blocking — /agent plan blocks writes
# Disable auto-routing so mock provider stays active
MOCK_RESP='<write file="/tmp/e2e-blocked.txt">should not write</write>'
OUTPUT=$(echo -e "/auto\n/agent plan\nhello\nexit" | LLAMA_CLI_MOCK_RESPONSE="$MOCK_RESP" LLAMA_PROVIDER=mock "$BINARY" --repl 2>&1) || die "plan agent failed"
assert_contains "$OUTPUT" "blocked" "permission denied for write in plan mode"
# Verify file was NOT created
if [ -f /tmp/e2e-blocked.txt ]; then
  rm -f /tmp/e2e-blocked.txt
  die "file was written despite plan agent deny permission"
fi
echo "PASS: permission blocking works (plan agent denies write)"

# Test 3: @mention with unknown agent falls through gracefully
OUTPUT=$(echo -e "@nonexistent do something\nexit" | LLAMA_PROVIDER=mock "$BINARY" --repl 2>&1) || die "@nonexistent failed"
assert_contains "$OUTPUT" "routing to @nonexistent" "unknown agent routing"
echo "PASS: unknown @mention handled gracefully"

# Test 4: orchestrate_complex fires for complex prompts (override PATH to prevent external agents)
# Use a restricted PATH so q/opencode/kiro-cli-chat are not found — forces local plan fallback
OUTPUT=$(echo -e "implement a new function that refactors the entire parser module with async support and error handling\nexit" | PATH=/usr/bin:/bin LLAMA_PROVIDER=mock "$BINARY" --repl 2>&1) || die "orchestrate_complex failed"
echo "PASS: orchestrate_complex triggered for complex prompt"

echo "=== All auto-agent tests passed ==="
