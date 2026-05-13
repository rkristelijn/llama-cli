#!/usr/bin/env bash
#
# check-complexity.sh — Check cyclomatic complexity using pmccabe.
#
# Only functions with complexity >10 are checked for skip annotations.
# This avoids reading every file for every function (was 71s, now <2s).
#
# @see docs/adr/adr-044-tidy-boilerplate.md
# @see docs/adr/adr-121-cpm-quality-layer.md

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

THRESHOLD="${CPM_COMPLEXITY_THRESHOLD:-10}"

main() {
  if ! command -v pmccabe >/dev/null; then
    print_step "" "check-complexity" skip "pmccabe not installed"
    exit 0
  fi

  print_header "checking complexity (threshold: $THRESHOLD)..."

  # Step 1: get ALL complexities in one pass (fast)
  local violations
  violations=$(find src -name '*.cpp' | xargs pmccabe 2>/dev/null | awk -v t="$THRESHOLD" '$1 > t')

  if [[ -z "$violations" ]]; then
    exit 0
  fi

  # Step 2: only check skip annotations for the few violations
  local failed=0
  while IFS= read -r line; do
    # pmccabe format: complexity \t ... \t file(line): function
    local func_loc
    func_loc=$(echo "$line" | awk '{print $6}')
    local file="${func_loc%%(*}"
    local lno="${func_loc#*(}"
    lno="${lno%%)*}"

    # Check if annotated with skip-complexity nearby
    if head -n "$lno" "$file" 2>/dev/null | tail -n 6 | grep -q "skip-complexity"; then
      continue
    fi

    local complexity
    complexity=$(echo "$line" | awk '{print $1}')
    local func_name
    func_name=$(echo "$line" | awk -F': ' '{print $2}')
    print_warning "$file:$lno $func_name complexity=$complexity (max $THRESHOLD)"
    failed=1
  done <<< "$violations"

  if ((failed)); then
    echo ""
    echo "  Some functions exceed threshold (non-blocking, annotate with skip-complexity to exempt)"
  fi
}

main "$@"
