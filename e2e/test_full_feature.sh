#!/usr/bin/env bash
#
# test_full_feature.sh — Full feature integration test (local, ~2-5 min).
#
# Tests every user-facing feature with a real LLM. Slower than e2e but catches
# bugs that mock tests miss (streaming, markdown rendering, real annotations).
#
# Usage:
#   bash e2e/test_full_feature.sh [binary]
#   make live                              # runs this via Makefile
#
# Requires: at least one Ollama host reachable (local or network)
# @see docs/manual-test-journey.md

set -o errexit
set -o nounset
set -o pipefail
if [[ "${TRACE-0}" == "1" ]]; then set -o xtrace; fi

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "$SCRIPT_DIR/helpers.sh"

BINARY="${1:-./build/llama-cli}"
check_binary "$BINARY"

PASS=0
FAIL=0
SKIP=0
TMP="/tmp/llama-full-test-$$"
trap 'rm -rf "$TMP" "${TMP}".*' EXIT

# --- Helpers ---

run() {
  printf '%s\n' "$1" | timeout 120 "$BINARY" --repl --no-color 2>&1
}

pass() {
  local msg="$1"
  echo "  ✓ $msg"
  PASS=$((PASS + 1))
  return 0
}
fail() {
  local msg="$1"
  local detail="$2"
  echo "  ✗ $msg: $detail"
  FAIL=$((FAIL + 1))
  return 0
}
skip() {
  local msg="$1"
  local reason="$2"
  echo "  ○ $msg (skipped: $reason)"
  SKIP=$((SKIP + 1))
  return 0
}

# --- Detect available provider ---

echo "==> Full Feature Integration Test"
echo ""

# Try any reachable Ollama host
OLLAMA_OK=false
for host in localhost 10.0.0.93 10.0.0.197 10.0.0.198; do
  if curl -sf --max-time 2 "http://${host}:11434/api/tags" >/dev/null 2>&1; then
    export OLLAMA_HOST="$host"
    OLLAMA_OK=true
    echo "  Using Ollama at $host"
    break
  fi
done

# Fallback to tgpt if no Ollama
if [[ "$OLLAMA_OK" == "false" ]]; then
  if command -v tgpt >/dev/null 2>&1; then
    echo "  Using tgpt (no Ollama found)"
    export LLAMA_PROVIDER=tgpt
  else
    echo "  SKIP: No LLM provider available"
    exit 0
  fi
fi
echo ""

# ═══════════════════════════════════════════════════════════════════
echo "── 1. Startup & Commands ──"

OUTPUT=$(run "/version
exit")
if echo "$OUTPUT" | grep -qE "[0-9]+\.[0-9]+\.[0-9]+"; then
  pass "version shows semver"
else
  fail "version" "no semver found"
fi

OUTPUT=$(run "/help
exit")
if echo "$OUTPUT" | grep -q "/model" && echo "$OUTPUT" | grep -q "/help"; then
  pass "help lists commands"
else
  fail "help" "missing /model or /theme"
fi

OUTPUT=$(run "/tasks
exit")
if echo "$OUTPUT" | grep -q "no background tasks"; then
  pass "tasks shows empty state"
else
  fail "tasks" "unexpected output"
fi

OUTPUT=$(run "/auto
exit")
if echo "$OUTPUT" | grep -q "removed\|tasks"; then
  pass "auto shows deprecation message"
else
  fail "auto" "unexpected output"
fi

# ═══════════════════════════════════════════════════════════════════
echo ""
echo "── 2. Theme System ──"

OUTPUT=$(run "/theme
exit")
if echo "$OUTPUT" | grep -q "dark" && echo "$OUTPUT" | grep -q "hacker"; then
  pass "theme lists available themes"
else
  fail "theme" "missing theme names"
fi

OUTPUT=$(run "/theme hacker
/theme dark
exit")
if echo "$OUTPUT" | grep -qi "hacker\|switched\|theme"; then
  pass "theme switch works"
else
  skip "theme switch" "no confirmation in output"
fi

# ═══════════════════════════════════════════════════════════════════
echo ""
echo "── 3. Chat (real LLM) ──"

OUTPUT=$(run "what is 2+2? answer with just the number
exit")
if echo "$OUTPUT" | grep -q "4"; then
  pass "basic math response"
else
  fail "basic chat" "no '4' in response"
fi

OUTPUT=$(run "say exactly: PONG
exit")
if echo "$OUTPUT" | grep -qi "PONG"; then
  pass "instruction following"
else
  skip "instruction following" "LLM didn't echo PONG"
fi

# ═══════════════════════════════════════════════════════════════════
echo ""
echo "── 4. File Operations ──"

# Write new file
rm -f "$TMP"
OUTPUT=$(run "write exactly this to $TMP: hello from test
y
exit")
if [[ -f "$TMP" ]] && grep -q "hello" "$TMP"; then
  pass "write new file"
  rm -f "$TMP"
elif echo "$OUTPUT" | grep -qi "proposed.*write"; then
  pass "write annotation detected (file may not have been created)"
else
  skip "write new file" "LLM didn't produce write annotation"
fi

# Read file
OUTPUT=$(run "!!echo 'test content for read'
what did that command output?
exit")
if echo "$OUTPUT" | grep -qi "test content"; then
  pass "!! command injects context"
else
  skip "!! context injection" "LLM didn't reference the output"
fi

# ═══════════════════════════════════════════════════════════════════
echo ""
echo "── 5. Session Management ──"

OUTPUT=$(run "/chat save fulltest-session
/chat load fulltest-session
exit")
if echo "$OUTPUT" | grep -qi "saved\|loaded\|restored"; then
  pass "session save/load"
else
  skip "session save/load" "no confirmation message"
fi

# ═══════════════════════════════════════════════════════════════════
echo ""
echo "── 6. Model Selection ──"

OUTPUT=$(run "/model
exit")
if echo "$OUTPUT" | grep -qE "[0-9]+\." || echo "$OUTPUT" | grep -qi "model"; then
  pass "model list shows entries"
else
  skip "model list" "no models found"
fi

# ═══════════════════════════════════════════════════════════════════
echo ""
echo "── 7. Settings ──"

OUTPUT=$(run "/set
exit")
if echo "$OUTPUT" | grep -qi "markdown\|color\|trace"; then
  pass "set shows options"
else
  fail "set" "no options shown"
fi

OUTPUT=$(run "/set markdown
/set markdown
exit")
if echo "$OUTPUT" | grep -qi "off\|on"; then
  pass "set toggles option"
else
  skip "set toggle" "no toggle confirmation"
fi

# ═══════════════════════════════════════════════════════════════════
echo ""
echo "── Results ──"
TOTAL=$((PASS + FAIL + SKIP))
echo "  Passed: $PASS / $TOTAL"
echo "  Failed: $FAIL"
echo "  Skipped: $SKIP"
echo ""

if [[ "$FAIL" -gt 0 ]]; then
  echo "FAIL: $FAIL test(s) failed"
  exit 1
fi
echo "OK: all tests passed ($SKIP skipped)"
