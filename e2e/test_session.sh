#!/usr/bin/env bash
#
# test_session.sh — E2E tests for --session, --capabilities, and --sandbox.
#
# Tests the stateful sync mode (ADR-056) using mock provider.
# Mock echoes the prompt back as the response, so annotation tags
# in the prompt are returned and processed by the capabilities engine.
#
# Usage:
#   bash e2e/test_session.sh [path-to-binary]

set -o errexit
set -o nounset
set -o pipefail
if [[ "${TRACE-0}" == "1" ]]; then set -o xtrace; fi

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "$SCRIPT_DIR/helpers.sh"

BINARY="${1:-./build/llama-cli}"
TMPDIR_TEST=$(mktemp -d)
trap 'rm -rf "$TMPDIR_TEST"' EXIT

echo "=== E2E: session + capabilities + sandbox ==="

check_binary "$BINARY"

# --- Session tests ---

echo "Testing: session file creation..."
SESSION="$TMPDIR_TEST/session.json"
OUTPUT=$("$BINARY" --provider=mock --session="$SESSION" "hello world" 2>&1)
assert_contains "$OUTPUT" "hello world" "session sync response"
test -f "$SESSION" || die "session file not created"
assert_contains "$(cat "$SESSION")" '"role":"user"' "session has user message"
assert_contains "$(cat "$SESSION")" '"role":"assistant"' "session has assistant message"
echo "PASS: session file creation"

echo "Testing: session follow-up preserves history..."
OUTPUT=$("$BINARY" --provider=mock --session="$SESSION" "follow up" 2>&1)
# Session should now have 2 user + 2 assistant messages
COUNT=$(grep -c '"role":"user"' "$SESSION")
test "$COUNT" -eq 2 || die "expected 2 user messages, got $COUNT"
COUNT=$(grep -c '"role":"assistant"' "$SESSION")
test "$COUNT" -eq 2 || die "expected 2 assistant messages, got $COUNT"
echo "PASS: session follow-up preserves history"

# --- Capabilities: read ---

echo "Testing: capabilities=read processes <read> tags..."
SESSION="$TMPDIR_TEST/cap-read.json"
echo "test-content-123" > "$TMPDIR_TEST/testfile.txt"
# Mock echoes prompt back — the <read> tag in the response triggers file read
OUTPUT=$("$BINARY" --provider=mock --session="$SESSION" \
  --capabilities=read --sandbox="$TMPDIR_TEST" \
  '<read path="'"$TMPDIR_TEST"'/testfile.txt"/>' 2>&1)
assert_contains "$OUTPUT" "test-content-123" "read capability returned file content"
echo "PASS: capabilities=read processes <read> tags"

# --- Capabilities: exec (read-only) ---

echo "Testing: capabilities=read allows safe exec..."
SESSION="$TMPDIR_TEST/cap-exec.json"
OUTPUT=$("$BINARY" --provider=mock --session="$SESSION" \
  --capabilities=read --sandbox="$TMPDIR_TEST" \
  '<exec>cat '"$TMPDIR_TEST"'/testfile.txt</exec>' 2>&1)
assert_contains "$OUTPUT" "test-content-123" "safe exec returned file content"
echo "PASS: capabilities=read allows safe exec"

echo "Testing: capabilities=read blocks dangerous exec..."
SESSION="$TMPDIR_TEST/cap-block.json"
OUTPUT=$("$BINARY" --provider=mock --session="$SESSION" \
  --capabilities=read --sandbox="$TMPDIR_TEST" \
  '<exec>rm '"$TMPDIR_TEST"'/testfile.txt</exec>' 2>&1)
assert_contains "$OUTPUT" "blocked" "dangerous exec was blocked"
test -f "$TMPDIR_TEST/testfile.txt" || die "file was deleted despite read-only mode"
echo "PASS: capabilities=read blocks dangerous exec"

# --- Sandbox ---

echo "Testing: sandbox blocks read outside directory..."
SESSION="$TMPDIR_TEST/sandbox.json"
OUTPUT=$("$BINARY" --provider=mock --session="$SESSION" \
  --capabilities=read --sandbox="$TMPDIR_TEST" \
  '<read path="/etc/hostname"/>' 2>&1)
assert_contains "$OUTPUT" "blocked" "read outside sandbox was blocked"
echo "PASS: sandbox blocks read outside directory"

echo "Testing: sandbox blocks write outside directory..."
SESSION="$TMPDIR_TEST/sandbox-w.json"
OUTSIDE="$TMPDIR_TEST/../outside-sandbox.txt"
OUTPUT=$("$BINARY" --provider=mock --session="$SESSION" \
  --capabilities=read,write --sandbox="$TMPDIR_TEST" \
  '<write file="'"$OUTSIDE"'">pwned</write>' 2>&1)
assert_contains "$OUTPUT" "blocked" "write outside sandbox was blocked"
test ! -f "$OUTSIDE" || die "file was written outside sandbox"
echo "PASS: sandbox blocks write outside directory"

# --- No capabilities = raw passthrough ---

echo "Testing: no capabilities passes annotations through..."
SESSION="$TMPDIR_TEST/nocap.json"
OUTPUT=$("$BINARY" --provider=mock --session="$SESSION" \
  '<exec>echo hi</exec>' 2>&1)
# Without capabilities, the <exec> tag should appear in output as-is
assert_contains "$OUTPUT" "<exec>" "annotations passed through without capabilities"
echo "PASS: no capabilities passes annotations through"

echo "=== All session tests passed ==="
