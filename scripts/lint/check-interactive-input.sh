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

source lib/cpm/shell/init.sh 2>/dev/null || true
print_header "checking interactive input patterns (ADR-088)..."

violations=0

# Patterns that indicate direct cin usage (should use read_answer instead)
# Exclude: test files, the read_answer function itself, comments
while IFS= read -r file; do
  # Skip test files — they legitimately use stringstreams named 'in'
  [[ "$file" == *_test.cpp ]] && continue
  [[ "$file" == *_it.cpp ]] && continue

  # Check for direct std::cin reads
  if grep -n 'std::getline(std::cin\|std::cin >>\|std::cin\.get(' "$file" | grep -v '^\s*//' | grep -v 'read_answer' >/dev/null 2>&1; then
    print_error "$file: direct std::cin usage (use read_answer instead, ADR-088)"
    grep -n 'std::getline(std::cin\|std::cin >>\|std::cin\.get(' "$file" | grep -v '^\s*//' | grep -v 'read_answer' | sed 's/^/     /'
    violations=$((violations + 1))
  fi
done < <(find src -name '*.cpp' -not -name '*_test.cpp' -not -name '*_it.cpp')

if [[ $violations -gt 0 ]]; then
  print_error "$violations file(s) with direct std::cin usage"
  echo "  Fix: use read_answer(in, answer) — see ADR-088"
  exit 1
fi
