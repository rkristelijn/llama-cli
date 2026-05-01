#!/usr/bin/env bash
#
# check-feature-coverage.sh — Verify e2e tests exercise all LOG_FEATURE paths (ADR-063).
#
# Usage:
#   bash scripts/test/check-feature-coverage.sh [build_dir]
#
# Runs e2e tests with TRACE=1, then checks that every LOG_FEATURE token
# in the source code appeared in the test output.

set -o errexit
set -o nounset
set -o pipefail
if [[ "${TRACE-0}" == "1" ]]; then set -o xtrace; fi

BUILD_DIR="${1:-build}"
BINARY="${BUILD_DIR}/llama-cli"

# Extract all feature IDs from source code
EXPECTED=$(grep -roh 'LOG_FEATURE("[^"]*")' src/ | sed 's/LOG_FEATURE("//;s/")//' | sort -u)

# Run e2e tests with feature logging to a file
LOG=$(mktemp)
trap 'rm -f "$LOG"' EXIT
echo "==> running e2e with feature logging..."
LLAMA_FEATURE_LOG="$LOG" bash scripts/test/run-e2e.sh "$BUILD_DIR" "$BINARY" > /dev/null 2>&1 || true

# Check which features were hit
PASS=0
FAIL=0
echo ""
echo "Feature coverage report (ADR-063):"
for feature in $EXPECTED; do
  if grep -q "\[FEATURE: ${feature}\]" "$LOG"; then
    printf "  ✓ %s\n" "$feature"
    PASS=$((PASS + 1))
  else
    printf "  ✗ %s  [MISSING]\n" "$feature"
    FAIL=$((FAIL + 1))
  fi
done

TOTAL=$((PASS + FAIL))
echo ""
echo "Coverage: ${PASS}/${TOTAL} features exercised by e2e tests"

if [ "$FAIL" -gt 0 ]; then
  echo "FAIL: ${FAIL} feature(s) not covered by e2e tests"
  exit 1
fi
echo "PASS: all features covered"
