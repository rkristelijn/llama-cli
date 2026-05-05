#!/usr/bin/env bash
#
# check-feature-density.sh — Ensure LOG_FEATURE marker density stays healthy (ADR-063).
#
# Checks:
# 1. Every REPL command handler has a LOG_FEATURE marker
# 2. Markers per KLOC doesn't drop below threshold
# 3. Lists commands missing markers
#
# Usage:
#   bash scripts/test/check-feature-density.sh

set -o errexit
set -o nounset
set -o pipefail
if [[ "${TRACE-0}" == "1" ]]; then set -o xtrace; fi

MIN_MARKERS_PER_KLOC=2

main() {
  echo "==> checking feature marker density (ADR-063)..."

  # Count markers
  local markers
  markers=$(grep -rc 'LOG_FEATURE' src/ --include="*.cpp" 2>/dev/null | awk -F: '{s+=$2}END{print s}')

  # Count net LOC (non-test, non-blank, non-comment)
  local loc
  loc=$(find src -name "*.cpp" -not -name "*_test*" -not -name "*_it*" -not -name "*fuzz*" \
    | xargs cat | grep -cv "^$\|^/\|^\s*\*\|^\s*//")

  local kloc=$((loc / 1000))
  local density=$((markers * 10 / (kloc > 0 ? kloc : 1)))  # x10 for 1 decimal

  echo "  Markers: ${markers}"
  echo "  LOC: ${loc} (${kloc}K)"
  echo "  Density: ${markers}/${kloc}K = $(echo "scale=1; ${markers}/${kloc}" | bc) markers/KLOC (min: ${MIN_MARKERS_PER_KLOC})"

  # Check commands without markers
  echo ""
  echo "  Commands without LOG_FEATURE:"
  local missing=0
  while IFS= read -r cmd; do
    # Check if there's a LOG_FEATURE near this command handler
    local line_num
    line_num=$(grep -n "command == \"${cmd}\"" src/repl/repl_commands.cpp | head -1 | cut -d: -f1)
    if [[ -n "$line_num" ]]; then
      # Check 5 lines after the match for a LOG_FEATURE
      local has_marker
      has_marker=$(sed -n "$((line_num)),$((line_num + 5))p" src/repl/repl_commands.cpp | grep -c "LOG_FEATURE" || true)
      if [[ "$has_marker" -eq 0 ]]; then
        echo "    ✗ /${cmd}"
        ((missing++)) || true
      fi
    fi
  done < <(grep -oP 'command == "\K[^"]+' src/repl/repl_commands.cpp | sort -u)

  if [[ $missing -eq 0 ]]; then
    echo "    ✓ all commands have markers"
  else
    echo "  [${missing} commands missing markers]"
  fi

  # Threshold check
  local ratio
  ratio=$(echo "${markers} * 1000 / ${loc}" | bc)
  if [[ $ratio -lt $MIN_MARKERS_PER_KLOC ]]; then
    echo ""
    echo "  [WARN] Density below ${MIN_MARKERS_PER_KLOC}/KLOC — add markers to new features"
  fi

  echo "  [done] feature-density"
}

main "$@"
