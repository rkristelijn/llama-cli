#!/usr/bin/env bash
#
# check-duplication.sh — Detect duplicated code blocks in src/.
#
# Uses PMD CPD (Copy-Paste Detector) for token-based clone detection.
# Without CPD, skips with a recommendation to install it.
#
# Install CPD: brew install pmd (macOS) or download from https://pmd.github.io/
#
# Usage:
#   bash scripts/lint/check-duplication.sh

set -o errexit
set -o nounset
set -o pipefail
if [[ "${TRACE-0}" == "1" ]]; then set -o xtrace; fi

MIN_TOKENS="${MIN_TOKENS:-100}"

echo "==> checking for code duplication..."

# Use PMD CPD if available (token-based, catches renamed clones)
if command -v cpd >/dev/null 2>&1; then
  output=$(cpd --minimum-tokens "$MIN_TOKENS" --language cpp \
    --dir src/ --non-recursive=false \
    --format text 2>&1) || true

  dupes=$(echo "$output" | grep -c "^Found a" || true)
  if [[ $dupes -eq 0 ]]; then
    echo "  ✓ no duplication detected (CPD, min ${MIN_TOKENS} tokens)"
  else
    echo "$output" | grep -A2 "^Found a" | head -30
    echo "  [${dupes} duplicate blocks found]"
  fi
  echo "  [done] duplication"
  exit 0
fi

if command -v pmd >/dev/null 2>&1; then
  output=$(pmd cpd --minimum-tokens "$MIN_TOKENS" --language cpp \
    --dir src/ \
    --format text 2>&1) || true

  dupes=$(echo "$output" | grep -c "^Found a" || true)
  if [[ $dupes -eq 0 ]]; then
    echo "  ✓ no duplication detected (PMD CPD, min ${MIN_TOKENS} tokens)"
  else
    echo "$output" | grep -A2 "^Found a" | head -30
    echo "  [${dupes} duplicate blocks found]"
  fi
  echo "  [done] duplication"
  exit 0
fi

# No CPD available — skip gracefully
echo "  [skip] CPD not installed (brew install pmd)"
echo "  [done] duplication"
