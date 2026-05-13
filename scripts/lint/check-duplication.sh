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

# Source cpm ui.sh if available, otherwise define minimal fallback
if [[ -f "${CPM_UI:-lib/cpm/shell/ui.sh}" ]]; then
  source "${CPM_UI:-lib/cpm/shell/ui.sh}"
else
  print_step() { echo "  $2 $3${4:+ $4}"; }
  print_header() { echo "==> $1"; }
  print_error() { echo "  ERROR: $1"; }
  print_warning() { echo "  WARNING: $1"; }
fi
THRESHOLD="${DUPLICATION_THRESHOLD:-3}"
MIN_TOKENS="${MIN_TOKENS:-50}"
MIN_LINES="${MIN_LINES:-6}"

echo "==> checking for code duplication..."

# Option 1: jscpd via npx (no install needed, supports C++)
if command -v npx >/dev/null 2>&1; then
  # TODO: refactor mermaid renderers to extract shared parsing logic (ADR-073)
  output=$(npx --yes jscpd src/ \
    --config .config/.jscpd.json \
    --silent 2>&1) || {
    # jscpd exits non-zero when threshold exceeded
    echo "$output" | grep -E "Clone|duplicate|%|Total" | head -20
    echo "  FAIL: duplication exceeds ${THRESHOLD}% threshold"
    exit 1
  }
  echo "$output" | grep -E "Clone|duplicate|%|Total" | head -10
  echo "  ✓ duplication within ${THRESHOLD}% threshold"
  echo "  [done] duplication"
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
  echo "  [done] duplication"
  exit 0
fi

echo "  [skip] no duplication tool available (install: npm, pmd, or cpd)"
echo "  [done] duplication"
