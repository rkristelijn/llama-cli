#!/usr/bin/env bash
# lint-code.sh — Run cppcheck static analysis on C++ code.

set -o errexit
set -o nounset
set -o pipefail
if [[ "${TRACE-0}" == "1" ]]; then set -o xtrace; fi

if ! command -v cppcheck >/dev/null; then
  echo "  [skip] cppcheck not installed"
  exit 0
fi

echo "==> running cppcheck..."
# Use cppcheck with specific suppressions for tests and internal libraries
output=$(cppcheck --enable=all \
  --inline-suppr \
  --suppress=missingIncludeSystem \
  --suppress=missingInclude \
  --suppress=unusedFunction \
  --suppress=unmatchedSuppression \
  --suppress=normalCheckLevelMaxBranches \
  --suppress=checkersReport \
  --suppress=useStlAlgorithm \
  --suppress='knownConditionTrueFalse:*_it.cpp' \
  --suppress='knownConditionTrueFalse:*_test.cpp' \
  --error-exitcode=1 -I src/ src/ 2>&1) || {
    echo "$output" | grep -v "Checking\|files checked"
    exit 1
  }

echo "$output" | grep -v "Checking\|files checked" || true
echo "  [done] lint-code"
