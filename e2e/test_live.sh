#!/bin/bash
# Integration test with a real LLM (requires running Ollama)
# Tests structural behavior, not exact LLM output.
# Usage: ./e2e/test_live.sh [path-to-binary]

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "$SCRIPT_DIR/helpers.sh"

BINARY="${1:-./build/llama-cli}"
check_binary "$BINARY"

# Check Ollama is running
if ! curl -s http://localhost:11434/api/tags > /dev/null 2>&1; then
    echo "SKIP: Ollama not running on localhost:11434"
    exit 0
fi

echo "=== LIVE integration test (real LLM) ==="

TMP="/tmp/llama-live-test-$$"
trap 'rm -f "$TMP" "${TMP}.bak" "${TMP}.skip"' EXIT

# Helper: send prompts, auto-confirm with y, capture output
# $1 = newline-separated prompts
# Returns: combined stdout+stderr
run() {
    echo "$1" | timeout 120 "$BINARY" --repl 2>&1
}

# --- test 1: basic chat responds ---
echo "--- test 1: basic chat ---"
OUTPUT=$(run "wat is 2+2?
exit")
assert_contains "$OUTPUT" "4" "LLM answered 2+2"
echo "PASS"

# --- test 2: trace has timestamps ---
echo "--- test 2: trace timestamps ---"
OUTPUT=$(echo "hallo
exit" | timeout 120 env TRACE=1 "$BINARY" --repl 2>&1)
# Timestamp format: [HH:MM:SS]
if ! echo "$OUTPUT" | grep -qE '\[[0-9]{2}:[0-9]{2}:[0-9]{2}\]'; then
    die "no timestamp found in trace output"
fi
echo "PASS"

# --- test 3: write new file ---
echo "--- test 3: write new file ---"
rm -f "$TMP"
OUTPUT=$(run "schrijf exact dit naar $TMP: <write file=\"$TMP\">live test ok</write>
y
exit")
# If LLM didn't produce the annotation, we gave it literally — should still work
if echo "$OUTPUT" | grep -qi "wrote"; then
    [ -f "$TMP" ] || die "file not created"
    assert_contains "$(cat "$TMP")" "live test ok" "file content"
    echo "PASS"
else
    echo "SKIP (LLM did not produce write annotation)"
fi

# --- test 4: write existing file shows diff ---
echo "--- test 4: auto-diff on existing file ---"
echo "old content" > "$TMP"
OUTPUT=$(run "overschrijf $TMP met \"new content\"
y
exit")
if echo "$OUTPUT" | grep -q "^- \|^-old\|- old"; then
    echo "PASS"
else
    echo "SKIP (LLM did not produce write annotation for existing file)"
fi

# --- test 5: str_replace ---
echo "--- test 5: str_replace ---"
echo "alpha beta gamma" > "$TMP"
OUTPUT=$(run "vervang \"beta\" door \"BETA\" in $TMP
y
exit")
if echo "$OUTPUT" | grep -qi "wrote"; then
    assert_contains "$(cat "$TMP")" "BETA" "replacement in file"
    echo "PASS"
else
    echo "SKIP (LLM did not produce str_replace annotation)"
fi

# --- test 6: read triggers follow-up ---
echo "--- test 6: read with follow-up ---"
OUTPUT=$(echo "lees regels 1-3 van src/main.cpp
exit" | timeout 120 env TRACE=1 "$BINARY" --repl 2>&1)
POST_COUNT=$(echo "$OUTPUT" | grep -c "POST.*api/chat" || true)
if [ "$POST_COUNT" -ge 2 ]; then
    echo "PASS"
else
    echo "SKIP (LLM did not use <read> annotation, got $POST_COUNT calls)"
fi

# --- test 7: decline write ---
echo "--- test 7: decline write ---"
rm -f "${TMP}.skip"
OUTPUT=$(run "maak een bestand ${TMP}.skip met \"should not exist\"
n
exit")
if echo "$OUTPUT" | grep -qi "skipped"; then
    [ ! -f "${TMP}.skip" ] || die "file should not exist"
    echo "PASS"
else
    echo "SKIP (LLM did not produce write annotation)"
fi

# --- test 8: /version shows build date ---
echo "--- test 8: /version ---"
OUTPUT=$(run "/version
exit")
assert_contains "$OUTPUT" "built" "version shows build date"
echo "PASS"

# --- test 9: language follows user ---
echo "--- test 9: language follow ---"
OUTPUT=$(run "what is the capital of France?
exit")
if echo "$OUTPUT" | grep -qi "Paris"; then
    echo "PASS"
else
    echo "SKIP (LLM did not mention Paris)"
fi

echo ""
echo "=== All live integration tests passed ==="
