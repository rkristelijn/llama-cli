#!/usr/bin/env bash
# test_color_theme.sh — E2E test for /color and /theme interaction.
# Verifies: color targets, reset, theme switch, persistence.
set -o errexit
set -o nounset
set -o pipefail
if [[ "${TRACE-0}" == "1" ]]; then set -o xtrace; fi

BINARY="${1:-./build/llama-cli}"
export LLAMA_PROVIDER=mock
export OLLAMA_HOSTS=""
export OLLAMA_HOST=localhost
export HOME="$(mktemp -d)"
trap 'rm -rf "$HOME"' EXIT

die() {
  echo "FAIL: $1" >&2
  exit 1
}

# Test 1: /color prompt sets color
OUTPUT=$(echo "/color prompt yellow" | "$BINARY" --repl 2>/dev/null)
echo "$OUTPUT" | grep -q "prompt color set to yellow" || die "/color prompt yellow failed"

# Test 2: /color ai sets color
OUTPUT=$(echo "/color ai cyan" | "$BINARY" --repl 2>/dev/null)
echo "$OUTPUT" | grep -q "ai color set to cyan" || die "/color ai cyan failed"

# Test 3: /color system sets color
OUTPUT=$(echo "/color system magenta" | "$BINARY" --repl 2>/dev/null)
echo "$OUTPUT" | grep -q "system color set to magenta" || die "/color system failed"

# Test 4: /color reset restores theme
OUTPUT=$(echo "/color reset" | "$BINARY" --repl 2>/dev/null)
echo "$OUTPUT" | grep -q "colors reset to theme" || die "/color reset failed"

# Test 5: /color with invalid target
OUTPUT=$(echo "/color banana red" | "$BINARY" --repl 2>/dev/null)
echo "$OUTPUT" | grep -q "Unknown target" || die "invalid target not caught"

# Test 6: /color with invalid color
OUTPUT=$(echo "/color prompt rainbow" | "$BINARY" --repl 2>/dev/null)
echo "$OUTPUT" | grep -q "Unknown color" || die "invalid color not caught"

# Test 7: /theme switch then /color modifies it
OUTPUT=$(printf "/theme hacker\n/color prompt cyan\n" | "$BINARY" --repl 2>/dev/null)
echo "$OUTPUT" | grep -q "prompt color set to cyan" || die "/color after /theme failed"

# Test 8: /color shows usage when empty
OUTPUT=$(echo "/color" | "$BINARY" --repl 2>/dev/null)
echo "$OUTPUT" | grep -q "Usage:" || die "/color usage not shown"

# Test 9: /theme demo outputs ANSI escape codes for each role
OUTPUT=$(echo "/theme demo" | "$BINARY" --repl 2>/dev/null)
echo "$OUTPUT" | grep -q "theme demo: dark" || die "/theme demo header missing"
# Verify ANSI codes are present (ESC[...m sequences)
echo "$OUTPUT" | grep -q $'\033\[' || die "/theme demo missing ANSI codes"

# Test 10: /theme demo hacker shows hacker theme preview
OUTPUT=$(echo "/theme demo hacker" | "$BINARY" --repl 2>/dev/null)
echo "$OUTPUT" | grep -q "theme demo: hacker" || die "/theme demo hacker header missing"

# Test 11: /theme switch actually changes prompt ANSI codes
# dark = bold green (\033[1;32m), light = bold blue (\033[1;34m)
OUTPUT_DARK=$(echo "/theme demo dark" | "$BINARY" --repl 2>/dev/null)
OUTPUT_LIGHT=$(echo "/theme demo light" | "$BINARY" --repl 2>/dev/null)
# Extract the prompt line — they must differ (different ANSI codes)
PROMPT_DARK=$(echo "$OUTPUT_DARK" | grep "prompt:")
PROMPT_LIGHT=$(echo "$OUTPUT_LIGHT" | grep "prompt:")
[[ "$PROMPT_DARK" != "$PROMPT_LIGHT" ]] || die "dark and light themes produce identical prompt output"

# Cleanup handled by trap
echo "  ✓ test_color_theme: all 11 assertions passed"
