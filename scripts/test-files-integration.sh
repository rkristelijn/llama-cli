#!/bin/bash
# Integration test: --files flag with benchmarking
# Tests: single file, multiple files, stdin, error handling
# Logs: ~/git/hub/personal/tools/index-test-logs/integration-test-*.md

set -euo pipefail

LLAMA="${1:-$HOME/git/hub/llama-cli/build/llama-cli}"
LOGDIR="${2:-$HOME/git/hub/personal/tools/index-test-logs}"
TIMEOUT=30

mkdir -p "$LOGDIR"
LOGFILE="$LOGDIR/integration-test-$(date +%Y%m%d-%H%M%S).md"

echo "# llama-cli --files Integration Test" > "$LOGFILE"
echo "Date: $(date)" >> "$LOGFILE"
echo "Binary: $LLAMA" >> "$LOGFILE"
echo "" >> "$LOGFILE"
echo "| Test | Result | Duration | Notes |" >> "$LOGFILE"
echo "|------|--------|----------|-------|" >> "$LOGFILE"

echo "⏳ Running integration tests (timeout: ${TIMEOUT}s per test)..."
echo ""

# Test 1: Single file
echo -n "  [1/5] Single file... "
tmpfile=$(mktemp)
echo "The quick brown fox jumps over the lazy dog." > "$tmpfile"
start=$(date +%s%N)
result=$(timeout $TIMEOUT "$LLAMA" --files="$tmpfile" "what animal?" 2>/dev/null || echo "TIMEOUT")
end=$(date +%s%N)
dur=$(( (end - start) / 1000000 ))
status="✅"
[ "$result" = "TIMEOUT" ] && status="❌"
echo "$status (${dur}ms)"
echo "| Single file | $status | ${dur}ms | $result |" >> "$LOGFILE"
rm -f "$tmpfile"

# Test 2: Multiple files
echo -n "  [2/5] Multiple files... "
file1=$(mktemp)
file2=$(mktemp)
echo "Fox and dog" > "$file1"
echo "Cat and mouse" > "$file2"
start=$(date +%s%N)
result=$(timeout $TIMEOUT "$LLAMA" --files="$file1 $file2" "list animals" 2>/dev/null || echo "TIMEOUT")
end=$(date +%s%N)
dur=$(( (end - start) / 1000000 ))
status="✅"
[ "$result" = "TIMEOUT" ] && status="❌"
echo "$status (${dur}ms)"
echo "| Multiple files | $status | ${dur}ms | $(echo "$result" | head -c 40)... |" >> "$LOGFILE"
rm -f "$file1" "$file2"

# Test 3: Stdin pipe
echo -n "  [3/5] Stdin pipe... "
start=$(date +%s%N)
result=$(echo "The sky is blue" | timeout $TIMEOUT "$LLAMA" "translate to Dutch" 2>/dev/null || echo "TIMEOUT")
end=$(date +%s%N)
dur=$(( (end - start) / 1000000 ))
status="✅"
[ "$result" = "TIMEOUT" ] && status="❌"
echo "$status (${dur}ms)"
echo "| Stdin pipe | $status | ${dur}ms | $(echo "$result" | head -c 40)... |" >> "$LOGFILE"

# Test 4: Nonexistent file
echo -n "  [4/5] Nonexistent file... "
start=$(date +%s%N)
result=$(timeout $TIMEOUT "$LLAMA" --files="/nonexistent/file.txt" "test" 2>&1 || true)
end=$(date +%s%N)
dur=$(( (end - start) / 1000000 ))
if echo "$result" | grep -q "cannot read"; then
  echo "✅ (${dur}ms)"
  echo "| Nonexistent file | ✅ | ${dur}ms | Error handling works |" >> "$LOGFILE"
else
  echo "❌ (${dur}ms)"
  echo "| Nonexistent file | ❌ | ${dur}ms | Should error |" >> "$LOGFILE"
fi

# Test 5: Special characters in content
echo -n "  [5/5] Special chars... "
tmpfile=$(mktemp)
cat > "$tmpfile" << 'EOF'
Line 1: "quoted text"
Line 2: \backslash
Line 3: newline
EOF
start=$(date +%s%N)
result=$(timeout $TIMEOUT "$LLAMA" --files="$tmpfile" "count lines" 2>/dev/null || echo "TIMEOUT")
end=$(date +%s%N)
dur=$(( (end - start) / 1000000 ))
status="✅"
[ "$result" = "TIMEOUT" ] && status="❌"
echo "$status (${dur}ms)"
echo "| Special chars | $status | ${dur}ms | $(echo "$result" | head -c 40)... |" >> "$LOGFILE"
rm -f "$tmpfile"

echo ""

echo "" >> "$LOGFILE"
echo "Test completed: $LOGFILE" >> "$LOGFILE"

cat "$LOGFILE"
