#!/bin/bash
# E2E test: confirmation feedback — decline with reason retries LLM
# When user declines a proposed action with a reason, the LLM retries.
# Uses mock provider with scripted responses keyed by prompt content.
# Usage: ./e2e/test_confirmation_feedback.sh [path-to-binary]

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "$SCRIPT_DIR/helpers.sh"

BINARY="${1:-./build/llama-cli}"
check_binary "$BINARY"

echo "=== E2E: confirmation feedback ==="

TMP_FILE=$(mktemp)
MOCK_SCRIPT=$(mktemp)
trap 'rm -f "$TMP_FILE" "$TMP_FILE.bak" "$MOCK_SCRIPT"' EXIT

# Mock script: first response proposes a write, second (after decline feedback)
# responds with acknowledgment text containing "noted"
cat >"$MOCK_SCRIPT" <<'EOF'
[fix]
<write file="TMPFILE">bad content using spaces</write>
---
[declined]
I see, you want tabs instead. Let me fix that.
<write file="TMPFILE">bad content using tabs</write>
---
[default]
ok
EOF

# Replace TMPFILE placeholder with actual path
sed -i '' "s|TMPFILE|$TMP_FILE|g" "$MOCK_SCRIPT"

# --- test 1: decline with reason triggers retry ---
echo "--- test 1: decline with reason triggers retry ---"

rm -f "$TMP_FILE"
OUTPUT=$(printf 'fix the file\nn, use tabs instead\ny\nexit\n' |
  LLAMA_CLI_MOCK_SCRIPT="$MOCK_SCRIPT" LLAMA_PROVIDER=mock \
    "$BINARY" --repl 2>&1)

assert_contains "$OUTPUT" "[skipped]" "first write was skipped"
assert_contains "$OUTPUT" "[wrote" "second write was confirmed"
assert_contains "$OUTPUT" "tabs" "retry response mentions tabs"
echo "PASS: decline with reason triggers retry"

# --- test 2: bare decline does NOT trigger retry ---
echo "--- test 2: bare decline does not retry ---"

rm -f "$TMP_FILE"
OUTPUT=$(printf 'fix the file\nn\nexit\n' |
  LLAMA_CLI_MOCK_SCRIPT="$MOCK_SCRIPT" LLAMA_PROVIDER=mock \
    "$BINARY" --repl 2>&1)

assert_contains "$OUTPUT" "[skipped]" "write was skipped"
# Should NOT see a second write proposal (no retry without reason)
if printf '%s' "$OUTPUT" | grep -c "\[wrote" >/dev/null 2>&1; then
  # grep -c returns 0 if no match, which is what we want
  COUNT=$(printf '%s' "$OUTPUT" | grep -c "tabs" || true)
  if [ "$COUNT" -gt 0 ]; then
    die "bare decline should not trigger retry with tabs response"
  fi
fi
echo "PASS: bare decline does not retry"

echo ""
echo "=== All confirmation feedback tests passed ==="
