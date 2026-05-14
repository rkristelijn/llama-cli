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

source lib/cpm/shell/init.sh 2>/dev/null || true
BUILD_DIR="${1:-build}"
BINARY="${BUILD_DIR}/llama-cli"

# Features excluded from e2e coverage (require real LLM or are in planned-but-unused paths)
EXCLUDE="orchestrate_complex delegate_async cmd_tasks cmd_browse cmd_review cmd_spinner"

# Extract all feature IDs from source code, minus exclusions
EXPECTED=$(grep -roh 'LOG_FEATURE("[^"]*")' src/ | sed 's/LOG_FEATURE("//;s/")//' | sort -u)
for skip in $EXCLUDE; do
  EXPECTED=$(echo "$EXPECTED" | grep -v "^${skip}$")
done

# Run e2e tests with feature logging to a file
LOG=$(mktemp)
trap 'rm -f "$LOG"' EXIT
echo "==> running e2e with feature logging..."
# Run each test individually (don't stop on failure — we want coverage from all passing tests)
export LLAMA_FEATURE_LOG="$LOG"
export LLAMA_PROVIDER="${LLAMA_PROVIDER:-mock}"
export OLLAMA_HOSTS=""
export OLLAMA_HOST="localhost"
for t in e2e/test_*.sh; do
  case "$t" in *test_live* | *test_full_feature*) continue ;; esac
  bash "$t" "$BINARY" >/dev/null 2>&1 || true
done

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
