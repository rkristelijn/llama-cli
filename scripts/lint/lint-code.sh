#!/usr/bin/env bash
# lint-code.sh — Run cppcheck static analysis on C++ code.

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
if ! command -v cppcheck >/dev/null; then
  print_step "" "$(basename "$0" .sh)" skip "cppcheck not installed"
  exit 0
fi

print_header "running cppcheck..."
# Use cppcheck with specific suppressions for tests and internal libraries
output=$(cppcheck --enable=all \
  --inline-suppr \
  --suppress=missingIncludeSystem \
  --suppress=missingInclude \
  --suppress=unusedFunction \
  --suppress=unmatchedSuppression \
  --suppress=normalCheckLevelMaxBranches \
  --suppress=checkLevelNormal \
  --suppress=checkersReport \
  --suppress=useStlAlgorithm \
  --suppress=stlIfStrFind \
  --suppress=uselessCallsSubstr \
  --suppress='constVariableReference:*_test.cpp' \
  --suppress='knownConditionTrueFalse:*_it.cpp' \
  --suppress='knownConditionTrueFalse:*_test.cpp' \
  --suppress='knownConditionTrueFalse:*repl_chat.cpp' \
  --suppress='variableScope:*highlight.cpp' \
  --error-exitcode=1 -I src/ src/ 2>&1) || {
  echo "$output" | grep -v "Checking\|files checked"
  exit 1
}

echo "$output" | grep -v "Checking\|files checked" || true
