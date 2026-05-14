#!/usr/bin/env bash
# syntax-bash.sh — Validate bash syntax on all shell scripts.
#
# Universal check (any repo with .sh files).
# Runs: bash -n on each script. Fast, no deps, catches parse errors.
#
# Exit: 0 = all pass, 1 = syntax errors found
# Output: first-time-right (file + line + error for each failure)

source lib/cpm/shell/init.sh 2>/dev/null || true

main() {
  local failed=0 checked=0

  while IFS= read -r file; do
    [[ -z "$file" ]] && continue
    checked=$((checked + 1))
    local output
    output=$(bash -n "$file" 2>&1)
    if [[ $? -ne 0 ]]; then
      print_error "$file"
      echo "$output" | sed 's/^/    /'
      failed=$((failed + 1))
    fi
  done < <(find scripts e2e -name '*.sh' 2>/dev/null)

  if ((failed > 0)); then
    echo ""
    echo "  $failed/$checked scripts have syntax errors"
    exit 1
  fi
}

main
