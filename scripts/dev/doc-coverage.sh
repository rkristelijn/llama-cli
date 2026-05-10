#!/usr/bin/env bash
#
# doc-coverage.sh — Check documentation coverage for functions/classes
#
# Usage:
#   bash scripts/dev/doc-coverage.sh

set -o errexit
set -o nounset
set -o pipefail
if [[ "${TRACE-0}" == "1" ]]; then set -o xtrace; fi

main() {
  echo "Extracting symbols from src/..."

  # Functions
  funcs=$(grep -rh "^[a-zA-Z_][a-zA-Z0-9_]* [a-zA-Z_][a-zA-Z0-9_]*(" src/ --include="*.cpp" --include="*.h" |
    sed 's/(.*//' | awk '{print $NF}' | sort -u)

  # Classes and structs
  classes=$(grep -rh "^class [a-zA-Z_][a-zA-Z0-9_]*\|^struct [a-zA-Z_][a-zA-Z0-9_]*" src/ --include="*.cpp" --include="*.h" |
    awk '{print $2}' | sed 's/[:{].*//' | sort -u)

  symbols=$(echo -e "$funcs\n$classes" | sort -u)
  total=$(echo "$symbols" | wc -l)
  documented=0
  undocumented=()

  echo "Checking documentation coverage in docs/..."
  while IFS= read -r sym; do
    [[ -z "$sym" ]] && continue
    if grep -rq "$sym" docs/ 2>/dev/null; then
      ((documented++)) || true
    else
      undocumented+=("$sym")
    fi
  done <<<"$symbols"

  coverage=$((documented * 100 / total))

  echo ""
  echo "Documentation coverage: ${documented}/${total} symbols = ${coverage}%"

  if [ ${#undocumented[@]} -gt 0 ]; then
    echo ""
    echo "Undocumented symbols (${#undocumented[@]}):"
    printf '  %s\n' "${undocumented[@]}"
  fi

  # Check for orphaned docs
  echo ""
  echo "Checking for orphaned documentation..."
  doc_symbols=$(grep -roh '\b[a-zA-Z_][a-zA-Z0-9_]*(' docs/ | sed 's/(//' | sort -u)
  orphaned=()

  while IFS= read -r sym; do
    [[ -z "$sym" ]] && continue
    if ! echo "$symbols" | grep -q "^${sym}$"; then
      orphaned+=("$sym")
    fi
  done <<<"$doc_symbols"

  if [ ${#orphaned[@]} -gt 0 ]; then
    echo "Orphaned documentation (${#orphaned[@]}):"
    printf '  %s\n' "${orphaned[@]}"
  else
    echo "No orphaned documentation found."
  fi
}

main "$@"
