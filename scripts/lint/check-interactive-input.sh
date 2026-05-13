#!/usr/bin/env bash
# Check for direct std::cin usage in interactive code (ADR-088).
# All interactive input must go through read_answer() which uses linenoise.
# Direct cin reads bypass terminal mode handling and break Enter/Ctrl-C.
#
# Usage: bash scripts/lint/check-interactive-input.sh
# Returns: 0 if clean, 1 if violations found

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
echo "==> checking interactive input patterns (ADR-088)..."

violations=0

# Patterns that indicate direct cin usage (should use read_answer instead)
# Exclude: test files, the read_answer function itself, comments
while IFS= read -r file; do
  # Skip test files — they legitimately use stringstreams named 'in'
  [[ "$file" == *_test.cpp ]] && continue
  [[ "$file" == *_it.cpp ]] && continue

  # Check for direct std::cin reads
  if grep -n 'std::getline(std::cin\|std::cin >>\|std::cin\.get(' "$file" | grep -v '^\s*//' | grep -v 'read_answer' >/dev/null 2>&1; then
    echo "  [FAIL] $file: direct std::cin usage (use read_answer instead, ADR-088)"
    grep -n 'std::getline(std::cin\|std::cin >>\|std::cin\.get(' "$file" | grep -v '^\s*//' | grep -v 'read_answer' | sed 's/^/     /'
    violations=$((violations + 1))
  fi
done < <(find src -name '*.cpp' -not -name '*_test.cpp' -not -name '*_it.cpp')

if [[ $violations -gt 0 ]]; then
  echo "  [FAIL] $violations file(s) with direct std::cin usage"
  echo "  Fix: use read_answer(in, answer) — see ADR-088"
  exit 1
fi

echo "  [done] check-interactive-input"
