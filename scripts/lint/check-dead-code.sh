#!/usr/bin/env bash
#
# check-dead-code.sh — Detect unused functions and orphaned scripts (ADR-064).
#
# Warning-only: does not fail the build, but makes dead code visible.
#
# Usage:
#   bash scripts/lint/check-dead-code.sh

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
main() {
  echo "==> checking for dead code (ADR-064)..."
  local count=0

  # 1. Unused functions (cppcheck, skip test files)
  local unused
  unused=$(cppcheck --enable=unusedFunction --quiet \
    --suppress='*:*_test.cpp' --suppress='*:*_it.cpp' \
    src/ 2>&1 | grep "unusedFunction" || true)

  if [[ -n "$unused" ]]; then
    echo "  [warn] Unused functions detected:"
    echo "$unused" | sed 's/^/    /' | head -10
    count=$(echo "$unused" | wc -l)
  fi

  # 2. Orphaned scripts (not referenced in Makefile or CI)
  local orphans=""
  for script in scripts/**/*.sh; do
    [[ -f "$script" ]] || continue
    local base
    base=$(basename "$script")
    if ! grep -qr "$base\|$script" Makefile .github/ 2>/dev/null; then
      orphans+="    $script\n"
    fi
  done

  if [[ -n "$orphans" ]]; then
    echo "  [warn] Orphaned scripts (not in Makefile or CI):"
    echo -e "$orphans" | head -10
    count=$((count + $(echo -e "$orphans" | grep -c "." || true)))
  fi

  if [[ $count -eq 0 ]]; then
    echo "  ✓ no dead code detected"
  else
    echo "  [${count} items — review and remove or suppress]"
  fi
  echo "  [done] dead-code"
}

main "$@"
