#!/usr/bin/env bash
#
# test_new_commands.sh — E2E tests for new REPL commands (multi-provider).
#
# Tests: /provider, /agent, /theme, /auto, /chat, /nick, /compress, /usage

set -o errexit
set -o nounset
set -o pipefail

BINARY="${1:-./build/llama-cli}"
export LLAMA_PROVIDER=mock
export LLAMA_FEATURE_LOG="/tmp/llama-e2e-features-$$.log"
: > "$LLAMA_FEATURE_LOG"

pass=0
fail=0

assert_contains() {
  local output="$1" expected="$2" test_name="$3"
  if echo "$output" | grep -q "$expected"; then
    pass=$((pass + 1))
  else
    echo "  FAIL: $test_name — expected '$expected'"
    fail=$((fail + 1))
  fi
}

assert_feature() {
  local feature="$1"
  if grep -q "FEATURE: $feature" "$LLAMA_FEATURE_LOG"; then
    pass=$((pass + 1))
  else
    echo "  FAIL: feature $feature not logged"
    fail=$((fail + 1))
  fi
}

# --- /nick ---
output=$(printf "/nick TestUser\nexit\n" | "$BINARY" --repl 2>/dev/null)
assert_contains "$output" "nick set to: TestUser" "/nick sets name"
assert_feature "cmd_nick"

# --- /theme ---
output=$(printf "/theme hacker\nexit\n" | "$BINARY" --repl 2>/dev/null)
assert_contains "$output" "theme: hacker" "/theme switches"
assert_feature "cmd_theme"

# --- /agent ---
output=$(printf "/agent bofh\nexit\n" | "$BINARY" --repl 2>/dev/null)
assert_contains "$output" "agent: bofh" "/agent activates"
assert_feature "cmd_agent"

# --- /auto ---
output=$(printf "/auto\nexit\n" | "$BINARY" --repl 2>/dev/null)
assert_contains "$output" "auto routing: on" "/auto toggles on"
assert_feature "cmd_auto"

# --- /chat save/load ---
output=$(printf "hello\n/chat save e2etest\n/chat list\n/chat delete e2etest\nexit\n" | "$BINARY" --repl 2>/dev/null)
assert_contains "$output" "chat saved: e2etest" "/chat save"
assert_contains "$output" "e2etest" "/chat list"
assert_contains "$output" "chat deleted: e2etest" "/chat delete"
assert_feature "cmd_chat"

# --- /usage ---
output=$(printf "hello\n/usage\nexit\n" | "$BINARY" --repl 2>/dev/null)
assert_contains "$output" "Messages:" "/usage shows stats"
assert_feature "cmd_usage"

# --- /compress ---
output=$(printf "first question\nsecond question\nthird question\n/compress\nexit\n" | "$BINARY" --repl 2>/dev/null)
assert_contains "$output" "compressed:" "/compress works"
assert_feature "cmd_compress"

# --- /provider ---
output=$(printf "/provider\nexit\n" | "$BINARY" --repl 2>/dev/null)
assert_contains "$output" "current:" "/provider shows current"
assert_feature "provider_list"

# Cleanup
rm -f "$LLAMA_FEATURE_LOG"

echo "  new_commands: ${pass} passed, ${fail} failed"
[[ $fail -eq 0 ]]
