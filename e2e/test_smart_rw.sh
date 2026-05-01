#!/bin/bash
# E2E test: smart read/write annotations (diff, str_replace, read)
# Uses a wrapper that injects canned LLM responses via OLLAMA_SYSTEM_PROMPT
# and a mock provider that returns the system prompt as the response.
# Usage: ./e2e/test_smart_rw.sh [path-to-binary]

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "$SCRIPT_DIR/helpers.sh"

BINARY="${1:-./build/llama-cli}"
check_binary "$BINARY"

echo "=== E2E: smart read/write ==="

TMP_WRITE=$(mktemp)
TMP_REPLACE=$(mktemp)
TMP_READ=$(mktemp)
trap 'rm -f "$TMP_WRITE" "$TMP_WRITE.bak" "$TMP_REPLACE" "$TMP_REPLACE.bak" "$TMP_READ"' EXIT

# Run the binary in REPL mode (--repl forces REPL even with piped stdin).
# The mock provider echoes msgs.back().content — so the annotation sent as
# the user prompt is echoed back as the LLM response, which handle_response
# then processes.
run_repl_with() {
    local prompt="$1"
    local confirm="$2"
    printf '%s\n%s\nexit\n' "$prompt" "$confirm" \
        | LLAMA_PROVIDER=mock "$BINARY" --repl 2>&1
}

# --- test 1: write diff shown automatically for existing file ----------------
echo "--- test 1: write diff shown automatically ---"

echo "existing content" > "$TMP_WRITE"

OUTPUT=$(run_repl_with \
    "<write file=\"$TMP_WRITE\">new content</write>" \
    "y")

assert_contains "$OUTPUT" "existing content" "diff shows removed line"
assert_contains "$OUTPUT" "new content" "diff shows added line"
assert_contains "$OUTPUT" "Write to" "write prompt shown"
echo "PASS: diff shown automatically"

# --- test 2: write confirmed --------------------------------------------------
echo "--- test 2: write confirmed ---"

rm -f "$TMP_WRITE"
OUTPUT=$(run_repl_with \
    "<write file=\"$TMP_WRITE\">hello world</write>" \
    "y")

assert_contains "$OUTPUT" "wrote" "wrote confirmation"
[ -f "$TMP_WRITE" ] || die "file was not written"
assert_contains "$(cat "$TMP_WRITE")" "hello world" "file content correct"
echo "PASS: write confirmed and file written"

# --- test 3: write declined ---------------------------------------------------
echo "--- test 3: write declined ---"

rm -f "$TMP_WRITE"
OUTPUT=$(run_repl_with \
    "<write file=\"$TMP_WRITE\">hello world</write>" \
    "n")

assert_contains "$OUTPUT" "skipped" "skipped shown"
[ ! -f "$TMP_WRITE" ] || die "file should not have been written"
echo "PASS: write declined"

# --- test 4: str_replace ------------------------------------------------------
echo "--- test 4: str_replace ---"

printf 'line one\nline two\nline three\n' > "$TMP_REPLACE"

OUTPUT=$(run_repl_with \
    "<str_replace path=\"$TMP_REPLACE\"><old>line two</old><new>line TWO</new></str_replace>" \
    "y")

assert_contains "$OUTPUT" "wrote" "str_replace wrote confirmation"
grep -q "line TWO" "$TMP_REPLACE" || die "str_replace did not apply replacement"
grep -q "line two" "$TMP_REPLACE" && die "str_replace old string still present"
echo "PASS: str_replace applied"

# --- test 5: str_replace diff shown ------------------------------------------
echo "--- test 5: str_replace diff shown ---"

printf 'foo\nbar\nbaz\n' > "$TMP_REPLACE"

OUTPUT=$(run_repl_with \
    "<str_replace path=\"$TMP_REPLACE\"><old>bar</old><new>BAR</new></str_replace>" \
    "n")

assert_contains "$OUTPUT" "bar" "str_replace diff shows removed"
assert_contains "$OUTPUT" "BAR" "str_replace diff shows added"
echo "PASS: str_replace diff shown"

# --- test 6: read injects context and triggers follow-up ---------------------
echo "--- test 6: read annotation ---"

printf 'alpha\nbeta\ngamma\n' > "$TMP_READ"

OUTPUT=$(run_repl_with \
    "<read path=\"$TMP_READ\" lines=\"1-2\"/>" \
    "")  # no confirmation needed for read

assert_contains "$OUTPUT" "alpha" "read file content in follow-up"
echo "PASS: read annotation processed"

echo ""
echo "=== All smart read/write e2e tests passed ==="
