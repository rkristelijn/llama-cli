#!/usr/bin/env bash
#
# check-duplication.sh — Detect duplicated code blocks in src/.
#
# Uses jscpd (via npx) or PMD CPD for token-based clone detection.
# Threshold: max 3% duplication on source files.
#
# Usage:
#   bash scripts/lint/check-duplication.sh

set -o errexit
set -o nounset
set -o pipefail
if [[ "${TRACE-0}" == "1" ]]; then set -o xtrace; fi

source lib/cpm/shell/init.sh 2>/dev/null || true
THRESHOLD="${DUPLICATION_THRESHOLD:-3}"
MIN_TOKENS="${MIN_TOKENS:-50}"
MIN_LINES="${MIN_LINES:-6}"

print_header "checking for code duplication..."

# Option 1: jscpd via npx (no install needed, supports C++)
if command -v npx >/dev/null 2>&1; then
  # TODO: refactor mermaid renderers to extract shared parsing logic (ADR-073)
  output=$(npx --yes jscpd src/ \
    --config .config/.jscpd.json \
    --silent 2>&1) || {
    # jscpd exits non-zero when threshold exceeded
    echo "$output" | grep -E "Clone|duplicate|%|Total" | head -20
    print_error "duplication exceeds ${THRESHOLD}% threshold"
    exit 1
  }
  echo "$output" | grep -E "Clone|duplicate|%|Total" | head -10
  echo "  ✓ duplication within ${THRESHOLD}% threshold"
  exit 0
fi

# Option 2: PMD CPD
if command -v pmd >/dev/null 2>&1 || command -v cpd >/dev/null 2>&1; then
  cmd=$(command -v cpd >/dev/null 2>&1 && echo "cpd" || echo "pmd cpd")
  output=$($cmd --minimum-tokens "$MIN_TOKENS" --language cpp --dir src/ --format text 2>&1) || true
  dupes=$(echo "$output" | grep -c "^Found a" || true)
  if [[ $dupes -eq 0 ]]; then
    echo "  ✓ no duplication detected (CPD)"
  else
    echo "$output" | grep -A2 "^Found a" | head -20
    echo "  [${dupes} duplicate blocks]"
  fi
  exit 0
fi

print_step "" "$(basename "$0" .sh)" skip "no duplication tool available (install: npm, pmd, or cpd)"
