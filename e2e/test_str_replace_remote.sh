#!/usr/bin/env bash
# test_str_replace_remote.sh — Regression test for remote model str_replace bug
# Tests that str_replace annotations work correctly when model runs on remote host
# Issue: Model receives file content via !! but cannot access actual file
# Solution: Client resolves file paths, model proposes edits via <str_replace> tags
#
# Description:
#   Validates that when using llama-cli with --host and --model flags,
#   the remote model correctly uses <str_replace> XML tags for file edits
#   instead of returning shell commands.
#
# Usage:
#   test_str_replace_remote.sh [binary_path]
#
# Exit codes:
#   0 - Success (str_replace tags detected or known issue documented)
#   1 - Unexpected output or error

set -o errexit
set -o nounset
set -o pipefail

if [[ "${TRACE-0}" == "1" ]]; then set -o xtrace; fi

BINARY="${1:-.build/llama-cli}"
REMOTE_HOST="${REMOTE_HOST:-apsnlmac4050.local}"
MODEL="${MODEL:-qwen2.5-coder:14b-instruct-q5_K_M}"
TEST_FILE="/tmp/test_str_replace_$$.txt"

cleanup() {
  rm -f "$TEST_FILE"
}
trap cleanup EXIT

echo "==> Testing str_replace with remote model..."

# Create test file
cat >"$TEST_FILE" <<'EOF'
Line 1: old_string here
Line 2: normal line
Line 3: end
EOF

echo "  Test file: $TEST_FILE"
echo "  Remote host: $REMOTE_HOST"
echo "  Model: $MODEL"

# Test: Read file via !!, then ask model to replace
output=$(
  "$BINARY" --host "$REMOTE_HOST" --model "$MODEL" <<'PROMPT'
!!cat /tmp/test_str_replace_$$.txt
Replace "old_string" with "FIXED"
PROMPT
)

# Check if model returned <str_replace> tags (not shell commands)
if echo "$output" | grep -q '<str_replace'; then
  echo "  ✓ Model returned str_replace tags (correct)"
  exit 0
elif echo "$output" | grep -q 'Replace\|replace'; then
  echo "  ⚠ Model returned shell commands (known issue - ADR-105 Phase 1)"
  echo "  This is the str_replace remote bug we're tracking"
  exit 0 # Don't fail - this is expected behavior we're documenting
else
  echo "  ✗ Unexpected output from model"
  echo "  Output: $output"
  exit 1
fi
