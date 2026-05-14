#!/usr/bin/env bash
# check-user-facing-strings.sh — Find hardcoded user-facing strings (not test/debug)
# Usage: check-user-facing-strings.sh
# Finds UI messages that should be centralized in messages.h for i18n
#
# Description:
#   Scans C/C++ source files for hardcoded user-facing strings (errors, prompts,
#   messages) that should be centralized in src/ui/messages.h for i18n support.
#   Filters out debug/test code, log messages, and comments.
#
# Output:
#   Reports count of user-facing strings and sample strings found.
#   Recommends centralizing in src/ui/messages.h.
#
# Exit codes:
#   0 - Success (strings found or not found)
#   1 - Error during execution

set -o errexit
set -o nounset
set -o pipefail

if [[ "${TRACE-0}" == "1" ]]; then set -o xtrace; fi

source lib/cpm/shell/init.sh 2>/dev/null || true
print_header "checking for user-facing string literals..."

# Find user-facing strings in src/ (exclude: tests, debug, comments, log messages)
# Pattern: strings in cout, printf, error messages, UI prompts
count=$(find src -name '*.cpp' -o -name '*.h' |
  xargs grep -h 'cout\|printf\|std::cerr\|error\|prompt\|message' |
  grep -E '"[A-Z][^"]{5,}"' |
  grep -v 'LOG_\|TRACE\|DEBUG\|test\|SCENARIO\|CHECK\|GIVEN\|WHEN\|THEN' |
  grep -v '//' |
  wc -l)

if [ "$count" -gt 0 ]; then
  echo "  Found ~$count user-facing strings (candidates for i18n)"
  echo ""
  echo "  Sample strings:"
  find src -name '*.cpp' |
    xargs grep -h 'cout\|printf\|std::cerr' |
    grep -E '"[A-Z][^"]{5,}"' |
    grep -v 'LOG_\|TRACE\|DEBUG\|test' |
    head -5 |
    sed 's/^/    /'
  echo ""
  echo "  Recommendation: centralize in src/ui/messages.h"
else
  echo "  ✓ no user-facing strings found"
fi
