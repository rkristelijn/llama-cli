#!/usr/bin/env bash
#
# complexity.sh — Check cyclomatic complexity of source files using pmccabe.
#
# Functions with complexity >10 fail unless annotated with pmccabe:skip-complexity
# or clang-tidy:skip-complexity.
#
# Usage:
#   bash scripts/check/complexity.sh
#
# @see docs/adr/adr-44-tidy-boilerplate.md

set -o errexit
set -o nounset
set -o pipefail
if [[ "${TRACE-0}" == "1" ]]; then set -o xtrace; fi

main() {
  echo "==> make complexity (running pmccabe...)"
  find src -name '*.cpp' | xargs pmccabe 2>/dev/null | while read -r line; do
    file="$(echo "${line}" | awk '{print $6}' | cut -d'(' -f1)"
    lno="$(echo "${line}" | awk '{print $6}' | cut -d'(' -f2 | cut -d')' -f1)"
    if ! head -n "${lno}" "${file}" | tail -n 6 | grep -q "pmccabe:skip-complexity\|clang-tidy:skip-complexity"; then
      echo "${line}" | awk '$1 > 10 {print; exit 1}'
    fi
  done || exit 1
  echo "  [done] complexity"
}

main "$@"
